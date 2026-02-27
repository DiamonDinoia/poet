POET
==================

POET is a small, header-only C++ utility library that provides compile-time-oriented loop and dispatch primitives to make performance-sensitive metaprogramming simpler and safer. It exposes a few focused building blocks to enable concise, zero-cost-specialized kernels driven by runtime values.

Core primitives
---------------
- static_for — compile-time unrolled iteration for integer ranges or template packs.
- dynamic_for — runtime loops emitted as compile-time unrolled blocks for low-overhead iteration.
- dispatch / DispatchSet — map runtime integers or tuples to compile-time non-type template parameters for zero-cost specialization.
- register_info — compile-time CPU register and SIMD capability detection for tuning unroll factors and block sizes to the target ISA.

Why it matters
--------------
Exposing iteration counts and dispatch choices to the compiler enables inlining, unrolling, and better code generation. The result is tighter, faster code with fewer branches and improved register usage — especially useful in small, hot kernels and when selecting specialized implementations at runtime.

Performance benefits
--------------------
- Better register allocation and reduced spills via compile-time unrolling and specialization.
- Improved instruction scheduling and vectorization opportunities.
- Zero-cost runtime-to-compile-time dispatch for selecting specialized codepaths.

Common hotspots that benefit
----------------------------
- Small fixed-size linear algebra (small GEMM, batched matrix ops)
- Tight convolution / stencil kernels and DSP micro-kernels
- Fixed-size FFT/DFT implementations
- Hot parsing/serialization loops and small-state machines
- Kernel selection/specialization where runtime choices select optimized codepaths

Quick start
------------------------
Prerequisites
- C++17-capable compiler (GCC, Clang, MSVC)
- CMake 3.20+ (recommended for downstream integration)

Clone:
   .. code-block:: bash

      git clone https://github.com/DiamonDinoia/poet.git

Header-only usage
- Add the repository's ``include/`` directory to the compiler include path and include the umbrella header:

  .. code-block:: cpp

     #include <poet/poet.hpp>

  .. code-block:: bash

     g++ -std=c++17 -I/path/to/poet/include app.cpp -o app

CMake integration
- add_subdirectory (local development):

.. code-block:: cmake

   add_subdirectory(path/to/poet)
   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE poet::poet)

- find_package (after install):

.. code-block:: cmake

   # After installing POET with ``cmake --install``, discover it via CMake's package config
   find_package(poet CONFIG REQUIRED)
   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE poet::poet)

- FetchContent (downstream projects):

.. code-block:: cmake

   include(FetchContent)
   FetchContent_Declare(poet GIT_REPOSITORY https://github.com/DiamonDinoia/poet.git GIT_TAG main)
   FetchContent_MakeAvailable(poet)
   target_link_libraries(my_app PRIVATE poet::poet)

- CPM.cmake (downstream projects):

.. code-block:: cmake

   # Requires CPM.cmake available in the project
   # See https://github.com/cpm-cmake/CPM.cmake for setup

   CPMAddPackage(
     NAME poet
     GITHUB_REPOSITORY DiamonDinoia/poet
     GIT_TAG main
   )

   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE poet::poet)

Install with CMake
------------------

.. code-block:: bash

   # Configure and build
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel

   # Install (choose a prefix or rely on default)
   cmake --install build --prefix /custom/prefix

High-level usage pointers
-------------------------
- Read the short guides for practical patterns:
  - guides/static_for — when compile-time unrolling is needed.
  - guides/dynamic_for — when low-overhead runtime loops in unrolled blocks are desired.
  - guides/dispatch — when runtime choices must map to compile-time specializations.
- Consult the API Reference for exact signatures and template parameters.

Benchmark results
-----------------
POET ships with comprehensive nanobench benchmarks that run across GCC and Clang.
CI generates SVG charts automatically — see the :doc:`guides/benchmarks` page for
detailed results and analysis, or view the charts directly in the README.

Key findings:

- **dynamic_for** delivers consistent multi-accumulator ILP speedups by exposing
  independent accumulator chains via compile-time lane indices.
- **static_for** with register-aware block sizing avoids spill pressure while
  maximizing throughput.
- **dispatch** enables 2-5x speedups by letting the compiler fully unroll and
  optimize loops when N is a compile-time constant.
- Results are consistent across GCC and Clang, confirming the patterns are robust.

Throwing dispatch
-----------------
Some dispatch overloads accept the tag ``poet::throw_t`` (alias of ``throw_on_no_match_t``) and will throw ``poet::no_match_error`` (inherits from ``std::runtime_error``) when no compile-time match exists — useful for fatal configuration errors.

Documentation & license
-----------------------
- Full API docs and guides: https://poet.readthedocs.io/en/latest/
- License: MIT (see LICENSE file)

Contributing
------------
PRs and issues welcome.

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   install

.. toctree::
   :maxdepth: 2
   :caption: Guides

   guides/static_for
   guides/dynamic_for
   guides/dispatch
   guides/benchmarks

.. toctree::
   :maxdepth: 2
   :caption: API Reference

   api/library_root
