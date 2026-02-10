#ifndef POET_BENCH_REAL_SWITCH_GEN_HPP
#define POET_BENCH_REAL_SWITCH_GEN_HPP

#include <utility>
#include <poet/core/macros.hpp>

// ============================================================================
// Real switch statement generation using template specialization
// ============================================================================

namespace switch_gen {

// Helper to generate switch for a range of compile-time indices
template<typename Functor, typename Seq, std::size_t... Is>
POET_FORCEINLINE void dispatch_switch_impl(
    int runtime_val,
    Functor& func,
    std::index_sequence<Is...>) {

    // This generates a real switch statement!
    // Each case is instantiated from the parameter pack
    switch(runtime_val) {
        // Expand parameter pack: each Is becomes a case label
        // We use a fold expression with comma operator
        case ([]<std::size_t I>() {
            using seq_t = Seq;
            if constexpr (I < seq_t::size()) {
                return poet::detail::sequence_nth_value<seq_t, I>::value;
            } else {
                return -1; // Invalid
            }
        }.template operator()<Is>()): ...
        {
            // Now we need to call the functor with the right compile-time value
            // We have to match runtime_val to the compile-time value again
            ((runtime_val == poet::detail::sequence_nth_value<Seq, Is>::value ?
                (func.template operator()<poet::detail::sequence_nth_value<Seq, Is>::value>(), true) :
                false) || ...);
            return;
        }
    }
}

// Alternative: Generate explicit case statements using constexpr lambda
template<typename Functor, int... Values>
struct real_switch_sparse;

// Specialization for explicit values
template<typename Functor, int V0>
struct real_switch_sparse<Functor, V0> {
    static POET_FORCEINLINE void dispatch(int val, Functor& func) {
        switch(val) {
            case V0: func.template operator()<V0>(); break;
        }
    }
};

template<typename Functor, int V0, int V1>
struct real_switch_sparse<Functor, V0, V1> {
    static POET_FORCEINLINE void dispatch(int val, Functor& func) {
        switch(val) {
            case V0: func.template operator()<V0>(); break;
            case V1: func.template operator()<V1>(); break;
        }
    }
};

template<typename Functor, int V0, int V1, int V2>
struct real_switch_sparse<Functor, V0, V1, V2> {
    static POET_FORCEINLINE void dispatch(int val, Functor& func) {
        switch(val) {
            case V0: func.template operator()<V0>(); break;
            case V1: func.template operator()<V1>(); break;
            case V2: func.template operator()<V2>(); break;
        }
    }
};

template<typename Functor, int V0, int V1, int V2, int V3>
struct real_switch_sparse<Functor, V0, V1, V2, V3> {
    static POET_FORCEINLINE void dispatch(int val, Functor& func) {
        switch(val) {
            case V0: func.template operator()<V0>(); break;
            case V1: func.template operator()<V1>(); break;
            case V2: func.template operator()<V2>(); break;
            case V3: func.template operator()<V3>(); break;
        }
    }
};

template<typename Functor, int V0, int V1, int V2, int V3, int V4>
struct real_switch_sparse<Functor, V0, V1, V2, V3, V4> {
    static POET_FORCEINLINE void dispatch(int val, Functor& func) {
        switch(val) {
            case V0: func.template operator()<V0>(); break;
            case V1: func.template operator()<V1>(); break;
            case V2: func.template operator()<V2>(); break;
            case V3: func.template operator()<V3>(); break;
            case V4: func.template operator()<V4>(); break;
        }
    }
};

template<typename Functor, int V0, int V1, int V2, int V3, int V4, int V5, int V6, int V7>
struct real_switch_sparse<Functor, V0, V1, V2, V3, V4, V5, V6, V7> {
    static POET_FORCEINLINE void dispatch(int val, Functor& func) {
        switch(val) {
            case V0: func.template operator()<V0>(); break;
            case V1: func.template operator()<V1>(); break;
            case V2: func.template operator()<V2>(); break;
            case V3: func.template operator()<V3>(); break;
            case V4: func.template operator()<V4>(); break;
            case V5: func.template operator()<V5>(); break;
            case V6: func.template operator()<V6>(); break;
            case V7: func.template operator()<V7>(); break;
        }
    }
};

// Continue for more sizes...
template<typename Functor, int V0, int V1, int V2, int V3, int V4, int V5, int V6, int V7,
         int V8, int V9, int V10, int V11, int V12, int V13, int V14, int V15>
struct real_switch_sparse<Functor, V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15> {
    static POET_FORCEINLINE void dispatch(int val, Functor& func) {
        switch(val) {
            case V0: func.template operator()<V0>(); break;
            case V1: func.template operator()<V1>(); break;
            case V2: func.template operator()<V2>(); break;
            case V3: func.template operator()<V3>(); break;
            case V4: func.template operator()<V4>(); break;
            case V5: func.template operator()<V5>(); break;
            case V6: func.template operator()<V6>(); break;
            case V7: func.template operator()<V7>(); break;
            case V8: func.template operator()<V8>(); break;
            case V9: func.template operator()<V9>(); break;
            case V10: func.template operator()<V10>(); break;
            case V11: func.template operator()<V11>(); break;
            case V12: func.template operator()<V12>(); break;
            case V13: func.template operator()<V13>(); break;
            case V14: func.template operator()<V14>(); break;
            case V15: func.template operator()<V15>(); break;
        }
    }
};

// Wrapper for integer_sequence
template<typename Seq, typename Functor>
struct real_switch_from_seq;

template<int... Vals, typename Functor>
struct real_switch_from_seq<std::integer_sequence<int, Vals...>, Functor> {
    static POET_FORCEINLINE void dispatch(int val, Functor& func) {
        real_switch_sparse<Functor, Vals...>::dispatch(val, func);
    }
};

// ============================================================================
// Flattened 2D real switch generation
// ============================================================================

template<typename Functor, typename Seq1, typename Seq2, std::size_t... FlatIndices>
struct real_switch_2d_impl;

// Specialization for 4x4 = 16 cases
template<typename Functor, typename Seq1, typename Seq2>
struct real_switch_2d_impl<Functor, Seq1, Seq2,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15> {

