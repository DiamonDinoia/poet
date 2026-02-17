#ifndef POET_TESTS_DYNAMIC_FOR_TEST_SUPPORT_HPP
#define POET_TESTS_DYNAMIC_FOR_TEST_SUPPORT_HPP

#include <poet/core/dynamic_for.hpp>

#include <cstddef>

inline constexpr std::size_t kMaxDynamicForTestUnroll = poet::detail::kMaxStaticLoopBlock;

#endif// POET_TESTS_DYNAMIC_FOR_TEST_SUPPORT_HPP
