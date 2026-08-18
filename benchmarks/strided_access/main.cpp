#include <sys/mman.h>
#include <stdio.h>

import std;
import perf;

struct Record
{
  std::string const kernelName;
  perf::Stats const stats;
};

void writeToCsv(std::span<Record const> records, auto gridSize, std::string_view filePath)
{
  auto outFile = std::ofstream{filePath.data()};

  if (!outFile.is_open())
    throw std::runtime_error{std::format("writeToCsv: could not open '{}' for writing", filePath)};

  std::println(outFile, "Kernel, N, mean, std-dev");
  std::ranges::for_each(records, [&outFile, &gridSize](auto const &r){
    std::println(outFile,"{},{},{},{}", r.kernelName,
                                        gridSize,
                                        r.stats.mean_,
                                        r.stats.stdDev_);
  });
}

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

  static constexpr std::size_t hugePageSize{2uz*1024*1024}; // bytes (2Mb)

  constexpr std::size_t roundUpTo(std::size_t n)
  {
    return (n + hugePageSize - 1) / hugePageSize * hugePageSize;
  }

  std::size_t const _allocSize;
  PageSize const _pages;
  T *_data;
};

int main(int argc, char *argv[])
{

  auto const args = std::span(argv+1, argc-1);
  auto const pages = (!args.empty()  && std::string_view(args[0]).starts_with("--Huge")) 
                                      ? PageSize::Huge : PageSize::Regular;

  auto results = std::vector<Record>{};

  using DataType = int;
  auto constexpr Nx = 1024;
  auto constexpr Ny = Nx;
  auto constexpr Nz = Ny;
  auto constexpr vectorSize = Nx*Ny*Nz;
  // auto const data = std::views::iota(0, vectorSize) | std::ranges::to<std::vector>();
  auto data = MMapBuffer<int>{vectorSize, pages};

  static_assert(std::ranges::range<decltype(data)>);

  auto dataRange = std::span(data.data(), vectorSize);

  std::ranges::iota(dataRange, vectorSize);
  auto const mdSpan = std::mdspan<const int, std::dextents<size_t, 3>, std::layout_left>(data.data(),Nx, Ny, Nz);
  auto const nIter = 10;

  auto const expected = [&]{

      decltype(data)::value_type sum = 0;
      for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
        for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
          for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
            sum += mdSpan[i-2, j, k] + mdSpan[i-1, j, k] + mdSpan[i,j,k] + mdSpan[i+1, j, k] + mdSpan[i+2, j, k];

      return sum;
    }();

  auto const assertResult = [expected](DataType value){
    if (value != expected) 
    {
      std::println("FAIL: got {}, expected {}", value, expected);
      std::abort();
    }
  };
  
  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    results.emplace_back("X-sweep k-j-i",
      perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

        decltype(data)::value_type sum = 0;
        for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
          for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
            for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
              sum += mdSpan[i-2, j, k] + mdSpan[i-1, j, k] + mdSpan[i,j,k] + mdSpan[i+1, j, k] + mdSpan[i+2, j, k];

        return sum;
      }, assertResult, regionsToFlush)
    );

  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    results.emplace_back("X-sweep j-k-i", 
      perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

        decltype(data)::value_type sum = 0;
        for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
          for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
            for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
              sum += mdSpan[i-2, j, k] + mdSpan[i-1, j, k] + mdSpan[i,j,k] + mdSpan[i+1, j, k] + mdSpan[i+2, j, k];

        return sum;
      }, assertResult, regionsToFlush)
    );

  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    results.emplace_back("Y-sweep k-i-j",
      perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

        decltype(data)::value_type sum = 0;
        for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
          for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
            for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
              sum += mdSpan[i, j-2, k] + mdSpan[i, j-1, k] + mdSpan[i,j,k] + mdSpan[i, j+1, k] + mdSpan[i, j+2, k];

        return sum;
      }, assertResult, regionsToFlush)
    );

  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    results.emplace_back("Y-sweep i-k-j",
      perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

        decltype(data)::value_type sum = 0;
        for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
          for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
            for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
              sum += mdSpan[i, j-2, k] + mdSpan[i, j-1, k] + mdSpan[i,j,k] + mdSpan[i, j+1, k] + mdSpan[i, j+2, k];

        return sum;
      }, assertResult, regionsToFlush)
    );

  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    results.emplace_back("Y-sweep j-k-i",
      perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

        decltype(data)::value_type sum = 0;
        for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
          for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
            for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
              sum += mdSpan[i, j-2, k] + mdSpan[i, j-1, k] + mdSpan[i,j,k] + mdSpan[i, j+1, k] + mdSpan[i, j+2, k];

        return sum;
      }, assertResult, regionsToFlush)
    );

  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    results.emplace_back("Y-sweep k-j-i", 
      perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

      decltype(data)::value_type sum = 0;
      for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
        for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
          for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
            sum += mdSpan[i, j-2, k] + mdSpan[i, j-1, k] + mdSpan[i,j,k] + mdSpan[i, j+1, k] + mdSpan[i, j+2, k];

      return sum;
    }, assertResult, regionsToFlush)
    );

  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    results.emplace_back("Z-sweep j-i-k", 
      perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

        decltype(data)::value_type sum = 0;
        for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
          for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
            for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
              sum += mdSpan[i, j, k-2] + mdSpan[i, j, k-1] + mdSpan[i,j,k] + mdSpan[i, j, k+1] + mdSpan[i, j, k+2];

        return sum;
      }, assertResult, regionsToFlush)
    );

  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    results.emplace_back("Z-sweep j-k-i",
      perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

        decltype(data)::value_type sum = 0;
        for (std::size_t j = 2; j != mdSpan.extent(1)-2; j++)
          for (std::size_t k = 2; k != mdSpan.extent(2)-2; k++)
            for (std::size_t i = 2; i != mdSpan.extent(0)-2; i++)
              sum += mdSpan[i, j, k-2] + mdSpan[i, j, k-1] + mdSpan[i,j,k] + mdSpan[i, j, k+1] + mdSpan[i, j, k+2];

        return sum;
      }, assertResult, regionsToFlush)
    );

  }

  {
    perf::warmUpCpu();

    auto regionsToFlush = perf::makeFlushRegions(std::span{data});

    auto const span = std::span(data.data(), vectorSize);

    auto pencil = [](perf::contiguous_range auto r, std::size_t base, auto outerIndex, auto stride, auto len){
      typename decltype(r)::value_type sum = 0;
      for (std::size_t index = 2; index != len-2; ++index)
        sum += r[index+base + (outerIndex-2)*stride] + r[index+base + (outerIndex-1)*stride] + r[index+base + outerIndex*stride] + 
          r[index+base + (outerIndex+1)*stride] + r[index+base + (outerIndex+2)*stride];

      return sum;
    };

    results.emplace_back("Z-sweep pencil",
      perf::measure<perf::CachePolicy::Cold>(nIter, [&]{

        decltype(data)::value_type sum = 0;

        for (std::size_t j = 2; j != Ny-2; j++)
          for (std::size_t k = 2; k != Nz-2; k++)
          {
            auto const base = j*Nx;
            auto const stride = Nx*Ny;
            sum += pencil(span, base, k, stride, Nx);
            
          }

        return sum;
      }, assertResult, regionsToFlush)
    );

  }

  auto const filePath = "benchmarks/strided_access/results/strided_access_"+std::to_string(Nx)+".csv";
  writeToCsv(results, Nx, filePath);

  return 0;
}