# GenerateVersion.cmake
#
# Composes POET_VERSION_FULL from the tracked VERSION file plus git state and
# writes include/poet/version.hpp from include/poet/version.hpp.in.
#
# Usage
# -----
# - From the main project: include(cmake/GenerateVersion.cmake) during configure.
# - Standalone: cmake -P cmake/GenerateVersion.cmake, required once for
#   non-CMake consumers, since include/poet/version.hpp is not tracked in git.
#   With -DCHECK=ON, exits 1 if the generated file would change.
#
# Version composition
# -------------------
# BASE = contents of ./VERSION (e.g. 0.0.0). TAG = `git describe --exact-match
# --tags HEAD`, stripped of a leading `v`. When TAG == BASE, HEAD is an exact
# release commit and POET_VERSION_FULL = BASE. Otherwise the suffix is `-dev.N`,
# where N is the number of commits since the nearest reachable tag (whole
# history when the repo has no tags). Without git (release tarball, vendored
# copy) POET_VERSION_FULL = BASE.
#
# N comes from committed history only. N advances on a new commit, never on
# a change in the index or the working tree.

cmake_minimum_required(VERSION 3.20)

# poet's root, resolved relative to this file (poet/cmake/GenerateVersion.cmake).
# Works standalone, under add_subdirectory, and with `cmake -P`. Do NOT use
# CMAKE_SOURCE_DIR: under add_subdirectory it points at the consumer, not poet.
get_filename_component(_poet_src "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

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

# No git to consult (release tarball, vendored copy): the VERSION file is the
# only authority. Reporting `-dev.0` would label a pristine release archive as
# a prerelease, and semver sorts a prerelease below the release itself.
set(_have_git_info FALSE)

if(Git_FOUND AND EXISTS "${_poet_src}/.git")
  set(_have_git_info TRUE)
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
    # Count from the nearest reachable tag, not from v<BASE>. Right after a
    # release the base is already bumped, so no v<BASE> tag exists yet, and
    # counting the whole history would report a meaningless total.
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" -C "${_poet_src}" describe --tags --abbrev=0
      OUTPUT_VARIABLE _last_tag
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _rc
    )
    if(NOT _rc EQUAL 0)
      set(_range "HEAD")# no tag anywhere: count all of history
    else()
      set(_range "${_last_tag}..HEAD")
    endif()
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" -C "${_poet_src}" rev-list --count "${_range}"
      OUTPUT_VARIABLE _commit_count
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()

endif()

if(_on_exact_tag OR NOT _have_git_info)
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
