Static Loops
============

``poet::static_for`` expands an integer range at compile time.

Basic form
----------

.. code-block:: cpp

   poet::static_for<0, 4>([](auto I) {
       use(I);
   });

The callable may also be a template call operator:

.. code-block:: cpp

   poet::static_for<4>([]<auto I>() {
       use_compile_time_index<I>();
   });

Step and direction
------------------

.. code-block:: cpp

   poet::static_for<0, 10, 2>([](auto I) {
       // 0, 2, 4, 6, 8
   });

   poet::static_for<9, 4, -1>([](auto I) {
       // 9, 8, 7, 6, 5
   });

Block size
----------

The default block size covers the full range. Pass the fourth template parameter
when a large body benefits from smaller outlined blocks:

.. code-block:: cpp

   poet::static_for<0, 64, 1, 8>([](auto I) {
       heavy_work(I);
   });

Use this only when profiling shows register-pressure or compile-time issues.

Runnable example
----------------

- Full source: `examples/static_for.cpp
  <https://github.com/flatironinstitute/poet/blob/main/examples/static_for.cpp>`_
- |ce-badge| `Try on Compiler Explorer <https://flatironinstitute.github.io/poet/static_for.html>`_

.. |ce-badge| image:: https://img.shields.io/badge/Compiler%20Explorer-open-67c52a?logo=compilerexplorer&logoColor=white
   :alt: Try on Compiler Explorer
