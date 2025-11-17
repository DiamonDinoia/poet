#ifndef POET_CORE_DYNAMIC_FOR_HPP
#define POET_CORE_DYNAMIC_FOR_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <poet/core/static_for.hpp>
#include <poet/core/static_dispatch.hpp>

namespace poet {

namespace detail {

template <typename Func>
struct dynamic_block_invoker {
  Func &func;
  std::size_t base;

  template <auto I>
  constexpr void operator()() const {
    func(base + static_cast<std::size_t>(I));
  }
};

template <typename Func, std::size_t BlockSize>
inline void execute_runtime_block([[maybe_unused]] Func &func,
                                  [[maybe_unused]] std::size_t base) {
  if constexpr (BlockSize > 0) {
    dynamic_block_invoker<Func> invoker{func, base};
    static_for<0, static_cast<std::intmax_t>(BlockSize), 1, BlockSize>(
        invoker);
  } else {
    // Nothing to do when the block size is zero.
  }
}

// Helper functor used by dynamic_for to dispatch tail sizes. It must live at
// namespace scope because local classes are not allowed to declare member
// templates in C++.
template <typename Callable>
struct tail_caller_for_dynamic_for {
  Callable &callable;

  template <int Tail>
  void operator()(std::size_t base) const {
    execute_runtime_block<Callable, static_cast<std::size_t>(Tail)>(
        callable, base);
  }
};

} // namespace poet::detail

/// \brief Executes a runtime-sized loop using compile-time unrolling.
///
/// The helper iterates over the half-open range `[begin, end)` and invokes the
/// supplied callable with the current runtime index. Blocks of `Unroll`
/// iterations are dispatched through `static_for`, allowing the compiler to
/// reason about each unrolled step.
///
/// \tparam Unroll Number of iterations emitted per unrolled block.
/// \tparam Func Callable type accepting a `std::size_t` index.
/// \param begin Inclusive lower bound of the loop range.
/// \param end Exclusive upper bound of the loop range.
/// \param func Callable invoked for each iteration.
/// \note Extremely large unroll factors can harm instruction-cache behaviour.
/// For portability, the implementation limits the unroll factor to
/// `kMaxStaticLoopBlock`, which keeps the generated fold expressions within the
/// bound enforced by `static_for`.
template <std::size_t Unroll = 8, typename Func>
inline void dynamic_for(std::size_t begin, std::size_t end, Func &&func) {
  static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
  static_assert(Unroll <= detail::kMaxStaticLoopBlock,
                "dynamic_for supports unroll factors up to kMaxStaticLoopBlock");

  using Callable = std::remove_reference_t<Func>;
  [[maybe_unused]] Callable callable(std::forward<Func>(func));

  if (end <= begin) {
    return;
  }

  std::size_t index = begin;
  [[maybe_unused]] std::size_t remaining = end - begin;

  while (remaining >= Unroll) {
    detail::execute_runtime_block<Callable, Unroll>(callable, index);
    index += Unroll;
    remaining -= Unroll;
  }

  if constexpr (Unroll > 1) {
    if (remaining > 0) {
      // Use the generic poet::dispatch machinery to dispatch the tail size
      // to a compile-time template parameter. This builds a single
      // DispatchParam describing the candidate tail sizes [0, Unroll-1] and
      // forwards the runtime base index to the TailCaller functor.
      detail::tail_caller_for_dynamic_for<Callable> tail_caller{callable};

      using TailRange = poet::make_range<0, (static_cast<int>(Unroll) - 1)>;
      auto params = std::make_tuple(poet::DispatchParam<TailRange>{
          static_cast<int>(remaining)});
      poet::dispatch(tail_caller, params, index);
    }
  } else {
    // When Unroll == 1 the `remaining` variable is unused.
  }
}

/// \brief Executes a runtime-sized loop from zero using compile-time unrolling.
///
/// This overload iterates over the range `[0, count)`.
template <std::size_t Unroll = 8, typename Func>
inline void dynamic_for(std::size_t count, Func &&func) {
  dynamic_for<Unroll>(static_cast<std::size_t>(0), count,
                      std::forward<Func>(func));
}

} // namespace poet

#endif // POET_CORE_DYNAMIC_FOR_HPP
