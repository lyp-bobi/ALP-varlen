#include "alp_c.h"
#include "alp.hpp"

int ALP_VECTOR_NOFILLING_SIZE    = 512;
int ALP_VECTOR_MINIMUM_COMPRESS_GAIN = 128;

int alp_compress_maxsize(int num_dbl)
{
	return alp::AlpCompressor_varlen<double>::get_compressed_max_size(num_dbl);
}

int alp_decompress_maxsize(int num_dbl)
{
	return alp::AlpDecompressor_varlen<double>::get_decompressed_max_size(num_dbl);
}

void alp_compress(double* input, int num_dbl, char* output, int *output_size)
{
	alp::AlpCompressor_varlen<double> cps;
	cps.compress(input, num_dbl, reinterpret_cast<uint8_t*>(output));
	*output_size = cps.get_size();
}

void alp_decompress(char* input, int num_dbl, double* output)
{
	alp::AlpDecompressor_varlen<double> dcps;
	dcps.decompress(reinterpret_cast<uint8_t*>(input), num_dbl, output);
}

ALPCompressor alp_compress_cached(double* input, int num_dbl, char* output, int *output_size, ALPCompressor cached_cps)
{
	if (cached_cps == NULL)
	{
		cached_cps = (void*) new alp::AlpCompressor_varlen<double>();
	}
	alp::AlpCompressor_varlen<double> *cps = static_cast<alp::AlpCompressor_varlen<double>*>(cached_cps);
	cps->compress(input, num_dbl, reinterpret_cast<uint8_t*>(output));
	*output_size = cps->get_size();
	cps->reset();
	return cached_cps;
}

ALPDecompressor alp_decompress_cached(char* input, int num_dbl, double* output, ALPDecompressor cached_dcps)
{
	if (cached_dcps == NULL)
	{
		cached_dcps = (void*) new alp::AlpDecompressor_varlen<double>();
	}
	alp::AlpDecompressor_varlen<double> *dcps = static_cast<alp::AlpDecompressor_varlen<double>*>(cached_dcps);
	dcps->decompress(reinterpret_cast<uint8_t*>(input), num_dbl, output);
	dcps->reset();
	return cached_dcps;
}
