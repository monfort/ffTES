#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MIN(a,b) ((a)<(b)?(a):(b))

#ifdef __GNUC__
#define _alloca alloca
#endif

int hexToBin(char hex)
{
	if (hex >= '0' && hex <= '9')
		return hex - '0';
	else if (hex >= 'A' && hex <= 'F')
		return hex - 'A' + 10;
	else if (hex >= 'a' && hex <= 'f')
		return hex - 'a' + 10;
	else
		return -1;
}

int hexStringToBin(unsigned char *binArray, const char *hexString)
{
	for (;*hexString; binArray++)
	{
		int x = hexToBin(*hexString);
		if (x < 0)
			return 0;
		*binArray = x << 4;
		hexString++;
		if (!*hexString)
			break;
		x = hexToBin(*hexString);
		if (x < 0)
			return 0;
		*binArray |= x;
		hexString++;
	}
	return 1;
}

// http://aggregate.org/MAGIC/#Population%20Count%20%28Ones%20Count%29
unsigned int ones32(register unsigned int x)
{
		/* 32-bit recursive reduction using SWAR...
		but first step is mapping 2-bit values
		into sum of 2 1-bit values in sneaky way
		*/
		x -= ((x >> 1) & 0x55555555);
		x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
		x = (((x >> 4) + x) & 0x0f0f0f0f);
		x += (x >> 8);
		x += (x >> 16);
		return(x & 0x0000003f);
}

double normalizedHammingDistance(const unsigned char *a, const unsigned char *b, unsigned int bitCount)
{
	unsigned int offset = 0;
	unsigned int dist = 0;
	unsigned int *pa = (unsigned int*)a;
	unsigned int *pb = (unsigned int*)b;
	unsigned int bytesLeft = bitCount/8;
	unsigned int xa = 0;
	unsigned int xb = 0;
	unsigned int remainderBits = bitCount&7;
	assert(sizeof(unsigned int) == 4);
	while (bytesLeft >= 4)
	{
		unsigned int wa = *pa++;
		unsigned int wb = *pb++;
		dist += ones32(wa ^ wb);
		offset += 4;
		bytesLeft -= 4;
	}
	memcpy(&xa, a+offset, bytesLeft + (remainderBits?1:0));
	memcpy(&xb, b+offset, bytesLeft + (remainderBits?1:0));
	if (remainderBits)
	{
		((unsigned char*)&xa)[bytesLeft] &= (1 << remainderBits)-1;
		((unsigned char*)&xb)[bytesLeft] &= (1 << remainderBits)-1;
	}
	dist += ones32(xa ^ xb);		
	
	return (double)dist / (double)bitCount;	
}

double handleInput(const char* a, const char* b, const char *c)
{
	int bitcount,len;
	unsigned char *str1,*str2;

	bitcount = atoi(c);
	if (bitcount == 0)
	{
		fprintf(stderr, "invalid bitcount %s\n", c);
		return -1;
	}

	len = strlen(a)/2 + strlen(a)%2;
	bitcount = MIN(bitcount, len*8);
	str1 = _alloca(len);
	memset(str1, 0, len);
	if (!hexStringToBin(str1, a))
	{
		fprintf(stderr, "invalid hex string %s\n", a);
		return -1;
	}

	len = strlen(b)/2 + strlen(b)%2;
	bitcount = MIN(bitcount, len*8);
	str2 = _alloca(len);
	memset(str2, 0, len);
	if (!hexStringToBin(str2, b))
	{
		fprintf(stderr, "invalid hex string %s\n", b);
		return -1;
	}

	return normalizedHammingDistance(str1, str2, bitcount);
}

int main(int argc, const char **argv)
{
	int readStdIn = 1;

	if (argc > 1)
	{
		readStdIn = 0;
		if (argc != 4)
		{
			printf("usage: %s hexsting1 hexstring2 bitcount\n", argv[0]);
			return 1;
		}
	}

	// Handle command line input
	if (!readStdIn)
	{
		double res = handleInput(argv[1], argv[2], argv[3]);
		if (res >= 0)
		{
			printf("%f\n", res);
			return 0;
		}
		return 1;
	}

	// Handle stdin input
	for (;;)
	{
		char a[1024],b[1024],c[1024];
		double res;
		if (fscanf(stdin, "%s %s %s", a, b, c) != 3)
			return 0;
		res = handleInput(a, b, c);
		if (res >= 0)
		{
			printf("%f\n", res);
			fflush(stdout);
		}
	}

	return 0;
}
