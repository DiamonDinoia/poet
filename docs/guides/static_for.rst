Static Loops (static_for)
=========================

Overview
--------
The poet::static_for family provides compile-time unrolled loops. It is suitable when
iteration indices are known (or reduced to) compile-time constants so the
compiler can inline and optimize each iteration. To keep compile-time cost
bounded, the implementation partitions large ranges into fixed-size blocks and
expands each block as an unrolled sequence.

Minimal example
------------------------------------
Iterate 0..N (exclusive) where N is a compile-time constant.

.. code-block:: cpp

   // Iterates indices 0,1,2
   poet::static_for<3>([](auto I) {
       // I is a std::integral_constant; read value with:
       constexpr int idx = decltype(I)::value;
   });

What I represents
-----------------
- In the callable form that takes an argument (e.g. ``[](auto i){ ... }``),
  ``i`` is a ``std::integral_constant<std::intmax_t, K>`` wrapper for the
  compile-time integer K. Retrieve the integer as ``decltype(i)::value`` or
  ``i.value``.
- In the C++20 template-lambda form (``[]<auto I>(){...}``), ``I`` is the
  compile-time integer directly and may be used as a non-type template argument.

Template-lambda example (C++20)
-------------------------------
.. code-block:: cpp

   poet::static_for<3>([]<auto I>() {
       // I is the compile-time constant 0, then 1, then 2
       some_template<I>();
   });

Block partitioning and semantics
-------------------------------
To avoid excessive template recursion/instantiation for very large ranges,
static_for partitions the iteration space into blocks (BlockSize). This is an
implementation detail and does not change the sequence of indices observed by
the callable.

- Given range [Begin, End) and BlockSize B:
  - The implementation expands floor((End-Begin)/B) full blocks of size B.
  - A final partial block covers the remainder when (End - Begin) % B != 0.
- Semantically the callable sees the exact indices Begin .. End-1 (respecting
  Step), not block-local indices.

Illustrative example (0..100, default blocking)
-----------------------------------------------
.. code-block:: cpp

   // Semantics: iterates indices 0..99 in order
   poet::static_for<0, 100>([](auto i) {
       constexpr int idx = decltype(i)::value; // 0..99
       (void)idx;
   });

Manual BlockSize (control unroll granularity)
---------------------------------------------
The per-block unroll size can be controlled via the fourth template parameter.
If the total count isn't divisible by the block size, a final partial block
handles the leftover iterations.

.. code-block:: cpp

   // Uses BlockSize = 8: expands twelve full blocks (0..95) + partial (96..99)
   poet::static_for<0, 100, 1, 8>([](auto i) {
       constexpr int idx = decltype(i)::value; // 0..99
       (void)idx;
   });

Worked example: size 10, unroll by 3
------------------------------------
This example iterates the range [0, 10) using a block size of 3. Internally,
the loop is partitioned as 3 + 3 + 3 + 1, but the callable still sees the
global indices 0,1,2,3,4,5,6,7,8,9 in order.

Argument form (I is std::integral_constant)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. code-block:: cpp

   #include <iostream>
   #include <poet/poet.hpp>

   int main() {
       // Begin=0, End=10, Step=1, BlockSize=3
       poet::static_for<0, 10, 1, 3>([](auto I) {
           // I is std::integral_constant<std::intmax_t, K>
           constexpr int idx = static_cast<int>(decltype(I)::value);
           std::cout << idx << ' ';
       });
       // Expected output (order): 0 1 2 3 4 5 6 7 8 9
   }

Template-lambda form (C++20; I is a non-type template argument)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. code-block:: cpp

   #include <iostream>
   #include <poet/poet.hpp>

   int main() {
       poet::static_for<0, 10, 1, 3>([]<auto I>() {
           // I is the compile-time integer itself; there is no parameter.
           std::cout << I << ' ';
       });
       // Expected output (order): 0 1 2 3 4 5 6 7 8 9
   }

Notes
^^^^^
- static_for never passes a runtime int index. In the argument form the index
  arrives wrapped as std::integral_constant; in the template-lambda form the
  index is a non-type template parameter.

Reverse iteration (negative Step)
---------------------------------
``static_for`` supports negative steps for reverse iteration.

.. code-block:: cpp

   // Iterates 9, 8, 7, 6, 5
   poet::static_for<9, 4, -1>([](auto I) {
       constexpr int idx = static_cast<int>(decltype(I)::value);
       // use idx ...
   });

Even step (stride > 1)
----------------------
A step other than 1 can be specified to skip values.

.. code-block:: cpp

   // Iterates 0, 2, 4, 6, 8
   poet::static_for<0, 10, 2>([](auto I) {
       constexpr int idx = static_cast<int>(decltype(I)::value);
       // use idx ...
   });

What if the range exceeds kMaxStaticLoopBlock?
----------------------------------------------
- The default BlockSize is computed as the minimum of the total iteration count and `poet::kMaxStaticLoopBlock` (256 on most compilers, 128 on MSVC).
- static_for still emits all iterations at compile time. There is no outer
  runtime loop. The implementation divides the total iterations into blocks and
  then emits those blocks in compile-time "chunks" to reduce template recursion
  depth.
- Specifically:
  - Default BlockSize: computed as `min(total_count, poet::kMaxStaticLoopBlock)`.
  - Full blocks and a possible remainder block are computed as constexpr values
    and expanded entirely at compile time.
  - Internally, large numbers of blocks are processed in compile-time chunks of
    up to `kMaxStaticLoopBlock` to avoid a single enormous fold expression.

Why this matters (practical note)
--------------------------------
- The callable always receives the true iteration index (not a block-local id).
- Block partitioning trades compile-time depth for manageable instantiation
  sizes; tuning BlockSize can reduce compile-time memory/time or limit code
  expansion.
- Prefer the minimal form for small ranges; tune BlockSize for very large
  compile-time ranges.

Common pitfalls & tips
---------------------
- If the integer is needed at runtime, use the callable's captured value or cast
  ``decltype(i)::value`` into a constexpr context; do not assume a runtime
  integer is provided by the template-lambda form.
- If large compile times are observed, reduce BlockSize to shrink per-block
  expansion or split the problem into smaller compile-time ranges.

See also
--------
- guides/dynamic_for — runtime-driven loops implemented with compile-time blocks.
- api/library_root — API reference for exact signatures and overloads.
