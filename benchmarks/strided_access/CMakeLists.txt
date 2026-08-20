add_executable(strided_access main.cpp)
target_link_libraries(strided_access PRIVATE perf)
target_compile_options(strided_access PRIVATE -O3 -march=native -g -save-temps -fverbose-asm)