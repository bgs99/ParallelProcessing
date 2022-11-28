#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int get_num_procs() { return sysconf(_SC_NPROCESSORS_ONLN); }

double get_wtime() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + ((double)t.tv_usec) / 1000000.0;
}

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
};

void split_sort(float array[], float array_copy_buf[], const int array_len,
                const int tid, pthread_barrier_t *barrier, bool is_serial) {

    const int span_count = get_num_procs();
    const int span_len = array_len / span_count + (array_len % span_count > 0);
    struct span spans[span_count];

    for (int i = 0; i < span_count; ++i) {
        const int span_start = i * span_len;
        const int span_end = span_start + span_len;
        const int span_size =
            (span_end > array_len ? array_len : span_end) - span_start;

        spans[i].start = array_copy_buf + span_start;
        spans[i].size = span_size;
    }

    if (is_serial) {
        for (int element_idx = 0; element_idx < array_len; ++element_idx) {
            array_copy_buf[element_idx] = array[element_idx];
        }
    }

    pthread_barrier_wait(barrier);

    if (tid < span_count) {
        selection_sort(spans[tid].start, spans[tid].size);
    }

    if (pthread_barrier_wait(barrier) != PTHREAD_BARRIER_SERIAL_THREAD) {
        return;
    }

    for (int element_idx = 0; element_idx < array_len; ++element_idx) {
        float min = INFINITY;
        int min_span_idx = 0;
        for (int span_idx = 0; span_idx < span_count; ++span_idx) {
            struct span *cur_span = spans + span_idx;
            if (cur_span->size <= 0) {
                continue;
            }
            if (*cur_span->start < min) {
                min = *cur_span->start;
                min_span_idx = span_idx;
            }
        }
        array[element_idx] = min;
        ++spans[min_span_idx].start;
        --spans[min_span_idx].size;
    }
}

struct counting_thread_args {
    const int max_iterations;
    atomic_int *iterations_done;
    pthread_mutex_t *done_mutex;
    pthread_cond_t *done_cv;
};

void *counting_thread(void *args_vptr) {
    struct counting_thread_args *args_ptr = args_vptr;
    struct counting_thread_args args = *args_ptr;
    while (true) {
        int done_copy = atomic_load(args.iterations_done);

        printf("%f%% done\n", (double)done_copy / args.max_iterations * 100);

        if (done_copy == args.max_iterations) {
            break;
        }

        struct timeval now;
        gettimeofday(&now, NULL);
        struct timespec wakeup_time = {.tv_sec = now.tv_sec + 1,
                                       .tv_nsec = now.tv_usec * 1000};

        pthread_mutex_lock(args.done_mutex);
        pthread_cond_timedwait(args.done_cv, args.done_mutex, &wakeup_time);
        pthread_mutex_unlock(args.done_mutex);
    }

    return NULL;
}

struct guided_data {
    atomic_int *unassigned_iters;
    int min_chunk_size;
};

struct span get_guided_span(struct guided_data data, float array[], int size,
                            int thread_count) {
    struct span result;
    int old_unassigned = atomic_load(data.unassigned_iters);
    while (true) {
        if (old_unassigned == 0) {
            result.size = 0;
            return result;
        }

        int chunk_size = old_unassigned / thread_count / 4;
        if (chunk_size < data.min_chunk_size) {
            chunk_size = data.min_chunk_size;
        }
        if (chunk_size > old_unassigned) {
            chunk_size = old_unassigned;
        }

        if (atomic_compare_exchange_weak(data.unassigned_iters, &old_unassigned,
                                         old_unassigned - chunk_size)) {
            result.start = array + size - old_unassigned;
            result.size = chunk_size;
            return result;
        }
    }
}

struct work_args {
    int tid;
    int N;
    int M;
    float *M1;
    float *M2;
    float *M2_copy;
    float *result;
    pthread_barrier_t *barrier;
    struct guided_data reduction;
};

