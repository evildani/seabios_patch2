#ifndef __XEN_H
#define __XEN_H

#include "util.h"

extern u32 xen_cpuid_base;

void xen_probe(void);
void xen_setup(void);
void xen_init_hypercalls(void);
void xen_copy_biostables(void);

static inline int usingXen(void) {
    if (!CONFIG_XEN)
	return 0;
    return (xen_cpuid_base != 0);
}

unsigned long xen_hypercall_page;
typedef unsigned long xen_ulong_t;
typedef unsigned long xen_pfn_t;

#define _hypercall0(type, name)                                         \
({                                                                      \
    unsigned long __hentry = xen_hypercall_page+__HYPERVISOR_##name*32; \
    long __res;                                                         \
    asm volatile (                                                      \
        "call *%%eax"                                                   \
        : "=a" (__res)                                                  \
        : "0" (__hentry)                                                \
        : "memory" );                                                   \
    (type)__res;                                                        \
})

#define _hypercall1(type, name, a1)                                     \
({                                                                      \
    unsigned long __hentry = xen_hypercall_page+__HYPERVISOR_##name*32; \
    long __res, __ign1;                                                 \
    asm volatile (                                                      \
        "call *%%eax"                                                   \
        : "=a" (__res), "=b" (__ign1)                                   \
        : "0" (__hentry), "1" ((long)(a1))                              \
        : "memory" );                                                   \
    (type)__res;                                                        \
})

#define _hypercall2(type, name, a1, a2)                                 \
({                                                                      \
    unsigned long __hentry = xen_hypercall_page+__HYPERVISOR_##name*32; \
    long __res, __ign1, __ign2;                                         \
    asm volatile (                                                      \
        "call *%%eax"                                                   \
        : "=a" (__res), "=b" (__ign1), "=c" (__ign2)                    \
        : "0" (__hentry), "1" ((long)(a1)), "2" ((long)(a2))            \
        : "memory" );                                                   \
    (type)__res;                                                        \
})

#define _hypercall3(type, name, a1, a2, a3)                             \
({                                                                      \
    unsigned long __hentry = xen_hypercall_page+__HYPERVISOR_##name*32; \
    long __res, __ign1, __ign2, __ign3;                                 \
    asm volatile (                                                      \
        "call *%%eax"                                                   \
        : "=a" (__res), "=b" (__ign1), "=c" (__ign2),                   \
          "=d" (__ign3)                                                 \
        : "0" (__hentry), "1" ((long)(a1)), "2" ((long)(a2)),           \
          "3" ((long)(a3))                                              \
        : "memory" );                                                   \
    (type)__res;                                                        \
})

#define _hypercall4(type, name, a1, a2, a3, a4)                         \
({                                                                      \
    unsigned long __hentry = xen_hypercall_page+__HYPERVISOR_##name*32; \
    long __res, __ign1, __ign2, __ign3, __ign4;                         \
    asm volatile (                                                      \
        "call *%%eax"                                                   \
        : "=a" (__res), "=b" (__ign1), "=c" (__ign2),                   \
          "=d" (__ign3), "=S" (__ign4)                                  \
        : "0" (__hentry), "1" ((long)(a1)), "2" ((long)(a2)),           \
          "3" ((long)(a3)), "4" ((long)(a4))                            \
        : "memory" );                                                   \
    (type)__res;                                                        \
})

#define _hypercall5(type, name, a1, a2, a3, a4, a5)                     \
({                                                                      \
    unsigned long __hentry = xen_hypercall_page+__HYPERVISOR_##name*32; \
    long __res, __ign1, __ign2, __ign3, __ign4, __ign5;                 \
    asm volatile (                                                      \
        "call *%%eax"                                                   \
        : "=a" (__res), "=b" (__ign1), "=c" (__ign2),                   \
          "=d" (__ign3), "=S" (__ign4), "=D" (__ign5)                   \
        : "0" (__hentry), "1" ((long)(a1)), "2" ((long)(a2)),           \
          "3" ((long)(a3)), "4" ((long)(a4)),                           \
          "5" ((long)(a5))                                              \
        : "memory" );                                                   \
    (type)__res;                                                        \
})


/******************************************************************************
 *
 * The following interface definitions are taken from Xen and have the
 * following license:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/******************************************************************************
 * arch-x86/xen.h
 *
 * Guest OS interface to x86 Xen.
 *
 * Copyright (c) 2004-2006, K A Fraser
 */


