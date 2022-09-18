set(CMAKE_C_COMPILER gcc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(make_exec ExecName)
    add_executable(${ExecName} lab1.c)
    target_link_libraries(${ExecName} m)
    target_compile_options(${ExecName} PRIVATE -O3 -Wall -Werror)
endfunction()

make_exec(lab1-seq)

function(make_par_exec ExecName K)
    make_exec(${ExecName})
    target_link_libraries(${ExecName} OpenMP::OpenMP_C)
    target_compile_options(${ExecName} PRIVATE -floop-parallelize-all -ftree-parallelize-loops=${K})
endfunction()

foreach(K 1 2 3 4 6)
    make_par_exec(lab1-par-${K} ${K})
endforeach()

make_exec(lab1-seq-verify)
target_compile_definitions(lab1-seq-verify PRIVATE VERIFY)

make_par_exec(lab1-par-verify 2)
target_compile_definitions(lab1-par-verify PRIVATE VERIFY)
