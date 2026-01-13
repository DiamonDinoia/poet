include_guard(GLOBAL)

# Prepare CPM (only used for developer/test builds)
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

include(${cpm_SOURCE_DIR}/CPM.cmake)

# -------------------------
# Warnings helper (from PoetWarnings.cmake)
# -------------------------
option(POET_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

function(poet_enable_warnings target)
  if (NOT TARGET "${target}")
    message(FATAL_ERROR "poet_enable_warnings called with non-existent target '${target}'")
  endif()

  get_target_property(_target_type "${target}" TYPE)
  if(_target_type STREQUAL "INTERFACE_LIBRARY")
    set(_scope INTERFACE)
  else()
    set(_scope PRIVATE)
  endif()

  set(_clang_like $<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>)
  set(_gnu $<CXX_COMPILER_ID:GNU>)
  set(_gnu_or_clang $<OR:${_gnu},${_clang_like}>)
  set(_msvc $<CXX_COMPILER_ID:MSVC>)
  set(_lang_is_cxx $<COMPILE_LANGUAGE:CXX>)

  set(_warnings_clang_like
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wdouble-promotion
    -Wold-style-cast
    -Wnon-virtual-dtor
    -Wnull-dereference
    -Woverloaded-virtual
    -Wcast-align
    -Wunused
    -Wimplicit-fallthrough
    -Wformat=2
  )

  # Additional curated warnings to enable where supported (only applies to project sources)
  set(_additional_warnings
    -Wduplicated-cond
    -Wlogical-op
    -Wuseless-cast
    -Winit-self
    -Wmissing-include-dirs
    -Wredundant-decls
  )

  include(CheckCXXCompilerFlag)
  foreach(_f IN LISTS _additional_warnings)
    check_cxx_compiler_flag("${_f}" _flag_supported)
    if(_flag_supported)
      list(APPEND _warnings_clang_like ${_f})
    endif()
  endforeach()

  set(_warnings_gnu_only
    -Wmisleading-indentation
    -Wsuggest-override
  )

  set(_warnings_msvc
    /W4
    /permissive-
    /bigobj
    /w14242
    /w14254
    /w14263
    /w14265
    /w14287
    /we4289
    /w14296
    /w14311
    /w14545
    /w14546
    /w14547
    /w14549
    /w14555
    /w14619
    /w14640
    /w14826
    /w14905
    /w14906
    /w14928
  )

  set(_compile_options)
  foreach(flag IN LISTS _warnings_clang_like)
    list(APPEND _compile_options $<$<AND:${_lang_is_cxx},${_gnu_or_clang}>:${flag}>)
  endforeach()
  foreach(flag IN LISTS _warnings_gnu_only)
    list(APPEND _compile_options $<$<AND:${_lang_is_cxx},${_gnu}>:${flag}>)
  endforeach()
  foreach(flag IN LISTS _warnings_msvc)
    list(APPEND _compile_options $<$<AND:${_lang_is_cxx},${_msvc}>:${flag}>)
  endforeach()

  if(POET_WARNINGS_AS_ERRORS)
    list(APPEND _compile_options $<$<AND:${_lang_is_cxx},${_gnu_or_clang}>:-Werror>)
    list(APPEND _compile_options $<$<AND:${_lang_is_cxx},${_msvc}>:/WX>)
  endif()

  target_compile_options(${target} ${_scope} ${_compile_options})
endfunction()

# -------------------------
# Sanitizers helper (from PoetSanitizers.cmake)
# -------------------------
option(POET_ENABLE_SANITIZERS "Master switch for all sanitizers" OFF)

option(POET_ENABLE_ASAN "Enable AddressSanitizer" ${POET_ENABLE_SANITIZERS})
option(POET_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" ${POET_ENABLE_SANITIZERS})

function(_poet_any_sanitizer_enabled out_var)
  if(POET_ENABLE_ASAN OR POET_ENABLE_UBSAN)
    set(${out_var} TRUE PARENT_SCOPE)
  else()
    set(${out_var} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(_poet_prepare_sanitizers out_var)
  get_property(_prepared GLOBAL PROPERTY _poet_sanitizers_prepared SET)
  if(_prepared)
    get_property(_prepared_value GLOBAL PROPERTY _poet_sanitizers_prepared)
    set(${out_var} ${_prepared_value} PARENT_SCOPE)
    return()
  endif()

  set(_status TRUE)

  CPMAddPackage(
    NAME SanitizersCMake
    GITHUB_REPOSITORY arsenm/sanitizers-cmake
    GIT_TAG 0573e2ea8651b9bb3083f193c41eb086497cc80a
    DOWNLOAD_ONLY YES
    OPTIONS "CMAKE_POLICY_VERSION_MINIMUM 3.10"
  )

  if(NOT SanitizersCMake_SOURCE_DIR)
    set(_status FALSE)
  endif()

  if(_status)
    list(APPEND CMAKE_MODULE_PATH "${SanitizersCMake_SOURCE_DIR}/cmake")

    set(SANITIZE_ADDRESS ${POET_ENABLE_ASAN} CACHE BOOL
      "Enable AddressSanitizer for sanitized targets." FORCE)
    mark_as_advanced(SANITIZE_ADDRESS)

    set(SANITIZE_UNDEFINED ${POET_ENABLE_UBSAN} CACHE BOOL
      "Enable UndefinedBehaviorSanitizer for sanitized targets." FORCE)
    mark_as_advanced(SANITIZE_UNDEFINED)

    find_package(Sanitizers REQUIRED QUIET)
  endif()

  set_property(GLOBAL PROPERTY _poet_sanitizers_prepared ${_status})
  set(${out_var} ${_status} PARENT_SCOPE)
endfunction()

function(poet_enable_sanitizers target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "poet_enable_sanitizers called with non-existent target '${target}'")
  endif()

  _poet_any_sanitizer_enabled(_poet_any_enabled)
  if(NOT _poet_any_enabled)
    return()
  endif()

  _poet_prepare_sanitizers(_poet_sanitizers_ready)
  if(NOT _poet_sanitizers_ready)
    return()
  endif()

  get_target_property(_target_type "${target}" TYPE)

  if(_target_type STREQUAL "INTERFACE_LIBRARY")
    if(NOT CMAKE_CXX_COMPILER_ID)
      message(FATAL_ERROR "poet_enable_sanitizers requires a C++ compiler when sanitizers are enabled")
    endif()

    set(_requested_sanitizers)
    if(POET_ENABLE_ASAN)
      list(APPEND _requested_sanitizers ASan)
    endif()
    if(POET_ENABLE_UBSAN)
      list(APPEND _requested_sanitizers UBSan)
    endif()

    foreach(_sanitizer IN LISTS _requested_sanitizers)
      set(_flag_var "${_sanitizer}_${CMAKE_CXX_COMPILER_ID}_FLAGS")
      if(NOT DEFINED ${_flag_var} OR "${${_flag_var}}" STREQUAL "")
        message(FATAL_ERROR
          "${_sanitizer} is not supported for compiler '${CMAKE_CXX_COMPILER_ID}' in the current toolchain")
      endif()

      separate_arguments(_sanitizer_flag_list UNIX_COMMAND "${${_flag_var}}")
      if(_sanitizer_flag_list)
        target_compile_options(${target} INTERFACE ${_sanitizer_flag_list})
        target_link_options(${target} INTERFACE ${_sanitizer_flag_list})
      endif()
    endforeach()
  else()
    add_sanitizers(${target})
  endif()
endfunction()

# -------------------------
# Static analysis helper (from PoetStaticAnalysis.cmake)
# -------------------------
option(POET_ENABLE_CLANG_TIDY "Enable clang-tidy diagnostics" ON)
option(POET_CLANG_TIDY_CHECKS "Override clang-tidy checks" "")
option(POET_CLANG_TIDY_WARNINGS_AS_ERRORS "Treat clang-tidy warnings as errors" ON)
option(POET_ENABLE_CPPCHECK "Enable cppcheck diagnostics" ON)
option(POET_CPPCHECK_OPTIONS "Additional cppcheck options" "--enable=warning,style,performance,portability")

function(poet_configure_static_analysis target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "poet_configure_static_analysis called with non-existent target '${target}'")
  endif()

  if(POET_ENABLE_CLANG_TIDY)
    find_program(_clang_tidy_exe NAMES clang-tidy clang-tidy-17 clang-tidy-16)
    if(_clang_tidy_exe)
      set(_clang_tidy_command "${_clang_tidy_exe}")
      if(POET_CLANG_TIDY_CHECKS)
        set(_clang_tidy_command "${_clang_tidy_command};-checks=${POET_CLANG_TIDY_CHECKS},-bugprone-argument-comment")
      else()
        set(_clang_tidy_command "${_clang_tidy_command};-checks=*,-bugprone-argument-comment,-llvmlibc-*,-altera-*,-fuchsia-*,-cppcoreguidelines-avoid-do-while,-cert-err58-cpp,-misc-use-anonymous-namespace")
      endif()
      set(_clang_tidy_command "${_clang_tidy_command};-header-filter=^${PROJECT_SOURCE_DIR}/(include/poet|src)")
      set(_clang_tidy_command "${_clang_tidy_command};--extra-arg=-fsyntax-only")
      if(POET_CLANG_TIDY_WARNINGS_AS_ERRORS)
        set(_clang_tidy_command "${_clang_tidy_command};-warnings-as-errors=*")
      endif()
      set_property(TARGET ${target} PROPERTY CXX_CLANG_TIDY "${_clang_tidy_command}")
    else()
      message(WARNING "POET_ENABLE_CLANG_TIDY is ON but clang-tidy was not found on PATH")
    endif()
  endif()

  if(POET_ENABLE_CPPCHECK)
    find_program(_cppcheck_exe NAMES cppcheck)
    if(_cppcheck_exe)
      set(_cppcheck_command "${_cppcheck_exe};${POET_CPPCHECK_OPTIONS}")
      set_property(TARGET ${target} PROPERTY CXX_CPPCHECK "${_cppcheck_command}")
    else()
      message(WARNING "POET_ENABLE_CPPCHECK is ON but cppcheck was not found on PATH")
    endif()
  endif()
endfunction()

# -------------------------
# Docs helper (from PoetDocs.cmake)
# -------------------------
option(POET_GENERATE_DOCS "Generate documentation (Doxygen + Sphinx)" OFF)

if(POET_GENERATE_DOCS)
    find_package(Doxygen REQUIRED)
    find_program(SPHINX_BUILD_EXECUTABLE NAMES sphinx-build REQUIRED)
    find_package(Python COMPONENTS Interpreter REQUIRED)

    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import breathe, exhale"
        RESULT_VARIABLE DOCS_DEPS_CHECK_RESULT
        OUTPUT_QUIET ERROR_QUIET
    )

    if(NOT DOCS_DEPS_CHECK_RESULT EQUAL 0)
        message(WARNING "Python packages 'breathe' and 'exhale' not found. Docs generation may fail. Please run 'pip install -r docs/requirements.txt'.")
    endif()

    configure_file(${CMAKE_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_BINARY_DIR}/docs/Doxyfile @ONLY)

    add_custom_target(doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/docs
        COMMENT "Generating API documentation with Doxygen"
    )

    add_custom_target(sphinx
        DEPENDS doxygen
        COMMAND ${CMAKE_COMMAND} -E env DOXYGEN_XML_OUTPUT=${CMAKE_BINARY_DIR}/docs/xml
                ${SPHINX_BUILD_EXECUTABLE} -b html
                ${CMAKE_SOURCE_DIR}/docs ${CMAKE_BINARY_DIR}/docs/_build/html
        COMMENT "Generating HTML documentation with Sphinx"
    )

    add_custom_target(docs DEPENDS sphinx)
    message(STATUS "POET: Documentation targets enabled (doxygen, sphinx, docs)")
endif()

# -------------------------
# Coverage target (moved from top-level)
# -------------------------
# Adds a `coverage` custom target that runs tests and produces an HTML report.
# It prefers `gcovr` if available, otherwise falls back to `lcov` + `genhtml`.
find_program(GCOVR_EXECUTABLE gcovr)
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

# Prefer lcov+genhtml when available (more robust on complex build trees).
if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
  set(LCOV_INFO ${CMAKE_BINARY_DIR}/coverage.info)
  set(LCOV_FILTERED ${CMAKE_BINARY_DIR}/coverage.filtered.info)
  set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)

  add_custom_target(coverage
    DEPENDS poet_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
    COMMAND ${LCOV_EXECUTABLE} --capture --directory ${CMAKE_BINARY_DIR} --output-file ${LCOV_INFO} --ignore-errors inconsistent,unused
    # remove system and external deps (allow patterns to be adjusted)
    COMMAND ${LCOV_EXECUTABLE} --remove ${LCOV_INFO} "/usr/*" "*/_deps/*" --output-file ${LCOV_FILTERED} --ignore-errors inconsistent,unused
    COMMAND ${GENHTML_EXECUTABLE} -o ${COVERAGE_DIR} ${LCOV_FILTERED}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests and generating coverage report (lcov+genhtml) -> ${COVERAGE_DIR}/index.html"
    VERBATIM
  )
elseif(GCOVR_EXECUTABLE)
  # Fall back to gcovr if lcov/genhtml aren't available
  add_custom_target(coverage
    DEPENDS poet_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
    # Filter to project sources (include/poet and tests) to avoid dependency noise
    COMMAND ${GCOVR_EXECUTABLE} -r ${CMAKE_SOURCE_DIR} --filter "include/poet/|tests/" --exclude ".*/_deps/.*" --exclude "/usr/.*" --gcov-ignore-errors=no_working_dir_found --html --html-details -o ${CMAKE_BINARY_DIR}/coverage-report.html
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests and generating coverage report (gcovr) -> ${CMAKE_BINARY_DIR}/coverage-report.html"
    VERBATIM
  )
else()
  message(STATUS "Coverage tools not found: install 'lcov'+'genhtml' or 'gcovr' to enable the 'coverage' target.")
  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage tools missing. Install 'lcov'+'genhtml' or 'gcovr' and re-run CMake to enable coverage generation."
  )
endif()

# Ensure coverage target builds all test executables before running ctest.
if(TARGET coverage)
  set(POET_TEST_STANDARDS 23 20 17)
  foreach(POET_STD IN LISTS POET_TEST_STANDARDS)
    if(TARGET poet_tests_std${POET_STD})
      add_dependencies(coverage poet_tests_std${POET_STD})
    endif()
  endforeach()
  if(TARGET poet_tests)
    add_dependencies(coverage poet_tests)
  endif()
endif()
