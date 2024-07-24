#include "alp.hpp"
#include "helper.hpp"
#include <cassert>

int main() {
	srand((unsigned int)time(NULL));
	for (int tcount = 0; tcount < 100; tcount++) {
		size_t                    tuples_count    = rand() * rand() % 20480;
		alp::AlpCompressor_varlen compressor      = alp::AlpCompressor_varlen<double>();
		size_t                    out_buffer_size = compressor.get_compressed_max_size(tuples_count);
		; // the negative compression is at most 1
		size_t uncompressed_size = tuples_count * sizeof(double);

		double  in[tuples_count];
		uint8_t out[out_buffer_size];
		int     prec = rand() % 30;
		example::fill_random_data<double>(in, tuples_count, prec);
		/*
		 * Compress
		 */
		printf("Precision: %d.\n", prec);
		printf("Compressing with ALP...\n");
		compressor.compress(in, tuples_count, out);
		size_t expected_compresss_size = compressor.get_compressed_max_size(tuples_count);
		size_t compressed_size         = compressor.get_size();
		double compression_ratio       = (double)uncompressed_size / compressed_size;
		printf("Tuples count: %zu\n", tuples_count);
		printf("Uncompressed size (in bytes): %zu\n", uncompressed_size);
		printf("Expected Compressed size (in bytes): %zu\n", expected_compresss_size);
		printf("Compressed size (in bytes): %zu\n", compressed_size);
		assert(expected_compresss_size > compressed_size);
		printf("Compression Ratio: %f\n\n", compression_ratio);

		/*
		 * Decompress
		 */
		size_t decompressed_buffer_size =
		    alp::AlpApiUtils<double>::align_value<size_t, alp::config::VECTOR_SIZE>(tuples_count);
		double                      decompressed[decompressed_buffer_size];
		alp::AlpDecompressor_varlen decompressor = alp::AlpDecompressor_varlen<double>();
		printf("Decompressing with ALP...\n");
		decompressor.decompress(out, tuples_count, decompressed);

		/*
		 * Validity Test
		 */
		for (size_t i = 0; i < tuples_count; i++) {
			if (in[i] != decompressed[i]) { printf("wrong at %d: %f vs %f", i, in[i], decompressed[i]); }
			assert(in[i] == decompressed[i]);
		}
		printf("OK\n");
	}
	return 0;
}
