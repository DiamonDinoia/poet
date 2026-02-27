Installation
============

The POET library is header-only, which makes integration straightforward. The code can be obtained via Git and then integrated using CMake or standard Makefiles into a project.

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

If POET has been cloned (or added as a submodule) into a subdirectory (e.g., ``extern/poet``), it can be added to ``CMakeLists.txt``:

.. code-block:: cmake

   add_subdirectory(extern/poet)

   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE poet::poet)

Option 2: FetchContent
~~~~~~~~~~~~~~~~~~~~~~

CMake can download specific versions using ``FetchContent``:

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

Option 3: CPM.cmake
~~~~~~~~~~~~~~~~~~~

If `CPM.cmake <https://github.com/cpm-cmake/CPM.cmake>`_ is already used, POET can be fetched with:

.. code-block:: cmake

   CPMAddPackage(
     NAME poet
     GITHUB_REPOSITORY DiamonDinoia/poet
     GIT_TAG main
   )

   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE poet::poet)

Option 4: System Install
~~~~~~~~~~~~~~~~~~~~~~~~

POET can be installed to the system and used via ``find_package``:

.. code-block:: bash

   # Build and install POET
   git clone https://github.com/DiamonDinoia/poet.git
   cd poet
   cmake -B build
   cmake --build build
   sudo cmake --install build

   # Optional: install to a custom prefix
   # cmake --install build --prefix /custom/prefix

Then in the consuming project:

.. code-block:: cmake

   find_package(poet CONFIG REQUIRED)

   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE poet::poet)

Makefile Integration
--------------------

Since POET is header-only, no libraries need to be built. The compiler's include path should point to the ``include`` directory.

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
