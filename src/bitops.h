#ifndef _BIT_OPS_H
#define _BIT_OPS_H

//int event, shinfo->evtchn_pending[sizeof(unsigned long) * 8]
static inline int test_and_clear_bit(int nr, volatile void *addr)
{
    int oldbit;
    asm volatile (
        "lock ; btrl %2,%1 ; sbbl %0,%0"
        : "=r" (oldbit), "=m" (*(volatile long *)addr)
        : "Ir" (nr), "m" (*(volatile long *)addr) : "memory");
    return oldbit;
}

#endif
