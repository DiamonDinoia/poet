CPMAddPackage(
  NAME Catch2
  GITHUB_REPOSITORY catchorg/Catch2
  VERSION 3.5.3
)

if(Catch2_SOURCE_DIR)
  list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
endif()

add_executable(poet_tests
  dynamic_for_tests.cpp
  poet_header_tests.cpp
  static_dispatch_tests.cpp
  static_for_tests.cpp
)

target_link_libraries(poet_tests PRIVATE Catch2::Catch2WithMain poet)

target_compile_features(poet_tests PRIVATE cxx_std_17)

poet_enable_sanitizers(poet_tests)

include(Catch)
catch_discover_tests(poet_tests)
