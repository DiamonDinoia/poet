#pragma once

/// \file poet.hpp
/// \brief Umbrella header for the public POET API.

// clang-format off
// Include order matters: macros.hpp must come first and undef_macros.hpp last.
// NOLINTBEGIN(llvm-include-order)
#include <poet/core/macros.hpp>
#include <poet/core/cpu_info.hpp>
#include <poet/core/dynamic_for.hpp>
#include <poet/core/dispatch.hpp>
#include <poet/core/static_for.hpp>
#include <poet/core/undef_macros.hpp>
// NOLINTEND(llvm-include-order)
// clang-format on
