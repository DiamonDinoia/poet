Static Dispatch
====================

Overview
--------

``poet::dispatch`` translates a set of runtime values (e.g., function arguments, configuration options) into compile-time template parameters. It essentially performs a highly optimized lookup from ``runtime value -> compile-time constant`` and then invokes your templated code.

This allows you to write "runtime-polymorphic" code that is actually compiled as a set of specialized, highly-optimized template instantiations.

Features
--------

- **Fast Dispatch**: Uses compile-time generated function-pointer tables for direct specialization calls.
- **Range Support**: Easily specify ranges of valid values (e.g., dispatch an integer from 0 to 10).
- **Multiple Arguments**: Dispatch on multiple runtime values (e.g., *both* width and height).
- **Sparse Dispatch**: Use ``DispatchSet`` to list only specific valid combinations, avoiding combinatorial explosion.

Usage
-----

Basic Range Dispatch
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <poet/poet.hpp>
   
   struct Kernel {
       template <int N>
       void operator()(float* data) {
           // This code is compiled specifically for N
           // Compiler can optimize given N is constant.
       }
   };

   // Runtime value
   int n = get_runtime_size(); // e.g., 5

   // Define possible compile-time candidates: [1, 16]
   using Range = poet::make_range<1, 16>;
   poet::DispatchParam<Range> param{n};

   // Dispatch!
   // If n is in [1, 16], calls Kernel::operator()<n>(data)
   poet::dispatch(Kernel{}, param, data_ptr);

Cartesian product of ranges (multiple DispatchParam)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Dispatch over two independent ranges without enumerating a set:

.. code-block:: cpp

   struct Kernel2D {
       template<int A, int B>
       void operator()(float* data) const {
           // Specialized for A,B
       }
   };

   using AR = poet::make_range<1, 4>;  // 1..4 (inclusive)
   using BR = poet::make_range<2, 3>;  // 2..3 (inclusive)
   int a = getA(); int b = getB();
   poet::dispatch(Kernel2D{}, poet::DispatchParam<AR>{a}, poet::DispatchParam<BR>{b}, data_ptr);

Explicit Sets (Sparse Dispatch)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you only allow specific combinations (e.g., square matrices of size 2x2, 4x4, or non-square 2x4), use ``DispatchSet``.

.. code-block:: cpp

    using namespace poet;

    // Define valid (rows, cols) pairs
    using Shapes = DispatchSet<int,
        T<2, 2>,
        T<4, 4>,
        T<2, 4>
    >;

    struct MatMul {
        template <int Rows, int Cols>
        void operator()() {
            // Specialized matrix multiplication
        }
    };

    void run_matmul(int r, int c) {
        Shapes valid_shapes(r, c);
        
        // Dispatches only if (r,c) matches one of the tuples above
        poet::dispatch(MatMul{}, valid_shapes);
    }

Error Handling
--------------

By default, if the runtime values do not match any compile-time candidate, ``dispatch`` returns without invoking the functor (for void return types) or returns a default-constructed result value (for non-void return types).

You can force a check by passing ``poet::throw_t`` as the first argument:

.. code-block:: cpp

   poet::dispatch(poet::throw_t, Kernel{}, poet::DispatchParam<Range>{n}, args...);
   // Throws std::runtime_error if no match found.

The throwing overload with ``DispatchSet`` is also available:

.. code-block:: cpp

   using Shapes = poet::DispatchSet<int, poet::T<2,2>, poet::T<4,4>>;
   Shapes s(r, c);
   poet::dispatch(poet::throw_t, MatMul{}, s); // throws if (r,c) not allowed

Implementation Details
----------------------

Internally, ``dispatch`` uses compile-time generated function-pointer tables. For contiguous ranges, runtime values map to table indices in O(1) via arithmetic offsets. For sparse/non-contiguous ranges, it uses compact sorted key metadata with O(log N) binary search to map values to table indices before invoking the specialization.

Advanced: ``poet::dispatch_tuples`` exposes the tuple-of-sequences form used under the hood for ``DispatchSet`` and cartesian ParamTuples. Most users should call ``poet::dispatch`` instead.

.. note:: ``poet::make_range<Start, End>`` is inclusive on both ends.
