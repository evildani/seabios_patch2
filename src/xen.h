#ifndef __XEN_H
#define __XEN_H

#include "util.h"
#include "config.h"

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

/****
 * This is needed to get the shared info page
 ****/


struct arch_shared_info {
	unsigned long max_pfn;                  /* max pfn that appears in table */
	/* Frame containing list of mfns containing list of mfns containing p2m. */
	xen_pfn_t     pfn_to_mfn_frame_list_list;
	unsigned long nmi_reason;
	u64 pad[32];
};
typedef struct arch_shared_info arch_shared_info_t;

struct arch_vcpu_info {
	unsigned long cr2;
	unsigned long pad[5]; /* sizeof(vcpu_info_t) == 64 */
};
typedef struct arch_vcpu_info arch_vcpu_info_t;

struct vcpu_time_info {
	/*
	 * Updates to the following values are preceded and followed by an
	 * increment of 'version'. The guest can therefore detect updates by
	 * looking for changes to 'version'. If the least-significant bit of
	 * the version number is set then an update is in progress and the guest
	 * must wait to read a consistent set of values.
	 * The correct way to interact with the version number is similar to
	 * Linux's seqlock: see the implementations of read_seqbegin/read_seqretry.
	 */
	u32 version;
	u32 pad0;
	u64 tsc_timestamp;   /* TSC at last update of time vals.  */
	u64 system_time;     /* Time, in nanosecs, since boot.    */
	/*
	 * Current system time:
	 *   system_time +
	 *   ((((tsc - tsc_timestamp) << tsc_shift) * tsc_to_system_mul) >> 32)
	 * CPU frequency (Hz):
	 *   ((10^9 << 32) / tsc_to_system_mul) >> tsc_shift
	 */
	u32 tsc_to_system_mul;
	u8   tsc_shift;
	u8   pad1[3];
}; /* 32 bytes */
typedef struct vcpu_time_info vcpu_time_info_t;


struct vcpu_info {
	u8 evtchn_upcall_pending;
	u8 evtchn_upcall_mask;
	unsigned long evtchn_pending_sel;
	struct arch_vcpu_info arch;
	struct vcpu_time_info time;
};  /*64 bytes (x86)*/

#define XEN_LEGACY_MAX_VCPUS 32

struct shared_info {
	struct vcpu_info vcpu_info[XEN_LEGACY_MAX_VCPUS];
	unsigned long evtchn_pending[sizeof(unsigned long) * 8];
	unsigned long evtchn_mask[sizeof(unsigned long) * 8];
	u32 wc_version;      /* Version counter: see vcpu_time_info_t. */
	u32 wc_sec;          /* Secs  00:00:00 UTC, Jan 1, 1970.  */
	u32 wc_nsec;         /* Nsecs 00:00:00 UTC, Jan 1, 1970.  */

	struct arch_shared_info arch;

};
typedef struct shared_info shared_info_t;


struct shared_info *get_shared_info(void);

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

#define __HYPERVISOR_memory_op            12
#define __HYPERVISOR_xen_version          17
#define __HYPERVISOR_sched_op             29
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
    u16 domid;    //IN
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
 * EVTCHNOP_status: Get the current status of the communication channel which
 * has an endpoint at <dom, port>.
 * NOTES:
 *  1. <dom> may be specified as DOMID_SELF.
 *  2. Only a sufficiently-privileged domain may obtain the status of an event
 *     channel for which <dom> is not DOMID_SELF.
 */
#define EVTCHNOP_status           5
struct evtchn_status {
    /* IN parameters */
    u16  dom;
    evtchn_port_t port;
    /* OUT parameters */
#define EVTCHNSTAT_closed       0  /* Channel is not in use.                 */
#define EVTCHNSTAT_unbound      1  /* Channel is waiting interdom connection.*/
#define EVTCHNSTAT_interdomain  2  /* Channel is connected to remote domain. */
#define EVTCHNSTAT_pirq         3  /* Channel is bound to a phys IRQ line.   */
#define EVTCHNSTAT_virq         4  /* Channel is bound to a virtual IRQ line */
#define EVTCHNSTAT_ipi          5  /* Channel is bound to a virtual IPI line */
    u32 status;
    u32 vcpu;                 /* VCPU to which this channel is bound.   */
    union {
        struct {
        	u16 dom;
        } unbound; /* EVTCHNSTAT_unbound */
        struct {
        	u16 dom;
            evtchn_port_t port;
        } interdomain; /* EVTCHNSTAT_interdomain */
        u32 pirq;      /* EVTCHNSTAT_pirq        */
        u32 virq;      /* EVTCHNSTAT_virq        */
    } u;
};
typedef struct evtchn_status evtchn_status_t;


