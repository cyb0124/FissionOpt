cmake_minimum_required(VERSION 3.14)
project(FissionOpt)
set(CMAKE_CXX_STANDARD 17)

add_executable(FissionOpt
  Fission.h
  Fission.cpp
  OptFission.h
  OptFission.cpp
  Benchmark.cpp
)

target_include_directories(FissionOpt PRIVATE
  ../xtensor/include
  ../xtl/include
)
