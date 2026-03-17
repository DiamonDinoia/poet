# Additional configuration helpers for projects consuming POET.
# This file is installed alongside the CMake package configuration when present.

include_guard(GLOBAL)

# Developer helper modules (warnings/sanitizers/static-analysis) are not
# installed with the package. Consumers should only get the exported targets
# from poet-targets.cmake.
