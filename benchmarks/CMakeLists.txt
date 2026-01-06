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
  static_dispatch_bench.cpp
)

target_link_libraries(poet_benchmarks
  PRIVATE
    poet::poet
    ${_poet_nanobench_target}
)

poet_enable_warnings(poet_benchmarks)
poet_enable_sanitizers(poet_benchmarks)

add_custom_target(poet_run_benchmarks
  COMMAND $<TARGET_FILE:poet_benchmarks>
  DEPENDS poet_benchmarks
  USES_TERMINAL
  COMMENT "Running POET benchmarks"
)

option(POET_REGISTER_BENCHMARKS_AS_TESTS "Register benchmarks as CTest tests" OFF)

if(POET_BUILD_TESTS AND POET_REGISTER_BENCHMARKS_AS_TESTS)
  add_test(NAME poet.benchmarks COMMAND $<TARGET_FILE:poet_benchmarks>)
  set_tests_properties(poet.benchmarks PROPERTIES LABELS benchmarks)
endif()