/******************************************************************************
 * sched.h
 *
 * Scheduler state interactions
 *
 * Copyright (c) 2005, Keir Fraser <keir@xensource.com>
 */

/*
 * Voluntarily yield the CPU.
 * @arg == NULL.
 */
#define SCHEDOP_yield       0

/*
 * Block execution of this VCPU until an event is received for processing.
 * If called with event upcalls masked, this operation will atomically
 * reenable event delivery and check for pending events before blocking the
 * VCPU. This avoids a "wakeup waiting" race.
 * @arg == NULL.
 */
#define SCHEDOP_block       1


/*
 * Poll a set of event-channel ports. Return when one or more are pending. An
 * optional timeout may be specified.
 * @arg == pointer to sched_poll structure.
 */
#define SCHEDOP_poll        3
struct sched_poll {
    XEN_GUEST_HANDLE(evtchn_port_t) ports;
    unsigned int nr_ports;
    u64 timeout;
};
typedef struct sched_poll sched_poll_t;
DEFINE_XEN_GUEST_HANDLE(sched_poll_t);

/*
 * Error base
 */
#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	EIO		 5	/* I/O error */
#define	EACCES		13	/* Permission denied */
#define	EINVAL		22	/* Invalid argument */
#define	ENOSYS		38	/* Function not implemented */
#define	EISCONN		106	/* Transport endpoint is already connected */

/*
 * xs_wire.h
 * Details of the "wire" protocol between Xen Store Daemon and client
 * library or guest kernel.
 *
 * Copyright (C) 2005 Rusty Russell IBM Corporation
 */
#define XENSTORE_PAYLOAD_MAX 4096

enum xsd_sockmsg_type
{
    XS_DEBUG,
    XS_DIRECTORY,
    XS_READ,
    XS_GET_PERMS,
    XS_WATCH,
    XS_UNWATCH,
    XS_TRANSACTION_START,
    XS_TRANSACTION_END,
    XS_INTRODUCE,
    XS_RELEASE,
    XS_GET_DOMAIN_PATH,
    XS_WRITE,
    XS_MKDIR,
    XS_RM,
    XS_SET_PERMS,
    XS_WATCH_EVENT,
    XS_ERROR,
    XS_IS_DOMAIN_INTRODUCED,
    XS_RESUME,
    XS_SET_TARGET,
    XS_RESTRICT
};

#define XS_WRITE_NONE "NONE"
#define XS_WRITE_CREATE "CREATE"
#define XS_WRITE_CREATE_EXCL "CREATE|EXCL"

/* We hand errors as strings, for portability. */
struct xsd_errors
{
    int errnum;
    const char *errstring;
};

struct xsd_sockmsg
{
    u32 type;  /* XS_??? */
    u32 req_id;/* Request identifier, echoed in daemon's response.  */
    u32 tx_id; /* Transaction id (0 if not related to a transaction). */
    u32 len;   /* Length of data following this. */

    /* Generally followed by nul-terminated string(s). */
};

enum xs_watch_type
{
    XS_WATCH_PATH = 0,
    XS_WATCH_TOKEN
};

/* Inter-domain shared memory communications. */
#define XENSTORE_RING_SIZE 1024
typedef u32 XENSTORE_RING_IDX;
#define MASK_XENSTORE_IDX(idx) ((idx) & (XENSTORE_RING_SIZE-1))
struct xenstore_domain_interface {
    char req[XENSTORE_RING_SIZE]; /* Requests to xenstore daemon. */
    char rsp[XENSTORE_RING_SIZE]; /* Replies and async watch events. */
    XENSTORE_RING_IDX req_cons, req_prod;
    XENSTORE_RING_IDX rsp_cons, rsp_prod;
};

