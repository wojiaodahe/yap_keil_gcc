
#include "page.h"
#include "mm.h"

#if 0
static struct fs_struct init_fs = INIT_FS;
static struct files_struct init_files = INIT_FILES;
static struct signal_struct init_signals = INIT_SIGNALS;
#endif

//static struct vm_area_struct init_mmap;
struct mm_struct init_mm;