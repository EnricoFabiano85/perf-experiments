#include <cstddef>
#include <immintrin.h>
#include <sys/mman.h>
#include <stdio.h>

import std;
import perf;

enum class PageSize : std::uint8_t {Regular, Huge};

template<typename T>
requires std::is_implicit_lifetime_v<T>
class MMapBuffer
{
public:

  using value_type = T;

  MMapBuffer(std::size_t size, PageSize pages = PageSize::Regular) : _allocSize(size), _pages(pages)
  {
    auto sizeInBytes = _allocSize * sizeof(T);
    auto flags = MAP_PRIVATE | MAP_ANONYMOUS;

    if (_pages == PageSize::Huge) 
    {
      std::println("IN HERE");
      sizeInBytes = roundUpTo(sizeInBytes) ;
      flags |= MAP_HUGETLB;
    }


    void *ptr = mmap(nullptr, sizeInBytes, PROT_READ | PROT_WRITE, flags, -1, 0);

    if (ptr == MAP_FAILED) 
    {
      perror("mmap");
      throw std::bad_alloc{};
    }

    _data = static_cast<T*>(ptr);
  }

  T* data()
  { return _data; }

  T* begin()
  { return data(); }

  T* end()
  { return _data+_allocSize; }

  ~MMapBuffer()
  {
    auto sizeInBytes = _allocSize * sizeof(T);
    if (_pages == PageSize::Huge) sizeInBytes = roundUpTo(sizeInBytes) ;
    munmap(_data, sizeInBytes);
  }

private:

  constexpr std::size_t roundUpTo(std::size_t n)
  {
    auto constexpr hugePageSize{2uz*1024*1024};
    return (n + hugePageSize - 1) / hugePageSize * hugePageSize;
  }

  std::size_t const _allocSize;
  PageSize const _pages;
  T *_data;
  std::size_t const hugePageSize{2uz*1024*1024};
};

int main(int argc, char *argv[])
{

  auto const args = std::span(argv+1, argc-1);
  auto const pages = (!args.empty()  && std::string_view(args[0]).starts_with("--Huge")) 
                                      ? PageSize::Huge : PageSize::Regular;

  using DataType = int;
  auto constexpr Nx = 256;
  auto constexpr Ny = Nx;
  auto constexpr Nz = Ny;
  auto constexpr vectorSize = Nx*Ny*Nz;
  // auto const data = std::views::iota(0, vectorSize) | std::ranges::to<std::vector>();
  auto data = MMapBuffer<int>{vectorSize, pages};
  auto dataRange = std::span(data.data(), vectorSize);

  std::ranges::iota(dataRange, vectorSize);
  auto const mdSpan = std::mdspan<const int, std::dextents<size_t, 3>, std::layout_left>(data.data(),Nx, Ny, Nz);
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

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    auto const s = perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

      decltype(data)::value_type sum = 0;
      for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
        for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
          for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
            sum += mdSpan[i-2, j, k] + mdSpan[i-1, j, k] + mdSpan[i,j,k] + mdSpan[i+1, j, k] + mdSpan[i+2, j, k];

      return sum;
    }, {}, regionsToFlush);

    std::println("X-loop timing results:");
    std::println("{}\n\n\n", s);
  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    auto const s = perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

      decltype(data)::value_type sum = 0;
      for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
        for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
          for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
            sum += mdSpan[i-2, j, k] + mdSpan[i-1, j, k] + mdSpan[i,j,k] + mdSpan[i+1, j, k] + mdSpan[i+2, j, k];

      return sum;
    }, {}, regionsToFlush);

    std::println("X-loop timing results:");
    std::println("{}\n\n\n", s);
  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    auto const s = perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

      decltype(data)::value_type sum = 0;
      for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
        for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
          for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
            sum += mdSpan[i, j-2, k] + mdSpan[i, j-1, k] + mdSpan[i,j,k] + mdSpan[i, j+1, k] + mdSpan[i, j+2, k];

      return sum;
    }, {}, regionsToFlush);

    std::println("Y-loop timing results:");
    std::println("{}\n\n\n", s);
  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    auto const s = perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

      decltype(data)::value_type sum = 0;
      for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
        for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
          for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
            sum += mdSpan[i, j-2, k] + mdSpan[i, j-1, k] + mdSpan[i,j,k] + mdSpan[i, j+1, k] + mdSpan[i, j+2, k];

      return sum;
    }, {}, regionsToFlush);

    std::println("Y-loop timing results:");
    std::println("{}\n\n\n", s);
  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    auto const s = perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

      decltype(data)::value_type sum = 0;
      for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
        for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
          for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
            sum += mdSpan[i, j, k-2] + mdSpan[i, j, k-1] + mdSpan[i,j,k] + mdSpan[i, j, k+1] + mdSpan[i, j, k+2];

      return sum;
    }, {}, regionsToFlush);

    std::println("Z-loop timing results:");
    std::println("{}\n\n\n", s);
  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    auto const s = perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

      decltype(data)::value_type sum = 0;
      for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
        for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
          for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
            sum += mdSpan[i, j, k-2] + mdSpan[i, j, k-1] + mdSpan[i,j,k] + mdSpan[i, j, k+1] + mdSpan[i, j, k+2];

      return sum;
    }, {}, regionsToFlush);

    std::println("Z-loop timing results:");
    std::println("{}\n\n\n", s);
  }

  return 0;
}