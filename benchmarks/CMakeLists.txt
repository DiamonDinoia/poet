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

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512F)

function(_poet_configure_benchmark_target TARGET_NAME)
  add_executable(${TARGET_NAME} ${ARGN})
  target_link_libraries(${TARGET_NAME}
    PRIVATE
      poet::poet
      ${_poet_nanobench_target}
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

# ── Register-tuned benchmarks (default ISA) ────────────────────────────────
_poet_configure_benchmark_target(poet_static_for_bench static_for_bench.cpp)
_poet_configure_benchmark_target(poet_dynamic_for_bench dynamic_for_bench.cpp)

# ── Register-tuned benchmarks (-march=native) ─────────────────────────────
if(COMPILER_SUPPORTS_MARCH_NATIVE)
  _poet_configure_benchmark_target(poet_compiler_comparison_bench_native compiler_comparison_bench.cpp)
  target_compile_options(poet_compiler_comparison_bench_native PRIVATE -march=native)

  _poet_configure_benchmark_target(poet_static_for_bench_native static_for_bench.cpp)
  target_compile_options(poet_static_for_bench_native PRIVATE -march=native)

  _poet_configure_benchmark_target(poet_dynamic_for_bench_native dynamic_for_bench.cpp)
  target_compile_options(poet_dynamic_for_bench_native PRIVATE -march=native)

  message(STATUS "POET: building native benchmarks (poet_*_bench_native)")
endif()

# ── Register-tuned benchmarks (-mavx512f, gated on hardware) ───────────────
if(COMPILER_SUPPORTS_AVX512F)
  # Check if the host CPU actually supports AVX-512
  set(_poet_host_has_avx512 FALSE)
  if(EXISTS "/proc/cpuinfo")
    file(READ "/proc/cpuinfo" _poet_cpuinfo)
    string(FIND "${_poet_cpuinfo}" "avx512f" _poet_avx512_pos)
    if(NOT _poet_avx512_pos EQUAL -1)
      set(_poet_host_has_avx512 TRUE)
    endif()
  endif()

  if(_poet_host_has_avx512)
    _poet_configure_benchmark_target(poet_static_for_bench_avx512 static_for_bench.cpp)
    target_compile_options(poet_static_for_bench_avx512 PRIVATE -mavx512f)

    _poet_configure_benchmark_target(poet_dynamic_for_bench_avx512 dynamic_for_bench.cpp)
    target_compile_options(poet_dynamic_for_bench_avx512 PRIVATE -mavx512f)

    message(STATUS "POET: building AVX-512 benchmarks (poet_*_bench_avx512)")
  else()
    message(STATUS "POET: skipping AVX-512 benchmarks (host CPU lacks avx512f)")
  endif()
endif()

# ── Umbrella build target (CI uses --target poet_benchmarks) ───────────────
add_custom_target(poet_benchmarks
  DEPENDS poet_compiler_comparison_bench poet_dispatch_bench poet_static_for_bench poet_dynamic_for_bench
)

# ── Run targets ────────────────────────────────────────────────────────────
add_custom_target(poet_run_bench
  COMMAND $<TARGET_FILE:poet_dispatch_bench>
  COMMAND $<TARGET_FILE:poet_static_for_bench>
  COMMAND $<TARGET_FILE:poet_dynamic_for_bench>
  DEPENDS poet_dispatch_bench poet_static_for_bench poet_dynamic_for_bench
  USES_TERMINAL
  COMMENT "Running POET benchmarks"
)
# Alias used by CI
add_custom_target(poet_run_benchmarks DEPENDS poet_run_bench)

if(COMPILER_SUPPORTS_MARCH_NATIVE)
  add_custom_target(poet_run_bench_native
    COMMAND $<TARGET_FILE:poet_dispatch_bench>
    COMMAND $<TARGET_FILE:poet_static_for_bench_native>
    COMMAND $<TARGET_FILE:poet_dynamic_for_bench_native>
    DEPENDS poet_dispatch_bench poet_static_for_bench_native poet_dynamic_for_bench_native
    USES_TERMINAL
    COMMENT "Running POET benchmarks (-march=native)"
  )
  # Alias used by CI
  add_custom_target(poet_run_benchmarks_native DEPENDS poet_run_bench_native)
endif()

if(POET_BUILD_TESTS)
  option(POET_REGISTER_BENCHMARKS_AS_TESTS "Register benchmarks as CTest tests" OFF)
  if(POET_REGISTER_BENCHMARKS_AS_TESTS)
    add_test(NAME poet.bench.dispatch COMMAND $<TARGET_FILE:poet_dispatch_bench>)
    add_test(NAME poet.bench.static_for COMMAND $<TARGET_FILE:poet_static_for_bench>)
    add_test(NAME poet.bench.dynamic_for COMMAND $<TARGET_FILE:poet_dynamic_for_bench>)
    set_tests_properties(poet.bench.dispatch poet.bench.static_for poet.bench.dynamic_for
      PROPERTIES LABELS benchmarks)
  endif()
endif()
