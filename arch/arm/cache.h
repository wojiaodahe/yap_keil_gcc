#ifndef __ARMASM_CACHE_H__
#define __ARMASM_CACHE_H__

#define L1_CACHE_BYTES 32
#define L1_CACHE_ALIGN(x) (((x) + (L1_CACHE_BYTES - 1)) & ~(L1_CACHE_BYTES - 1))
#define SMP_CACHE_BYTES L1_CACHE_BYTES

#endif

