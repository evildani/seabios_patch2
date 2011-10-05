// Xen HVM support
//
// Copyright (C) 2011 Citrix Systems.
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#include "config.h"
#include "xen.h"

#include "memmap.h" // add_e820
#include "types.h" // ASM32FLAT
#include "util.h" // copy_acpi_rsdp

#define INFO_PHYSICAL_ADDRESS 0x00001000

u32 xen_cpuid_base = 0;

struct xen_seabios_info {
    char signature[14]; /* XenHVMSeaBIOS\0 */
    u8 length;     /* Length of this struct */
    u8 checksum;   /* Set such that the sum over bytes 0..length == 0 */
    /*
     * Physical address of an array of tables_nr elements.
     *
     * Each element is a 32 bit value contianing the physical address
     * of a BIOS table.
     */
    u32 tables;
    u32 tables_nr;
    /*
     * Physical address of the e820 table, contains e820_nr entries.
     */
    u32 e820;
    u32 e820_nr;
} PACKED;

static void validate_info(struct xen_seabios_info *t)
{
    if ( memcmp(t->signature, "XenHVMSeaBIOS", 14) )
        panic("Bad Xen info signature\n");

    if ( t->length < sizeof(struct xen_seabios_info) )
        panic("Bad Xen info length\n");

    if (checksum(t, t->length) != 0)
        panic("Bad Xen info checksum\n");
}

void xen_probe(void)
{
    u32 base, eax, ebx, ecx, edx;
    char signature[13];

    if (!CONFIG_XEN)
        return;

    for (base = 0x40000000; base < 0x40010000; base += 0x100) {
        cpuid(base, &eax, &ebx, &ecx, &edx);
        memcpy(signature + 0, &ebx, 4);
        memcpy(signature + 4, &ecx, 4);
        memcpy(signature + 8, &edx, 4);
        signature[12] = 0;

        dprintf(1, "Found hypervisor signature \"%s\" at %x\n",
                signature, base);
        if (strcmp(signature, "XenVMMXenVMM") == 0) {
            if ((eax - base) < 2)
                panic("Insufficient Xen cpuid leaves. eax=%x at base %x\n",
                      eax, base);
            xen_cpuid_base = base;
            break;
        }
    }
}

/* Fill in hypercall transfer pages. */
void xen_init_hypercalls(void)
{
    u32 eax, ebx, ecx, edx;
    xen_extraversion_t extraversion;
    unsigned long i;

    if (!usingXen())
        return;

    cpuid(xen_cpuid_base + 2, &eax, &ebx, &ecx, &edx);

    xen_hypercall_page = (unsigned long)memalign_high(PAGE_SIZE, eax*PAGE_SIZE);
    if (!xen_hypercall_page)
        panic("unable to allocate Xen hypercall page\n");

    dprintf(1, "Allocated Xen hypercall page at %lx\n", xen_hypercall_page);
    for ( i = 0; i < eax; i++ )
        wrmsr(ebx, xen_hypercall_page + (i << PAGE_SHIFT) + i);

    /* Print version information. */
    cpuid(xen_cpuid_base + 1, &eax, &ebx, &ecx, &edx);
    hypercall_xen_version(XENVER_extraversion, extraversion);
    dprintf(1, "Detected Xen v%u.%u%s\n", eax >> 16, eax & 0xffff, extraversion);
}

void xen_copy_biostables(void)
{
    struct xen_seabios_info *info = (void *)INFO_PHYSICAL_ADDRESS;
    u32 *tables = (u32 *)info->tables;
    int i;

    dprintf(1, "xen: copy BIOS tables...\n");
    for (i=0; i<info->tables_nr; i++) {
        void *table = (void *)tables[i];
        copy_acpi_rsdp(table);
        copy_mptable(table);
        copy_pir(table);
        copy_smbios(table);
    }
}

void xen_setup(void)
{
    u64 maxram = 0, maxram_over4G = 0;
    int i;
    struct xen_seabios_info *info = (void *)INFO_PHYSICAL_ADDRESS;
    struct e820entry *e820 = (struct e820entry *)info->e820;
    validate_info(info);

    dprintf(1, "xen: copy e820...\n");

    for (i = 0; i < info->e820_nr; i++) {
        struct e820entry *e = &e820[i];
        if (e->type == E820_ACPI || e->type == E820_RAM) {
            u64 end = e->start + e->size;
            if (end > 0x100000000ull) {
                end -= 0x100000000ull;
                if (end > maxram_over4G)
                    maxram_over4G = end;
            } else if (end > maxram)
                maxram = end;
        }
        add_e820(e->start, e->size, e->type);
    }

    RamSize = maxram;
    RamSizeOver4G = maxram_over4G;
}

struct shared_info *get_shared_info(void)
{
    static struct shared_info *shared_info = NULL;
    struct xen_add_to_physmap xatp;

    if (shared_info != NULL)
        return shared_info;

    xatp.domid = DOMID_SELF;
    xatp.space = XENMAPSPACE_shared_info;
    xatp.idx   = 0;
    shared_info = memalign_high(PAGE_SIZE, PAGE_SIZE);
    memset(shared_info, 0, PAGE_SIZE);
    xatp.gpfn  = ((unsigned long)shared_info >> PAGE_SHIFT);
    dprintf(1, "allocated shared info %d bytes at %p, gpfn 0x%lx\n",sizeof(*shared_info), shared_info, xatp.gpfn);
    //xatp.gpfn  = malloc_high(sizeof(shared_info));
    //shared_info = (struct shared_info *)(xatp.gpfn << PAGE_SHIFT);
    if (hypercall_memory_op(XENMEM_add_to_physmap, &xatp) != 0)
    	panic("MAP info page fail");
    dprintf(1, "time is %d\n", shared_info->wc_sec);
    dprintf(1, "evtchn_pending[0] 0x%08lx\n", shared_info->evtchn_pending[0]);
    dprintf(1, "evtchn_mask[0] 0x%08lx\n", shared_info->evtchn_mask[0]);
    dprintf(1, "VCPU0 evtchn_upcall_pending 0x%x\n", shared_info->vcpu_info[0].evtchn_upcall_pending);
    dprintf(1, "VCPU0 evtchn_upcall_mask 0x%x\n", shared_info->vcpu_info[0].evtchn_upcall_mask);
    dprintf(1, "VCPU0 evtchn_pending_sel 0x%08lx\n", shared_info->vcpu_info[0].evtchn_pending_sel);
    return shared_info;
}
