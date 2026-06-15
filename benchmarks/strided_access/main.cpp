#include <immintrin.h>
#include <cstddef>
#include <cstdint>

import std;
import perf;

int main()
{
  using DataType = int;
  auto constexpr Nx = 1024;
  auto constexpr Ny = Nx;
  auto constexpr Nz = Ny;
  auto constexpr vectorSize = Nx*Ny*Nz;
  auto const data = std::views::iota(0, vectorSize) | std::ranges::to<std::vector>();
  auto const mdSpan = std::mdspan(data.data(),Nx, Ny, Nz);
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

    auto const s = perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

      decltype(data)::value_type sum = 0;
      for (std::size_t k = 2; k != mdSpan.extent(3)-2; k++)
        for (std::size_t j = 2; j != mdSpan.extent(2)-2; j++)
          for (std::size_t i = 2; i != mdSpan.extent(1)-2; i++)
            sum += mdSpan[i,j,k];

      return sum;
    }, {}, regionsToFlush);

    std::println("Sequential access timing results:");
    std::println("{}\n\n\n", s);
  }

  return 0;
}