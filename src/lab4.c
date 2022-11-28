#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>

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
    const float temp = *xp;
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

        swap(&arr[i], &arr[min_idx]);
    }
}

struct span {
    float *start;
    int size;
    int pos;
};

void split_sort(float array[], const int array_len) {
    const int span_count = omp_get_num_procs();
    const int span_len = array_len / span_count + (array_len % span_count > 0);
    struct span spans[span_count];
    float array_copy[array_len];
    for (int element_idx = 0; element_idx < array_len; ++element_idx) {
        array_copy[element_idx] = array[element_idx];
    }

#pragma omp parallel for default(none)                                         \
    shared(span_count, array_len, span_len, spans, array_copy)
    for (int span_idx = 0; span_idx < span_count; ++span_idx) {
        const int span_start = span_idx * span_len;
        const int span_end = span_start + span_len;
        const int span_size =
            (span_end > array_len ? array_len : span_end) - span_start;

        spans[span_idx].start = array_copy + span_start;
        spans[span_idx].size = span_size;
        spans[span_idx].pos = 0;

        selection_sort(spans[span_idx].start, spans[span_idx].size);
    }

    for (int element_idx = 0; element_idx < array_len; ++element_idx) {
        float min = INFINITY;
        int min_span_idx = 0;
        for (int span_idx = 0; span_idx < span_count; ++span_idx) {
            struct span *cur_span = spans + span_idx;
            if (cur_span->pos >= cur_span->size) {
                continue;
            }
            if (cur_span->start[cur_span->pos] < min) {
                min = cur_span->start[cur_span->pos];
                min_span_idx = span_idx;
            }
        }
        array[element_idx] = min;
        ++spans[min_span_idx].pos;
    }
}

struct timing {
    double gen;
    double map;
    double merge;
    double sort;
    double reduce;
};

void print_timing(struct timing *timing) {
    printf("Generation time: %f sec\n", timing->gen);
    printf("Map time: %f sec\n", timing->map);
    printf("Merge time: %f sec\n", timing->merge);
    printf("Sort time: %f sec\n", timing->sort);
    printf("Reduce time: %f sec\n", timing->reduce);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Expected 1 arg with M and N\n");
        return -1;
    }

    const unsigned int M = atoi(argv[1]);
    omp_set_num_threads(M);
    omp_set_nested(true);

    const unsigned int N = atoi(argv[2]);

    float *M1 = malloc(sizeof(float) * N);
    float *M2 = malloc(sizeof(float) * N / 2);
    float *M2_copy = malloc(sizeof(float) * N / 2 + 1);
    int m1_i;
    int m2_i;

    const int loop_size = 100;

    const double T1 = omp_get_wtime();
    double T2;

    float Xs[loop_size];

    int iterations_done = 0;

    struct timing total_timing = {};

#pragma omp parallel sections
    {
#pragma omp section
        {
            for (int i = 0; i < loop_size; i++) {
                struct timing iteration_timing;

                double t_start = omp_get_wtime();
                unsigned int rand_seed = i;

                // Generate

                for (int m1_i = 0; m1_i < N; m1_i++) {
                    M1[m1_i] = rand_f_r(&rand_seed, 1, A);
                }

                for (int m2_i = 0; m2_i < N / 2; m2_i++) {
                    M2[m2_i] = rand_f_r(&rand_seed, A, 10 * A);
                }

                iteration_timing.gen = omp_get_wtime();

                // Map

#pragma omp parallel for default(none) private(m1_i) shared(M1, N, M)
                for (m1_i = 0; m1_i < N; m1_i++) {
                    M1[m1_i] = sinh_sqr(M1[m1_i]);
                }

#pragma omp parallel for default(none) private(m2_i) shared(M2, M2_copy, N, M)
                for (m2_i = 0; m2_i < N / 2; m2_i++) {
                    M2_copy[m2_i] = M2[m2_i];
                }

#pragma omp parallel for default(none) private(m2_i) shared(M2, M2_copy, N, M)
                for (m2_i = 0; m2_i < N / 2; m2_i++) {
                    M2[m2_i] =
                        tan_abs((m2_i == 0 ? 0 : M2_copy[m2_i - 1]) + M2[m2_i]);
                }

                iteration_timing.map = omp_get_wtime();

#pragma omp parallel for default(none) private(m2_i) shared(M1, M2, N, M)
                for (m2_i = 0; m2_i < N / 2; m2_i++) {
                    M2[m2_i] = powf(M1[m2_i], M2[m2_i]);
                }


                iteration_timing.merge = omp_get_wtime();

                split_sort(M2, N / 2);

                iteration_timing.sort = omp_get_wtime();

                float min = 0;
                for (int m2_i = 0; m2_i < N / 2; ++m2_i) {
                    if (M2[m2_i] != 0) {
                        min = M2[m2_i];
                        break;
                    }
                }
                if (min == 0) {
                    abort();
                }

                float X = 0;

#pragma omp parallel for reduction(+:X) default(none) private(m2_i) shared(M1, M2, N, min, M)
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


                iteration_timing.reduce = omp_get_wtime();

                total_timing.gen += iteration_timing.gen - t_start;
                total_timing.map += iteration_timing.map - iteration_timing.gen;
                total_timing.merge += iteration_timing.merge - iteration_timing.map;
                total_timing.sort += iteration_timing.sort - iteration_timing.merge;
                total_timing.reduce += iteration_timing.reduce - iteration_timing.sort;

#pragma omp atomic
                ++iterations_done;
            }

            T2 = omp_get_wtime();
        }

#pragma omp section
        {
            int done_copy;
            while (true) {
#pragma omp atomic read
                done_copy = iterations_done;

                printf("%f%% done\n", (double)done_copy / loop_size * 100);

                if (done_copy == loop_size) {
                    break;
                }

                usleep(1000000);
            }
        }
    }

    print_timing(&total_timing);

    printf("\nN=%d. Milliseconds passed: %f\n", N, (T2 - T1) * 1000);

    for (int i = 0; i < loop_size; i++) {
        printf("X=%f\n", Xs[i]);
    }

    return 0;
}
