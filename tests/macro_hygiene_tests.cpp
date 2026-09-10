#include <catch2/catch_test_macros.hpp>

// The macros.hpp/undef_macros.hpp cycle must be repeatable. An include guard on
// either side turns every pass after the first into a no-op.

#include <poet/poet.hpp>// ends with undef_macros.hpp

#ifdef POET_FORCEINLINE
#error "poet.hpp leaked POET_FORCEINLINE"
#endif
#ifdef POET_CPP20_CONSTEVAL
#error "poet.hpp leaked POET_CPP20_CONSTEVAL"
#endif
#ifdef POET_CPLUSPLUS
#error "poet.hpp leaked POET_CPLUSPLUS"
#endif

#include <poet/core/macros.hpp>

#ifndef POET_FORCEINLINE
#error "macros.hpp did not restore POET_FORCEINLINE after undef_macros.hpp"
#endif

#include <poet/core/undef_macros.hpp>

#ifdef POET_FORCEINLINE
#error "second undef_macros.hpp pass was a no-op"
#endif
#ifdef POET_PUSH_OPTIMIZE
#error "second undef_macros.hpp pass left POET_PUSH_OPTIMIZE defined"
#endif
#ifdef POET_NO_UNROLL
#error "second undef_macros.hpp pass left POET_NO_UNROLL defined"
#endif
#ifdef POET_IS_CONSTANT
#error "second undef_macros.hpp pass left POET_IS_CONSTANT defined"
#endif

// The templates and count_trailing_zeros survive the macro cleanup.
TEST_CASE("POET API stays usable after undef_macros", "[macros]") {
    int sum = 0;
    poet::static_for<0, 4>([&sum](auto idx) { sum += static_cast<int>(idx); });
    REQUIRE(sum == 6);
    REQUIRE(poet::detail::count_trailing_zeros(8) == 3);
}
