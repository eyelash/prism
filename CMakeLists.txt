cmake_minimum_required(VERSION 3.8)
project(prism)

add_executable(prism-terminal terminal.cpp)
target_compile_features(prism-terminal PUBLIC cxx_std_17)

add_executable(scope scope.cpp)
target_compile_features(scope PUBLIC cxx_std_17)
