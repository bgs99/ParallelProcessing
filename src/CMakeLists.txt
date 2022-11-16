set(CMAKE_C_COMPILER gcc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenMP)

add_executable(lab4 main.c)

target_compile_options(lab4 PRIVATE -O3 -Wall -Werror -fopenmp)

target_link_libraries(lab4 m OpenMP::OpenMP_C)


add_executable(lab4-noomp main.c)

target_compile_options(lab4-noomp PRIVATE -O3 -Wall -Werror)

target_link_libraries(lab4-noomp m)
