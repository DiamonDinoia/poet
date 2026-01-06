include_guard(GLOBAL)

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
      # Default clang-tidy checks: enable everything but exclude checks known to
      # crash or produce false-positives on bundled third-party headers.
      if(POET_CLANG_TIDY_CHECKS)
        # allow user to override but append safe exclusions
        set(_clang_tidy_command "${_clang_tidy_command};-checks=${POET_CLANG_TIDY_CHECKS},-bugprone-argument-comment")
      else()
        set(_clang_tidy_command "${_clang_tidy_command};-checks=*,-bugprone-argument-comment,-llvmlibc-*,-altera-*,-fuchsia-*,-cppcoreguidelines-avoid-do-while,-cert-err58-cpp,-misc-use-anonymous-namespace")
      endif()
      # Restrict analysis to POET sources/headers to exclude third-party code
      # append a header-filter that matches project include and src trees
      set(_clang_tidy_command "${_clang_tidy_command};-header-filter=^${PROJECT_SOURCE_DIR}/(include/poet|src)")
      # Ensure clang invoked in syntax-only mode to avoid emitting object files
      set(_clang_tidy_command "${_clang_tidy_command};--extra-arg=-fsyntax-only")
      # treat warnings as errors for project headers (honour global flag)
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
