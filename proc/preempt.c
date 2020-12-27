#include "proc.h"
#include "sched.h"


inline void add_preempt_count(unsigned int val)
{   
    current->preempt_count += val;
}

inline void sub_preempt_count(unsigned int val)
{
    if (current->preempt_count >= val)
        current->preempt_count -= val;
    else 
        current->preempt_count = 0;
}
