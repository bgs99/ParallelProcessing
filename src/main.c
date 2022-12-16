#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <CL/opencl.h>

#define CL_ASSERT(expr)                                                        \
    {                                                                          \
        cl_int ec = expr;                                                      \
        if (ec != CL_SUCCESS) {                                                \
            fprintf(stderr, "Error: %d at %d\n", ec, __LINE__);                \
            abort();                                                           \
        }                                                                      \
    }

double get_wtime() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + ((double)t.tv_usec) / 1000000.0;
}

const unsigned int A = 324;

float rand_f_r(unsigned int *const seed, const float min, const float max) {
    return min + (float)rand_r(seed) / ((float)RAND_MAX / (max - min));
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
    printf("Map        time: %f sec\n", timing->map);
    printf("Merge      time: %f sec\n", timing->merge);
    printf("Sort       time: %f sec\n", timing->sort);
    printf("Reduce     time: %f sec\n", timing->reduce);
}

struct span {
    size_t start;
    size_t size;
};

struct span get_span(const size_t threads, const size_t idx, const int n) {
    const size_t overhead = n % threads;
    const size_t base_span_len = n / threads;
    const size_t overhead_spent = idx > overhead ? overhead : idx;
    const size_t span_start = overhead_spent + base_span_len*idx;

    const size_t span_len_with_overhead = base_span_len + (overhead > overhead_spent ? 1 : 0);

    size_t span_end = span_start + span_len_with_overhead;

    if (span_end > n) {
        span_end = n;
    }

    const size_t span_len = span_end - span_start;

    struct span res = {.start = span_start, .size = span_len};
    return res;
}

void merge_sort(float array[], float array_copy[], const int array_len,
                const size_t threads) {
    struct span spans[threads];
    for (size_t i = 0; i < threads; ++i) {
        spans[i] = get_span(threads, i, array_len);
    }

    for (int element_idx = 0; element_idx < array_len; ++element_idx) {
        float min = INFINITY;
        int min_span_idx = 0;
        for (int span_idx = 0; span_idx < threads; ++span_idx) {
            struct span *cur_span = spans + span_idx;
            if (cur_span->size <= 0) {
                continue;
            }
            if (array_copy[cur_span->start] < min) {
                min = array_copy[cur_span->start];
                min_span_idx = span_idx;
            }
        }
        if (isinf(min) && min > 0) {
            for (; element_idx < array_len; ++element_idx) {
                array[element_idx] = INFINITY;
            }
            return;
        }
        array[element_idx] = min;
        ++spans[min_span_idx].start;
        --spans[min_span_idx].size;
    }
}

