#include <stdio.h>
#include <math.h>

#include "decoder.h"

#define MAX_DURATION_SEC 4096

int		g_sample_rate = 22050;
char*		g_filename;

void ReadParams(int argc, char* argv[])
{
	if ( argc != 2 || strlen(argv[1]) == 0) {
		printf("usage: <audio file>\n");
		exit(1);
	}
	g_filename = argv[1];
}

void CalcTES(DecoderState* input, int sampling_rate, unsigned char output_bits[], int* bits_size)
{
	int two_seconds = 2 * sampling_rate;

	unsigned char frame[two_seconds];
	double lookup[two_seconds + 1]; // +1 because the range is [0..two_seconds] inclusive

	// Prepare lookup table for num of occurances 
	lookup[0] = 0.0; // 0 occurences don't contribute to histogram
	int i;
	for ( i = 1 ; i < two_seconds + 1 ; i++ )
	{
		double i_over_N = (double)i / (double)two_seconds;
		lookup[i] = - i_over_N * log(i_over_N); // note the minus
	}

	// Run over first second and remove leading silence (if thre's no leading silense this will loose a single sample, oh, well).
	for  ( i = sampling_rate; i; i--)
	{
		unsigned char sample;
		if (deocoderGet(input, &sample, sizeof(sample)) != 1)
		{
			decoderFree(input);
			printf("input file too short\n");
			exit(1);
		}
		// If input value isn't silence then break
		if (sample != 0x80 && sample != 0x7F && sample != 0x81)
			break;
	}

	// Fill out first frame
	if (deocoderGet(input, (unsigned char*)&frame[0], two_seconds) != two_seconds)
	{
		decoderFree(input);
		printf("input file too short\n");
		exit(1);
	}


	// Calc histogram for first frame
	int histogram[256];
	memset(histogram, 0, sizeof(histogram));
	for ( i = 0 ; i < two_seconds ; i++ )
		histogram[frame[i]]++;
	
	// Calc entropy of first frame using histogram
	double entropy = 0.0;
	for ( i = 0 ; i < 256 ; i++ )
		entropy += lookup[histogram[i]];

	// Now we use the frame buffer as cyclic buffer;
	double prev_entropy = entropy;
	unsigned int cyclic_index = 0; // head/tail of cyclic buffer
	int num_in_frame = 0;
	int bit_count = 0;
	while ( input )
	{		
		unsigned char in_sample;
		if (deocoderGet(input, &in_sample, 1) != 1)
		{
			decoderFree(input);
			input = NULL;
			continue;
		}
		unsigned char out_sample = frame[cyclic_index];
		frame[cyclic_index] = in_sample;
		cyclic_index = (cyclic_index + 1) % two_seconds;		
	
		// Histogram for in and out samples is no longer current, so remove contributions of old values
		entropy -= lookup[histogram[out_sample]];
		entropy -= lookup[histogram[in_sample]];

		// Update histogram values for in and out samples
		histogram[out_sample]--;
		histogram[in_sample]++;

		// Now use updated histogram to update the entropy
		entropy += lookup[histogram[out_sample]];
		entropy += lookup[histogram[in_sample]];

		num_in_frame++;

		// Output new entropy value every second
		if ( num_in_frame == sampling_rate )
		{
			num_in_frame = 0;
			unsigned char bit = entropy > prev_entropy ? 1 : 0;
			prev_entropy = entropy;
			output_bits[bit_count] = bit;
			bit_count++;
		}		
	}
	*bits_size = bit_count - 1;
}

void PrintBits(const unsigned char bits[], int bits_size)
{
	char table[16 + 1] = "0123456789ABCDEF";
	unsigned int x = 0;
	unsigned int shift = 0;
	printf("%d ", bits_size);
	unsigned int i;
	for ( i = 0 ; i < bits_size ; i++ )
	{
		x |= bits[i] << shift;
		shift++;
		if ( shift == 4 || (i + 1) == bits_size ) 
		{
			printf("%c", table[x]);
			shift = 0;
			x = 0;
		}
	}
	printf("\n");
}

int main(int argc, char* argv[])
{
	ReadParams(argc, argv);
	DecoderState *input = decoderInit(g_filename);
	if (!input){
		printf("Invalid input file\n");
		exit(1);
	}
	unsigned char bits[MAX_DURATION_SEC];
	int bits_size = 0;
	CalcTES(input, g_sample_rate, bits, &bits_size);
	PrintBits(bits, bits_size);
	return 0;
}
