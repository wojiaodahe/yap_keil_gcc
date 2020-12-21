#ifndef __ARM_CPU_H__
#define __ARM_CPU_H__

#define cpu_set_pgd			cpu_fn(CPU_NAME, _set_pgd)
#define cpu_set_pmd			cpu_fn(CPU_NAME, _set_pmd)
#define cpu_set_pte			cpu_fn(CPU_NAME, _set_pte)

#endif

