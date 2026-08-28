set(POET_EXAMPLES
  static_for
  dynamic_for
  dispatch
  dispatch_set
  cpu_info
  polynomial
  dot_product
)

foreach(example IN LISTS POET_EXAMPLES)
  set(target example_${example})
  add_executable(${target} ${example}.cpp)
  target_link_libraries(${target} PRIVATE poet::poet)
  target_compile_features(${target} PRIVATE cxx_std_17)

  if(POET_STRICT_WARNINGS AND COMMAND poet_enable_warnings)
    poet_enable_warnings(${target})
  endif()
  if(COMMAND poet_enable_sanitizers)
    poet_enable_sanitizers(${target})
  endif()

  add_test(NAME ${target} COMMAND ${target})
endforeach()

# Google Benchmark microbench example. Off by default so plain example builds
# do not pull the dependency; enable with -DPOET_BUILD_EXAMPLE_BENCHMARK=ON.
# When benchmarks are also enabled, the benchmark/ subdirectory's copy of
# Google Benchmark is reused.
option(POET_BUILD_EXAMPLE_BENCHMARK
  "Build the Google Benchmark example (requires benchmark::benchmark)" OFF)

if(POET_BUILD_EXAMPLE_BENCHMARK)
  if(NOT TARGET benchmark::benchmark)
    find_package(benchmark CONFIG REQUIRED)
  endif()

  add_executable(example_benchmark benchmark.cpp)
  target_link_libraries(example_benchmark PRIVATE poet::poet benchmark::benchmark)
  target_compile_features(example_benchmark PRIVATE cxx_std_17)

  # poet_enable_warnings is not applied: Google Benchmark's BENCHMARK_MAIN
  # declares main twice, and -Wredundant-decls with -Werror would fail the build.
  if(COMMAND poet_enable_sanitizers)
    poet_enable_sanitizers(example_benchmark)
  endif()
endif()
