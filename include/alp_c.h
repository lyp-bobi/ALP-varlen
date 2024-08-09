#ifndef ALP_ALP_C_H
#define ALP_ALP_C_H


#ifdef __cplusplus
extern "C"
{
#endif

typedef void* ALPCompressor;

typedef void* ALPDecompressor;

// ALP size where we don't use fastlanes but original FFOR to minize the compression size
extern int ALP_VECTOR_NOFILLING_SIZE;

// We store the vector directly if the gain of compress less than this value
extern int ALP_VECTOR_MINIMUM_COMPRESS_GAIN;

/**
 * @brief get the minimum required buffer size for compressing
 * @param num_dbl
 * @return
 */
int alp_compress_maxsize(int num_dbl);

/**
 * @brief get the minimum required buffer size for decompressing
 * @param num_dbl
 * @return
 */
int alp_decompress_maxsize(int num_dbl);

/**
 * @brief compress a double vector into binary
 * @param input
 * @param num_dbl
 * @param output
 * @param output_size
 */
void alp_compress(double* input, int num_dbl, char* output, int *output_size);

/**
 * @brief decompress a binary into double vector
 * @param input
 * @param num_dbl
 * @param output
 */
void alp_decompress(char* input, int num_dbl, double* output);

/**
 * @brief use an existing compressor to avoid allocate a new one each time
 * @param input
 * @param num_dbl
 * @param output
 * @param output_size
 * @param cached_cps
 * @return
 */
ALPCompressor alp_compress_cached(double* input, int num_dbl, char* output, int *output_size, ALPCompressor cached_cps);

/**
 * @brief use an existing decompressor to avoid allocate a new one each time
 * @param input
 * @param num_dbl
 * @param output
 * @param cached_dcps
 * @return
 */
ALPDecompressor alp_decompress_cached(char* input, int num_dbl, double* output, ALPDecompressor cached_dcps);

#ifdef __cplusplus
}
#endif

#endif // ALP_ALP_C_H
