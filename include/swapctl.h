#ifndef __SWAPCTL_H__
#define __SWAPCTL_H__

typedef struct freepages_v1
{
	unsigned int	min;
	unsigned int	low;
	unsigned int	high;
} freepages_v1;
typedef freepages_v1 freepages_t;
extern freepages_t freepages;

#endif