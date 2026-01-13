Dynamic Loops (dynamic_for)
===========================

Overview
--------

``poet::dynamic_for`` allows unrolling a loop whose boundaries are only known at runtime. It processes the loop in unrolled "chunks", helping the compiler vectorize or optimize the body.

Usage
-----

Basic Usage (Zero-based)
~~~~~~~~~~~~~~~~~~~~~~~~

The simplest overload iterates from ``0`` to ``count``. It uses a default unroll factor of 8.

.. code-block:: cpp

   std::size_t n = 100; // runtime value
   
   // Loop i from 0 to 99
   poet::dynamic_for(n, [&](auto i) {
       data[i] = compute(i);
   });

Explicit Range
~~~~~~~~~~~~~~

Specifying both the start and end of the range ``[begin, end)`` is possible via the overload that accepts three arguments.

.. code-block:: cpp

   std::size_t start = 10;
   std::size_t end = 50;
   
   poet::dynamic_for(start, end, [&](auto i) {
       // i goes from 10 to 49
   });

Negative and Backward Ranges
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``dynamic_for`` supports signed integers and automatically detects the iteration direction.

If ``begin <= end``, it iterates forward:

.. code-block:: cpp

   // -5, -4, -3, -2, -1
   poet::dynamic_for(-5, 0, [](auto i) { ... });

If ``begin > end``, it iterates backward (exclusive of ``end``):

.. code-block:: cpp

   // 10, 9, 8, 7, 6
   poet::dynamic_for(10, 5, [](auto i) { ... });

Backward + custom unroll and step
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It is possible to combine a negative step with a custom unroll factor:

.. code-block:: cpp

   // 10, 8, 6, 4, 2
   poet::dynamic_for<4>(10, 0, -2, [](auto i) {
       // i is a runtime value
   });

Explicit Step
~~~~~~~~~~~~~

An arbitrary step can be provided as a third argument. The `step` parameter is the per-element stride: successive indices differ by `step`. The unroll factor (`Unroll`) controls how many consecutive elements are emitted in each compile-time unrolled block. After emitting a block of `Unroll` elements, the next block starts at `current_index + Unroll * step`.

For example, with Unroll=4 and step=2 starting at 0:

- Element sequence: 0, 2, 4, 6, 8, 10, 12, ... (individual indices)
- Block starts: 0, 8, 16, ... (each block covers 4 elements: [0,2,4,6] then [8,10,12,14])

.. code-block:: cpp

   // 0, 2, 4, 6, 8
   poet::dynamic_for(0, 10, 2, [](auto i) { ... });

   // 10, 8, 6, 4, 2
   poet::dynamic_for(10, 0, -2, [](auto i) { ... });

Custom Unroll Factor
~~~~~~~~~~~~~~~~~~~~

If it is known that the loop body is small (or large), the unroll factor can be tuned via the template parameter.

.. code-block:: cpp

   // Unroll by 4 instead of default 8
   poet::dynamic_for<4>(n, [&](auto i) {
       // ...
   });

   // Unroll by 16
   poet::dynamic_for<16>(start, end, func);

Tail Handling
-------------

When the total count isn't divisible by the unroll factor (e.g., 100 items with unroll 8 -> 12 chunks of 8 + 4 leftover), ``dynamic_for`` automatically handles the tail. It uses ``poet::dispatch`` to jump to a specialized unrolled block for exactly those remaining items, ensuring the entire loop (head + tail) is efficient.

.. note:: The unroll factor must satisfy ``Unroll <= poet::kMaxStaticLoopBlock``.

Example (tail not divisible by unroll)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Unroll by 3 with 10 iterations: 3+3+3 full blocks, 1-tail handled via poet::dispatch
   poet::dynamic_for<3>(0, 10, 1, [](auto i) {
       // i runs 0..9; the last 1 iteration is dispatched to a specialized unrolled block
   });

Use with C++ ranges and piping
-----------------------------

POET's loop utilities operate on indices and invoke user callables. You can combine them with C++20 ranges and the pipe (`|`) syntax using the piping adaptor embedded in the header to get concise examples. The adaptor is an eager sink: it computes the range length (single-pass for non-sized ranges) and calls ``poet::dynamic_for``.

Prerequisite: include the main header and ranges:

.. code-block:: cpp

   #include <poet/core/dynamic_for.hpp>
   #include <ranges>

Example 1 — Basic indexed range (iota + take)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto r = std::views::iota(0) | std::views::take(10);
   // Equivalent to: poet::dynamic_for<4>(0, 10, 1, lambda);
   r | poet::make_dynamic_for<4>([](int i){
       // i iterates 0..9 (eagerly)
   });

Example 2 — Tuple (explicit begin, end, step)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Useful for non-unit step or precomputed bounds:
   std::tuple{0, 24, 2} | poet::make_dynamic_for<4>([](int i){
       // i = 0,2,4,...,22
   });

Example 3 — Transformed ranges (regular pattern)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If the transformation is a regular stride, prefer calling ``dynamic_for`` with the corresponding step:

.. code-block:: cpp

   auto t = std::views::iota(0)
            | std::views::transform([](int x){ return x * 2; })
            | std::views::take(5);
   // Materialize or compute pattern: values are 0,2,4,6,8 -> step 2
   poet::dynamic_for<4>(0, 10, 2, [](int v){
       // handles 0,2,4,6,8 efficiently
   });

If the transformed range is not a simple arithmetic progression, prefer std::ranges::for_each or materialize the values.

Example 4 — Negative step via tuple
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   std::tuple{10, 0, -2} | poet::make_dynamic_for<4>([](int i){
       // i = 10,8,6,4,2
   });

Example 5 — Captures and mutable state
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   std::vector<int> out;
   auto r = std::views::iota(0) | std::views::take(8);
   r | poet::make_dynamic_for<4>([&out](int i){
       out.push_back(i * i);
   });
   // out now contains {0,1,4,9,16,25,36,49}

Notes & caveats
---------------

- The adaptor interprets a range as a sequence of consecutive values starting at ``*begin(range)`` and advancing by ``+1``. For non-consecutive sequences the adaptor may not be appropriate; prefer an explicit tuple with a `step` argument or use ``std::ranges::for_each``.
- The adaptor computes the range length by iterating the range; this means non-sized ranges incur a single-pass traversal to determine the count.
- The adaptor is an eager sink: it calls ``poet::dynamic_for`` (not a lazy view).

Callable Signature
------------------

The callable passed to ``dynamic_for`` must accept an argument compatible with the loop index type (typically the common type of ``begin`` and ``end``).

.. code-block:: cpp

   poet::dynamic_for(n, [](auto i) {
       // i is a runtime value
   });

Unlike ``static_for``, ``dynamic_for`` always passes the index as a runtime value, so template lambdas (C++20) or ``std::integral_constant`` arguments (C++17) are not used to convey the index, as it is inherently dynamic.
