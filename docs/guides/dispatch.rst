Dispatch
========

``poet::dispatch`` maps runtime integers to compile-time specializations.

Single parameter
----------------

.. code-block:: cpp

   struct Kernel {
       template<int N>
       int operator()(int x) const {
           return N + x;
       }
   };

   int result = poet::dispatch(
       Kernel{},
       poet::DispatchParam<poet::make_range<0, 4>>{choice},
       10);

``poet::make_range<Start, End>`` is inclusive on both ends.

Multiple parameters
-------------------

Pass multiple ``DispatchParam`` objects to dispatch over a cartesian product:

.. code-block:: cpp

   auto result = poet::dispatch(
       Kernel2D{},
       poet::DispatchParam<poet::make_range<1, 4>>{rows},
       poet::DispatchParam<poet::make_range<1, 4>>{cols},
       data);

The tuple form is equivalent:

.. code-block:: cpp

   auto params = std::make_tuple(
       poet::DispatchParam<poet::make_range<1, 4>>{rows},
       poet::DispatchParam<poet::make_range<1, 4>>{cols});

   poet::dispatch(Kernel2D{}, params, data);

Sparse combinations
-------------------

Use ``DispatchSet`` when only specific tuples are valid:

.. code-block:: cpp

   using Shapes = poet::DispatchSet<int,
       poet::T<2, 2>,
       poet::T<4, 4>,
       poet::T<2, 4>>;

   poet::dispatch(MatMul{}, Shapes{rows, cols}, a, b, c);

Error handling
--------------

The default behavior on a miss is:

- ``void`` return: do nothing
- non-``void`` return: return ``R{}``

Use ``poet::throw_t`` when a miss should fail:

.. code-block:: cpp

   auto result = poet::dispatch(
       poet::throw_t,
       Kernel{},
       poet::DispatchParam<poet::make_range<0, 4>>{choice},
       10);

The same tag works with ``DispatchSet``.
