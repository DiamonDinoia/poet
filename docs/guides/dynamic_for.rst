Dynamic Loops
=============

``poet::dynamic_for`` runs a runtime range by emitting compile-time unrolled blocks.

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
an ordinary ``for`` loop can be just as good or better.

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
- The generic range overload treats the input as a consecutive ``[start, start + count)`` sequence.
- Tuple input preserves explicit ``(begin, end, step)`` semantics.
