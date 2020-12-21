#include "atomic.h"
#include "interrupt.h"


void atomic_add(int i, volatile atomic_t *v)
{
	unsigned long flags;

	local_irq_save(flags);
	v->counter += i;
	local_irq_restore(flags);
}

void atomic_sub(int i, volatile atomic_t *v)
{
	unsigned long flags;

	local_irq_save(flags);
	v->counter -= i;
	local_irq_restore(flags);
}

void atomic_inc(volatile atomic_t *v)
{
	unsigned long flags;

	local_irq_save(flags);
	v->counter += 1;
	local_irq_restore(flags);
}

void atomic_dec(volatile atomic_t *v)
{
	unsigned long flags;

	local_irq_save(flags);
	v->counter -= 1;
	local_irq_restore(flags);
}

int atomic_dec_and_test(volatile atomic_t *v)
{
	unsigned long flags;
	int result;

	local_irq_save(flags);
	v->counter -= 1;
	result = (v->counter == 0);
	local_irq_restore(flags);

	return result;
}

int atomic_add_negative(int i, volatile atomic_t *v)
{
	unsigned long flags;
	int result;

	local_irq_save(flags);
	v->counter += i;
	result = (v->counter < 0);
	local_irq_restore(flags);

	return result;
}

void atomic_clear_mask(unsigned long mask, unsigned long *addr)
{
	unsigned long flags;

	local_irq_save(flags);
	*addr &= ~mask;
	local_irq_restore(flags);
}

void atomic_set(atomic_t *v, int i)
{
    unsigned long flags;

	local_irq_save(flags);
    v->counter = i;
	local_irq_restore(flags);
}