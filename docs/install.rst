Installation
============

POET is header-only. You can consume it directly from source or as an installed CMake package.

Local CMake project
-------------------

.. code-block:: cmake

   add_subdirectory(extern/poet)
   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE poet::poet)

Installed package
-----------------

Install:

.. code-block:: bash

   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   cmake --install build --prefix /custom/prefix

Consume:

.. code-block:: cmake

   find_package(poet CONFIG REQUIRED)
   target_link_libraries(my_app PRIVATE poet::poet)

FetchContent
------------

.. code-block:: cmake

   include(FetchContent)
   FetchContent_Declare(
     poet
     GIT_REPOSITORY https://github.com/DiamonDinoia/poet.git
     GIT_TAG main
   )
   FetchContent_MakeAvailable(poet)
   target_link_libraries(my_app PRIVATE poet::poet)

Non-CMake builds
----------------

Add ``include/`` to your compiler include path and include ``<poet/poet.hpp>``.
