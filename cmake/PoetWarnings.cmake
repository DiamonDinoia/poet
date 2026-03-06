#[=======================================================================[.rst:
PoetWarnings
-------------

Helper functions to enable consistent compiler warnings.
#]=======================================================================]

include_guard(GLOBAL)

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
