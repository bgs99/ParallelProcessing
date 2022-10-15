#include <fwBase.h>
#include <fwSignal.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

const unsigned int A = 324;

float rand_f_r(unsigned int *const seed, const float min, const float max) {
    return min + (float)rand_r(seed) / ((float)RAND_MAX / (max - min));
}

void swap(float *const xp, float *const yp) {
    const int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void selection_sort(float arr[], int n) {
    int min_idx;

    for (int i = 0; i < n - 1; i++) {
        min_idx = i;
        for (int j = i + 1; j < n; j++) {
            if (arr[j] < arr[min_idx]) {
                min_idx = j;
            }
        }

        swap(&arr[min_idx], &arr[i]);
    }
}

int main(int argc, char *argv[]) {
    struct timeval T1, T2;

    if (argc < 3) {
        fprintf(stderr, "Expected 2 args with M and N\n");
        return -1;
    }

    const unsigned int M = atoi(argv[1]);
    fwSetNumThreads(M);

    const unsigned int N = atoi(argv[2]);
    
    float *M1 = malloc(sizeof(float) * N);
    float *M2 = malloc(sizeof(float) * N / 2);
    float *M2_copy = malloc(sizeof(float) * N / 2 + 1);

    const int loop_size = 100;

    gettimeofday(&T1, NULL);

    float Xs[loop_size];

    for (int i = 0; i < loop_size; i++) {
        unsigned int rand_seed = i;

        // Generate

        for (int m1_i = 0; m1_i < N; m1_i++) {
            M1[m1_i] = rand_f_r(&rand_seed, 1, A);
        }

        for (int m2_i = 0; m2_i < N / 2; m2_i++) {
            M2[m2_i] = rand_f_r(&rand_seed, A, 10 * A);
        }

        // Map 1

        fwsSinh_32f_A24(M1, M1, N);
        fwsMul_32f_I(M1, M1, N);

        // Map 2

        M2_copy[0] = 0;
        for (int m2_i = 0; m2_i < N / 2; m2_i++) {
            M2_copy[m2_i + 1] = M2[m2_i];
        }

        fwsAdd_32f_I(M2_copy, M2, N / 2);
        fwsTan_32f_A24(M2, M2, N / 2);
        fwsAbs_32f_I(M2, N / 2);

        // Merge

        fwsPow_32f_A24(M1, M2, M2, N / 2);

        // Uncomment to improve correctness
        /*for (int m2_i = 0; m2_i < N / 2; ++m2_i) {
            if (isinf(M1[m2_i])) {
                M2[m2_i] = INFINITY;
            }
        }*/

        // Sort

        selection_sort(M2, N / 2);

        // Reduce

        float X = 0;
        float min = 0;
        for (int m2_i = 0; m2_i < N / 2; ++m2_i) {
            if (M2[m2_i] != 0) {
                min = M2[m2_i];
                break;
            }
        }
        if (min == 0) {
            return -2;
        }

        for (int m2_i = 0; m2_i < N / 2; ++m2_i) {
            if (isinf(M2[m2_i])) {
                continue;
            }
            const float sin_res = sinf(M2[m2_i]);
            const unsigned int quotient = sin_res / min;
            if (quotient % 2 == 0) {
                X += sin_res;
            }
        }

        Xs[i] = X;
    }

    gettimeofday(&T2, NULL);

    const long delta_ms =
        1000 * (T2.tv_sec - T1.tv_sec) + (T2.tv_usec - T1.tv_usec) / 1000;

    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms);

    for (int i = 0; i < loop_size; i++) {
        printf("X=%f\n", Xs[i]);
    }

    return 0;
}
