cmake_minimum_required(VERSION 3.20)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FetchContent)

FetchContent_Declare(
  CPM
  URL https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.0/CPM.cmake
  URL_HASH SHA256=2020b4fc42dba44817983e06342e682ecfc3d2f484a581f11cc5731fbe4dce8a
  DOWNLOAD_NO_EXTRACT TRUE
)

FetchContent_GetProperties(CPM)
if(NOT CPM_POPULATED)
  FetchContent_MakeAvailable(CPM)
endif()

include(${cpm_SOURCE_DIR}/CPM.cmake)

project(POET LANGUAGES CXX)

include(CTest)
include(PoetStaticAnalysis)


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


include(PoetWarnings)
include(PoetSanitizers)
include(PoetStaticAnalysis)
include(PoetDocs)

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

install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PoetWarnings.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PoetSanitizers.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PoetStaticAnalysis.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poet
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/poet-config.cmake")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/poet-config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poet
  )
endif()

set(CPACK_PACKAGE_NAME "poet")
set(CPACK_PACKAGE_VENDOR "POET Project")
set(CPACK_PACKAGE_CONTACT "support@example.com")
set(CPACK_GENERATOR "TGZ")

include(CPack)

## Coverage target -----------------------------------------------------------
# Adds a `coverage` custom target that runs tests and produces an HTML report.
# It prefers `gcovr` if available, otherwise falls back to `lcov` + `genhtml`.
find_program(GCOVR_EXECUTABLE gcovr)
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

# Prefer lcov+genhtml when available (more robust on complex build trees).
if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
  set(LCOV_INFO ${CMAKE_BINARY_DIR}/coverage.info)
  set(LCOV_FILTERED ${CMAKE_BINARY_DIR}/coverage.filtered.info)
  set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)

  add_custom_target(coverage
    DEPENDS poet_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
    COMMAND ${LCOV_EXECUTABLE} --capture --directory ${CMAKE_BINARY_DIR} --output-file ${LCOV_INFO} --ignore-errors inconsistent,unused
    # remove system and external deps (allow patterns to be adjusted)
    COMMAND ${LCOV_EXECUTABLE} --remove ${LCOV_INFO} "/usr/*" "*/_deps/*" --output-file ${LCOV_FILTERED} --ignore-errors inconsistent,unused
    COMMAND ${GENHTML_EXECUTABLE} -o ${COVERAGE_DIR} ${LCOV_FILTERED}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests and generating coverage report (lcov+genhtml) -> ${COVERAGE_DIR}/index.html"
    VERBATIM
  )
elseif(GCOVR_EXECUTABLE)
  # Fall back to gcovr if lcov/genhtml aren't available
  add_custom_target(coverage
    DEPENDS poet_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
    # Filter to project sources (include/poet and tests) to avoid dependency noise
    COMMAND ${GCOVR_EXECUTABLE} -r ${CMAKE_SOURCE_DIR} --filter "include/poet/|tests/" --exclude ".*/_deps/.*" --exclude "/usr/.*" --gcov-ignore-errors=no_working_dir_found --html --html-details -o ${CMAKE_BINARY_DIR}/coverage-report.html
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests and generating coverage report (gcovr) -> ${CMAKE_BINARY_DIR}/coverage-report.html"
    VERBATIM
  )
else()
  message(STATUS "Coverage tools not found: install 'lcov'+'genhtml' or 'gcovr' to enable the 'coverage' target.")
  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage tools missing. Install 'lcov'+'genhtml' or 'gcovr' and re-run CMake to enable coverage generation."
  )
endif()
