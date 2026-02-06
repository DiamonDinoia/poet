CPMAddPackage(
  NAME Catch2
  GITHUB_REPOSITORY catchorg/Catch2
  VERSION 3.5.3
)

# Mark Catch2 includes as SYSTEM so dependency warnings don't leak into project builds.
if(TARGET Catch2::Catch2WithMain)
  get_target_property(_catch2_includes Catch2::Catch2WithMain INTERFACE_INCLUDE_DIRECTORIES)
  if(_catch2_includes)
    set_target_properties(Catch2::Catch2WithMain PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_catch2_includes}")
  endif()
endif()

if(Catch2_SOURCE_DIR)
  list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
endif()

include(Catch)

# Disable LTO/IPO for test targets to reduce memory usage during compilation.
# Template-heavy test instantiations can cause excessive memory consumption with IPO enabled.
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

set(HEADERS_TEST_SRCS
  poet_header_tests.cpp
)
set(DYNAMIC_FOR_TEST_SRCS
  dynamic_for_tests.cpp
)
set(STATIC_FOR_TEST_SRCS
  static_for_tests.cpp
)
set(DISPATCH_TEST_SRCS
  static_dispatch_tests.cpp
  static_dispatch_tuple_tests.cpp
)
set(HEAVY_TEST_SRCS
  static_dispatch_heavy_tests.cpp
)

set(POET_STATIC_DISPATCH_HEAVY_TEST_COUNT 12)
option(POET_ENABLE_TEST_PCH "Enable precompiled headers for test targets" ON)

# Internal macro: applies common configuration to all test targets
macro(_configure_poet_test target_name cxx_feature)
  # Disable IPO for this target (already disabled globally for tests, but be explicit)
  set_property(TARGET ${target_name} PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)

  # Mark Catch2 includes as system to suppress warnings
  if(_catch2_includes)
    target_include_directories(${target_name} SYSTEM PRIVATE ${_catch2_includes})
  endif()

  # Apply precompiled headers to speed up compilation
  if(POET_ENABLE_TEST_PCH)
    target_precompile_headers(${target_name} PRIVATE
      "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/test_pch.hpp>")
  endif()

  target_link_libraries(${target_name} PRIVATE Catch2::Catch2WithMain poet)

  # Add coverage instrumentation if enabled
  if(POET_ENABLE_COVERAGE)
    target_compile_options(${target_name} PRIVATE ${POET_COVERAGE_COMPILE_FLAGS})
    target_link_options(${target_name} PRIVATE ${POET_COVERAGE_LINK_FLAGS})
  endif()

  target_compile_features(${target_name} PRIVATE ${cxx_feature})

  if(POET_STRICT_WARNINGS)
    poet_enable_warnings(${target_name})
  endif()

  poet_enable_sanitizers(${target_name})

  # Suppress C2y extension warnings on Clang
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-Wno-c2y-extensions" HAS_WNO_C2Y_EXTENSIONS)
  target_compile_options(${target_name} PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<BOOL:${HAS_WNO_C2Y_EXTENSIONS}>>:-Wno-c2y-extensions>
  )
endmacro()

# Creates a test executable with Catch2 test discovery
# Usage: add_poet_test_exec(target_name cxx_feature sources...)
#        add_poet_test_exec(target_name cxx_feature HEAVY_TEST test_id)
macro(add_poet_test_exec target_name cxx_feature)
  # Parse optional HEAVY_TEST argument
  set(_is_heavy_test FALSE)
  set(_heavy_test_id "")
  if("${ARGV2}" STREQUAL "HEAVY_TEST")
    set(_is_heavy_test TRUE)
    set(_heavy_test_id "${ARGV3}")
    add_executable(${target_name} ${HEAVY_TEST_SRCS})
    target_compile_definitions(${target_name} PRIVATE POET_STATIC_DISPATCH_HEAVY_TEST_ID=${_heavy_test_id})
  else()
    add_executable(${target_name} ${ARGN})
  endif()

  _configure_poet_test(${target_name} ${cxx_feature})
  catch_discover_tests(${target_name})

  # Track all test executables for coverage target
  set_property(GLOBAL APPEND PROPERTY POET_TEST_EXEC_TARGETS ${target_name})