/* Structural guest handles introduced in 0x00030201. */
#define ___DEFINE_XEN_GUEST_HANDLE(name, type) \
    typedef struct { type *p; } __guest_handle_ ## name

#define __DEFINE_XEN_GUEST_HANDLE(name, type) \
    ___DEFINE_XEN_GUEST_HANDLE(name, type);   \
    ___DEFINE_XEN_GUEST_HANDLE(const_##name, const type)
#define DEFINE_XEN_GUEST_HANDLE(name)   __DEFINE_XEN_GUEST_HANDLE(name, name)
#define __XEN_GUEST_HANDLE(name)        __guest_handle_ ## name
#define XEN_GUEST_HANDLE(name)          __XEN_GUEST_HANDLE(name)
#define set_xen_guest_handle_raw(hnd, val)  do { (hnd).p = val; } while (0)
#define set_xen_guest_handle(hnd, val) set_xen_guest_handle_raw(hnd, val)

/******************************************************************************
 * version.h
 *
 * Xen version, type, and compile information.
 *
 * Copyright (c) 2005, Nguyen Anh Quynh <aquynh@gmail.com>
 * Copyright (c) 2005, Keir Fraser <keir@xensource.com>
 */

/* arg == xen_extraversion_t. */
#define XENVER_extraversion 1
typedef char xen_extraversion_t[16];
#define XEN_EXTRAVERSION_LEN (sizeof(xen_extraversion_t))

/******************************************************************************
 * xen.h
 *
 * Guest OS interface to Xen.
 *
 * Copyright (c) 2004, K A Fraser
 */

#define DOMID_SELF  (0x7FF0U)

/* Guest handles for primitive C types. */
DEFINE_XEN_GUEST_HANDLE(char);
__DEFINE_XEN_GUEST_HANDLE(uchar, unsigned char);
DEFINE_XEN_GUEST_HANDLE(int);
__DEFINE_XEN_GUEST_HANDLE(uint,  unsigned int);
DEFINE_XEN_GUEST_HANDLE(long);
__DEFINE_XEN_GUEST_HANDLE(ulong, unsigned long);
DEFINE_XEN_GUEST_HANDLE(void);

DEFINE_XEN_GUEST_HANDLE(u64);
DEFINE_XEN_GUEST_HANDLE(xen_pfn_t);

__DEFINE_XEN_GUEST_HANDLE(u8,  u8);
__DEFINE_XEN_GUEST_HANDLE(u16, u16);
__DEFINE_XEN_GUEST_HANDLE(u32, u32);

#define __HYPERVISOR_xen_version          17
#define __HYPERVISOR_event_channel_op     32
#define __HYPERVISOR_hvm_op               34

/*
 *  from: xen/include/public/hvm/hvm_op.h
 */

/*
 *  in an HVM guest you find your xenstore ring and evtchn via hvmparams,
 *  HVM_PARAM_STORE_PFN and HVM_PARAM_STORE_EVTCHN.
 *  it's a hypercall (type) hvmop, subcommand HVMOP_get_param (this is the hypercall actually) with this:
 *  Get/set subcommands: extra argument == pointer to xen_hvm_param struct.
*/
#define HVMOP_set_param  0
#define HVMOP_get_param  1

/*
 * from: include/public/hvm/params.h
 */

#define HVM_PARAM_STORE_PFN    1 //pass as index
#define HVM_PARAM_STORE_EVTCHN 2 //pass as index

struct xen_hvm_param {
    u32 domid;    //IN
    u32 index;    //IN
    u64 value;    //IN/OUT
};

/******************************************************************************
 * event_channel.h
 *
 * Event channels between domains.
 *
 * Copyright (c) 2003-2004, K A Fraser.
 */

typedef u32 evtchn_port_t;
DEFINE_XEN_GUEST_HANDLE(evtchn_port_t);

#define EVTCHNOP_send             4
struct evtchn_send {
    /* IN parameters. */
    evtchn_port_t port;
};
typedef struct evtchn_send evtchn_send_t;

/*
 * Wrappers for hypercalls
 */
static int hypercall_xen_version( int cmd, void *arg)
{
	return _hypercall2(int, xen_version, cmd, arg);
}

static inline int hypercall_hvm_op(int cmd, void *arg)
{
	return _hypercall2(int, hvm_op, cmd, arg);
}

static inline int hypercall_event_channel_op(int cmd, void *arg)
{
	return _hypercall2(int, event_channel_op, cmd, arg);
}

#endif
