POET Documentation
==================

POET is a small, header-only C++ utility library that provides compile-time-oriented loop and dispatch primitives to make performance-sensitive metaprogramming simpler and safer. It exposes a few focused building blocks so you can write concise, zero-cost-specialized kernels driven by runtime values.

Core primitives
---------------
- static_for — compile-time unrolled iteration for integer ranges or template packs.
- dynamic_for — runtime loops emitted as compile-time unrolled blocks for low-overhead iteration.
- dispatch / dispatch_tuples / DispatchSet — map runtime integers or tuples to compile-time non-type template parameters for zero-cost specialization.

Why it matters
--------------
Exposing iteration counts and dispatch choices to the compiler enables inlining, unrolling, and better code generation. The result is tighter, faster code with fewer branches and improved register usage — especially useful in small, hot kernels and when selecting specialized implementations at runtime.

Performance benefits
--------------------
- Better register allocation and reduced spills via compile-time unrolling and specialization.
- Improved instruction scheduling and vectorization opportunities.
- Zero-cost runtime-to-compile-time dispatch for selecting specialized codepaths.

Common hotspots that benefit
- Small fixed-size linear algebra (small GEMM, batched matrix ops)
- Tight convolution / stencil kernels and DSP micro-kernels
- Fixed-size FFT/DFT implementations
- Hot parsing/serialization loops and small-state machines
- Kernel selection/specialization where runtime choices select optimized codepaths

Quick start (one minute)
------------------------
Prerequisites
- C++17-capable compiler (GCC, Clang, MSVC)
- CMake 3.20+ (recommended for downstream integration)

Clone:
.. code-block:: bash

   git clone https://github.com/DiamonDinoia/poet.git

Header-only usage
- Add the repository's `include/` directory to your compiler include path and include the umbrella header:

  .. code-block:: cpp

     #include <poet/poet.hpp>

CMake integration (recommended)
- add_subdirectory (local development):

.. code-block:: cmake

   add_subdirectory(path/to/poet)
   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE poet::poet)

- FetchContent (downstream projects):

.. code-block:: cmake

   include(FetchContent)
   FetchContent_Declare(poet GIT_REPOSITORY https://github.com/DiamonDinoia/poet.git GIT_TAG main)
   FetchContent_MakeAvailable(poet)
   target_link_libraries(my_app PRIVATE poet::poet)

High-level usage pointers
- Read the short guides for practical patterns:
  - guides/static_for — when you need compile-time unrolling.
  - guides/dynamic_for — when you want low-overhead runtime loops in unrolled blocks.
  - guides/static_dispatch — when you must map runtime choices to compile-time specializations.
- Consult the API Reference for exact signatures and template parameters.

Throwing dispatch
-----------------
Some dispatch overloads accept the tag `poet::throw_t` (alias of `throw_on_no_match_t`) and will throw `std::runtime_error` when no compile-time match exists — useful for fatal configuration errors.

Documentation & license
-----------------------
- Full API docs and guides: docs/ (Sphinx/RST in repository)
- License: MIT (see LICENSE file)

Contributing
------------
PRs and issues welcome. Keep changes small and include tests for performance-sensitive code.

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   install

.. toctree::
   :maxdepth: 2
   :caption: Guides

   guides/static_for
   guides/dynamic_for
   guides/static_dispatch

.. toctree::
   :maxdepth: 2
   :caption: API Reference

   api/library_root

