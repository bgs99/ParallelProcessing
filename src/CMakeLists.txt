set(CMAKE_C_COMPILER gcc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FramewaveBaseDir /home/$ENV{USER}/Downloads/framewave)

add_executable(lab2 main.c)

target_include_directories(lab2 PRIVATE ${FramewaveBaseDir})

target_compile_options(lab2 PRIVATE -O3 -Wall -Werror)

target_link_directories(lab2 PRIVATE ${FramewaveBaseDir}/lib)
target_link_libraries(lab2 m fwBase fwSignal)
