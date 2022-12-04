set(CMAKE_C_COMPILER gcc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCL REQUIRED)

add_custom_target(
    NAME lab6-cl
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/main.cl ${CMAKE_CURRENT_BINARY_DIR}/lab6.cl
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cl
)

add_executable(lab6 main.c)

add_dependencies(lab6 lab6-cl)

target_compile_options(lab6 PRIVATE -Og -g -Wall -Werror -pthread -DCL_TARGET_OPENCL_VERSION=300)

target_link_options(lab6 PRIVATE -Wl,--format=binary -Wl,src/lab6.cl -Wl,--format=default)
target_link_libraries(lab6 PRIVATE m pthread OpenCL::OpenCL)

