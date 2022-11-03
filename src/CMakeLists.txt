set(CMAKE_C_COMPILER gcc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenMP)

add_executable(lab3 main.c)

target_compile_options(lab3 PRIVATE -O3 -Wall -Werror -fopenmp)

target_link_libraries(lab3 m OpenMP::OpenMP_C)

foreach(schedule static dynamic guided)
    foreach(chunk_size 1 M-1 M M+1)
        add_executable(lab3-${schedule}-${chunk_size} main.c)

        target_compile_options(lab3-${schedule}-${chunk_size} PRIVATE -O3 -Wall -Werror -fopenmp "-DLAB3_SCHEDULE=${schedule}, ${chunk_size}")

        target_link_libraries(lab3-${schedule}-${chunk_size} m OpenMP::OpenMP_C)
    endforeach()
endforeach()
