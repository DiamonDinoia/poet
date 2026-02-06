#ifndef POET_CORE_MDSPAN_UTILS_HPP
#define POET_CORE_MDSPAN_UTILS_HPP

/// \file mdspan_utils.hpp
/// \brief Minimal multidimensional array utilities for index flattening.
///
/// Provides compile-time utilities for working with multidimensional indices:
/// - Flattening N-dimensional indices to 1D
/// - Computing strides for contiguous layouts
/// - Bounds checking with offset support
/// - Index adjustment for offset arrays
///
/// **Layout Assumption: Row-Major (C-style)**
/// All functions assume row-major (C-style) memory layout where the rightmost
/// index varies fastest. For a 3D array [D0][D1][D2]:
/// - Element [i][j][k] is at flat index: i*D1*D2 + j*D2 + k
/// - Strides are: [D1*D2, D2, 1]
/// - Fortran (column-major) layout is NOT supported
///
/// **Design Rationale:**
/// - **int for indices**: Allows negative offsets for offset arrays (e.g.,
///   array[-5:5][-10:10] in mathematical notation)
/// - **size_t for dimensions**: Dimensions are always non-negative, using
///   size_t avoids unnecessary signed/unsigned conversions
/// - **Minimal utility**: This is NOT a full mdspan implementation - it provides
///   only the bare minimum needed for POET's N-D dispatch table indexing
///
/// **Thread Safety:** All functions are constexpr and thread-safe (no mutable state).
///
/// **Usage Pattern:**
/// ```cpp
/// // Define 3D array dimensions [10][20][30]
/// std::array<std::size_t, 3> dims = {10, 20, 30};
/// std::array<std::size_t, 3> strides = compute_strides(dims);
///
/// // Access element [5][10][15]
/// std::array<int, 3> indices = {5, 10, 15};
/// std::size_t flat_idx = flatten_indices(indices, strides);
/// // flat_idx = 5*600 + 10*30 + 15 = 3315
///
/// // With offsets (e.g., indices start at [-5][-10][-15])
/// std::array<int, 3> offsets = {-5, -10, -15};
/// std::array<int, 3> runtime_indices = {0, 0, 0};  // User-facing indices
/// if (check_bounds(runtime_indices, offsets, dims)) {
///     auto adjusted = adjust_indices(runtime_indices, offsets);
///     std::size_t idx = flatten_indices(adjusted, strides);
///     // Access array[idx]
/// }
/// ```

#include <array>
#include <cstddef>
#include <poet/core/macros.hpp>
#include <type_traits>

namespace poet::detail {

    /// \brief Compute the product of a sequence of integers at compile-time.
    ///
    /// **Metaprogramming utility** for computing the product of dimension sizes
    /// at compile-time. Used internally for static dispatch table sizing.
    ///
    /// **How it works:**
    /// Recursive template specialization that multiplies the first value by the
    /// product of the rest. Base case (empty sequence) returns 1.
    ///
    /// \tparam Values Variadic template parameter pack of integer dimension sizes
    ///
    /// **Example:**
    /// ```cpp
    /// // Compute 3 * 4 * 5 = 60 at compile-time
    /// constexpr std::size_t size = sequence_product<3, 4, 5>::value;
    /// static_assert(size == 60);
    ///
    /// // Empty product is 1 (identity element)
    /// constexpr std::size_t empty = sequence_product<>::value;
    /// static_assert(empty == 1);
    /// ```
    ///
    /// **Note:** This is a metaprogramming helper evaluated entirely at compile-time.
    /// No runtime code is generated. Inherits from std::integral_constant so the
    /// result can be used in template parameters and constexpr contexts.
    template<int... Values>
    struct sequence_product : std::integral_constant<std::size_t, 1> {};

    template<int First, int... Rest>
    struct sequence_product<First, Rest...>
      : std::integral_constant<std::size_t,
          static_cast<std::size_t>(First) * sequence_product<Rest...>::value> {};

