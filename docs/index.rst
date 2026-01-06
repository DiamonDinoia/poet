POET Documentation
==================

POET (Performance Optimized Excessive Templates) is a collection of C++ templates 
designed for high-performance and embedded scenarios. It provides tools for 
static loop unrolling, compile-time dispatch, and other meta-programming utilities.

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

Usage
-----

To use POET, simply include the headers you need from `include/poet/`. 
The library is header-only.

Example::

   #include <poet/poet.hpp>
   
   void example() {
       poet::static_for<0, 10>([](auto i) {
           // efficient unrolled loop
       });
   }
