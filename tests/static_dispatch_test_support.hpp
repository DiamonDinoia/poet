#ifndef POET_TESTS_STATIC_DISPATCH_TEST_SUPPORT_HPP
#define POET_TESTS_STATIC_DISPATCH_TEST_SUPPORT_HPP

#include <poet/core/static_dispatch.hpp>

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace {
using poet::DispatchParam;
using poet::dispatch;
using poet::make_range;

static_assert(std::is_same_v<make_range<0, 0>, std::integer_sequence<int, 0>>,
  "make_range should include the end value");
static_assert(std::is_same_v<make_range<2, 4>, std::integer_sequence<int, 2, 3, 4>>,
  "make_range should produce inclusive ranges");
static_assert(std::is_same_v<make_range<-2, 1>, std::integer_sequence<int, -2, -1, 0, 1>>,
  "make_range should support negative bounds");

struct vector_dispatcher {
    std::vector<int> *values;

    template<int Width, int Height> void operator()(int scale) const { values->push_back(scale + Width * 10 + Height); }
};

struct sum_dispatcher {
    template<int X, int Y, int Z> int operator()(int base) const { return base + X + Y + Z; }
};

struct guard_dispatcher {
    bool *invoked;

    template<int Value> int operator()(int base) const {
        *invoked = true;
        return base + Value;
    }
};

struct return_type_dispatcher {
    template<int X> double operator()(double multiplier) const { return X * multiplier; }
};

struct mover {
    template<int X> std::unique_ptr<int> operator()(int base) const { return std::make_unique<int>(base + X); }
};

struct receiver {
    template<int X> int operator()(std::unique_ptr<int> p) const { return X + *p; }
};

struct probe {
    int *out;
    template<int X, int Y> void operator()(int base) const { *out = base + X + Y; }
};

struct duplicate_reporter {
    int *out;
    template<int X> void operator()() const { *out = X; }
};

struct throwing_dispatcher {
    int *counter;

    template<int X> void operator()(int threshold) const {
        (*counter)++;
        if (X >= threshold) { throw std::runtime_error("dispatch exception"); }
    }
};

struct triple_dispatcher {
    template<int X, int Y, int Z> int operator()(int base) const { return base + X * 100 + Y * 10 + Z; }
};

struct value_arg_functor {
    template<int X, int Y>
    int operator()(std::integral_constant<int, X>, std::integral_constant<int, Y>, int base) const {
        return base + X * 10 + Y;
    }
};

} // namespace

#endif // POET_TESTS_STATIC_DISPATCH_TEST_SUPPORT_HPP
