#pragma once

/// \file mdspan_utils.hpp
/// \brief Row-major stride computation for the N-D dispatch in dispatch.hpp.

#include <array>
#include <cstddef>
#include <poet/core/macros.hpp>

namespace poet::detail {

/// stride[i] = product of dims[i+1..N-1].
template<std::size_t N>
POET_CPP20_CONSTEVAL auto compute_strides(const std::array<std::size_t, N> &dims) -> std::array<std::size_t, N> {
    std::array<std::size_t, N> strides{};
    if constexpr (N > 0) {
        strides[N - 1] = 1;
        for (std::size_t i = N - 1; i > 0; --i) { strides[i - 1] = strides[i] * dims[i]; }
    }
    return strides;
}

}// namespace poet::detail
