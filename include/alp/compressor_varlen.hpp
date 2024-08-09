#ifndef ALP_COMPRESSOR_VARLEN_HPP
#define ALP_COMPRESSOR_VARLEN_HPP

#include "../../simpleffor/include/turbocompression.h"
#include "alp/encode.hpp"
#include "alp/rd.hpp"
#include "alp/state.hpp"
#include "alp/storer.hpp"
#include "alp/utils.hpp"
#include "fastlanes/ffor.hpp"
#include "helper_varlen.hpp"
#include "alp_c.h"

namespace alp {

/*
 * API Compressor
 */
template <class T, bool DRY = false>
struct AlpCompressor_varlen {

	using EXACT_TYPE = typename FloatingToExact<T>::type;

	state                  stt;
	storer::MemStorer<DRY> storer;

	T        input_vector[config::VECTOR_SIZE];
	T        exceptions[config::VECTOR_SIZE];
	T        sample_array[config::VECTOR_SIZE];
	int64_t  encoded_integers[config::VECTOR_SIZE];
	int64_t  alp_encoded_array[config::VECTOR_SIZE];
	uint16_t exceptions_rd[config::VECTOR_SIZE];
	uint16_t exceptions_position[config::VECTOR_SIZE];

	// 'right' & 'left' refer to the respective parts of the floating numbers after splitting (alprd)
	uint64_t   alp_bp_size;
	uint64_t   left_bp_size;
	uint64_t   right_bp_size;
	EXACT_TYPE right_parts[config::VECTOR_SIZE];
	EXACT_TYPE right_parts_encoded[config::VECTOR_SIZE];
	uint16_t   left_parts_encoded[config::VECTOR_SIZE];
	uint16_t   left_parts[config::VECTOR_SIZE];

	VECTOR_COMPRESS_TYPE store_type;

	EXACT_TYPE right_for_base = 0; // Always 0

	AlpCompressor_varlen() {}

	size_t get_size() { return storer.get_size(); }

	static size_t get_compressed_max_size(int32_t ndbl) {
		size_t header_max_size = sizeof(stt.right_bit_width) + sizeof(stt.left_bit_width) +
		                         sizeof(stt.actual_dictionary_size) +
		                         config::MAX_RD_DICTIONARY_SIZE * DICTIONARY_ELEMENT_SIZE_BYTES;
		return header_max_size + ((ndbl + config::VECTOR_SIZE - 1) / config::VECTOR_SIZE) * config::VECTOR_SIZE * sizeof(double);
	}

	void reset() {
		stt.scheme           = SCHEME::ALP;
		stt.vector_size      = config::VECTOR_SIZE;
		stt.exceptions_count = 0;
		stt.sampled_values_n = 0;
		stt.k_combinations   = 5;
		stt.right_bit_width  = 0;
		stt.left_bit_width   = 0;
		stt.right_for_base   = 0;
		stt.left_for_base    = 0;
		storer.buffer_offset = 0;
	}

	/*
	 * ALP Compression
	 * Note that Kernels of ALP and FFOR are not fused
	 */
	void compress_vector() {
		if (stt.scheme == SCHEME::ALP_RD) {
			compress_rd_vector();
		} else {
			compress_alp_vector();
		}
	}

	void pick_store_type() {
		if (stt.scheme == SCHEME::ALP) {
			AlpEncode<T>::encode(
			    input_vector, exceptions, exceptions_position, &stt.exceptions_count, encoded_integers, stt);
			AlpEncode<T>::analyze_ffor(encoded_integers, stt.bit_width, &stt.for_base);
			if (stt.vector_size * sizeof(double) <
			    expected_vector_compress_size() + ALP_VECTOR_MINIMUM_COMPRESS_GAIN) {
				store_type = VECTOR_COMPRESS_TYPE::COMPRESS_RAW;
			} else if (stt.vector_size < ALP_VECTOR_NOFILLING_SIZE) {
				store_type = VECTOR_COMPRESS_TYPE::COMPRESS_FOR;
			} else {
				store_type = VECTOR_COMPRESS_TYPE::COMPRESS_FASTLANE;
			}
		} else if (stt.scheme == SCHEME::ALP_RD) {
			AlpRD<T>::encode(
			    input_vector, exceptions_rd, exceptions_position, &stt.exceptions_count, right_parts, left_parts, stt);
			if (stt.vector_size * sizeof(double) <
			    expected_vector_compress_size() + ALP_VECTOR_MINIMUM_COMPRESS_GAIN) {
				store_type = VECTOR_COMPRESS_TYPE::COMPRESS_RAW;
			} else {
				store_type = VECTOR_COMPRESS_TYPE::COMPRESS_FASTLANE;
			}
		}
	}

