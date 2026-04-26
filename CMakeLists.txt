cmake_minimum_required(VERSION 3.30)

# Experimental gate — required for now; this UUID will go away in a future CMake release
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")

set(CMAKE_CXX_COMPILER "clang++-21")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

project(perf_experiments CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_MODULE_STD ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Generator check (modules need Ninja or VS) -----------------------------
if(NOT CMAKE_GENERATOR MATCHES "Ninja|Visual Studio")
  message(FATAL_ERROR "C++ modules require the Ninja or Visual Studio generator. "
                      "Run cmake with -G Ninja.")
endif()

add_library(perf)
target_sources(perf
  PUBLIC
    FILE_SET CXX_MODULES
    FILES src/perf.cppm
)

target_compile_features(perf PUBLIC cxx_std_23)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE perf)