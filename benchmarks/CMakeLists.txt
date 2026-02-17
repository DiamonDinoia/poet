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

# Check if -march=native is supported by the compiler
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
check_cxx_compiler_flag("-ffast-math" COMPILER_SUPPORTS_FAST_MATH)

# Helper function to create benchmark executables
function(_poet_configure_benchmark_target TARGET_NAME)
  add_executable(${TARGET_NAME} ${ARGN})
  target_link_libraries(${TARGET_NAME}
    PRIVATE
      poet::poet
      ${_poet_nanobench_target}
  )
  poet_enable_warnings(${TARGET_NAME})
  poet_enable_sanitizers(${TARGET_NAME})
  if (COMPILER_SUPPORTS_FAST_MATH)
    target_compile_options(${TARGET_NAME} PRIVATE -ffast-math)
  endif()
endfunction()

function(add_poet_benchmark NAME)
  set(source_files ${ARGN})
  set(base_target poet_bench_${NAME})
  _poet_configure_benchmark_target(${base_target} ${source_files})

  if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(native_target ${base_target}_native)
    _poet_configure_benchmark_target(${native_target} ${source_files})
    target_compile_options(${native_target} PRIVATE -march=native)
  endif()
endfunction()

# Create separate benchmark executables by category (each has its own main())
add_poet_benchmark(
  static_for
  "static_for_bench_main.cpp"
  "static_for_bench_dependent.cpp"
  "static_for_bench_simple.cpp"
  "static_for_bench_unroll.cpp"
  "static_for_bench_blocksize.cpp")
add_poet_benchmark(dynamic_for "dynamic_for_bench.cpp")
add_poet_benchmark(static_dispatch "static_dispatch_bench.cpp")

if(COMPILER_SUPPORTS_MARCH_NATIVE)
  message(STATUS "POET: -march=native is supported, building native benchmark versions")
endif()

function(add_poet_run_target NAME)
  add_custom_target(poet_run_bench_${NAME}
    COMMAND $<TARGET_FILE:poet_bench_${NAME}>
    DEPENDS poet_bench_${NAME}
    USES_TERMINAL
    COMMENT "Running ${NAME} benchmarks"
  )
endfunction()

# Custom targets to run benchmarks by category
add_poet_run_target(static_for)
add_poet_run_target(dynamic_for)
add_poet_run_target(static_dispatch)

add_custom_target(poet_benchmarks
  DEPENDS poet_bench_static_for poet_bench_dynamic_for poet_bench_static_dispatch
  COMMENT "Build all POET benchmark executables"
)

add_custom_target(poet_run_benchmarks
  COMMAND $<TARGET_FILE:poet_bench_static_for>
  COMMAND $<TARGET_FILE:poet_bench_dynamic_for>
  COMMAND $<TARGET_FILE:poet_bench_static_dispatch>
  DEPENDS poet_bench_static_for poet_bench_dynamic_for poet_bench_static_dispatch
  USES_TERMINAL
  COMMENT "Running all POET benchmarks"
)

if(COMPILER_SUPPORTS_MARCH_NATIVE)
  add_custom_target(poet_run_benchmarks_native
    COMMAND $<TARGET_FILE:poet_bench_static_for_native>
    COMMAND $<TARGET_FILE:poet_bench_dynamic_for_native>
    COMMAND $<TARGET_FILE:poet_bench_static_dispatch_native>
    DEPENDS poet_bench_static_for_native poet_bench_dynamic_for_native poet_bench_static_dispatch_native
    USES_TERMINAL
    COMMENT "Running all POET benchmarks (with -march=native)"
  )
endif()

option(POET_REGISTER_BENCHMARKS_AS_TESTS "Register benchmarks as CTest tests" OFF)

if(POET_BUILD_TESTS AND POET_REGISTER_BENCHMARKS_AS_TESTS)
  add_test(NAME poet.bench.static_for COMMAND $<TARGET_FILE:poet_bench_static_for>)
  add_test(NAME poet.bench.dynamic_for COMMAND $<TARGET_FILE:poet_bench_dynamic_for>)
  add_test(NAME poet.bench.static_dispatch COMMAND $<TARGET_FILE:poet_bench_static_dispatch>)
  set_tests_properties(poet.bench.static_for poet.bench.dynamic_for poet.bench.static_dispatch
    PROPERTIES LABELS benchmarks)
endif()
