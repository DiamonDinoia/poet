Static Loops (static_for)
=========================

Overview
--------

``poet::static_for`` implements a compile-time unrolled loop. It is useful when the loop boundaries are known at compile time and you want to generate code for each iteration independently. 

It splits the iteration space into blocks (default 256) to handle large counts without crashing the compiler.

Usage
-----

Simplified Iteration (Zero-based)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The most convenient overload iterates from ``0`` to ``End`` with a step of 1.

.. code-block:: cpp

   // Iterate: 0, 1, 2, 3, 4
   poet::static_for<5>([](auto i) {
       std::cout << "i = " << i << "\n";
   });

Explicit Range
~~~~~~~~~~~~~~

Specifying the range ``[Begin, End)`` is also possible.

.. code-block:: cpp

   // Iterate: 2, 3, 4
   poet::static_for<2, 5>([](auto i) {
       // ...
   });

Custom Step
~~~~~~~~~~~

A custom step size (positive or negative) can be specified as the third template parameter.

.. code-block:: cpp

   // Iterate: 0, 2, 4
   poet::static_for<0, 6, 2>([](auto i) {
       constexpr int val = i;
       process_even(val);
   });

   // Iterate: 4, 3, 2, 1
   poet::static_for<4, 0, -1>([](auto i) {
       // ...
   });

Controlling Block Size (Advanced)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For very large loops or complex loop bodies, it might be desirable to control the block partitioning size to manage compile times. The 4th template parameter sets ``BlockSize``.

.. code-block:: cpp

   // Iterate 0..100 with step 1, but process in blocks of 16 iterations
   poet::static_for<0, 100, 1, 16>([](auto i) {
       // each block of 16 is expanded into a single function call
   });

Callable Signatures
-------------------

Integral Constant (C++17)
~~~~~~~~~~~~~~~~~~~~~~~~~

The callable is invoked with a ``std::integral_constant``. This allows the index `i` to be used in constant expressions.

.. code-block:: cpp

   poet::static_for<3>([](auto i) {
       constexpr int val = i;
       static_assert(val >= 0 && val < 3);

       // Use as template argument
       some_template<i.value>();
   });

Template Lambda (C++20)
~~~~~~~~~~~~~~~~~~~~~~~

When C++20 is available, template lambdas can be used directly. This is supported and provides a cleaner syntax for accessing the compile-time value.

.. code-block:: cpp

   poet::static_for<3>([]<auto I>() {
       // I is available as a compile-time constant
       some_template<I>();
   });
