#ifndef __AOUT_H__
#define __AOUT_H__

#include "page.h"

#define OMAGIC 0407
#define NMAGIC 0410
#define ZMAGIC 0413
#define QMAGIC 0314

#define N_MAGIC(exec) ((exec).a_info & 0xffff)

#define N_TXTADDR(x) (N_MAGIC(x) == QMAGIC ? PAGE_SIZE : 0)

#endif