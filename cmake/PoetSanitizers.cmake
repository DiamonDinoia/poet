include_guard(GLOBAL)

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
