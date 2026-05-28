module;

#include <immintrin.h>
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

enum class CachePolicy : char {Cold, Warm};

template<typename T>
concept contiguous_range = std::ranges::contiguous_range<T>;

using FlushRegions = std::span<std::span<std::byte const> const>;

auto makeFlushRegions(contiguous_range auto const &...crs)
{ return std::array{std::as_bytes(std::span{crs})...}; }

void flushRegions(FlushRegions regions)
{
  if (regions.empty()) 
    std::println(std::cerr, "Warning: using Cold cache policy but no regions to flush");

  _mm_mfence();
  
  for (auto &r : regions) _mm_clflushopt(r.data());

  _mm_mfence();
}

[[gnu::always_inline]] void doNotOptimize(auto const &value)
{ asm volatile("" : : "r,m"(value) : "memory"); }

void warmUpCpu(std::chrono::milliseconds warmUpTime = std::chrono::milliseconds{100})
{
  auto current_time = std::chrono::steady_clock::now();
  auto i = 1uz;
  auto sink = 0uz;
  while(std::chrono::steady_clock::now() - current_time < warmUpTime)
  {
    sink += i*i;
    ++i;
  }

  doNotOptimize(sink);
}

template<CachePolicy Policy = CachePolicy::Warm, std::invocable Kernel, typename AssertResult = NoAssert>
auto measure(std::size_t nIter, Kernel f, AssertResult assertResult = {}, FlushRegions regions = {}) -> Stats
{
  auto timingResults = std::vector<std::uint64_t>();
  timingResults.reserve(nIter);
  ++nIter;

  for (auto iter = 0uz; iter != nIter; ++iter)
  {
    auto const start = std::chrono::steady_clock::now();

    auto value = f();
    doNotOptimize(value);

    auto const end = std::chrono::steady_clock::now();
    auto const elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();

    if (iter == 0) continue;

    timingResults.push_back(elapsedTime);

    assertResult(value);

    if constexpr (Policy == CachePolicy::Cold) flushRegions(regions);
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