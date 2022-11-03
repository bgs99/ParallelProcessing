#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#ifndef LAB3_SCHEDULE
#define LAB3_SCHEDULE auto
#endif // ndef LAB3_SCHEDULE

#ifndef _OPENMP
void omp_set_num_threads(int _threads) {}
#endif // ndef _OPENMP

const unsigned int A = 324;

float rand_f_r(unsigned int *const seed, const float min, const float max) {
    return min + (float)rand_r(seed) / ((float)RAND_MAX / (max - min));
}

float sinh_sqr(const float val) {
    const float t = sinhf(val);
    return t * t;
}

float tan_abs(const float val) {
    const float t = tanf(val);
    return t >= 0 ? t : -t;
}

void swap(float *const xp, float *const yp) {
    const int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void selection_sort(float arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        int min_idx = i;
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
        fprintf(stderr, "Expected 1 arg with M and N\n");
        return -1;
    }

    const unsigned int M = atoi(argv[1]);
    omp_set_num_threads(M);

    const unsigned int N = atoi(argv[2]);

    float *M1 = malloc(sizeof(float) * N);
    float *M2 = malloc(sizeof(float) * N / 2);
    float *M2_copy = malloc(sizeof(float) * N / 2 + 1);
    int m1_i;
    int m2_i;

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

        // Map

#pragma omp parallel for default(none) private(m1_i) shared(M1, N, M) schedule(LAB3_SCHEDULE)
        for (m1_i = 0; m1_i < N; m1_i++) {
            M1[m1_i] = sinh_sqr(M1[m1_i]);
        }

#pragma omp parallel for default(none) private(m2_i) shared(M2, M2_copy, N, M) schedule(LAB3_SCHEDULE)
        for (m2_i = 0; m2_i < N / 2; m2_i++) {
            M2_copy[m2_i] = M2[m2_i];
        }

#pragma omp parallel for default(none) private(m2_i) shared(M2, M2_copy, N, M) schedule(LAB3_SCHEDULE)
        for (m2_i = 0; m2_i < N / 2; m2_i++) {
            M2[m2_i] = tan_abs((m2_i == 0 ? 0 : M2_copy[m2_i - 1]) + M2[m2_i]);
        }

#pragma omp parallel for default(none) private(m2_i) shared(M1, M2, N, M) schedule(LAB3_SCHEDULE)
        for (m2_i = 0; m2_i < N / 2; m2_i++) {
            M2[m2_i] = powf(M1[m2_i], M2[m2_i]);
        }

        selection_sort(M2, N / 2);

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

        float X = 0;

#pragma omp parallel for reduction(+:X) default(none) private(m2_i) shared(M1, M2, N, min, M) schedule(LAB3_SCHEDULE)
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
