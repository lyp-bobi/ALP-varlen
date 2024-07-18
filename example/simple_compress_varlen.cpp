#include "alp.hpp"
#include "helper.hpp"
#include <cassert>

int main() {
	srand((unsigned int)time(NULL));
	size_t tuples_count = rand()*rand()%8092;
	size_t out_buffer_size =
	    (tuples_count * sizeof(double)) + 8096; // We leave some headroom in case of negative compression
	size_t uncompressed_size = tuples_count * sizeof(double);

	double  in[tuples_count];
	uint8_t out[out_buffer_size];
	example::fill_random_data<double>(in, tuples_count, 2);

	/*
	 * Compress
	 */
	alp::AlpCompressor_varlen compressor = alp::AlpCompressor_varlen<double>();
	printf("Compressing with ALP...\n");
	compressor.compress(in, tuples_count, out);
	size_t compressed_size   = compressor.get_size();
	double compression_ratio = (double)uncompressed_size / compressed_size;
	printf("Tuples count: %zu\n", tuples_count);
	printf("Uncompressed size (in bytes): %zu\n", uncompressed_size);
	printf("Compressed size (in bytes): %zu\n", compressed_size);
	printf("Compression Ratio: %f\n\n", compression_ratio);

	/*
	 * Decompress
	 */
	size_t decompressed_buffer_size =
	    alp::AlpApiUtils<double>::align_value<size_t, alp::config::VECTOR_SIZE>(tuples_count);
	double               decompressed[decompressed_buffer_size];
	alp::AlpDecompressor_varlen decompressor = alp::AlpDecompressor_varlen<double>();
	printf("Decompressing with ALP...\n");
	decompressor.decompress(out, tuples_count, decompressed);

	/*
	 * Validity Test
	 */
	for (size_t i = 0; i < tuples_count; i++) {
		assert(in[i] == decompressed[i]);
	}
	printf("OK\n");

	return 0;
}
