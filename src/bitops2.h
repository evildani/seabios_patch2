#ifndef _BIT_OPS_H
#define _BIT_OPS_H

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static inline int test_and_clear_bit(int nr, volatile void *addr)
{
    int oldbit;
    asm volatile (
        "lock; btrl %2,%1 ;\n \tsbbl %0,%0"
        : "=r" (oldbit), "=m" (*(volatile long *)addr)
        : "Ir" (nr), "m" (*(volatile long *)addr) : "memory");
    return oldbit;
}


static inline int test_bit(int nr, const volatile void *addr, int *val)
{
   int oldbit;
//was btl changed to
    asm volatile (
        "btl %2,%1 ;\n \tsbbl %0,%0"
        : "=r" (oldbit)
        : "m" (addr), "Ir" (nr) : "memory" );
    *val = oldbit;
    return oldbit;
}


#endif

