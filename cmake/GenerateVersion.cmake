# GenerateVersion.cmake
#
# Composes POET_VERSION_FULL from the tracked VERSION file plus git state and
# writes include/poet/version.hpp from include/poet/version.hpp.in.
#
# Usage
# -----
# - From the main project: include(cmake/GenerateVersion.cmake) during configure.
# - Standalone (pre-commit hook): cmake -P cmake/GenerateVersion.cmake
#   With -DCHECK=ON, exits 1 if the generated file would change — the hook then
#   re-stages the newly regenerated file.
#
# Version composition
# -------------------
# Let BASE = contents of ./VERSION (e.g. 0.0.0). Let TAG = `git describe
# --exact-match --tags HEAD` (stripped of a leading `v`). If TAG == BASE we are
# on an exact release commit and POET_VERSION_FULL = BASE. Otherwise the suffix
# is `-dev.N` where N = `git rev-list --count <last-v-tag>..HEAD` if any
# `v<BASE>` tag exists, else `git rev-list --count HEAD`.
#
# N is derived from committed history only — `git rev-list --count HEAD`
# ignores the staged index and the working tree, so during pre-commit the
# count is stable across retries of the same commit (HEAD hasn't moved).
# The count only advances when a new commit actually lands.

cmake_minimum_required(VERSION 3.20)

# Resolve project source dir — works both inside a CMake configure and under
# `cmake -P` (script mode).
if(CMAKE_SOURCE_DIR AND EXISTS "${CMAKE_SOURCE_DIR}/VERSION")
  set(_poet_src "${CMAKE_SOURCE_DIR}")
else()
  get_filename_component(_poet_src "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

file(READ "${_poet_src}/VERSION" POET_VERSION_STRING)
string(STRIP "${POET_VERSION_STRING}" POET_VERSION_STRING)

if(NOT POET_VERSION_STRING MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
  message(FATAL_ERROR "VERSION must be MAJOR.MINOR.PATCH, got: ${POET_VERSION_STRING}")
endif()
set(POET_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(POET_VERSION_MINOR "${CMAKE_MATCH_2}")
set(POET_VERSION_PATCH "${CMAKE_MATCH_3}")

find_package(Git QUIET)

set(_on_exact_tag FALSE)
set(_commit_count 0)

if(Git_FOUND AND EXISTS "${_poet_src}/.git")
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${_poet_src}" describe --exact-match --tags HEAD
    OUTPUT_VARIABLE _exact_tag
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _rc
  )
  if(_rc EQUAL 0)
    string(REGEX REPLACE "^v" "" _exact_tag_stripped "${_exact_tag}")
    if(_exact_tag_stripped STREQUAL POET_VERSION_STRING)
      set(_on_exact_tag TRUE)
    endif()
  endif()

  if(NOT _on_exact_tag)
    # Look for a v<BASE> tag so we can count commits since it.
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" -C "${_poet_src}" rev-parse --verify --quiet "v${POET_VERSION_STRING}"
      OUTPUT_QUIET
      ERROR_QUIET
      RESULT_VARIABLE _rc
    )
    if(_rc EQUAL 0)
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${_poet_src}" rev-list --count "v${POET_VERSION_STRING}..HEAD"
        OUTPUT_VARIABLE _commit_count
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    else()
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${_poet_src}" rev-list --count HEAD
        OUTPUT_VARIABLE _commit_count
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    endif()
  endif()

endif()

if(_on_exact_tag)
  set(POET_VERSION_FULL "${POET_VERSION_STRING}")
else()
  set(POET_VERSION_FULL "${POET_VERSION_STRING}-dev.${_commit_count}")
endif()

set(_out "${_poet_src}/include/poet/version.hpp")
set(_in "${_poet_src}/include/poet/version.hpp.in")
set(_tmp "${_out}.new")

configure_file("${_in}" "${_tmp}" @ONLY)

set(_changed FALSE)
if(NOT EXISTS "${_out}")
  set(_changed TRUE)
else()
  file(READ "${_tmp}" _tmp_content)
  file(READ "${_out}" _out_content)
  if(NOT "${_tmp_content}" STREQUAL "${_out_content}")
    set(_changed TRUE)
  endif()
endif()

if(_changed)
  file(RENAME "${_tmp}" "${_out}")
else()
  file(REMOVE "${_tmp}")
endif()

if(DEFINED CHECK AND CHECK)
  if(_changed)
    message(STATUS "POET version file regenerated to ${POET_VERSION_FULL}; please re-stage include/poet/version.hpp")
    message(FATAL_ERROR "version.hpp out of date")
  endif()
else()
  message(STATUS "POET version: ${POET_VERSION_FULL}")
endif()
