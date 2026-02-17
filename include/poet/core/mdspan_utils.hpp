#ifndef POET_CORE_MDSPAN_UTILS_HPP
#define POET_CORE_MDSPAN_UTILS_HPP

/// \file mdspan_utils.hpp
/// \brief Multidimensional index utilities for N-D dispatch table generation.
///
/// Provides row-major stride computation and total-size calculation used by
/// the N-D function-pointer-table dispatch in static_dispatch.hpp.

#include <array>
#include <cstddef>
#include <poet/core/macros.hpp>

namespace poet::detail {

/// Total size (product of all dimensions).
template<std::size_t N>
POET_FORCEINLINE POET_CPP23_CONSTEXPR auto compute_total_size(const std::array<std::size_t, N> &dims) -> std::size_t {
    std::size_t total = 1;
    for (std::size_t i = 0; i < N; ++i) { total *= dims[i]; }
    return total;
}

/// Compute row-major strides. stride[i] = product of dims[i+1..N-1].
template<std::size_t N>
POET_FORCEINLINE POET_CPP23_CONSTEXPR auto compute_strides(const std::array<std::size_t, N> &dims)
  -> std::array<std::size_t, N> {
    std::array<std::size_t, N> strides{};
    if constexpr (N > 0) {
        strides[N - 1] = 1;
        for (std::size_t i = N - 1; i > 0; --i) { strides[i - 1] = strides[i] * dims[i]; }
    }
    return strides;
}

}// namespace poet::detail

#endif// POET_CORE_MDSPAN_UTILS_HPP