    /// \brief Compute total size (product of all dimensions).
    ///
    /// Calculates the total number of elements in an N-dimensional array by
    /// multiplying all dimension sizes. This is the size needed for the
    /// flattened 1D storage array.
    ///
    /// \tparam N Number of dimensions
    /// \param dims Array of dimension sizes [D0, D1, ..., D(N-1)]
    /// \return Product of all dimensions (D0 * D1 * ... * D(N-1))
    ///
    /// **Example:**
    /// ```cpp
    /// // 3D array with dimensions [10][20][30]
    /// std::array<std::size_t, 3> dims = {10, 20, 30};
    /// std::size_t total = compute_total_size(dims);
    /// // total = 10 * 20 * 30 = 6000 elements
    ///
    /// // 2D array [5][8]
    /// std::array<std::size_t, 2> dims2 = {5, 8};
    /// std::size_t total2 = compute_total_size(dims2);
    /// // total2 = 5 * 8 = 40 elements
    ///
    /// // Empty dimensions (N=0)
    /// std::array<std::size_t, 0> empty = {};
    /// std::size_t total_empty = compute_total_size(empty);
    /// // total_empty = 1 (identity element for multiplication)
    /// ```
    ///
    /// **Constexpr**: Can be evaluated at compile-time if dims is constexpr.
    template<std::size_t N>
    POET_FORCEINLINE constexpr auto compute_total_size(const std::array<std::size_t, N>& dims) -> std::size_t {
        std::size_t total = 1;
        for (std::size_t i = 0; i < N; ++i) {
            total *= dims[i];
        }
        return total;
    }

