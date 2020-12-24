#include "lib.h"

void memset(void *src, unsigned char num, unsigned int len)
{
    unsigned char *tmp;
	int i = 0;
    
    if (!src || !len)
        return;
    tmp = src;
	for (i = 0; i < len; i++)
	{
		*tmp++ = num;
	}
}

void *memcpy(void *dest, void *src, unsigned int len)
{
    char *d = dest;
    char *s = src;

    if (!dest || !src || !len)
        return dest;

    while (len--)
        *d ++ = *s++;

    return dest;
}

int strncmp(char *s1, char *s2, unsigned int n)
{
    if (!s1 || !s2 || !n)
        return 0;

    while (*s1 && *s2 && (*s1 == *s2))
    {
       s1++;
       s2++;
       n--;
       if (n == 0)
    	   return 0;
    }

    return (*s1 - *s2);
}

int strcmp(char *s1, char *s2)
{
    if (!s1 || !s2)
        return 0;

    while (*s1 && *s2 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return (*s1 - *s2);
}

char *strcpy(char *dst, char *src)
{
    char *ret = dst;

    while ((*dst++ = *src++) != '\0')
        ;

    return ret;
}

char *strncpy(char *dest, const char *src, int len)
{
    int i = 0;
    char *temp = dest;

    while (i++ < len && (*temp++ = *src++) != '\0')
    {
    }

    if (*(temp) != '\0')
        *temp = '\0';

    return dest;
}

unsigned int strlen (const char * str)
{      
        const char *eos = str;
 
        while( *eos++ ) ;
 
        return( eos - str - 1 );
}