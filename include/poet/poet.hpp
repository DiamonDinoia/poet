#pragma once

/// \file poet.hpp
/// \brief Umbrella header aggregating the public POET API surface.
///
/// This header centralizes includes for the public headers found under
/// include/poet/ so that downstream projects can simply include <poet/poet.hpp>
/// to access the stable API surface.

// clang-format off
// IMPORTANT: Include order matters! macros.hpp must come first, undef_macros.hpp must come last
// NOLINTBEGIN(llvm-include-order)
#include <poet/core/macros.hpp>
#include <poet/core/register_info.hpp>
#include <poet/core/dynamic_for.hpp>
#include <poet/core/dispatch.hpp>
#include <poet/core/static_for.hpp>
#include <poet/core/undef_macros.hpp>
// NOLINTEND(llvm-include-order)
// clang-format on
