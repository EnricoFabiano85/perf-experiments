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

    auto const s = perf::measure<perf::CachePolicy::Cold>(nIter, [&]{
      decltype(data)::value_type sum = 0;
      for (auto const &e : data) sum += e;
      return sum;
    }, assertResult, regionsToFlush);
    std::println("Range-based timing results:");
    std::println("{}\n\n\n", s);
  }

  return 0;
}