	void compress_alp_vector() {
		if (store_type == VECTOR_COMPRESS_TYPE::COMPRESS_RAW) {
			alp_bp_size = stt.vector_size * sizeof(double);
		} else if (store_type == VECTOR_COMPRESS_TYPE::COMPRESS_FOR) {
			uint8_t* end = turbocompress64(reinterpret_cast<const uint64_t*>(encoded_integers),
			                               stt.vector_size,
			                               reinterpret_cast<uint8_t*>(alp_encoded_array));
			alp_bp_size  = end - (uint8_t*)alp_encoded_array;
		} else if (store_type == VECTOR_COMPRESS_TYPE::COMPRESS_FASTLANE) {
			ffor::ffor(encoded_integers, alp_encoded_array, stt.bit_width, &stt.for_base);
			alp_bp_size = AlpApiUtils<T>::get_size_after_bitpacking(stt.bit_width);
		}
	}

	void compress_rd_vector() {
		if (store_type == VECTOR_COMPRESS_TYPE::COMPRESS_RAW) {
			alp_bp_size = stt.vector_size * sizeof(double);
		} else if (store_type == VECTOR_COMPRESS_TYPE::COMPRESS_FASTLANE) {
			ffor::ffor(right_parts, right_parts_encoded, stt.right_bit_width, &right_for_base);
			ffor::ffor(left_parts, left_parts_encoded, stt.left_bit_width, &stt.left_for_base);
		}
	}

	void compress(T* values, size_t values_count, uint8_t* out) {
		storer                  = storer::MemStorer<DRY>(out);
		size_t rouwgroup_count  = AlpApiUtils<T>::get_rowgroup_count(values_count);
		size_t current_idx      = 0;
		size_t left_to_compress = values_count;
		for (size_t current_rowgroup = 0; current_rowgroup < rouwgroup_count; current_rowgroup++) {
			/*
			 * Rowgroup level
			 */
			AlpEncode<T>::init(values, current_idx, values_count, sample_array, stt);
			if (stt.scheme == SCHEME::ALP_RD) {
				AlpRD<T>::init(values, current_idx, values_count, sample_array, stt);
				left_bp_size  = AlpApiUtils<T>::get_size_after_bitpacking(stt.left_bit_width);
				right_bp_size = AlpApiUtils<T>::get_size_after_bitpacking(stt.right_bit_width);
			}
			store_rowgroup_metadata();

			size_t values_left_in_rowgroup = std::min(config::ROWGROUP_SIZE, left_to_compress);
			size_t vectors_in_rowgroup     = AlpApiUtils<T>::get_complete_vector_count(values_left_in_rowgroup);
			for (size_t vector_idx = 0; vector_idx < vectors_in_rowgroup; vector_idx++) {
				/*
				 * Vector level
				 */
				for (T& idx : input_vector) {
					idx = values[current_idx++];
				}
				pick_store_type();
				compress_vector();
				store_vector();
				left_to_compress -= config::VECTOR_SIZE;
			}
		}
		if (left_to_compress) { // Last vector which may be incomplete
			stt.vector_size = left_to_compress;
			for (size_t idx = 0; idx < left_to_compress; idx++) {
				input_vector[idx] = values[current_idx++];
			}
			if (stt.scheme == SCHEME::ALP_RD) {
				AlpApiUtils<T>::fill_incomplete_alprd_vector(input_vector, stt);
			} else {
				AlpApiUtils<T>::fill_incomplete_alp_vector(
				    input_vector, exceptions, exceptions_position, &stt.exceptions_count, encoded_integers, stt);
			}
			pick_store_type();
			compress_vector();
			store_vector();
		}
	};

