project(compute_node)

cmake_minimum_required(VERSION 3.22)

add_executable(main
 main.cpp
 ownership_policies.h
)

target_compile_features(main PUBLIC cxx_std_20)