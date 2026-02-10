CPMAddPackage(
  NAME nanobench
  GIT_REPOSITORY https://github.com/martinus/nanobench.git
  GIT_TAG v4.3.11
  OPTIONS "CMAKE_POLICY_VERSION_MINIMUM 3.10"
)

set(_poet_nanobench_target nanobench::nanobench)
if(TARGET nanobench::nanobench)
  set(_poet_nanobench_target nanobench::nanobench)
elseif(TARGET nanobench)
  set(_poet_nanobench_target nanobench)
else()
  add_library(nanobench INTERFACE)
  target_include_directories(nanobench INTERFACE
    ${nanobench_SOURCE_DIR}/include
    ${nanobench_SOURCE_DIR}/src/include
  )
  set(_poet_nanobench_target nanobench)
endif()

add_executable(poet_benchmarks
  main.cpp
  static_for_bench.cpp
  dynamic_for_bench.cpp
  dynamic_for_dispatch_bench.cpp
  static_dispatch_bench.cpp
  dispatch_optimization_bench.cpp
  switch_vs_fptr_bench.cpp
)

target_link_libraries(poet_benchmarks
  PRIVATE
    poet::poet
    ${_poet_nanobench_target}
)

poet_enable_warnings(poet_benchmarks)
poet_enable_sanitizers(poet_benchmarks)

# Check if -march=native is supported by the compiler
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)

# Create a second benchmark executable with -march=native if supported
if(COMPILER_SUPPORTS_MARCH_NATIVE)
  add_executable(poet_benchmarks_native
    main.cpp
    static_for_bench.cpp
    dynamic_for_bench.cpp
    dynamic_for_dispatch_bench.cpp
    static_dispatch_bench.cpp
    dispatch_optimization_bench.cpp
    switch_vs_fptr_bench.cpp
  )

  target_link_libraries(poet_benchmarks_native
    PRIVATE
      poet::poet
      ${_poet_nanobench_target}
  )

  target_compile_options(poet_benchmarks_native PRIVATE -march=native)

  poet_enable_warnings(poet_benchmarks_native)
  poet_enable_sanitizers(poet_benchmarks_native)

  message(STATUS "POET: -march=native is supported, building poet_benchmarks_native")
endif()

add_custom_target(poet_run_benchmarks
  COMMAND $<TARGET_FILE:poet_benchmarks>
  DEPENDS poet_benchmarks
  USES_TERMINAL
  COMMENT "Running POET benchmarks (baseline)"
)

if(COMPILER_SUPPORTS_MARCH_NATIVE)
  add_custom_target(poet_run_benchmarks_native
    COMMAND $<TARGET_FILE:poet_benchmarks_native>
    DEPENDS poet_benchmarks_native
    USES_TERMINAL
    COMMENT "Running POET benchmarks (with -march=native)"
  )
endif()

# Optional: build benchmarks for multiple C++ standards (17/20/23)
set(POET_BENCH_BUILD_ALL_STANDARDS OFF CACHE BOOL "Build benchmark targets for all supported C++ standards")

# Helper to create a benchmark executable with a specific C++ feature
macro(add_poet_bench_exec target_name cxx_feature)
  add_executable(${target_name}
    main.cpp
    static_for_bench.cpp
    dynamic_for_bench.cpp
    dynamic_for_dispatch_bench.cpp
    static_dispatch_bench.cpp
    dispatch_optimization_bench.cpp
    switch_vs_fptr_bench.cpp
  )

  target_link_libraries(${target_name}
    PRIVATE
      poet::poet
      ${_poet_nanobench_target}
  )

  target_compile_features(${target_name} PRIVATE ${cxx_feature})
  poet_enable_warnings(${target_name})
  poet_enable_sanitizers(${target_name})
endmacro()

# Detect highest supported C++ standard (used only for optional multi-std builds)
set(POET_CXX_STANDARD DETECT CACHE STRING "Detect highest supported C++ standard for benchmarks")
set_property(CACHE POET_CXX_STANDARD PROPERTY STRINGS DETECT)

if(POET_CXX_STANDARD STREQUAL "DETECT")
  foreach(CXX_STANDARD_VAR 23 20 17)
    if("cxx_std_${CXX_STANDARD_VAR}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
      set(POET_CXX_STANDARD ${CXX_STANDARD_VAR})
      break()
    endif()
  endforeach()
  if(NOT POET_CXX_STANDARD)
    message(FATAL_ERROR "No supported C++ standard available for benchmarks (need at least C++17)")
  endif()
endif()

if(POET_BENCH_BUILD_ALL_STANDARDS)
  set(POET_BENCH_STANDARDS 23 20 17)
  foreach(POET_STD IN LISTS POET_BENCH_STANDARDS)
    if(POET_CXX_STANDARD GREATER_EQUAL ${POET_STD})
      add_poet_bench_exec(poet_benchmarks_std${POET_STD} cxx_std_${POET_STD})
      add_custom_target(poet_run_benchmarks_std${POET_STD}
        COMMAND $<TARGET_FILE:poet_benchmarks_std${POET_STD}>
        DEPENDS poet_benchmarks_std${POET_STD}
        USES_TERMINAL
        COMMENT "Running POET benchmarks (C++${POET_STD})"
      )
    endif()
  endforeach()
else()
  # keep original poet_benchmarks target as baseline (uses project CMAKE_CXX_STANDARD)
  # nothing else to do
endif()

option(POET_REGISTER_BENCHMARKS_AS_TESTS "Register benchmarks as CTest tests" OFF)

if(POET_BUILD_TESTS AND POET_REGISTER_BENCHMARKS_AS_TESTS)
  add_test(NAME poet.benchmarks COMMAND $<TARGET_FILE:poet_benchmarks>)
  set_tests_properties(poet.benchmarks PROPERTIES LABELS benchmarks)
endif()
