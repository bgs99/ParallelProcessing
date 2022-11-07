set(CMAKE_C_COMPILER gcc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenMP)

add_executable(lab3 main.c)

target_compile_options(lab3 PRIVATE -O3 -Wall -Werror -fopenmp)

target_link_libraries(lab3 m OpenMP::OpenMP_C)


add_executable(schedule schedule.c)

target_compile_options(schedule PRIVATE -O3 -Wall -Werror -fopenmp)

target_link_libraries(schedule m OpenMP::OpenMP_C)

foreach(schedule static dynamic guided)
    foreach(chunk_size 1 M-1 M M+1)
        add_executable(lab3-${schedule}-${chunk_size} main.c)

        target_compile_options(lab3-${schedule}-${chunk_size} PRIVATE -O3 -Wall -Werror -fopenmp "-DLAB3_SCHEDULE=${schedule}, ${chunk_size}")

        target_link_libraries(lab3-${schedule}-${chunk_size} m OpenMP::OpenMP_C)
    endforeach()
endforeach()

foreach(opt_level -O0 -O1 -O2)
        add_executable(lab3-${opt_level} main.c)

        target_compile_options(lab3-${opt_level} PRIVATE ${opt_level} -Wall -Werror -fopenmp "-DLAB3_SCHEDULE=guided, M")

        target_link_libraries(lab3-${opt_level} m OpenMP::OpenMP_C)
endforeach()