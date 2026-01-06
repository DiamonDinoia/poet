Installation
============

The POET library is header-only, which makes integration straightforward. You can obtain the code via Git and then use CMake or standard Makefiles to include it in your project.

Git Clone
---------

Start by cloning the repository:

.. code-block:: bash

   git clone https://github.com/DiamonDinoia/poet.git

CMake Integration
-----------------

POET is designed to be easily integrated into CMake projects.

Option 1: add_subdirectory
~~~~~~~~~~~~~~~~~~~~~~~~~~

If you have cloned (or added as a submodule) POET into a subdirectory (e.g., ``extern/poet``), you can simply add it to your ``CMakeLists.txt``:

.. code-block:: cmake

   add_subdirectory(extern/poet)
   
   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE poet::poet)

Option 2: FetchContent
~~~~~~~~~~~~~~~~~~~~~~

You can let CMake download specific versions using ``FetchContent``:

.. code-block:: cmake

   include(FetchContent)
   
   FetchContent_Declare(
     poet
     GIT_REPOSITORY https://github.com/DiamonDinoia/poet.git
     GIT_TAG        main  # or a specific commit/tag
   )
   
   FetchContent_MakeAvailable(poet)
   
   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE poet::poet)

Option 3: System Install
~~~~~~~~~~~~~~~~~~~~~~~~

You can install POET to your system and use ``find_package``:

.. code-block:: bash

   # Build and install POET
   git clone https://github.com/DiamonDinoia/poet.git
   cd poet
   cmake -B build
   cmake --build build
   sudo cmake --install build

Then in your project:

.. code-block:: cmake

   find_package(poet CONFIG REQUIRED)
   
   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE poet::poet)

Makefile Integration
--------------------

Since POET is header-only, you do not need to build any libraries. Simply point your compiler's include path to the ``include`` directory.

Assuming the POET repository is located at ``/path/to/poet``:

.. code-block:: makefile

   CXX = g++
   CXXFLAGS = -std=c++17 -O3 -I/path/to/poet/include
   
   main: main.cpp
       $(CXX) $(CXXFLAGS) -o main main.cpp

Requirements
------------

*   C++17 compliant compiler (GCC 7+, Clang 6+, MSVC 2019+)
*   CMake 3.20+ (for CMake integration/testing)
