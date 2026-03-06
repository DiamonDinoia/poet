if(CODSPEED_MODE)
  # CodSpeed compatibility layer (wraps Google Benchmark with instrumentation).
  # The instrument-hooks C file requires Clang; use this path only in CI.
  CPMAddPackage(
    NAME google_benchmark
    GIT_REPOSITORY https://github.com/CodSpeedHQ/codspeed-cpp
    GIT_TAG v2.1.0
    SOURCE_SUBDIR google_benchmark
    OPTIONS
      "BENCHMARK_ENABLE_TESTING OFF"
      "BENCHMARK_ENABLE_INSTALL OFF"
      "BENCHMARK_ENABLE_WERROR OFF"
  )
else()
  # Upstream Google Benchmark for local development (works with any compiler).
  CPMAddPackage(
    NAME google_benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.5
    OPTIONS
      "BENCHMARK_ENABLE_TESTING OFF"
      "BENCHMARK_ENABLE_INSTALL OFF"
  )
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)


function(_poet_configure_benchmark_target TARGET_NAME)
  add_executable(${TARGET_NAME} ${ARGN})
  target_link_libraries(${TARGET_NAME}
    PRIVATE
      poet::poet
      benchmark::benchmark
  )
  if(cxx_std_23 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)
  else()
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
  endif()
  poet_enable_warnings(${TARGET_NAME})
  poet_enable_sanitizers(${TARGET_NAME})
endfunction()

# ── Compiler comparison benchmark ──────────────────────────────────────────
_poet_configure_benchmark_target(poet_compiler_comparison_bench compiler_comparison_bench.cpp)

# ── Dispatch benchmark ─────────────────────────────────────────────────────
_poet_configure_benchmark_target(poet_dispatch_bench dispatch_bench.cpp)

# ── Dispatch optimization benchmark ──────────────────────────────────────────
_poet_configure_benchmark_target(poet_dispatch_optimization_bench dispatch_optimization_bench.cpp)

# ── Register-tuned benchmarks (default ISA) ────────────────────────────────
_poet_configure_benchmark_target(poet_static_for_bench static_for_bench.cpp)
_poet_configure_benchmark_target(poet_dynamic_for_bench dynamic_for_bench.cpp)
_poet_configure_benchmark_target(poet_dynamic_for_forms_bench dynamic_for_forms_bench.cpp)
_poet_configure_benchmark_target(poet_dynamic_for_emission_bench dynamic_for_emission_bench.cpp)
# ── Register-tuned benchmarks (-march=native) ─────────────────────────────
if(COMPILER_SUPPORTS_MARCH_NATIVE)
  _poet_configure_benchmark_target(poet_compiler_comparison_bench_native compiler_comparison_bench.cpp)
  target_compile_options(poet_compiler_comparison_bench_native PRIVATE -march=native)

  _poet_configure_benchmark_target(poet_static_for_bench_native static_for_bench.cpp)
  target_compile_options(poet_static_for_bench_native PRIVATE -march=native)

  _poet_configure_benchmark_target(poet_dynamic_for_bench_native dynamic_for_bench.cpp)
  target_compile_options(poet_dynamic_for_bench_native PRIVATE -march=native)

  _poet_configure_benchmark_target(poet_dynamic_for_forms_bench_native dynamic_for_forms_bench.cpp)
  target_compile_options(poet_dynamic_for_forms_bench_native PRIVATE -march=native)

  _poet_configure_benchmark_target(poet_dynamic_for_emission_bench_native dynamic_for_emission_bench.cpp)
  target_compile_options(poet_dynamic_for_emission_bench_native PRIVATE -march=native)

  message(STATUS "POET: building native benchmarks (poet_*_bench_native)")
endif()


# ── Umbrella build target (CI uses --target poet_benchmarks) ───────────────
add_custom_target(poet_benchmarks
  DEPENDS poet_compiler_comparison_bench poet_dispatch_bench poet_dispatch_optimization_bench poet_static_for_bench poet_dynamic_for_bench poet_dynamic_for_forms_bench poet_dynamic_for_emission_bench
)

# ── Run targets ────────────────────────────────────────────────────────────
add_custom_target(poet_run_bench
  COMMAND $<TARGET_FILE:poet_dispatch_bench>
  COMMAND $<TARGET_FILE:poet_dispatch_optimization_bench>
  COMMAND $<TARGET_FILE:poet_static_for_bench>
  COMMAND $<TARGET_FILE:poet_dynamic_for_bench>
  COMMAND $<TARGET_FILE:poet_dynamic_for_forms_bench>
  COMMAND $<TARGET_FILE:poet_dynamic_for_emission_bench>
  DEPENDS poet_dispatch_bench poet_dispatch_optimization_bench poet_static_for_bench poet_dynamic_for_bench poet_dynamic_for_forms_bench poet_dynamic_for_emission_bench
  USES_TERMINAL
  COMMENT "Running POET benchmarks"
)
# Short aliases
add_custom_target(benchmarks DEPENDS poet_run_bench)
# Alias used by CI
add_custom_target(poet_run_benchmarks DEPENDS poet_run_bench)

if(COMPILER_SUPPORTS_MARCH_NATIVE)
  add_custom_target(poet_run_bench_native
    COMMAND $<TARGET_FILE:poet_dispatch_bench>
    COMMAND $<TARGET_FILE:poet_static_for_bench_native>
    COMMAND $<TARGET_FILE:poet_dynamic_for_bench_native>
    COMMAND $<TARGET_FILE:poet_dynamic_for_forms_bench_native>
    COMMAND $<TARGET_FILE:poet_dynamic_for_emission_bench_native>
    DEPENDS poet_dispatch_bench poet_static_for_bench_native poet_dynamic_for_bench_native poet_dynamic_for_forms_bench_native poet_dynamic_for_emission_bench_native
    USES_TERMINAL
    COMMENT "Running POET benchmarks (-march=native)"
  )
  # Short alias
  add_custom_target(benchmarks-native DEPENDS poet_run_bench_native)
  # Alias used by CI
  add_custom_target(poet_run_benchmarks_native DEPENDS poet_run_bench_native)
endif()

if(POET_BUILD_TESTS)
  option(POET_REGISTER_BENCHMARKS_AS_TESTS "Register benchmarks as CTest tests" OFF)
  if(POET_REGISTER_BENCHMARKS_AS_TESTS)
    add_test(NAME poet.bench.dispatch COMMAND $<TARGET_FILE:poet_dispatch_bench>)
    add_test(NAME poet.bench.dispatch_optimization COMMAND $<TARGET_FILE:poet_dispatch_optimization_bench>)
    add_test(NAME poet.bench.static_for COMMAND $<TARGET_FILE:poet_static_for_bench>)
    add_test(NAME poet.bench.dynamic_for COMMAND $<TARGET_FILE:poet_dynamic_for_bench>)
    add_test(NAME poet.bench.dynamic_for_forms COMMAND $<TARGET_FILE:poet_dynamic_for_forms_bench>)
    add_test(NAME poet.bench.dynamic_for_emission COMMAND $<TARGET_FILE:poet_dynamic_for_emission_bench>)
    set_tests_properties(poet.bench.dispatch poet.bench.dispatch_optimization poet.bench.static_for poet.bench.dynamic_for poet.bench.dynamic_for_forms poet.bench.dynamic_for_emission
      PROPERTIES LABELS benchmarks)
  endif()
endif()
