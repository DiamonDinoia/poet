Dynamic Loops (dynamic_for)
===========================

Overview
--------

``poet::dynamic_for`` executes runtime ranges using compile-time unrolled blocks.
The unroll factor is an explicit template parameter.

Usage
-----

Basic Usage (Zero-based)
~~~~~~~~~~~~~~~~~~~~~~~~

The zero-based overload iterates ``[0, count)``.

.. code-block:: cpp

   std::size_t n = 100;

   poet::dynamic_for<4>(n, [&](std::size_t i) {
       data[i] = compute(i);
   });

Explicit Range
~~~~~~~~~~~~~~

Use ``[begin, end)`` with automatic direction:
- ``begin <= end``: step ``+1``
- ``begin > end``: step ``-1``

.. code-block:: cpp

   poet::dynamic_for<4>(10, 50, [](int i) {
       // i = 10..49
   });

   poet::dynamic_for<4>(10, 5, [](int i) {
       // i = 10,9,8,7,6
   });

Explicit Step
~~~~~~~~~~~~~

Use the 4-argument overload when you need a custom stride.

.. code-block:: cpp

   poet::dynamic_for<4>(0, 10, 2, [](int i) {
       // i = 0,2,4,6,8
   });

   poet::dynamic_for<4>(10, 0, -2, [](int i) {
       // i = 10,8,6,4,2
   });

Custom Unroll Factor
~~~~~~~~~~~~~~~~~~~~

Choose unroll based on your profile and compile-time/code-size constraints.

.. code-block:: cpp

   poet::dynamic_for<2>(n, func);   // smaller codegen
   poet::dynamic_for<4>(n, func);   // balanced starting point
   poet::dynamic_for<8>(n, func);   // for profiled hot/simple loops

.. note:: ``Unroll`` must not exceed the library maximum unroll cap (currently 256 on most compilers, 128 on MSVC).

Callable Signatures
-------------------

``dynamic_for`` supports two callable forms:

- ``func(index)`` — index only
- ``func(lane, index)`` where ``lane`` is ``std::integral_constant<std::size_t, L>``

The ``lane`` value is the compile-time position inside the current unrolled block
(cycles from 0 to ``Unroll-1``).

.. code-block:: cpp

   poet::dynamic_for<4>(0, 10, [](auto lane_c, int i) {
       constexpr std::size_t lane = decltype(lane_c)::value;
       use(lane, i);
   });

Compile-Time Step
-----------------

When the stride is a compile-time constant, use the template-parameter overload.
This lets the compiler constant-fold per-lane stride multiplication and eliminate
the stride argument from tail dispatch.

.. code-block:: cpp

   // Step=2 known at compile time
   poet::dynamic_for<4, 2>(0, 100, [](int i) {
       // i = 0,2,4,...,98
   });

   // Negative compile-time step
   poet::dynamic_for<4, -1>(10, 0, [](int i) {
       // i = 10,9,...,1
   });

Tail Handling
-------------

When the iteration count is not divisible by ``Unroll``, ``dynamic_for`` routes
remaining iterations to a specialized tail block via ``poet::dispatch``.

.. code-block:: cpp

   // 10 iterations with Unroll=3: 3 + 3 + 3 + tail(1)
   poet::dynamic_for<3>(0, 10, 1, [](int i) {
       use(i);
   });

Use with C++20 Ranges and Piping
--------------------------------

.. note:: Requires C++20.

``<poet/core/dynamic_for.hpp>`` provides an eager piping adaptor:

.. code-block:: cpp

   #include <ranges>
   #include <poet/core/dynamic_for.hpp>

   auto r = std::views::iota(0) | std::views::take(10);
   r | poet::make_dynamic_for<4>([](int i) {
       // i = 0..9
   });

   std::tuple{0, 24, 2} | poet::make_dynamic_for<4>([](int i) {
       // i = 0,2,4,...,22
   });

Notes
-----

- The range adaptor interprets the input as consecutive values starting at
  ``*begin(range)`` with step ``+1``.
- For non-sized ranges, the adaptor performs a pass to compute the element count.
- The adaptor is eager: it immediately invokes ``poet::dynamic_for``.
