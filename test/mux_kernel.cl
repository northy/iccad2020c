__kernel void mux(__global const short *I0, __global const short *I1, __global const short *I2, __global const short *I3, __global const short *S0, __global const short *S1, __global short *O) {
    // Get the index of the current element to be processed
    int i = get_global_id(0);
    short m0,m1,m2,m3,ns0,ns1;
    ns0 = !S0[i];
    ns1 = !S1[i];
    m0 = I0[i]&&ns0&&ns1;
    m1 = I1[i]&&S0[i]&&ns1;
    m2 = I2[i]&&ns0&&S1[i];
    m3 = I3[i]&&S0[i]&&S1[i];

    // Do the operation
    O[i] = m0||m1||m2||m3;
}