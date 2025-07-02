# This file was based on the instructions found at 
# https://github.com/vincentlaucsb/csv-parser?tab=readme-ov-file#cmake-instructions
cmake_minimum_required(VERSION 3.28)
project(cpp_pt)
set(CMAKE_CXX_STANDARD 17)
add_subdirectory(/home/kjb3/D1V1/Documents/!Dell64docs/Programming/CPP/3rd_party_libraries/csv-parser /home/kjb3/D1V1/Documents/!Dell64docs/Programming/CPP/3rd_party_libraries/csv-parser_subdiroutput)
# Note RE subdirectory output: https://stackoverflow.com/a/35260629/13097194
add_executable(cpp_pt cpp_pivot_tables.cpp)
target_link_libraries(cpp_pt csv)
