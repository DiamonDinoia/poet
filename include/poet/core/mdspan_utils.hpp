#ifndef POET_CORE_MDSPAN_UTILS_HPP
#define POET_CORE_MDSPAN_UTILS_HPP

/// \file mdspan_utils.hpp
/// \brief Minimal multidimensional array utilities for index flattening.
///
/// Provides compile-time utilities for working with multidimensional indices:
/// - Flattening N-dimensional indices to 1D
/// - Computing strides for contiguous layouts
/// - Bounds checking
///
/// This is a minimal internal utility, not a full mdspan implementation.

#include <array>
#include <cstddef>
#include <type_traits>

namespace poet::detail {

    /// \brief Compute the product of a sequence of integers at compile-time.
    template<int... Values>
    struct sequence_product : std::integral_constant<std::size_t, 1> {};

    template<int First, int... Rest>
    struct sequence_product<First, Rest...>
      : std::integral_constant<std::size_t,
          static_cast<std::size_t>(First) * sequence_product<Rest...>::value> {};

    /// \brief Compute total size (product of all dimensions).
    template<std::size_t N>
    constexpr auto compute_total_size(const std::array<std::size_t, N>& dims) -> std::size_t {
        std::size_t total = 1;
        for (std::size_t i = 0; i < N; ++i) {
            total *= dims[i];
        }
        return total;
    }

    /// \brief Compute strides for row-major (C-style) layout.
    ///
    /// For dimensions [D0, D1, D2], strides are [D1*D2, D2, 1].
    /// This allows flattening: flat_idx = i0*stride[0] + i1*stride[1] + i2*stride[2]
    template<std::size_t N>
    constexpr auto compute_strides(const std::array<std::size_t, N>& dims) -> std::array<std::size_t, N> {
        std::array<std::size_t, N> strides{};
        if constexpr (N > 0) {
            strides[N - 1] = 1;
            for (std::size_t i = N - 1; i > 0; --i) {
                strides[i - 1] = strides[i] * dims[i];
            }
        }
        return strides;
    }

    /// \brief Flatten N-dimensional indices to a single 1D index.
    ///
    /// Given indices (i0, i1, ..., in) and strides, computes:
    /// flat_index = i0*stride[0] + i1*stride[1] + ... + in*stride[n]
    template<std::size_t N>
    constexpr auto flatten_indices(const std::array<int, N>& indices,
                                   const std::array<std::size_t, N>& strides) -> std::size_t {
        std::size_t flat = 0;
        for (std::size_t i = 0; i < N; ++i) {
            flat += static_cast<std::size_t>(indices[i]) * strides[i];
        }
        return flat;
    }

    /// \brief Check if all indices are within bounds (after offset adjustment).
    template<std::size_t N>
    constexpr auto check_bounds(const std::array<int, N>& indices,
                               const std::array<int, N>& offsets,
                               const std::array<std::size_t, N>& dims) -> bool {
        for (std::size_t i = 0; i < N; ++i) {
            const int adjusted = indices[i] - offsets[i];
            if (adjusted < 0 || static_cast<std::size_t>(adjusted) >= dims[i]) {
                return false;
            }
        }
        return true;
    }

    /// \brief Adjust indices by subtracting offsets.
    template<std::size_t N>
    constexpr auto adjust_indices(const std::array<int, N>& indices,
                                 const std::array<int, N>& offsets) -> std::array<int, N> {
        std::array<int, N> adjusted{};
        for (std::size_t i = 0; i < N; ++i) {
            adjusted[i] = indices[i] - offsets[i];
        }
        return adjusted;
    }

}// namespace poet::detail

#endif// POET_CORE_MDSPAN_UTILS_HPP
