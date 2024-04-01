cmake_minimum_required(VERSION 3.8)
project(prism)

add_library(prism prism.cpp)
target_compile_features(prism PUBLIC cxx_std_17)
target_include_directories(prism INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(prism-terminal terminal.cpp)
target_link_libraries(prism-terminal prism)
