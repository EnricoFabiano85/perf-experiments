import std;
import perf;

int main()
{
  using DataType = int;
  auto constexpr vectorSize = 100'000'000;
  auto const data = std::views::iota(0, vectorSize) | std::ranges::to<std::vector>();
  auto const nIter = 10;
  
  {
    auto const s = perf::measure(nIter, [&](){
      decltype(data)::value_type sum = 0;
      for (auto const &e : data) sum += e;
      return sum;
    });
    std::println("Range-based timing results:");
    std::println("{}\n\n\n", s);
  }

  return 0;
}