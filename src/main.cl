float sinh_sqr(const float val) {
    const float t = sinh(val);
    return t * t;
}

float tan_abs(const float val) {
    const float t = tan(val);
    return t >= 0 ? t : -t;
}

kernel void map_k(global float *M1) {
    const size_t idx = get_global_id(0);
    M1[idx] = sinh_sqr(M1[idx]);
}

kernel void map2_k(global float *M2, global const float *M2_copy) {
    const size_t idx = get_global_id(0);
    const float f = idx == 0 ? 0 : M2_copy[idx - 1];
    M2[idx] = tan_abs(f + M2[idx]);
}

kernel void merge_k(global const float *M1, global float *M2) {
    const size_t idx = get_global_id(0);
    M2[idx] = pow(M1[idx], M2[idx]);
}

void selection_sort(global float *arr, int n) {
    for (int i = 0; i < n - 1; i++) {
        int min_idx = i;
        for (int j = i + 1; j < n; j++) {
            if (arr[j] < arr[min_idx]) {
                min_idx = j;
            }
        }

        const float temp = arr[i];
        arr[i] = arr[min_idx];
        arr[min_idx] = temp;
    }
}

struct span {
    size_t start;
    size_t len;
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

    struct span res = {.start = span_start, .len = span_len};
    return res;
}

kernel void split_sort_k(global float *M2, const int n) {
    const size_t idx = get_global_id(0);
    const size_t threads = get_global_size(0);
    const struct span sort_span = get_span(threads, idx, n / 2);
    selection_sort(M2 + sort_span.start, sort_span.len);
}

kernel void reduce_k(global const float *M2, const int n, const float min, global float *result) {
    const size_t idx = get_global_id(0);
    const size_t threads = get_global_size(0);
    const struct span min_span = get_span(threads, idx, n / 2);

    float local_result = 0;

    for (const global float * i = M2 + min_span.start; i < M2 + min_span.len; ++i) {
        if (isinf(*i)) {
            continue;
        }
        const float sin_res = sin(*i);
        const unsigned int quotient = sin_res / min;
        if (quotient % 2 == 0) {
            local_result += sin_res;
        }
    }
    result[idx] = local_result;
}
