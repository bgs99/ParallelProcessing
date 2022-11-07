#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#pragma GCC diagnostic ignored "-Wunknown-pragmas"

int main(int argc, char *argv[]) {
    #pragma omp parallel for
    for (int i = 0; i < 100; i++) {
        printf("Thread %d is running number %d\n", omp_get_thread_num(), i);
    }

    return 0;
}
