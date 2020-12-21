

int set_bit(int nr,long * addr)
{
	int	mask, retval;

	addr += nr >> 5;
	mask = 1 << (nr & 0x1f);
	//cli();
	retval = (mask & *addr) != 0;
	*addr |= mask;
	//sti();
	return retval;
}

int clear_bit(int nr, long * addr)
{
	int	mask, retval;

	addr += nr >> 5;
	mask = 1 << (nr & 0x1f);
	//cli();
	retval = (mask & *addr) != 0;
	*addr &= ~mask;
	//sti();
	return retval;
}

int test_bit(int nr, long * addr)
{
	int	mask;

	addr += nr >> 5;
	mask = 1 << (nr & 0x1f);
	return ((mask & *addr) != 0);
}

int test_and_change_bit(int nr, volatile unsigned char *map)
{
    unsigned char oldvalue;

    oldvalue = *(map + (nr >> 3));

    *(map + (nr >> 3)) = oldvalue ^ (1 << (nr & 0x07));

    return oldvalue  & (1 << (nr & 0x07));
}

void change_bit(int nr, volatile unsigned char *map)
{
    unsigned char oldvalue;

    oldvalue = *(map + (nr >> 3));

    *(map + (nr >> 3)) = oldvalue ^ (1 << (nr & 0x07));
}

int test_and_set_bit(int nr, volatile unsigned char *map)
{
    unsigned char oldvalue;

    oldvalue = *(map + (nr >> 3));

    *(map + (nr >> 3)) |= (1 << (nr & 0x07));

    return oldvalue  & (1 << (nr & 0x07));
}