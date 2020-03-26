
#include "easypap.h"

#include <omp.h>

///////////////////////////// Simple sequential version (seq)
// Suggested cmdline(s):
// ./run -l images/1024.png -k scrollup -v seq
//
unsigned scrollup_compute_seq (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {

    for (int i = 0; i < DIM; i++) {
      int src = (i < DIM - 1) ? i + 1 : 0;
      for (int j = 0; j < DIM; j++)
        next_img (i, j) = cur_img (src, j);
    }

    swap_images ();
  }

  return 0;
}

// Tile inner computation
static void do_tile_reg (int x, int y, int width, int height)
{
  for (int i = y; i < y + height; i++) {
    int src = (i < DIM - 1) ? i + 1 : 0;

    for (int j = x; j < x + width; j++)
      next_img (i, j) = cur_img (src, j);
  }
}

static void do_tile (int x, int y, int width, int height, int who)
{
  monitoring_start_tile (who);

  do_tile_reg (x, y, width, height);

  monitoring_end_tile (x, y, width, height, who);
}

///////////////////////////// Tiled sequential version (tiled)
// Suggested cmdline(s):
// ./run -l images/1024.png -k scrollup -v tiled
//
unsigned scrollup_compute_tiled (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {

    for (int y = 0; y < DIM; y += TILE_SIZE)
      for (int x = 0; x < DIM; x += TILE_SIZE)
        do_tile (x, y, TILE_SIZE, TILE_SIZE, 0 /* CPU id */);

    swap_images ();

  }

  return 0;
}
