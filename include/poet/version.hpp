#pragma once

/// \file version.hpp
/// \brief POET version macros and constants.
///
/// Generated from version.hpp.in by cmake/GenerateVersion.cmake.
/// Do not edit by hand; re-run CMake configure or the pre-commit hook.

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-macro-to-enum,modernize-macro-to-enum)
#define POET_VERSION_MAJOR 0
#define POET_VERSION_MINOR 0
#define POET_VERSION_PATCH 0
#define POET_VERSION_STRING "0.0.0"
#define POET_VERSION_FULL "0.0.0-dev.91"
// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-macro-to-enum,modernize-macro-to-enum)

namespace poet {

inline constexpr int version_major = POET_VERSION_MAJOR;
inline constexpr int version_minor = POET_VERSION_MINOR;
inline constexpr int version_patch = POET_VERSION_PATCH;
inline constexpr const char *version_string = POET_VERSION_STRING;
inline constexpr const char *version_full = POET_VERSION_FULL;

}// namespace poet