endmacro()

# Creates a complete test suite with separate executables for each test category
# Heavy tests are split into individual executables to reduce compilation memory usage
macro(add_poet_tests_suite suite_target cxx_feature)
  set(_suite_execs
    ${suite_target}_headers
    ${suite_target}_dynamic_for
    ${suite_target}_static_for
    ${suite_target}_dispatch
  )

  # Create separate executables for each test category to enable parallel compilation
  add_poet_test_exec(${suite_target}_headers ${cxx_feature} ${HEADERS_TEST_SRCS})
  add_poet_test_exec(${suite_target}_dynamic_for ${cxx_feature} ${DYNAMIC_FOR_TEST_SRCS})
  add_poet_test_exec(${suite_target}_static_for ${cxx_feature} ${STATIC_FOR_TEST_SRCS})
  add_poet_test_exec(${suite_target}_dispatch ${cxx_feature} ${DISPATCH_TEST_SRCS})

  # Heavy tests: split each test case into its own executable to reduce peak memory usage.
  # Each executable compiles static_dispatch_heavy_tests.cpp with POET_STATIC_DISPATCH_HEAVY_TEST_ID
  # set to enable only one test case via preprocessor guards (see static_dispatch_heavy_tests.cpp).
  foreach(_id RANGE 1 ${POET_STATIC_DISPATCH_HEAVY_TEST_COUNT})
    set(_heavy_target ${suite_target}_heavy_${_id})
    add_poet_test_exec(${_heavy_target} ${cxx_feature} HEAVY_TEST ${_id})
    list(APPEND _suite_execs ${_heavy_target})
  endforeach()

  # Create umbrella target for building all tests in this suite
  add_custom_target(${suite_target} DEPENDS ${_suite_execs})
endmacro()

set(POET_CXX_STANDARD DETECT CACHE STRING "Detect highest supported C++ standard")
set_property(CACHE POET_CXX_STANDARD PROPERTY STRINGS DETECT)

if(POET_CXX_STANDARD STREQUAL "DETECT")
  foreach(CXX_STANDARD_VAR 23 20 17)
    if("cxx_std_${CXX_STANDARD_VAR}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
      message(STATUS "Detected support for C++${CXX_STANDARD_VAR} standard")
      set(POET_CXX_STANDARD ${CXX_STANDARD_VAR})
      break()
    endif()
  endforeach()
  if(NOT POET_CXX_STANDARD)
    message(FATAL_ERROR "No supported C++ standard available (need at least C++17)")
  endif()
  message(STATUS "Using C++${POET_CXX_STANDARD} for extra test targets")
endif()

set(POET_TEST_STANDARDS 23 20 17)
foreach(POET_STD IN LISTS POET_TEST_STANDARDS)
  if(POET_CXX_STANDARD GREATER_EQUAL ${POET_STD})
    add_poet_tests_suite(poet_tests_std${POET_STD} cxx_std_${POET_STD})
  endif()
endforeach()

# Compatibility aggregate target: avoid recompiling a duplicate std17 monolith.
if(TARGET poet_tests_std17)
  add_custom_target(poet_tests DEPENDS poet_tests_std17)
endif()

add_executable(poet_header_analysis header_analysis.cpp)
target_link_libraries(poet_header_analysis PRIVATE poet)
target_compile_features(poet_header_analysis PRIVATE cxx_std_17)
poet_configure_static_analysis(poet_header_analysis)

if(TARGET coverage)
  get_property(_poet_all_tests GLOBAL PROPERTY POET_TEST_EXEC_TARGETS)
  if(_poet_all_tests)
    add_dependencies(coverage ${_poet_all_tests})
  endif()
endif()
