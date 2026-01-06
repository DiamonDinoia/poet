CPMAddPackage(
  NAME Catch2
  GITHUB_REPOSITORY catchorg/Catch2
  VERSION 3.5.3
)

# Mark Catch2 includes as SYSTEM so dependency warnings don't leak into project builds.
# This queries the imported target's INTERFACE_INCLUDE_DIRECTORIES and mirrors them
# to INTERFACE_SYSTEM_INCLUDE_DIRECTORIES so consumers see them as system headers.
if(TARGET Catch2::Catch2WithMain)
  get_target_property(_catch2_includes Catch2::Catch2WithMain INTERFACE_INCLUDE_DIRECTORIES)
  if(_catch2_includes)
    set_target_properties(Catch2::Catch2WithMain PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_catch2_includes}")
  endif()
endif()

if(Catch2_SOURCE_DIR)
  list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
endif()

add_executable(poet_tests
  dynamic_for_tests.cpp
  poet_header_tests.cpp
  static_dispatch_tests.cpp
  static_dispatch_tuple_tests.cpp
  static_for_tests.cpp
)

# Ensure Catch2 include directories are treated as system includes for the
# test target so dependency warnings (e.g. __COUNTER__ usage) do not surface.
if(_catch2_includes)
  target_include_directories(poet_tests SYSTEM PRIVATE ${_catch2_includes})
endif()

target_link_libraries(poet_tests PRIVATE Catch2::Catch2WithMain poet)


if(POET_ENABLE_COVERAGE)
  target_compile_options(poet_tests PRIVATE ${POET_COVERAGE_COMPILE_FLAGS})
  target_link_options(poet_tests PRIVATE ${POET_COVERAGE_LINK_FLAGS})
endif()

target_compile_features(poet_tests PRIVATE cxx_std_17)

if(POET_STRICT_WARNINGS)
  poet_enable_warnings(poet_tests)
endif()

poet_enable_sanitizers(poet_tests)

add_executable(poet_header_analysis header_analysis.cpp)
target_link_libraries(poet_header_analysis PRIVATE poet)
target_compile_features(poet_header_analysis PRIVATE cxx_std_17)
poet_configure_static_analysis(poet_header_analysis)


# Suppress C2y extension warnings (e.g. __COUNTER__ from Catch2) in tests.
# This is required because macro expansion happens in the test source files,
# bypassing SYSTEM header suppression. We disable this specific warning to
# avoid false positives from the dependency while keeping strict checks for
# the rest of the project code.
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-Wno-c2y-extensions" HAS_WNO_C2Y_EXTENSIONS)
target_compile_options(poet_tests PRIVATE
  $<$<AND:$<CXX_COMPILER_ID:Clang>,$<BOOL:${HAS_WNO_C2Y_EXTENSIONS}>>:-Wno-c2y-extensions>
)

include(Catch)
catch_discover_tests(poet_tests)
