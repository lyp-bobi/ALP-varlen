#ifndef ALP_CONFIG_VARLEN_HPP
#define ALP_CONFIG_VARLEN_HPP

#include <cstddef>

/*
 * ALP Configs
 */
namespace alp::config {
/// ALP size where we don't use fastlanes but original FFOR to minize the compression size
inline constexpr size_t VECTOR_NOFILLING_SIZE = 512;

/// ALP size where we store input directly
inline constexpr size_t VECTOR_NO_COMPRESS_SIZE = 32;
} // namespace alp::config

#endif