#include <stdio.h>

void __pte_error(const char *file, int line, unsigned long val)
{
	printf("%s:%d: bad pte %08lx.\n", file, line, val);
}

void __pmd_error(const char *file, int line, unsigned long val)
{
	printf("%s:%d: bad pmd %08lx.\n", file, line, val);
}

void __pgd_error(const char *file, int line, unsigned long val)
{
	printf("%s:%d: bad pgd %08lx.\n", file, line, val);
}