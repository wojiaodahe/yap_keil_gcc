#ifndef __BITOPS_H__
#define __BITOPS_H__

extern int set_bit(int nr, unsigned long * addr);
extern int clear_bit(int nr, unsigned long * addr);
extern int test_bit(int nr, unsigned long * addr);
extern int test_and_change_bit(int nr, volatile unsigned char *map);
extern int test_and_set_bit(int nr, volatile unsigned char *map);
extern void change_bit(int nr, volatile unsigned char *map);

#endif
