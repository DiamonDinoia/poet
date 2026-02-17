#ifndef POET_CORE_MDSPAN_UTILS_HPP
#define POET_CORE_MDSPAN_UTILS_HPP

/// \file mdspan_utils.hpp
/// \brief Minimal multidimensional index flattening utilities.
/// Disabled if C++23 std::mdspan is available.

// Disable if std::mdspan is available (C++23)
#if __cplusplus >= 202302L && __has_include(<mdspan>)
    // C++23 std::mdspan is available, skip POET's utilities
    #define POET_HAS_STD_MDSPAN 1 // NOLINT(cppcoreguidelines-macro-usage)
#else
    #define POET_HAS_STD_MDSPAN 0 // NOLINT(cppcoreguidelines-macro-usage)
#endif

#if !POET_HAS_STD_MDSPAN

#include <array>
#include <cstddef>
#include <poet/core/macros.hpp>

namespace poet::detail {

    /// Total size (product of all dimensions).
    template<std::size_t N>
    POET_FORCEINLINE POET_CPP23_CONSTEXPR auto compute_total_size(const std::array<std::size_t, N>& dims) -> std::size_t {
        std::size_t total = 1;
        for (std::size_t i = 0; i < N; ++i) {
            total *= dims[i];
        }
        return total;
    }

    /// Compute row-major strides. stride[i] = product of dims[i+1..N-1].
    template<std::size_t N>
    POET_FLATTEN POET_FORCEINLINE POET_CPP23_CONSTEXPR auto compute_strides(const std::array<std::size_t, N>& dims) -> std::array<std::size_t, N> {
        std::array<std::size_t, N> strides{};
        if constexpr (N > 0) {
            strides[N - 1] = 1;
            for (std::size_t i = N - 1; i > 0; --i) {
                strides[i - 1] = strides[i] * dims[i];
            }
        }
        return strides;
    }

    /// Flatten N-D indices to 1D using strides.
    /// Formula: flat = i0*stride[0] + i1*stride[1] + ... + i(N-1)*stride[N-1]
    template<std::size_t N>
    POET_FLATTEN POET_FORCEINLINE POET_CPP23_CONSTEXPR auto flatten_indices(
        const std::array<int, N>& indices,
        const std::array<std::size_t, N>& strides) -> std::size_t {
        std::size_t flat = 0;
        for (std::size_t i = 0; i < N; ++i) {
            flat += static_cast<std::size_t>(indices[i]) * strides[i];
        }
        return flat;
    }

    /// Check if indices are within bounds after offset adjustment.
    /// Returns true if all: 0 <= (indices[i] - offsets[i]) < dims[i]
    template<std::size_t N>
    POET_FORCEINLINE POET_CPP23_CONSTEXPR auto check_bounds(
        const std::array<int, N>& indices,
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

    /// Adjust indices by subtracting offsets: adjusted[i] = indices[i] - offsets[i]
    template<std::size_t N>
    POET_FLATTEN POET_FORCEINLINE POET_CPP23_CONSTEXPR auto adjust_indices(
        const std::array<int, N>& indices,
        const std::array<int, N>& offsets) -> std::array<int, N> {
        std::array<int, N> adjusted{};
        for (std::size_t i = 0; i < N; ++i) {
            adjusted[i] = indices[i] - offsets[i];
        }
        return adjusted;
    }

}// namespace poet::detail

#endif // !POET_HAS_STD_MDSPAN

#endif// POET_CORE_MDSPAN_UTILS_HPP
