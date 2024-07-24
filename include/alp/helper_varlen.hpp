#ifndef ALP_HELPER_VARLEN_HPP
#define ALP_HELPER_VARLEN_HPP

#include <cstddef>

namespace alp {

enum class VECTOR_COMPRESS_TYPE : uint8_t { COMPRESS_RAW, COMPRESS_FOR, COMPRESS_FASTLANE };
}

/*
 * ALP Configs
 */
namespace alp::config {
/// ALP size where we don't use fastlanes but original FFOR to minize the compression size
inline constexpr size_t VECTOR_NOFILLING_SIZE = 512;

// We store the vector directly if the gain of compress less than this value
inline constexpr size_t VECTOR_MINIMUM_COMPRESS_GAIN = 256;
} // namespace alp::config

#endif