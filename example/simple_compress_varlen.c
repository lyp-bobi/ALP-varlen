#include "alp_c.h"
#include "stdlib.h"
#include "time.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

double generate_random_double(double min, double max, int precision) {
	double scale = rand() / (double) RAND_MAX; // 随机生成一个[0, 1]之间的浮点数
	double number = min + scale * (max - min); // 将其缩放到[min, max]之间
	double factor = 1.0f;
	for (int i = 0; i < precision; i++) {
		factor *= 10.0f; // 精度控制
	}
	return ((long long)(number * factor)) / factor; // 保留指定精度
}

// 生成指定长度和精度的随机浮点数数组
void generate_random_double_array(double *array, int length, double min, double max, int precision) {
	for (int i = 0; i < length; i++) {
		array[i] = generate_random_double(min, max, precision);
	}
}

int main() {
	srand((unsigned int)time(NULL));
	for (int tcount = 0; tcount < 100; tcount++) {
		size_t                    tuples_count    = rand() * rand() % 20480;
		size_t                    out_buffer_size = alp_compress_maxsize(tuples_count);
		; // the negative compression is at most 1
		size_t uncompressed_size = tuples_count * sizeof(double);

		double  in[tuples_count];
		uint8_t out[out_buffer_size];
		int     prec = rand() % 10;
		generate_random_double_array(in, tuples_count, 50,150, prec);
		/*
		 * Compress
		 */
		printf("Precision: %d.\n", prec);
		printf("Compressing with ALP...\n");
		int compressed_size;
		alp_compress(in, tuples_count, out, &compressed_size);
		double compression_ratio       = (double)uncompressed_size / compressed_size;
		printf("Tuples count: %zu\n", tuples_count);
		printf("Uncompressed size (in bytes): %zu\n", uncompressed_size);
		printf("Compressed size (in bytes): %zu\n", compressed_size);
		printf("Compression Ratio: %f\n\n", compression_ratio);

		/*
		 * Decompress
		 */
		size_t decompressed_buffer_size = alp_decompress_maxsize(tuples_count);
		double                      decompressed[decompressed_buffer_size];

		printf("Decompressing with ALP...\n");
		alp_decompress(out, tuples_count, decompressed);


		/*
		 * Validity Test
		 */
		for (size_t i = 0; i < tuples_count; i++) {
			if (in[i] != decompressed[i]) {
				printf("wrong at %d: %f vs %f", i, in[i], decompressed[i]);
			}
			assert(in[i] == decompressed[i]);
		}
		printf("OK\n");
	}
	return 0;
}
