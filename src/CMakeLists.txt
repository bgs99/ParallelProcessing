set(CMAKE_C_COMPILER gcc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(lab5 main.c)

target_compile_options(lab5 PRIVATE -O3 -Wall -Werror -pthread)

target_link_libraries(lab5 m pthread)


add_executable(lab5-tsan main.c)

target_compile_options(lab5-tsan PRIVATE -Og -g -Wall -Werror -pthread -fsanitize=thread)

target_link_libraries(lab5-tsan m pthread tsan)


find_package(OpenMP)

add_executable(lab5_4 main.c)

target_compile_options(lab5_4 PRIVATE -O3 -Wall -Werror -fopenmp)

target_link_libraries(lab5_4 m OpenMP::OpenMP_C)
