cmake_minimum_required(VERSION 3.20)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(POET LANGUAGES CXX)

include(CTest)

# Enable coverage instrumentation when requested. This only adds flags to
# project/test targets (not dependencies) so we avoid instrumenting external
# libraries.
option(POET_ENABLE_COVERAGE "Enable code coverage instrumentation for POET and tests" OFF)
if(POET_ENABLE_COVERAGE)
  # Use list form so flags are added separately (avoids quoted single-argument issues)
  set(POET_COVERAGE_COMPILE_FLAGS -O0 -g --coverage)
  set(POET_COVERAGE_LINK_FLAGS --coverage)
endif()

option(POET_BUILD_BENCHMARKS "Build POET benchmark executables" OFF)
option(POET_STRICT_WARNINGS "Enable the project's default warning profile" ON)
option(POET_BUILD_DEVEL "Build POET development utilities" OFF)

if(NOT DEFINED POET_BUILD_TESTS)
  set(POET_BUILD_TESTS ${BUILD_TESTING})
endif()
option(POET_BUILD_TESTS "Build POET test suite" ${POET_BUILD_TESTS})

# Load developer helpers only when developer/test/benchmark builds are requested.
# dev_helpers provides CPM and helper functions (warnings/sanitizers/static-analysis/docs/coverage).
if(POET_BUILD_DEVEL OR POET_BUILD_TESTS OR POET_BUILD_BENCHMARKS)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/dev_helpers.cmake")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  # Add /bigobj to avoid "number of sections exceeded" on MSVC for heavy template instantiations
  add_compile_options(/bigobj)
endif()

add_library(poet INTERFACE)
add_library(poet::poet ALIAS poet)

target_compile_features(poet INTERFACE cxx_std_17)

target_include_directories(poet
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Treat known CPM-managed dependency include roots as system includes so warnings
# from third-party headers are suppressed. This scans the user's CPM cache under
# $HOME/.cpm for common include dirs and marks them SYSTEM on the poet interface
# so they apply transitively to consumers.
if(DEFINED ENV{HOME})
  file(GLOB CPM_POSSIBLE_INCLUDES LIST_DIRECTORIES true
       "$ENV{HOME}/.cpm/*/src/include"
       "$ENV{HOME}/.cpm/*/include"
       "$ENV{HOME}/.cpm/*/include/*")
  foreach(_inc IN LISTS CPM_POSSIBLE_INCLUDES)
    if(IS_DIRECTORY "${_inc}")
      target_include_directories(poet INTERFACE SYSTEM "${_inc}")
    endif()
  endforeach()
endif()



if(POET_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

if(POET_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

install(TARGETS poet
  EXPORT poet-targets
)

install(EXPORT poet-targets
  NAMESPACE poet::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poet
)

configure_package_config_file(
  cmake/poet-config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/poet-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poet
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/poet-config.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poet
)



set(CPACK_PACKAGE_NAME "poet")
set(CPACK_PACKAGE_VENDOR "POET Project")
set(CPACK_PACKAGE_CONTACT "support@example.com")
set(CPACK_GENERATOR "TGZ")

include(CPack)
