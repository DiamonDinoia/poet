POET Documentation
==================

POET is a compact, header-only C++ utility library providing three focused primitives for making performance-sensitive code easier to express and optimize:

- static_for — compile-time unrolled iteration for fixed ranges or template packs.
- dynamic_for — runtime loops implemented as compile-time unrolled blocks.
- dispatch / dispatch_tuples / DispatchSet — map runtime integers/tuples to compile-time non-type template parameters.

Quick start (one minute)
------------------------
1. Clone the repository:
```bash
git clone https://github.com/DiamonDinoia/poet.git
```

2. Use as headers-only:
- Add the repository's `include/` directory to your compiler include path and include the umbrella header:
```cpp
#include <poet/poet.hpp>
```

3. Or consume via CMake (recommended for downstream projects):
```cmake
add_subdirectory(path/to/poet)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE poet::poet)
```

When to read which section
- Getting started / install — minimal integration instructions for CMake and manual includes.
- Guides — focused how-tos for static_for, dynamic_for, and static_dispatch (one short, practical example per guide).
- API Reference — complete generated reference.

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

Overview
--------
POET aims to give you low-cost, compile-time-specialized control over small, hot kernels and dispatch logic. Read the short guides for practical usage patterns, then consult the API reference for detailed type/function signatures.