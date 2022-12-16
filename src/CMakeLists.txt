set(CMAKE_C_COMPILER gcc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCL REQUIRED)

add_executable(lab6 main.c)

target_compile_options(lab6 PRIVATE -O3 -g -Wall -Werror -DCL_TARGET_OPENCL_VERSION=300)

target_link_libraries(lab6 PRIVATE OpenCL::OpenCL)


add_executable(lab6-asan main.c)

target_compile_options(lab6-asan PRIVATE -O3 -g -Wall -Werror -fsanitize=address -DCL_TARGET_OPENCL_VERSION=300)

target_link_libraries(lab6-asan PRIVATE asan OpenCL::OpenCL)