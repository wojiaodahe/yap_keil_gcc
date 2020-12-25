#ifndef __KERNEL_LIB_H__
#define __KERNEL_LIB_H__

extern void memset(void *src, unsigned char num, unsigned int len);
extern void *memcpy(void *dest, void *src, unsigned int len);
extern int strncmp(char *s1, char *s2, unsigned int n);
extern int strcmp(char *s1, char *s2);
extern char *strcpy(char *dst, char *src);
extern char *strncpy(char *dest, const char *src, int len);
extern unsigned int strlen (const char * str);
#endif

