#include "kernel/ocl/common.cl"


static unsigned scale_component (unsigned c, unsigned percentage)
{
  unsigned coul;

  coul = c * percentage / 100;
  if (coul > 255)
    coul = 255;

  return coul;
}

static unsigned scale_color (unsigned c, unsigned percentage)
{
  int4 v = color_to_int4 (c);

  v.s1 = scale_component (v.s1, percentage); // Blue
  v.s2 = scale_component (v.s2, percentage); // Green
  v.s3 = scale_component (v.s3, percentage); // Red

  return int4_to_color (v);
}

static unsigned brighten (unsigned c)
{
  for (int i = 0; i < 15; i++)
    c = scale_color (c, 101);

  return c;
}

static unsigned darken (unsigned c)
{
  for (int i = 0; i < 15; i++)
    c = scale_color (c, 99);

  return c;
}

__kernel void stripes_ocl (__global unsigned *in, __global unsigned *out)
{
  int y = get_global_id (1);
  int x = get_global_id (0);

  #ifdef PARAM
    unsigned mask = (1 << PARAM);
  #else
    unsigned mask = 1;
  #endif
  
  if (x & mask)
    out [y * DIM + x] = brighten (in [y * DIM + x]);
  else
    out [y * DIM + x] = darken (in [y * DIM + x]);
}
