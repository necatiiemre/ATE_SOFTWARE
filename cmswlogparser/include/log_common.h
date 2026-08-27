#pragma once
#include <stdint.h>
#if _WIN32_WINNT < _WIN32_WINNT_WIN8
static int64_t ntohll(int64_t network)
{
	uint32_t checker = 0x10203040;
	char *a_byte = (char*) &checker;
	int64_t ret_val;
	char *data = (char *) &ret_val;
	if( 0x10 == *a_byte)
	{//big endian
		ret_val = network;
	}
	else
	{//little endian
		data[0] = network >> 56;
		data[1] = network >> 48;
		data[2] = network >> 40;
		data[3] = network >> 32;
		data[4] = network >> 24;
		data[5] = network >> 16;
		data[6] = network >> 8;
		data[7] = network >> 0;
	}
	return ret_val;
}
#endif