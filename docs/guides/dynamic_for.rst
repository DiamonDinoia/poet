Dynamic Loops
=============

``poet::dynamic_for`` runs a runtime range by emitting compile-time unrolled blocks.

``Unroll`` is exact: the compiler does not unroll the main loop further, even
under ``-funroll-loops`` or with a constant iteration count. A range of exactly
``Unroll`` is one block and no loop.

Basic form
----------

.. code-block:: cpp

   poet::dynamic_for<4>(0u, n, [](std::size_t i) {
       out[i] = f(i);
   });

Other overloads:

- ``poet::dynamic_for<Unroll>(count, func)`` for ``[0, count)``
- ``poet::dynamic_for<Unroll>(begin, end, func)`` for inferred ``+1`` or ``-1`` step
- ``poet::dynamic_for<Unroll>(begin, end, step, func)`` for runtime step
- ``poet::dynamic_for<Unroll, Step>(begin, end, func)`` for compile-time step

Lane-aware callbacks
--------------------

The two-argument form exposes the lane within the current unrolled block:

.. code-block:: cpp

   std::array<double, 4> acc{};
   poet::dynamic_for<4>(0u, n, [&](auto lane, std::size_t i) {
       acc[lane] += work(i);
   });

This is the main performance-oriented use case. For trivial index-only work,
a plain ``for`` loop has less overhead.

Compile-time step
-----------------

.. code-block:: cpp

   poet::dynamic_for<4, 2>(0, 16, [](int i) {
       use(i); // 0, 2, 4, ..., 14
   });

   poet::dynamic_for<4, -1>(10, 0, [](int i) {
       use(i); // 10, 9, ..., 1
   });

C++20 adaptor
-------------

.. code-block:: cpp

   auto r = std::views::iota(0) | std::views::take(10);
   r | poet::make_dynamic_for<4>([](int i) {
       use(i);
   });

   std::tuple{0, 24, 2} | poet::make_dynamic_for<4>([](int i) {
       use(i);
   });

Notes:

- The adaptor is eager: it invokes ``dynamic_for`` immediately.
- The range overload passes the range's own elements to the callable and
  requires a random-access range, since the unrolled body indexes off ``begin``.
- Tuple input preserves explicit ``(begin, end, step)`` semantics.

Runnable example
----------------

- Full source: `examples/dynamic_for.cpp
  <https://github.com/flatironinstitute/poet/blob/main/examples/dynamic_for.cpp>`_
- |ce-badge| `Try on Compiler Explorer <https://flatironinstitute.github.io/poet/dynamic_for.html>`_

For a worked lane-aware-ILP example, see
`examples/dot_product.cpp
<https://github.com/flatironinstitute/poet/blob/main/examples/dot_product.cpp>`_
(|ce-badge| `Compiler Explorer <https://flatironinstitute.github.io/poet/dot_product.html>`_).
The microbench `examples/benchmark.cpp
<https://github.com/flatironinstitute/poet/blob/main/examples/benchmark.cpp>`_
runs scalar vs ``dynamic_for<4>`` vs ``dynamic_for<8>``. It also runs on
Compiler Explorer in Execute mode
(|ce-badge| `Run benchmark on Compiler Explorer <https://flatironinstitute.github.io/poet/benchmark.html>`_).

.. |ce-badge| image:: https://img.shields.io/badge/Compiler%20Explorer-open-67c52a?logo=compilerexplorer&logoColor=white
   :alt: Try on Compiler Explorer