void *work(void *args_vptr) {
    struct work_args *args_ptr = args_vptr;
    struct work_args args = *args_ptr;

    const int m1_slice_len = args.N / args.M + (args.N % args.M == 0 ? 0 : 1);
    const int m1_slice_start = args.tid * m1_slice_len;
    const int m1_slice_end = m1_slice_start + m1_slice_len > args.N
                                 ? args.N
                                 : m1_slice_start + m1_slice_len;
    const int m2_slice_len =
        (args.N / 2) / args.M + ((args.N % (2 * args.M)) == 0 ? 0 : 1);
    const int m2_slice_start = args.tid * m2_slice_len;
    const int m2_slice_end = m2_slice_start + m2_slice_len > args.N / 2
                                 ? args.N / 2
                                 : m2_slice_start + m2_slice_len;

    for (int m1_i = m1_slice_start; m1_i < m1_slice_end; m1_i++) {
        args.M1[m1_i] = sinh_sqr(args.M1[m1_i]);
    }

    for (int m2_i = m2_slice_start; m2_i < m2_slice_end; m2_i++) {
        args.M2_copy[m2_i] = args.M2[m2_i];
    }

    if (pthread_barrier_wait(args.barrier) == PTHREAD_BARRIER_SERIAL_THREAD) {
        fprintf(stdout, "Map time: %f\n", get_wtime());
    }

    for (int m2_i = m2_slice_start; m2_i < m2_slice_end; m2_i++) {
        args.M2[m2_i] =
            tan_abs((m2_i == 0 ? 0 : args.M2_copy[m2_i - 1]) + args.M2[m2_i]);
    }

    for (int m2_i = m2_slice_start; m2_i < m2_slice_end; m2_i++) {
        args.M2[m2_i] = powf(args.M1[m2_i], args.M2[m2_i]);
    }

    bool is_serial =
        pthread_barrier_wait(args.barrier) == PTHREAD_BARRIER_SERIAL_THREAD;

    if (is_serial) {
        fprintf(stdout, "Merge time: %f\n", get_wtime());
    }

    split_sort(args.M2, args.M2_copy, args.N / 2, args.tid, args.barrier,
               is_serial);

    if (pthread_barrier_wait(args.barrier) == PTHREAD_BARRIER_SERIAL_THREAD) {
        fprintf(stdout, "Sort time: %f\n", get_wtime());
    }

    float min = 0;
    for (int m2_i = 0; m2_i < args.N / 2; ++m2_i) {
        if (args.M2[m2_i] != 0) {
            min = args.M2[m2_i];
            break;
        }
    }
    if (min == 0) {
        abort();
    }

    *args.result = 0;
    struct span guided_span;
    while (guided_span =
               get_guided_span(args.reduction, args.M2, args.N / 2, args.M),
           guided_span.size > 0) {
        for (int i = 0; i < guided_span.size; ++i) {
            if (isinf(guided_span.start[i])) {
                continue;
            }
            const float sin_res = sinf(guided_span.start[i]);
            const unsigned int quotient = sin_res / min;
            if (quotient % 2 == 0) {
                *args.result += sin_res;
            }
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Expected 1 arg with M and N\n");
        return -1;
    }

    int M = atoi(argv[1]);
    if (M < 1) {
        M = 1;
    }

    const unsigned int N = atoi(argv[2]);

    float *M1 = malloc(sizeof(float) * N);
    float *M2 = malloc(sizeof(float) * N / 2);
    float *M2_copy = malloc(sizeof(float) * N / 2 + 1);

    const int loop_size = 100;

    const double T1 = get_wtime();
    double T2;

    float Xs[loop_size];

    atomic_int iterations_done = 0;

    pthread_cond_t done_cv = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t done_mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_t progress_thread;

    struct counting_thread_args counting_thread_args = {
        .max_iterations = loop_size,
        .iterations_done = &iterations_done,
        .done_cv = &done_cv,
        .done_mutex = &done_mutex,
    };

    pthread_create(&progress_thread, NULL, &counting_thread,
                   &counting_thread_args);

    for (int i = 0; i < loop_size; i++) {
        unsigned int rand_seed = i;

        // Generate

        for (int m1_i = 0; m1_i < N; m1_i++) {
            M1[m1_i] = rand_f_r(&rand_seed, 1, A);
        }

        for (int m2_i = 0; m2_i < N / 2; m2_i++) {
            M2[m2_i] = rand_f_r(&rand_seed, A, 10 * A);
        }


        pthread_barrier_t barrier;
        pthread_barrier_init(&barrier, NULL, M);

        atomic_int unassigned_iters = N / 2;
        float results[M];

        struct work_args args[M];

        pthread_t work_threads[M];

        for (int m = 0; m < M; ++m) {
            args[m].tid = m;
            args[m].N = N;
            args[m].M = M;
            args[m].M1 = M1;
            args[m].M2 = M2;
            args[m].M2_copy = M2_copy;
            args[m].result = &results[m];
            args[m].barrier = &barrier;
            args[m].reduction.unassigned_iters = &unassigned_iters;
            args[m].reduction.min_chunk_size = 100;

            if (m > 0) {
                pthread_create(&work_threads[m], NULL, &work, &args[m]);
            }
        }
        work(&args[0]);
        Xs[i] = results[0];

        for (int m = 1; m < M; ++m) {
            pthread_join(work_threads[m], NULL);
            Xs[i] += results[m];
        }

        fprintf(stdout, "Reduce time: %f\n", get_wtime());

        atomic_fetch_add(&iterations_done, 1);
    }

    T2 = get_wtime();

    pthread_mutex_lock(&done_mutex);
    pthread_cond_signal(&done_cv);
    pthread_mutex_unlock(&done_mutex);

    pthread_join(progress_thread, NULL);

    printf("\nN=%d. Milliseconds passed: %f\n", N, (T2 - T1) * 1000);

    for (int i = 0; i < loop_size; i++) {
        printf("X=%f\n", Xs[i]);
    }

    free(M1);
    free(M2);
    free(M2_copy);

    return 0;
}
