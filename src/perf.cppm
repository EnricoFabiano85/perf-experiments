module;

// #include <immintrin.h>
#include <sched.h>
#include <unistd.h>

#include <concepts>

export module perf;

import std;

export namespace perf
{

struct NoAssert
{
  void operator()(auto const &value){}
};

struct Stats
{
  std::uint64_t const min_;
  double const mean_;
  std::uint64_t const median_;
  double const stdDev_;
  std::uint64_t const p90_;
  std::uint64_t const p99_;
  std::uint64_t const max_;
};

[[gnu::always_inline]] void doNotOptimize(auto const &value)
{ asm volatile("" : : "r,m"(value) : "memory"); }

template<std::invocable Kernel, typename AssertResult = NoAssert>
requires std::invocable<AssertResult, std::invoke_result_t<Kernel>>
auto measure(std::size_t nIter, Kernel f, AssertResult assertResult = {}) -> Stats
{
  auto timingResults = std::vector<std::uint64_t>();
  timingResults.reserve(nIter);

  for (auto iter = 0uz; iter != nIter; ++iter)
  {
    auto const start = std::chrono::steady_clock::now();

    auto value = f();
    doNotOptimize(value);

    auto const end = std::chrono::steady_clock::now();
    auto const elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    timingResults.push_back(elapsedTime);

    assertResult(value);
  }

  std::ranges::sort(timingResults);

  auto const n = static_cast<double>(nIter);

  double const mean = std::accumulate(timingResults.begin(), timingResults.end(), 0.)/n;
    
  double const sqSum = std::accumulate(timingResults.begin(), timingResults.end(), 0., 
      [mean](double acc, double x) { return acc + ((x - mean) * (x - mean)); });
  double const stdDev = std::sqrt(sqSum / n);

  auto getPercentile = [&](double p) {
      std::size_t const idx = static_cast<std::size_t>(std::ceil((p / 100.0) * n)) - 1;
      return timingResults[std::min(idx, nIter - 1)];
  };

  return {.min_=timingResults.front(), 
    .mean_ = mean, 
    .median_ = getPercentile(50), 
    .stdDev_ = stdDev, 
    .p90_ = getPercentile(90), 
    .p99_ = getPercentile(99), 
    .max_ = timingResults.back()};
}

}

template <>
struct std::formatter<perf::Stats> 
{
  constexpr auto parse(std::format_parse_context& ctx) {
      return ctx.begin();
  }

  auto format(const perf::Stats &s, std::format_context& ctx) const {
      return std::format_to(ctx.out(), "====== Stats results ======\n"
      "min {}ms\n"
      "mean {}ms\n"
      "median {}ms\n"
      "std dev {}ms\n"
      "p90 {}ms\n"
      "p99 {}ms\n"
      "max {}ms\n"
      "===============================", s.min_, s.mean_, s.median_, s.stdDev_, s.p90_, s.p99_, s.max_);
  }
};