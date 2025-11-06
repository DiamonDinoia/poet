cmake_minimum_required(VERSION 3.20)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FetchContent)

FetchContent_Declare(
  CPM
  URL https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.0/CPM.cmake
  URL_HASH SHA256=2020b4fc42dba44817983e06342e682ecfc3d2f484a581f11cc5731fbe4dce8a
  DOWNLOAD_NO_EXTRACT TRUE
)

FetchContent_GetProperties(CPM)
if(NOT CPM_POPULATED)
  FetchContent_MakeAvailable(CPM)
endif()

set(CPM_DOWNLOAD_VERSION 0.42.0)
include(${cpm_SOURCE_DIR}/CPM.cmake)

project(POET LANGUAGES CXX)

include(CTest)

option(POET_BUILD_BENCHMARKS "Build POET benchmark executables" OFF)
option(POET_STRICT_WARNINGS "Enable the project's default warning profile" ON)

if(NOT DEFINED POET_BUILD_TESTS)
  set(POET_BUILD_TESTS ${BUILD_TESTING})
endif()
option(POET_BUILD_TESTS "Build POET test suite" ${POET_BUILD_TESTS})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(poet INTERFACE)
add_library(poet::poet ALIAS poet)

target_compile_features(poet INTERFACE cxx_std_17)

target_include_directories(poet
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

include(PoetWarnings)
include(PoetSanitizers)
include(PoetStaticAnalysis)

if(POET_STRICT_WARNINGS)
  poet_enable_warnings(poet)
endif()

poet_enable_sanitizers(poet)
poet_configure_static_analysis(poet)

if(POET_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

if(POET_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

install(TARGETS poet
  EXPORT poet-targets
)

install(EXPORT poet-targets
  NAMESPACE poet::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poet
)

configure_package_config_file(
  cmake/poet-config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/poet-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poet
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/poet-config.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poet
)

install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PoetWarnings.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PoetSanitizers.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PoetStaticAnalysis.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poet
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/poet-config.cmake")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/poet-config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/poet
  )
endif()

set(CPACK_PACKAGE_NAME "poet")
set(CPACK_PACKAGE_VENDOR "POET Project")
set(CPACK_PACKAGE_CONTACT "support@example.com")
set(CPACK_GENERATOR "TGZ")

include(CPack)
