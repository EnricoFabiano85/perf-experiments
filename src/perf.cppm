#include <type_traits>
module;

// #include <immintrin.h>
#include <sched.h>
#include <unistd.h>

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
  std::uint64_t const min;
  double const mean;
  std::uint64_t const median;
  double const stdDev;
  std::uint64_t const p90;
  std::uint64_t const p99;
  std::uint64_t const max;
};

[[gnu::always_inline]] void doNotOptimize(auto const &value)
{ asm volatile("" : : "r,m"(value) : "memory"); }

[[gun::always_inline]] 
auto measure(int nIter, std::invocable auto f, std::invocable auto assertResult = NoAssert{}) -> Stats
{
  auto timingResults = std::vector<std::uint64_t>();
  timingResults.reserve(nIter);

  for (int iter = 0; iter != nIter; ++iter)
  {
    auto const start = std::chrono::high_resolution_clock::now();

    auto value = f();
    doNotOptimize(value);

    auto const end = std::chrono::high_resolution_clock::now();
    auto const elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    timingResults.push_back(elapsedTime);
    
    assertResult(value);
  }

  std::ranges::sort(timingResults);
  double const mean = std::accumulate(timingResults.begin(), timingResults.end(), 0.)/nIter;
  
  double const sqSum = std::accumulate(timingResults.begin(), timingResults.end(), 0., 
      [mean](double acc, double x) { return acc + ((x - mean) * (x - mean)); });
  double const stdDev = std::sqrt(sqSum / nIter);

  auto getPercentile = [&](double p) {
      std::size_t const idx = static_cast<std::size_t>(std::ceil((p / 100.0) * nIter)) - 1;
      return timingResults[std::min(idx, std::size_t(nIter) - 1)];
  };

  return {timingResults.front(), mean, getPercentile(50), stdDev, getPercentile(90), getPercentile(99), timingResults.back()};
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
      "===============================", s.min, s.mean, s.median, s.stdDev, s.p90, s.p99, s.max);
  }
};