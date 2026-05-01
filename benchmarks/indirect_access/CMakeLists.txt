add_executable(indirect_access main.cpp)
target_link_libraries(indirect_access PRIVATE perf)
target_compile_options(indirect_access PRIVATE -O3 -march=native)