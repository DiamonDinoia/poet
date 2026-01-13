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

set(TEST_SRCS
  dynamic_for_tests.cpp
  poet_header_tests.cpp
  static_dispatch_tests.cpp
  static_dispatch_tuple_tests.cpp
  static_for_tests.cpp
)

# Helper macro to create a test executable for a specific standard
macro(add_poet_tests_target target_name cxx_feature)
  add_executable(${target_name} ${TEST_SRCS})

  if(_catch2_includes)
    target_include_directories(${target_name} SYSTEM PRIVATE ${_catch2_includes})
  endif()

  target_link_libraries(${target_name} PRIVATE Catch2::Catch2WithMain poet)

  if(POET_ENABLE_COVERAGE)
    target_compile_options(${target_name} PRIVATE ${POET_COVERAGE_COMPILE_FLAGS})
    target_link_options(${target_name} PRIVATE ${POET_COVERAGE_LINK_FLAGS})
  endif()

  target_compile_features(${target_name} PRIVATE ${cxx_feature})

  if(POET_STRICT_WARNINGS)
    poet_enable_warnings(${target_name})
  endif()

  poet_enable_sanitizers(${target_name})

  # Suppress C2y extension warnings for Clang if supported
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-Wno-c2y-extensions" HAS_WNO_C2Y_EXTENSIONS)
  target_compile_options(${target_name} PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<BOOL:${HAS_WNO_C2Y_EXTENSIONS}>>:-Wno-c2y-extensions>
  )

  include(Catch)
  catch_discover_tests(${target_name})
endmacro()

# Always build tests with C++17 target
# (created below by the foreach to avoid duplicate target errors)

# Detect supported C++ standards via CMake feature list (prefer highest available)
# This prefers C++23, then C++20, then C++17.
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
  if (NOT POET_CXX_STANDARD)
    message(FATAL_ERROR "No supported C++ standard available (need at least C++17)")
  endif ()
  message(STATUS "Using C++${POET_CXX_STANDARD} for extra test targets")
endif()

# Add test targets for each standard that is supported.
# Always add C++17 target (already done above). Add C++20/C++23 based on detection.
set(POET_TEST_STANDARDS 23 20 17)
foreach(POET_STD IN LISTS POET_TEST_STANDARDS)
  if(POET_CXX_STANDARD GREATER_EQUAL ${POET_STD})
    add_poet_tests_target(poet_tests_std${POET_STD} cxx_std_${POET_STD})
  endif()
endforeach()

# Also keep the single standard "poet_tests" target for compatibility (C++17)
add_executable(poet_tests ${TEST_SRCS})
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
check_cxx_compiler_flag("-Wno-c2y-extensions" HAS_WNO_C2Y_EXTENSIONS)
target_compile_options(poet_tests PRIVATE
  $<$<AND:$<CXX_COMPILER_ID:Clang>,$<BOOL:${HAS_WNO_C2Y_EXTENSIONS}>>:-Wno-c2y-extensions>
)
include(Catch)
catch_discover_tests(poet_tests)

add_executable(poet_header_analysis header_analysis.cpp)
target_link_libraries(poet_header_analysis PRIVATE poet)
target_compile_features(poet_header_analysis PRIVATE cxx_std_17)
poet_configure_static_analysis(poet_header_analysis)

# Ensure the coverage target, if present, builds all test executables first.
# This avoids Catch2's temporary NOT_BUILT test placeholders when ctest runs
# before the test binaries are built.
if(TARGET coverage)
  set(_poet_all_tests poet_tests)
  foreach(_std IN ITEMS 17 20 23)
    if(TARGET poet_tests_std${_std})
      list(APPEND _poet_all_tests poet_tests_std${_std})
    endif()
  endforeach()
  if(_poet_all_tests)
    add_dependencies(coverage ${_poet_all_tests})
  endif()
endif()
