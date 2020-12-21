#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include "arm/irqflags.h"

typedef struct atomic
{
    volatile int counter;
} atomic_t;

#define atomic_read(v)	((v)->counter)

void atomic_add(int i, volatile atomic_t *v);
void atomic_sub(int i, volatile atomic_t *v);
void atomic_inc(volatile atomic_t *v);
void atomic_dec(volatile atomic_t *v);
int atomic_dec_and_test(volatile atomic_t *v);
int atomic_add_negative(int i, volatile atomic_t *v);
void atomic_clear_mask(unsigned long mask, unsigned long *addr);
void atomic_set(atomic_t *v, int i);

#endif
