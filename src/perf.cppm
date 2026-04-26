module;

// #include <immintrin.h>
#include <sched.h>
#include <unistd.h>

export module perf;

import std;

export namespace perf
{
  int hello() { return 42; }
}