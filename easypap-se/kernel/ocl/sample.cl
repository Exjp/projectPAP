#include "kernel/ocl/common.cl"

__kernel void sample_ocl (__global unsigned *out)
{
    int y = get_global_id (1);
    int x = get_global_id (0);
    unsigned couleur;

    couleur = 0xFFFFFFFF;
    if((get_group_id (0) + get_group_id (1)) % 2)
        out [y*DIM + x] = couleur;

}