#define XSD_ERROR(x) { x, #x }
static struct xsd_errors __attribute__ ((unused)) xsd_errors[]
    = {
    XSD_ERROR(EINVAL),
    XSD_ERROR(EACCES),
    XSD_ERROR(EIO),
    XSD_ERROR(EISCONN)
};

/******************************************************************************
 * memory.h
 *
 * Memory reservation and information.
 *
 * Copyright (c) 2005, Keir Fraser <keir@xensource.com>
 */
/*
 * Sets the GPFN at which a particular page appears in the specified guest's
 * pseudophysical address space.
 * arg == addr of xen_add_to_physmap_t.
 */
#define XENMEM_add_to_physmap      7
struct xen_add_to_physmap {
    /* Which domain to change the mapping for. */
    u16 domid;

    /* Source mapping space. */
#define XENMAPSPACE_shared_info 0 /* shared info page */
#define XENMAPSPACE_grant_table 1 /* grant table page */
#define XENMAPSPACE_gmfn        2 /* GMFN */
    unsigned int space;

#define XENMAPIDX_grant_table_status 0x80000000

    /* Index into source mapping space. */
    xen_ulong_t idx;

    /* GPFN where the source mapping page should appear. */
    xen_pfn_t     gpfn;
};
typedef struct xen_add_to_physmap xen_add_to_physmap_t;
DEFINE_XEN_GUEST_HANDLE(xen_add_to_physmap_t);

/* -------------------------------------------------------------------------
 * xen/include/public/io/blkif.h
 * blkif.h
 *
 * Unified block-device I/O interface for Xen guest OSes.
 *
 * Copyright (c) 2003-2004, Keir Fraser
 */

enum blkif_state {
         BLKIF_STATE_DISCONNECTED,
         BLKIF_STATE_CONNECTED,
        BLKIF_STATE_SUSPENDED,
};

/*
 * REQUEST CODES.
 */
#define BLKIF_OP_READ              0
#define BLKIF_OP_WRITE             1
#define BLKIF_MAX_SEGMENTS_PER_REQUEST 8
/*
 * NB. first_sect and last_sect in blkif_request_segment, as well as
 * sector_number in blkif_request, are always expressed in 512-byte units.
 * However they must be properly aligned to the real sector size of the
 * physical disk, which is reported in the "sector-size" node in the backend
 * xenbus info. Also the xenbus "sectors" node is expressed in 512-byte units.
 */
struct blkif_request_segment {
    u32 gref;        /* reference to I/O buffer frame        */
    /* @first_sect: first sector in frame to transfer (inclusive).   */
    /* @last_sect: last sector in frame to transfer (inclusive).     */
    u8     first_sect, last_sect;
};



struct blkif_request {
    u8        operation;    /* BLKIF_OP_???                         */
    u8        nr_segments;  /* number of segments                   */
    u16   handle;       /* only for read/write requests    ????     */
    u64       id;           /* private guest value, echoed in resp  */
    u64 sector_number;/* start sector idx on disk (r/w only)  */
    struct blkif_request_segment seg[BLKIF_MAX_SEGMENTS_PER_REQUEST];
};
typedef struct blkif_request blkif_request_t;


struct blkif_response {
    u64        id;              /* copied from request */
    u8         operation;       /* copied from request */
    u16         status;          /* BLKIF_RSP_???       */
};
typedef struct blkif_response blkif_response_t;

/*
 * STATUS RETURN CODES.
 */
 /* Operation not supported (only happens on barrier writes). */
#define BLKIF_RSP_EOPNOTSUPP  -2
 /* Operation failed for some unspecified reason (-EIO). */
#define BLKIF_RSP_ERROR       -1
 /* Operation completed successfully. */
#define BLKIF_RSP_OKAY         0

/*
 * Wrappers for hypercalls
 */
static inline int hypercall_xen_version( int cmd, void *arg)
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

static inline int hypercall_memory_op(int cmd ,void *arg)
{
	return _hypercall2(int, memory_op, cmd ,arg);
}

static inline int hypercall_sched_op(int cmd, void *arg)
{
	return _hypercall2(int, sched_op, cmd, arg);
}


#endif
