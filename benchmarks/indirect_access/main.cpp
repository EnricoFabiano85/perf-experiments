#include <immintrin.h>
#include <cstddef>
#include <cstdint>

import std;
import perf;

int main()
{
  using DataType = int;
  auto constexpr vectorSize = 100'000'000;
  auto const data = std::views::iota(0, vectorSize) | std::ranges::to<std::vector>();
  auto const nIter = 10;

  auto const assertResult = [expected = (vectorSize*(vectorSize-1))/2](DataType value){
    if (value != expected) 
    {
      std::println("FAIL: got {}, expected {}", value, expected);
      std::abort();
    }
  };
  
  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(data);

    auto const s = perf::measure<perf::CachePolicy::Warm>(nIter, [&]{
      decltype(data)::value_type sum = 0;
      for (auto const &e : data) sum += e;
      return sum;
    }, assertResult, regionsToFlush);

    std::println("Sequential access timing results:");
    std::println("{}\n\n\n", s);
  }

  auto indices = std::views::iota(0, vectorSize) | std::ranges::to<std::vector<std::uint32_t>>();

  {
    perf::warmUpCpu();

    auto const regionsToFlush = perf::makeFlushRegions(data, indices);

    auto const s = perf::measure<perf::CachePolicy::Warm>(nIter, [&]{
      decltype(data)::value_type sum = 0;
      for (std::size_t idx = 0; idx != vectorSize; ++idx) sum += data[indices[idx]];
      return sum;
    }, assertResult, regionsToFlush);

    std::println("Index indirection access timing results:");
    std::println("{}\n\n\n", s);
  }

  std::ranges::shuffle(indices, std::mt19937{std::random_device{}()});

  {
    perf::warmUpCpu();

    auto const regionsToFlush = perf::makeFlushRegions(data, indices);

    auto const s = perf::measure<perf::CachePolicy::Warm>(nIter, [&]{
      decltype(data)::value_type sum = 0;
      for (std::size_t idx = 0; idx != vectorSize; ++idx) sum += data[indices[idx]];
      return sum;
    }, assertResult, regionsToFlush);

    std::println("Shuffled indices access timing results:");
    std::println("{}\n\n\n", s);
  }

  {
    perf::warmUpCpu();

    for (auto prefetchDistance : std::array{1, 10, 50, 90, 100, 200, 300, 400, 500, 1000, 10000})
    { 
      auto const paddedIndices = [&]
      {
        auto v = indices;
        v.resize(prefetchDistance, 0);
        return v;
      }();
    
      auto const regionsToFlush = perf::makeFlushRegions(data, paddedIndices);

      auto const s = perf::measure<perf::CachePolicy::Warm>(nIter, [&](){
        decltype(data)::value_type sum = 0;
        
        for (auto i = 0; i != vectorSize; ++i) 
        {
          __builtin_prefetch(data.data()+paddedIndices[i+prefetchDistance]);
          sum += data[paddedIndices[i]];
        }
        return sum;
      }, assertResult, regionsToFlush);

      std::println("Prefetch Distance {} timing results", prefetchDistance);
      std::println("{}\n\n\n", s);
    }
  }

#ifdef __AVX512F__
  auto constexpr lanes = 64/sizeof(DataType);
  
  {
    perf::warmUpCpu();

    auto const regionsToFlush = perf::makeFlushRegions(data, indices);

    auto const s = perf::measure<perf::CachePolicy::Cold>(nIter, [&](){
        auto vecSum = _mm512_setzero_si512();

        std::size_t i = 0;
        for (; i + lanes <= vectorSize; i+=lanes)
        {
          auto const localIndices = _mm512_loadu_si512((__m512i*)&indices[i]);
          auto const vals = _mm512_i32gather_epi32(localIndices, data.data(), sizeof(DataType));
          vecSum = _mm512_add_epi32(vecSum, vals);
        }

        auto const sum = _mm512_reduce_add_epi32(vecSum);

        return sum;
      }, assertResult, regionsToFlush);

      std::println("SIMD gather timing results");
      std::println("{}\n\n\n", s);
  }

  {
    for (auto prefetchDistance : std::array{1, 10, 50, 90, 100, 200, 300, 400, 500, 1000, 10000})
    {
      auto const paddedIndices = [&]
      {
        auto v = indices;
        v.resize(prefetchDistance, 0);
        return v;
      }();
    
      auto const regionsToFlush = perf::makeFlushRegions(data, paddedIndices);

      auto const s = perf::measure<perf::CachePolicy::Cold>(nIter, [&](){

        auto vecSum = _mm512_setzero_si512();

        std::size_t i = 0;
        for (; i + lanes <= vectorSize; i+=lanes)
        {
          __builtin_prefetch(data.data()+paddedIndices[i]+prefetchDistance);

          auto const localIndices = _mm512_loadu_si512((__m512i*)&paddedIndices[i]);
          auto const vals = _mm512_i32gather_epi32(localIndices, data.data(), sizeof(DataType));
          vecSum = _mm512_add_epi32(vecSum, vals);
        }

        auto const sum = _mm512_reduce_add_epi32(vecSum);

        return sum;
      }, assertResult, regionsToFlush);

      std::println("SIMD gather, prefetch Distance {} timing results", prefetchDistance);
      std::println("{}\n\n\n", s);
    }
  }
#endif

  return 0;
}