    static POET_FORCEINLINE void dispatch(std::size_t flat_idx, Functor& func) {
        constexpr std::size_t dim1 = Seq2::size();

        switch(flat_idx) {
            #define CASE_2D(FLAT) \
                case FLAT: { \
                    constexpr std::size_t i0 = FLAT / dim1; \
                    constexpr std::size_t i1 = FLAT % dim1; \
                    constexpr int v0 = poet::detail::sequence_nth_value<Seq1, i0>::value; \
                    constexpr int v1 = poet::detail::sequence_nth_value<Seq2, i1>::value; \
                    func.template operator()<v0, v1>(); \
                    break; \
                }

            CASE_2D(0) CASE_2D(1) CASE_2D(2) CASE_2D(3)
            CASE_2D(4) CASE_2D(5) CASE_2D(6) CASE_2D(7)
            CASE_2D(8) CASE_2D(9) CASE_2D(10) CASE_2D(11)
            CASE_2D(12) CASE_2D(13) CASE_2D(14) CASE_2D(15)

            #undef CASE_2D
        }
    }
};

// Dispatcher that computes flat index
template<typename Seq1, typename Seq2, typename Functor>
struct real_switch_2d {
    static POET_FORCEINLINE void dispatch(int v1, int v2, Functor& func) {
        constexpr std::size_t dim0 = Seq1::size();
        constexpr std::size_t dim1 = Seq2::size();

        const auto idx0 = poet::detail::sequence_runtime_index<Seq1>::find(v1);
        const auto idx1 = poet::detail::sequence_runtime_index<Seq2>::find(v2);

        if (idx0 && idx1) {
            const std::size_t flat_idx = (*idx0) * dim1 + (*idx1);

            if constexpr (dim0 == 4 && dim1 == 4) {
                real_switch_2d_impl<Functor, Seq1, Seq2,
                    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15>::dispatch(flat_idx, func);
            }
        }
    }
};

} // namespace switch_gen

#endif // POET_BENCH_REAL_SWITCH_GEN_HPP
