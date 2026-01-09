Static Loops (static_for)
=========================

Overview
--------

``poet::static_for`` implements a compile-time unrolled loop. It is useful when loop
bounds are known at compile time and you want the compiler to generate specialized,
unrolled code for each iteration. To keep compile-time costs bounded the iteration
space is partitioned into blocks (capped by default to 256); each block is expanded
as an unrolled sequence of calls.

What I represents
-----------------
- When the callable receives a parameter (e.g. `[](auto i){ ... }`), `i` is a
  `std::integral_constant` wrapper; use `decltype(i)::value` or `i.value` to access
  the integer at compile time.
- When using a C++20 template lambda (`[]<auto I>() { ... }`) the template parameter
  `I` is the compile-time integer itself and can be used directly as a non-type
  template argument.

Behavior for ranges not divisible by block size
----------------------------------------------
The partitioning into blocks is an implementation detail to bound template
instantiation depth; it does not change the iteration semantics:

- Given a range `[Begin, End)` and a BlockSize B (default computed/capped), the
  implementation expands full blocks of B iterations where possible, and a final
  partial block for the remaining iterations when `(End - Begin) % B != 0`.
- Example: `poet::static_for<0, 100, 1, 8>(...)` expands 12 full blocks of 8
  (0..95) and one final partial block (96..99). Every call inside the body is
  invoked with the actual iteration index (0..99), not a block-relative index.
- In other words, static_for yields the exact sequence of indices in `[Begin,End)`
  (respecting the step), but generates the code as a sequence of unrolled blocks.
- The block partitioning reduces compile-time blowup for large ranges; semantics
  remain identical to an unrolled loop over the full range.

BlockSize and compile-time tradeoffs
-----------------------------------
- The fourth template parameter sets `BlockSize`. Smaller block sizes reduce the
  size of each unrolled instantiation at the cost of more top-level block
  instantiations; larger block sizes increase per-block code expansion.
- A sensible default cap is used to balance compile time and generated code size.
- If you hit compilation or performance issues, tune `BlockSize` to trade between
  instantiation depth and per-block expansion.

Callable signatures (summary)
-----------------------------
- Callable receives a `std::integral_constant` (C++17 style):
  .. code-block:: cpp

     poet::static_for<3>([](auto i) {
         constexpr int val = decltype(i)::value;
         some_template<val>();
     });

- With C++20 template lambda:
  .. code-block:: cpp

     poet::static_for<3>([]<auto I>() {
         some_template<I>();
     });

Examples
--------
- Divisible range:

  .. code-block:: cpp

     // Iterates 0..7 (8 iterations), BlockSize default >= 8
     poet::static_for<0, 8>([]<auto I>() {
         // I = 0..7
     });

- Non-divisible range with BlockSize 8:

  .. code-block:: cpp

     // Iterates 0..99; expanded as 12 full blocks (8) + final block of 4
     poet::static_for<0, 100, 1, 8>([](auto i) {
         // decltype(i)::value ranges 0..99
     });
