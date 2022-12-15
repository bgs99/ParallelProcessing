kernel void k(int a, int b, global int * ret) {
    ret[0] = a + b;
}

kernel void setids(global int *ret, const int N) {
    const size_t element = get_global_id(0);
    const size_t group = get_group_id(0);
    if (element < N) {
        ret[element] = group;
    }
}