set(CMAKE_C_COMPILER gcc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenMP)

add_executable(lab3 main.c)

target_compile_options(lab3 PRIVATE -O3 -Wall -Werror -fopenmp)

target_link_libraries(lab3 m OpenMP::OpenMP_C)
