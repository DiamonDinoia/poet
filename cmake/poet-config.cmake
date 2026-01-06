# Additional configuration helpers for projects consuming POET.
# This file is installed alongside the CMake package configuration when present.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/PoetWarnings.cmake" OPTIONAL)
include("${CMAKE_CURRENT_LIST_DIR}/PoetSanitizers.cmake" OPTIONAL)
include("${CMAKE_CURRENT_LIST_DIR}/PoetStaticAnalysis.cmake" OPTIONAL)
