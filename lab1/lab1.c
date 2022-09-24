#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

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

    if (argc < 2) {
        fprintf(stderr, "Expected 1 arg with N\n");
        return -1;
    }

    const unsigned int N = atoi(argv[1]);
    const unsigned int NMAX = 500000;
    if (N > NMAX) {
        fprintf(stderr, "Expected N to be less than %d\n", NMAX);
        return -1;
    }

    const int loop_size = 100;

    gettimeofday(&T1, NULL);

    for (int i = 0; i < loop_size; i++) {
        unsigned int rand_seed = i;

        // Generate

        float M1[NMAX];
        for (int m1_i = 0; m1_i < N; m1_i++) {
            M1[m1_i] = rand_f_r(&rand_seed, 1, A);
        }

        float M2[NMAX / 2];
        for (int m2_i = 0; m2_i < N / 2; m2_i++) {
            M2[m2_i] = rand_f_r(&rand_seed, A, 10 * A);
        }

        // Map

        for (int m1_i = 0; m1_i < N; m1_i++) {
            M1[m1_i] = sinh_sqr(M1[m1_i]);
        }

        float M2_copy[NMAX / 2];
        for (int m2_i = 0; m2_i < N / 2; m2_i++) {
            M2_copy[m2_i] = M2[m2_i];
        }

        for (int m2_i = 0; m2_i < N / 2; m2_i++) {
            M2[m2_i] = tan_abs((m2_i == 0 ? 0 : M2_copy[m2_i - 1]) + M2[m2_i]);
        }

        for (int m2_i = 0; m2_i < N / 2; m2_i++) {
            M2[m2_i] = powf(M1[m2_i], M2[m2_i]);
        }

        selection_sort(M2, N / 2);

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

        printf("X=%f\n", X);
    }

    gettimeofday(&T2, NULL);

    const long delta_ms =
        1000 * (T2.tv_sec - T1.tv_sec) + (T2.tv_usec - T1.tv_usec) / 1000;

    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms);

    return 0;
}
