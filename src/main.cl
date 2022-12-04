kernel void k(int a, int b, global int * ret) {
    ret[0] = a + b;
}

kernel void setidsя(global int * ret, const int N) {
    const size_t id = get_global_id(0);
    ret[0] = id;
    ret[1] = 42;
    if (id < N) {
        ret[id] = 42; zzz
    }
}