	size_t expected_vector_compress_size() {
		size_t size = sizeof(store_type);
		if (stt.scheme == SCHEME::ALP_RD) {
			size += sizeof(stt.exceptions_count);
			size += left_bp_size;
			size += right_bp_size;
			size += stt.exceptions_count * (RD_EXCEPTION_SIZE_BYTES + RD_EXCEPTION_POSITION_SIZE_BYTES);
		} else {
			if (stt.vector_size < ALP_VECTOR_NOFILLING_SIZE) {
				size += sizeof(stt.exp);
				size += sizeof(stt.fac);
				size += sizeof(stt.exceptions_count);
				size += sizeof(alp_bp_size);
				size += 20 + stt.vector_size / 32 * 32 * stt.bit_width / 8 + stt.vector_size % 32 * sizeof(uint64_t);
				size += stt.exceptions_count * (Constants<T>::EXCEPTION_SIZE_BYTES + EXCEPTION_POSITION_SIZE_BYTES);
			} else {
				size += sizeof(stt.exp);
				size += sizeof(stt.fac);
				size += sizeof(stt.exceptions_count);
				size += sizeof(stt.for_base);
				size += sizeof(stt.bit_width);
				size += AlpApiUtils<T>::get_size_after_bitpacking(stt.bit_width);
				size += stt.exceptions_count * (Constants<T>::EXCEPTION_SIZE_BYTES + EXCEPTION_POSITION_SIZE_BYTES);
			}
		}
		return size;
	}

	void store_raw_vector() { storer.store((void*)input_vector, alp_bp_size); }

	void store_rd_vector() {
		storer.store((void*)&stt.exceptions_count, sizeof(stt.exceptions_count));
		storer.store((void*)left_parts_encoded, left_bp_size);
		storer.store((void*)right_parts_encoded, right_bp_size);
		if (stt.exceptions_count) {
			storer.store((void*)exceptions_rd, RD_EXCEPTION_SIZE_BYTES * stt.exceptions_count);
			storer.store((void*)exceptions_position, RD_EXCEPTION_POSITION_SIZE_BYTES * stt.exceptions_count);
		}
	}

	void store_alp_for_vector() {
		storer.store((void*)&stt.exp, sizeof(stt.exp));
		storer.store((void*)&stt.fac, sizeof(stt.fac));
		storer.store((void*)&stt.exceptions_count, sizeof(stt.exceptions_count));
		storer.store((void*)&alp_bp_size, sizeof(alp_bp_size));
		storer.store((void*)alp_encoded_array, alp_bp_size);
		if (stt.exceptions_count) {
			storer.store((void*)exceptions, Constants<T>::EXCEPTION_SIZE_BYTES * stt.exceptions_count);
			storer.store((void*)exceptions_position, EXCEPTION_POSITION_SIZE_BYTES * stt.exceptions_count);
		}
	}

	void store_alp_vector() {
		storer.store((void*)&stt.exp, sizeof(stt.exp));
		storer.store((void*)&stt.fac, sizeof(stt.fac));
		storer.store((void*)&stt.exceptions_count, sizeof(stt.exceptions_count));
		storer.store((void*)&stt.for_base, sizeof(stt.for_base));
		storer.store((void*)&stt.bit_width, sizeof(stt.bit_width));
		storer.store((void*)alp_encoded_array, alp_bp_size);
		if (stt.exceptions_count) {
			storer.store((void*)exceptions, Constants<T>::EXCEPTION_SIZE_BYTES * stt.exceptions_count);
			storer.store((void*)exceptions_position, EXCEPTION_POSITION_SIZE_BYTES * stt.exceptions_count);
		}
	}

	void store_schema() {
		uint8_t scheme_code = (uint8_t)stt.scheme;
		storer.store((void*)&scheme_code, sizeof(scheme_code));
	}

	void store_vector() {
		storer.store((void*)&store_type, sizeof(store_type));
		if (store_type == VECTOR_COMPRESS_TYPE::COMPRESS_RAW) {
			store_raw_vector();
		} else {
			if (stt.scheme == SCHEME::ALP_RD) {
				store_rd_vector();
			} else {
				if (store_type == VECTOR_COMPRESS_TYPE::COMPRESS_FOR) {
					store_alp_for_vector();
				} else {
					store_alp_vector();
				}
			}
		}
	}

	void store_rd_metadata() {
		storer.store((void*)&stt.right_bit_width, sizeof(stt.right_bit_width));
		storer.store((void*)&stt.left_bit_width, sizeof(stt.left_bit_width));
		storer.store((void*)&stt.actual_dictionary_size, sizeof(stt.actual_dictionary_size));
		storer.store((void*)stt.left_parts_dict, stt.actual_dictionary_size_bytes);
	}

	void store_rowgroup_metadata() {
		store_schema();
		if (stt.scheme == SCHEME::ALP_RD) { store_rd_metadata(); }
	}
};

} // namespace alp

#endif