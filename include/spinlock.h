#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "spinlock_types.h"

extern void raw_spin_lock(raw_spinlock_t *lock);
extern void raw_spin_unlock(raw_spinlock_t *lock);
extern void raw_spin_lock_init(raw_spinlock_t *lock);
extern void raw_spin_unlock(raw_spinlock_t *lock);
extern void raw_spin_lock_irq(raw_spinlock_t *lock);
extern void raw_spin_unlock_irq(raw_spinlock_t *lock);
extern void raw_spin_lock_irqsave(raw_spinlock_t *lock, unsigned long *flags);
extern void raw_spin_unlock_irqrestore(raw_spinlock_t *lock, unsigned long flags);
extern void raw_spin_lock_init(raw_spinlock_t *lock);


#define spinlock_check(_lock)

#define spin_lock_init(_lock)                   \
    do                                          \
    {                                           \
        raw_spin_lock_init(&((_lock)->rlock));  \
    } while (0)

#define spin_lock(lock)                 raw_spin_lock(&((lock)->rlock))
#define spin_lock_bh(lock)              raw_spin_lock_bh(&((lock)->rlock))
#define spin_try_lock(lock)             raw_spin_trylock(&((lock)->rlock))
#define spin_lock_irq(lock)             raw_spin_lock_irq(&((lock)->rlock))
#define spin_lock_irqsave(lock, flag)   raw_spin_lock_irqsave(&(lock)->rlock, &flag)  

#define spin_unlock(lock)                   raw_spin_unlock(&((lock)->rlock))
#define spin_unlock_bh(lock)                raw_spin_unlock_bh(&((lock)->rlock))
#define spin_unlock_irq(lock)               raw_spin_unlock_irq(&((lock)->rlock))
#define spin_unlock_irqrestore(lock, flag)  raw_spin_unlock_irqrestore(&(lock)->rlock, flag)

#define SPIN_LOCK_UNLOCKED (spinlock_t) {0}

#define spin_acquire()

#endif