    /// \brief Compute strides for row-major (C-style) layout.
    ///
    /// Calculates the stride (memory offset multiplier) for each dimension in
    /// row-major order. The rightmost dimension has stride 1, each preceding
    /// dimension's stride is the product of all dimensions to its right.
    ///
    /// **Formula (row-major):**
    /// ```
    /// stride[N-1] = 1
    /// stride[i] = dims[i+1] * dims[i+2] * ... * dims[N-1]
    /// ```
    ///
    /// \tparam N Number of dimensions
    /// \param dims Array of dimension sizes [D0, D1, ..., D(N-1)]
    /// \return Array of strides for each dimension
    ///
    /// **Example - 3D array [10][20][30]:**
    /// ```cpp
    /// std::array<std::size_t, 3> dims = {10, 20, 30};
    /// std::array<std::size_t, 3> strides = compute_strides(dims);
    /// // strides = [600, 30, 1]
    /// //            ^    ^   ^
    /// //            |    |   rightmost: always 1
    /// //            |    middle: 30
    /// //            leftmost: 20 * 30 = 600
    ///
    /// // To access element [5][10][15]:
    /// // flat_index = 5*600 + 10*30 + 15*1 = 3000 + 300 + 15 = 3315
    /// ```
    ///
    /// **Example - 2D array [8][5]:**
    /// ```cpp
    /// std::array<std::size_t, 2> dims2 = {8, 5};
    /// std::array<std::size_t, 2> strides2 = compute_strides(dims2);
    /// // strides2 = [5, 1]
    ///
    /// // Element [3][2] is at: 3*5 + 2*1 = 17
    /// ```
    ///
    /// **Note:** This computes row-major (C-style) strides. For Fortran/column-major
    /// layout, you would need a different stride calculation.
    ///
    /// **Constexpr**: Can be evaluated at compile-time if dims is constexpr.
    template<std::size_t N>
    POET_FLATTEN POET_FORCEINLINE constexpr auto compute_strides(const std::array<std::size_t, N>& dims) -> std::array<std::size_t, N> {
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
    /// Converts multi-dimensional array indices to a single flat index using
    /// precomputed strides. This is the core operation for accessing elements
    /// in a flattened N-dimensional array.
    ///
    /// **Formula:**
    /// ```
    /// flat_index = i0*stride[0] + i1*stride[1] + ... + i(N-1)*stride[N-1]
    /// ```
    ///
    /// \tparam N Number of dimensions
    /// \param indices Array of N-dimensional indices [i0, i1, ..., i(N-1)]
    /// \param strides Array of stride values (from compute_strides)
    /// \return Flattened 1D index into linear storage
    ///
    /// **Type rationale:**
    /// - `indices` is `int` (not `size_t`) to support negative values before offset
    ///   adjustment. After adjustment via `adjust_indices`, negative indices become
    ///   non-negative.
    /// - `strides` is `size_t` because strides are always positive
    /// - Return type is `size_t` for use as array index
    ///
    /// **Example - 3D array [10][20][30]:**
    /// ```cpp
    /// std::array<std::size_t, 3> dims = {10, 20, 30};
    /// std::array<std::size_t, 3> strides = compute_strides(dims);  // [600, 30, 1]
    ///
    /// // Access element [5][10][15]
    /// std::array<int, 3> indices = {5, 10, 15};
    /// std::size_t flat = flatten_indices(indices, strides);
    /// // flat = 5*600 + 10*30 + 15*1 = 3000 + 300 + 15 = 3315
    ///
    /// // Access element [0][0][0] (first element)
    /// std::array<int, 3> first = {0, 0, 0};
    /// std::size_t flat_first = flatten_indices(first, strides);
    /// // flat_first = 0*600 + 0*30 + 0*1 = 0
    ///
    /// // Access element [9][19][29] (last element in [10][20][30])
    /// std::array<int, 3> last = {9, 19, 29};
    /// std::size_t flat_last = flatten_indices(last, strides);
    /// // flat_last = 9*600 + 19*30 + 29*1 = 5400 + 570 + 29 = 5999
    /// ```
    ///
    /// **Performance:** Extremely hot path - marked POET_FLATTEN to inline all
    /// operations. Typically compiles to a few multiplies and adds with no loops
    /// (loop is fully unrolled).
    ///
    /// **Constexpr**: Can be evaluated at compile-time for constexpr inputs.
    template<std::size_t N>
    POET_FLATTEN POET_FORCEINLINE constexpr auto flatten_indices(const std::array<int, N>& indices,
                                   const std::array<std::size_t, N>& strides) -> std::size_t {
        std::size_t flat = 0;
        for (std::size_t i = 0; i < N; ++i) {
            flat += static_cast<std::size_t>(indices[i]) * strides[i];
        }
        return flat;
    }

    /// \brief Check if all indices are within bounds (after offset adjustment).
    ///
    /// Validates that all N-dimensional indices fall within the valid range after
    /// subtracting offsets. Used for bounds checking in offset arrays (arrays where
    /// indices don't start at 0).
    ///
    /// **Algorithm:**
    /// For each dimension i:
    /// 1. Compute adjusted_i = indices[i] - offsets[i]
    /// 2. Check: 0 <= adjusted_i < dims[i]
    /// Returns true only if ALL dimensions pass.
    ///
    /// \tparam N Number of dimensions
    /// \param indices Runtime multi-dimensional indices (user-facing)
    /// \param offsets Offset for each dimension (min valid index)
    /// \param dims Size of each dimension
    /// \return true if all indices are in-bounds after offset adjustment
    ///
    /// **Offset semantics:**
    /// - offset[i] is the minimum valid index for dimension i
    /// - Valid range for dimension i: [offset[i], offset[i] + dims[i])
    /// - Example: offset=-5, dims=10 → valid range is [-5, 5)
    ///
    /// **Example - Standard 0-based indexing:**
    /// ```cpp
    /// std::array<std::size_t, 2> dims = {10, 20};
    /// std::array<int, 2> offsets = {0, 0};  // Standard 0-based
    ///
    /// std::array<int, 2> idx1 = {5, 10};
    /// bool valid1 = check_bounds(idx1, offsets, dims);  // true
    ///
    /// std::array<int, 2> idx2 = {10, 10};  // First index out of bounds
    /// bool valid2 = check_bounds(idx2, offsets, dims);  // false
    /// ```
    ///
    /// **Example - Offset array (e.g., [-5:5][-10:10]):**
    /// ```cpp
    /// // Array with indices from [-5:5] x [-10:10]
    /// std::array<std::size_t, 2> dims = {10, 20};  // 10 and 20 elements
    /// std::array<int, 2> offsets = {-5, -10};      // Start at -5 and -10
    ///
    /// // Check index [0][0] (middle of array)
    /// std::array<int, 2> idx_middle = {0, 0};
    /// bool valid = check_bounds(idx_middle, offsets, dims);
    /// // Adjusted: [0-(-5), 0-(-10)] = [5, 10]
    /// // Check: 0 <= 5 < 10 AND 0 <= 10 < 20 → true
    ///
    /// // Check index [-5][-10] (first element)
    /// std::array<int, 2> idx_first = {-5, -10};
    /// bool valid_first = check_bounds(idx_first, offsets, dims);
    /// // Adjusted: [-5-(-5), -10-(-10)] = [0, 0]
    /// // Check: 0 <= 0 < 10 AND 0 <= 0 < 20 → true
    ///
    /// // Check index [5][10] (one past end)
    /// std::array<int, 2> idx_end = {5, 10};
    /// bool valid_end = check_bounds(idx_end, offsets, dims);
    /// // Adjusted: [5-(-5), 10-(-10)] = [10, 20]
    /// // Check: 0 <= 10 < 10 → false (first dimension out of bounds)
    /// ```
    ///
    /// **Typical usage pattern:** check_bounds → adjust_indices → flatten_indices
    template<std::size_t N>
    POET_FORCEINLINE constexpr auto check_bounds(const std::array<int, N>& indices,
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
    ///
    /// Converts user-facing indices (which may start at arbitrary offsets) to
    /// 0-based indices suitable for flattening. This is the transformation step
    /// between bounds-checked user indices and internal 0-based storage indices.
    ///
    /// **Formula:**
    /// ```
    /// adjusted[i] = indices[i] - offsets[i]
    /// ```
    ///
    /// \tparam N Number of dimensions
    /// \param indices User-facing N-dimensional indices
    /// \param offsets Offset for each dimension
    /// \return Adjusted indices (0-based for each dimension)
    ///
    /// **Typical usage pattern:**
    /// ```cpp
    /// // 1. Bounds check
    /// if (check_bounds(user_indices, offsets, dims)) {
    ///     // 2. Adjust to 0-based
    ///     auto adjusted = adjust_indices(user_indices, offsets);
    ///     // 3. Flatten to 1D
    ///     std::size_t flat_idx = flatten_indices(adjusted, strides);
    ///     // 4. Access array
    ///     data[flat_idx] = value;
    /// }
    /// ```
    ///
    /// **Example - Offset array [-5:5][-10:10]:**
    /// ```cpp
    /// std::array<int, 2> offsets = {-5, -10};
    ///
    /// // User wants to access element [0][0] (middle of array)
    /// std::array<int, 2> user_idx = {0, 0};
    /// auto adjusted = adjust_indices(user_idx, offsets);
    /// // adjusted = [0-(-5), 0-(-10)] = [5, 10]
    /// // These are the 0-based indices in storage
    ///
    /// // User wants to access element [-5][-10] (first element)
    /// std::array<int, 2> first_idx = {-5, -10};
    /// auto adjusted_first = adjust_indices(first_idx, offsets);
    /// // adjusted_first = [-5-(-5), -10-(-10)] = [0, 0]
    ///
    /// // User wants to access element [4][9] (last element)
    /// std::array<int, 2> last_idx = {4, 9};
    /// auto adjusted_last = adjust_indices(last_idx, offsets);
    /// // adjusted_last = [4-(-5), 9-(-10)] = [9, 19]
    /// ```
    ///
    /// **Example - Standard 0-based array:**
    /// ```cpp
    /// std::array<int, 2> offsets_zero = {0, 0};  // No offset
    ///
    /// std::array<int, 2> idx = {3, 7};
    /// auto adjusted = adjust_indices(idx, offsets_zero);
    /// // adjusted = [3-0, 7-0] = [3, 7] (unchanged)
    /// ```
    ///
    /// **Performance:** Hot path function - marked POET_FLATTEN for full inlining.
    /// Loop typically unrolls completely to direct subtractions.
    ///
    /// **Constexpr**: Can be evaluated at compile-time for constexpr inputs.
    template<std::size_t N>
    POET_FLATTEN POET_FORCEINLINE constexpr auto adjust_indices(const std::array<int, N>& indices,
                                 const std::array<int, N>& offsets) -> std::array<int, N> {
        std::array<int, N> adjusted{};
        for (std::size_t i = 0; i < N; ++i) {
            adjusted[i] = indices[i] - offsets[i];
        }
        return adjusted;
    }

}// namespace poet::detail

#endif// POET_CORE_MDSPAN_UTILS_HPP
