include_guard(GLOBAL)

# ==============================================================================
# POET Development Helpers
# ==============================================================================
# CMake helper functions and targets for POET development:
# - poet_enable_warnings
# - poet_enable_sanitizers
# - poet_configure_static_analysis
# - docs targets (doxygen, sphinx, docs)
# - coverage target
#
# Development and test builds only; not needed to consume POET header-only.
# ==============================================================================

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
# Warnings helper
# -------------------------
option(POET_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

# Apply the warning profile to a target. INTERFACE scope for interface
# libraries, PRIVATE otherwise.
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

  # Warnings enabled only when the compiler supports them (GCC-specific flags).
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

  # Build compiler-specific warning flags as generator expressions. A flag
  # applies only to C++ code built with the matching compiler.
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
# Sanitizers helper
# -------------------------
option(POET_ENABLE_SANITIZERS "Master switch to enable all sanitizers at once" OFF)

option(POET_ENABLE_ASAN "Enable AddressSanitizer (memory error detection)" ${POET_ENABLE_SANITIZERS})
option(POET_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer (undefined behavior detection)" ${POET_ENABLE_SANITIZERS})

# Apply sanitizer flags to a target. Two flag strings, so no dependency on a
# sanitizers module.
#
# -fno-sanitize-recover=all matters as much as the sanitizers themselves:
# without this flag, UBSan prints a diagnostic and continues, so ctest still
# reports success.
function(poet_enable_sanitizers target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "poet_enable_sanitizers called with non-existent target '${target}'")
  endif()

  set(_sanitizers)
  if(POET_ENABLE_ASAN)
    list(APPEND _sanitizers address)
  endif()
  if(POET_ENABLE_UBSAN)
    list(APPEND _sanitizers undefined)
  endif()
  if(NOT _sanitizers)
    return()
  endif()

  if(MSVC)
    # cl.exe implements only AddressSanitizer, and it takes no link flag.
    if(POET_ENABLE_ASAN)
      set(_flags /fsanitize=address)
    else()
      message(WARNING "POET: MSVC has no UndefinedBehaviorSanitizer; UBSan request ignored")
      return()
    endif()
    set(_link_flags)
  else()
    string(JOIN "," _list ${_sanitizers})
    set(_flags -fsanitize=${_list} -fno-omit-frame-pointer -fno-sanitize-recover=all)
    set(_link_flags -fsanitize=${_list})
  endif()

  get_target_property(_target_type "${target}" TYPE)
  if(_target_type STREQUAL "INTERFACE_LIBRARY")
    set(_scope INTERFACE)
  else()
    set(_scope PRIVATE)
  endif()

  target_compile_options(${target} ${_scope} ${_flags})
  if(_link_flags)
    target_link_options(${target} ${_scope} ${_link_flags})
  endif()

  # Record the applied flags, so poet_print_summary() reports effective state
  # instead of the requested options.
  set_property(GLOBAL PROPERTY POET_APPLIED_SANITIZERS "${_flags}")
  set_property(GLOBAL APPEND PROPERTY POET_SANITIZED_TARGETS "${target}")
endfunction()

# -------------------------
# Static analysis helper
# -------------------------
option(POET_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" ON)
# STRING, not option(): an option() default is a boolean, so a string default
# would collapse to OFF and the value would be lost.
set(POET_CLANG_TIDY_CHECKS "" CACHE STRING
  "Override default clang-tidy checks (leave empty to use the .clang-tidy config)")
option(POET_CLANG_TIDY_WARNINGS_AS_ERRORS "Treat clang-tidy warnings as errors" ON)
option(POET_ENABLE_CPPCHECK "Enable cppcheck static analysis" ON)
set(POET_CPPCHECK_OPTIONS "--enable=warning,style,performance,portability" CACHE STRING
  "Additional cppcheck options")

# Configure clang-tidy and/or cppcheck for a target. A tool that is absent from
# PATH produces a warning, not a failure.
function(poet_configure_static_analysis target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "poet_configure_static_analysis called with non-existent target '${target}'")
  endif()

  if(POET_ENABLE_CLANG_TIDY)
    find_program(_clang_tidy_exe NAMES clang-tidy clang-tidy-17 clang-tidy-16)
    if(_clang_tidy_exe)
      set(_clang_tidy_command "${_clang_tidy_exe}")
      # When POET_CLANG_TIDY_CHECKS is empty, clang-tidy reads the .clang-tidy
      # config from the source tree.
      if(POET_CLANG_TIDY_CHECKS)
        set(_clang_tidy_command "${_clang_tidy_command};-checks=${POET_CLANG_TIDY_CHECKS}")
      endif()
      # Restrict header analysis to the project's own headers, not external
      # dependencies.
      set(_clang_tidy_command "${_clang_tidy_command};-header-filter=^${PROJECT_SOURCE_DIR}/(include/poet|src)")
      # Syntax-only checks keep the analysis fast.
      set(_clang_tidy_command "${_clang_tidy_command};--extra-arg=-fsyntax-only")
      if(POET_CLANG_TIDY_WARNINGS_AS_ERRORS)
        set(_clang_tidy_command "${_clang_tidy_command};-warnings-as-errors=*")
      endif()
      set_property(TARGET ${target} PROPERTY CXX_CLANG_TIDY "${_clang_tidy_command}")
      set_property(GLOBAL PROPERTY POET_CLANG_TIDY_RESOLVED "${_clang_tidy_exe}")
    else()
      message(WARNING "POET_ENABLE_CLANG_TIDY is ON but clang-tidy was not found on PATH")
      set_property(GLOBAL PROPERTY POET_CLANG_TIDY_RESOLVED "NOT FOUND")
    endif()
  endif()

  if(POET_ENABLE_CPPCHECK)
    find_program(_cppcheck_exe NAMES cppcheck)
    if(_cppcheck_exe)
      set(_cppcheck_command "${_cppcheck_exe};--inline-suppr;${POET_CPPCHECK_OPTIONS}")
      set_property(TARGET ${target} PROPERTY CXX_CPPCHECK "${_cppcheck_command}")
      set_property(GLOBAL PROPERTY POET_CPPCHECK_RESOLVED "${_cppcheck_exe}")
    else()
      message(WARNING "POET_ENABLE_CPPCHECK is ON but cppcheck was not found on PATH")
      set_property(GLOBAL PROPERTY POET_CPPCHECK_RESOLVED "NOT FOUND")
    endif()
  endif()
endfunction()

# -------------------------
# Docs helper
# -------------------------
option(POET_GENERATE_DOCS "Generate documentation using Doxygen + Sphinx pipeline" OFF)

if(POET_GENERATE_DOCS)
    find_package(Doxygen REQUIRED)
    find_program(SPHINX_BUILD_EXECUTABLE NAMES sphinx-build REQUIRED)
    find_package(Python COMPONENTS Interpreter REQUIRED)

    # Sphinx needs the breathe and exhale Python packages.
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
# Coverage target
# -------------------------
# The `coverage` custom target builds all test executables, runs ctest,
# collects coverage, and writes an HTML report.
find_program(GCOVR_EXECUTABLE gcovr)
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

# Prefer lcov+genhtml; fall back to gcovr.
if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
  set(LCOV_INFO ${CMAKE_BINARY_DIR}/coverage.info)
  set(LCOV_FILTERED ${CMAKE_BINARY_DIR}/coverage.filtered.info)
  set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)

  add_custom_target(coverage
    DEPENDS poet_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
    COMMAND ${LCOV_EXECUTABLE} --capture --directory ${CMAKE_BINARY_DIR} --output-file ${LCOV_INFO} --ignore-errors inconsistent,unused
    # Drop system headers (/usr/*) and FetchContent dependencies (*/_deps/*);
    # the report covers project sources only.
    COMMAND ${LCOV_EXECUTABLE} --remove ${LCOV_INFO} "/usr/*" "*/_deps/*" --output-file ${LCOV_FILTERED} --ignore-errors inconsistent,unused
    COMMAND ${GENHTML_EXECUTABLE} -o ${COVERAGE_DIR} ${LCOV_FILTERED}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests and generating coverage report (lcov+genhtml) -> ${COVERAGE_DIR}/index.html"
    VERBATIM
  )
elseif(GCOVR_EXECUTABLE)
  add_custom_target(coverage
    DEPENDS poet_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
    # Cover project sources (include/poet and tests), not dependencies or system headers.
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

# The coverage target depends on every standard-specific test target (C++17,
# C++20, C++23), so ctest never runs against stale binaries.
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

# -------------------------
# Configuration summary
# -------------------------
# Report effective state, not requested options. This makes a silently inactive
# tool visible (missing binary, unsupported compiler, a helper that returned
# early); the only other way to tell is grepping compile_commands.json.
function(poet_print_summary)
  get_property(_san GLOBAL PROPERTY POET_APPLIED_SANITIZERS)
  get_property(_san_targets GLOBAL PROPERTY POET_SANITIZED_TARGETS)
  get_property(_tidy GLOBAL PROPERTY POET_CLANG_TIDY_RESOLVED)
  get_property(_cppcheck GLOBAL PROPERTY POET_CPPCHECK_RESOLVED)

  message(STATUS "")
  message(STATUS "── POET ${POET_VERSION_FULL} ──────────────────────────────")
  message(STATUS "  compiler        : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
  message(STATUS "  build type      : ${CMAKE_BUILD_TYPE}")
  message(STATUS "  tests/examples/benchmarks : ${POET_BUILD_TESTS}/${POET_BUILD_EXAMPLES}/${POET_BUILD_BENCHMARKS}")
  message(STATUS "  strict warnings : ${POET_STRICT_WARNINGS}")

  if(_san)
    list(LENGTH _san_targets _n)
    message(STATUS "  sanitizers      : ACTIVE on ${_n} target(s) -- ${_san}")
  elseif(POET_ENABLE_ASAN OR POET_ENABLE_UBSAN)
    message(WARNING "POET: sanitizers requested but no flags were applied to any target")
  else()
    message(STATUS "  sanitizers      : off")
  endif()

  foreach(_tool tidy cppcheck)
    if(_tool STREQUAL tidy)
      set(_want ${POET_ENABLE_CLANG_TIDY})
      set(_got "${_tidy}")
      set(_label "clang-tidy      ")
    else()
      set(_want ${POET_ENABLE_CPPCHECK})
      set(_got "${_cppcheck}")
      set(_label "cppcheck        ")
    endif()
    if(NOT _want)
      message(STATUS "  ${_label}: off")
    elseif(_got STREQUAL "NOT FOUND" OR _got STREQUAL "")
      message(STATUS "  ${_label}: REQUESTED BUT INACTIVE")
    else()
      message(STATUS "  ${_label}: ${_got}")
    endif()
  endforeach()
  message(STATUS "─────────────────────────────────────────────────────────")
  message(STATUS "")
endfunction()