double get_events_time(cl_event start, cl_event end) {
    cl_ulong start_us, end_us;
    CL_ASSERT(clGetEventProfilingInfo(start, CL_PROFILING_COMMAND_START,
                                      sizeof(cl_ulong), &start_us, NULL));

    CL_ASSERT(clGetEventProfilingInfo(end, CL_PROFILING_COMMAND_END,
                                      sizeof(cl_ulong), &end_us, NULL));

    const double factor = 1e9;

    return (end_us - start_us) / factor;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Expected 3 args with M, N and CL path\n");
        return -1;
    }

    size_t M = atoi(argv[1]);
    if (M < 1) {
        M = 1;
    }

    const unsigned int N = atoi(argv[3]);
    const size_t M1_sz = N;
    const size_t M2_sz = N / 2;

    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);

    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

    const cl_context context =
        clCreateContext(NULL, 1, &device, NULL, NULL, NULL);

    const char *cl_path = argv[2];
    const int cl_fd = open(cl_path, O_RDONLY);
    if (cl_fd < 0) {
        fprintf(stderr, "Failed to open file '%s': %s\n", cl_path,
                strerror(errno));
        return -1;
    }
    struct stat cl_stat;
    if (fstat(cl_fd, &cl_stat) != 0) {
        fprintf(stderr, "Cannot get stats of '%s': %s\n", cl_path,
                strerror(errno));
        return -1;
    }

    const size_t program_size = cl_stat.st_size;
    const char *program_source =
        mmap(NULL, program_size, PROT_READ, MAP_PRIVATE, cl_fd, 0);

    const cl_program program = clCreateProgramWithSource(
        context, 1, &program_source, &program_size, NULL);

    if (clBuildProgram(program, 1, &device, NULL, NULL, NULL) != CL_SUCCESS) {
        size_t log_size = 0;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL,
                              &log_size);
        char log[log_size];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size,
                              log, NULL);
        fprintf(stderr, "Failed to build program in %s: %s\n", cl_path, log);
        return -1;
    }

    cl_queue_properties props[] = {CL_QUEUE_PROPERTIES,
                                   CL_QUEUE_PROFILING_ENABLE, 0};

    cl_command_queue queue =
        clCreateCommandQueueWithProperties(context, device, props, NULL);

    cl_mem M1_buf = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                   M1_sz * sizeof(cl_float), NULL, NULL);

    cl_mem M2_buf = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                   M2_sz * sizeof(cl_float), NULL, NULL);

    cl_mem M2_copy_buf = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        M2_sz * sizeof(cl_float), NULL, NULL);

    cl_mem result_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                       M * sizeof(cl_float), NULL, NULL);

    const cl_kernel map_k = clCreateKernel(program, "map_k", NULL);
    clSetKernelArg(map_k, 0, sizeof(cl_mem), &M1_buf);

    const cl_kernel map2_k = clCreateKernel(program, "map2_k", NULL);
    clSetKernelArg(map2_k, 0, sizeof(cl_mem), &M2_buf);
    clSetKernelArg(map2_k, 1, sizeof(cl_mem), &M2_copy_buf);

    const cl_kernel merge_k = clCreateKernel(program, "merge_k", NULL);
    clSetKernelArg(merge_k, 0, sizeof(cl_mem), &M1_buf);
    clSetKernelArg(merge_k, 1, sizeof(cl_mem), &M2_buf);

    const cl_kernel split_sort_k =
        clCreateKernel(program, "split_sort_k", NULL);
    clSetKernelArg(split_sort_k, 0, sizeof(cl_mem), &M2_buf);
    clSetKernelArg(split_sort_k, 1, sizeof(cl_int), &N);

    const cl_kernel reduce_k = clCreateKernel(program, "reduce_k", NULL);
    clSetKernelArg(reduce_k, 0, sizeof(cl_mem), &M2_buf);
    clSetKernelArg(reduce_k, 1, sizeof(cl_int), &N);
    clSetKernelArg(reduce_k, 3, sizeof(cl_mem), &result_buf);

    float *M1 = malloc(sizeof(float) * M1_sz);
    float *M2 = malloc(sizeof(float) * M2_sz);
    float *M2_copy = malloc(sizeof(float) * M2_sz);
    float *results = malloc(sizeof(float) * M);

    const int loop_size = 100;

    const double T1 = get_wtime();
    double T2;

    float Xs[loop_size];

    struct timing total_timing = {};

    for (int i = 0; i < loop_size; i++) {
        const double t_start = get_wtime();

        unsigned int rand_seed = i;

        // Generate

        for (int m1_i = 0; m1_i < N; m1_i++) {
            M1[m1_i] = rand_f_r(&rand_seed, 1, A);
        }

        for (int m2_i = 0; m2_i < N / 2; m2_i++) {
            M2[m2_i] = rand_f_r(&rand_seed, A, 10 * A);
        }

        const double t_gen_done = get_wtime();

        cl_event buf_writes[2];
        CL_ASSERT(clEnqueueWriteBuffer(queue, M1_buf, CL_TRUE, 0,
                                       N * sizeof(cl_float), M1, 0, NULL,
                                       &buf_writes[0]));
        CL_ASSERT(clEnqueueWriteBuffer(queue, M2_buf, CL_TRUE, 0,
                                       N / 2 * sizeof(cl_float), M2, 0, NULL,
                                       &buf_writes[1]));

        cl_event map_e;
        CL_ASSERT(clEnqueueNDRangeKernel(queue, map_k, 1, NULL, &M1_sz, NULL, 1,
                                         &buf_writes[0], &map_e));

        cl_event copy1_e;
        CL_ASSERT(clEnqueueCopyBuffer(queue, M2_buf, M2_copy_buf, 0, 0,
                                      M2_sz * sizeof(cl_float), 1,
                                      &buf_writes[1], &copy1_e));

        cl_event wait[] = {map_e, copy1_e};
        cl_event map2_e;
        CL_ASSERT(clEnqueueNDRangeKernel(queue, map2_k, 1, NULL, &M2_sz, NULL,
                                         2, wait, &map2_e));

        cl_event merge_e;
        CL_ASSERT(clEnqueueNDRangeKernel(queue, merge_k, 1, NULL, &M2_sz, NULL,
                                         1, &map2_e, &merge_e));

        cl_event sort_e;
        CL_ASSERT(clEnqueueNDRangeKernel(queue, split_sort_k, 1, NULL, &M, NULL,
                                         1, &merge_e, &sort_e));

        cl_event read_e;
        // Complex move: load M2_buf as M2_copy so that merge_sort will write to
        // M2 and we can work with M2 later without additional buffer copies
        CL_ASSERT(clEnqueueReadBuffer(queue, M2_buf, CL_TRUE, 0,
                                      M2_sz * sizeof(cl_float), M2_copy, 1,
                                      &sort_e, &read_e));

        const double t_sort_start = get_wtime();

        merge_sort(M2, M2_copy, M2_sz, M);

        const double t_sort_end = get_wtime();

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

        cl_event write_e;
        CL_ASSERT(clEnqueueWriteBuffer(queue, M2_buf, CL_FALSE, 0,
                                       M2_sz * sizeof(cl_float), M2, 0, NULL,
                                       &write_e));

        cl_event reduce_e;
        CL_ASSERT(clSetKernelArg(reduce_k, 2, sizeof(cl_float), &min));
        CL_ASSERT(clEnqueueNDRangeKernel(queue, reduce_k, 1, NULL, &M, NULL, 1,
                                         &write_e, &reduce_e));

        CL_ASSERT(clEnqueueReadBuffer(queue, result_buf, CL_TRUE, 0,
                                      M * sizeof(cl_float), results, 1,
                                      &reduce_e, NULL));

        float X = 0;
        for (int m_i = 0; m_i < M; ++m_i) {
            X += results[m_i];
        }

        Xs[i] = X;

        const double t_reduce_done = get_wtime();

        total_timing.gen += t_gen_done - t_start;
        total_timing.map += get_events_time(map_e, map2_e);
        total_timing.merge += get_events_time(merge_e, merge_e);
        total_timing.sort +=
            t_sort_end - t_sort_start + get_events_time(sort_e, read_e);
        total_timing.reduce += t_reduce_done - t_sort_end;
    }

    T2 = get_wtime();

    print_timing(&total_timing);

    printf("\nN=%d. Milliseconds passed: %f\n", N, (T2 - T1) * 1000);

    for (int i = 0; i < loop_size; i++) {
        printf("X=%f\n", Xs[i]);
    }

    free(M1);
    free(M2);
    free(M2_copy);
    free(results);

    return 0;
}
