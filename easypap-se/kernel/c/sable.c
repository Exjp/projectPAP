#include "easypap.h"

#include <stdbool.h>

static long unsigned int *TABLE = NULL;

static volatile int changement;

static unsigned long int max_grains;

#define table(i, j) TABLE[(i)*DIM + (j)]

#define RGB(r, v, b) (((r) << 24 | (v) << 16 | (b) << 8) | 255)

void sable_init ()
{
  TABLE = calloc (DIM * DIM, sizeof (long unsigned int));
}

void sable_finalize ()
{
  free (TABLE);
}

///////////////////////////// Production d'une image
void sable_refresh_img ()
{
  unsigned long int max = 0;
  for (int i = 1; i < DIM - 1; i++)
    for (int j = 1; j < DIM - 1; j++) {
      int g = table (i, j);
      int r, v, b;
      r = v = b = 0;
      if (g == 1)
        v = 255;
      else if (g == 2)
        b = 255;
      else if (g == 3)
        r = 255;
      else if (g == 4)
        r = v = b = 255;
      else if (g > 4)
        r = b = 255 - (240 * ((double)g) / (double)max_grains);

      cur_img (i, j) = RGB (r, v, b);
      if (g > max)
        max = g;
    }
  max_grains = max;
}

///////////////////////////// Configurations initiales

static void sable_draw_4partout (void);

void sable_draw (char *param)
{
  // Call function ${kernel}_draw_${param}, or default function (second
  // parameter) if symbol not found
  hooks_draw_helper (param, sable_draw_4partout);
}

void sable_draw_4partout (void)
{
  max_grains = 8;
  for (int i = 1; i < DIM - 1; i++)
    for (int j = 1; j < DIM - 1; j++)
      table (i, j) = 4;
}

void sable_draw_DIM (void)
{
  max_grains = DIM;
  for (int i = DIM / 4; i < DIM - 1; i += DIM / 4)
    for (int j = DIM / 4; j < DIM - 1; j += DIM / 4)
      table (i, j) = i * j / 4;
}

void sable_draw_alea (void)
{
  max_grains = 5000;
  for (int i = 0; i<DIM>> 3; i++) {
    table (1 + random () % (DIM - 2), 1 + random () % (DIM - 2)) =
        1000 + (random () % (4000));
  }
}

// ///////////////////////////// Version séquentielle simple (seq)

static inline void compute_new_state (int y, int x)
{
  if (table (y, x) >= 4) {
    unsigned long int div4 = table (y, x) / 4;
    table (y, x - 1) += div4;
    table (y, x + 1) += div4;
    table (y - 1, x) += div4;
    table (y + 1, x) += div4;
    table (y, x) %= 4;
    changement = 1;
  }
}

static void do_tile (int x, int y, int width, int height, int who)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", x, x + width - 1, y,
               y + height - 1);

  monitoring_start_tile (who);

  for (int i = y; i < y + height; i++)
    for (int j = x; j < x + width; j++) {
      compute_new_state (i, j);
    }
  monitoring_end_tile (x, y, width, height, who);
}

// Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
unsigned sable_compute_seq (unsigned nb_iter)
{

  for (unsigned it = 1; it <= nb_iter; it++) {
    changement = 0;
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    do_tile (1, 1, DIM - 2, DIM - 2, 0);
    if (changement == 0)  
      return it;
  }
  return 0;
}

// /////////////////////////////// Version séquentielle simple mais optimisé (seq_opt)

static inline void compute_new_state_tiled (int y, int x)
{
  if (table (y, x) >= 4) {
    unsigned long int div4 = table (y, x) / 4;
    table (y, x - 1) += div4;
    table (y, x + 1) += div4;
    table (y - 1, x) += div4;
    table (y + 1, x) += div4;
    table (y, x) %= 4;
    changement = 1;
  }
}

static void do_tile_tiled (int x, int y, int width, int height, int who)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", x, x + width - 1, y,
               y + height - 1);

  monitoring_start_tile (who);

  for (int i = y; i < y + height; i++)
    for (int j = x; j < x + width; j++) {
      compute_new_state (i, j);
    }
  #pragma omp barrier
  for (int i = y; i < y + height; i++)
    for (int j = x; j < x + width; j++) {
      compute_new_state (i, j);
    }
  
  monitoring_end_tile (x, y, width, height, who);
}

unsigned sable_compute_tiled (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {
    changement = 0;
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < DIM; y += TILE_SIZE)
      for (int x = 0; x < DIM; x += TILE_SIZE)
        do_tile_tiled (x + (x == 0), y + (y == 0),
                  TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                  TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                    omp_get_thread_num());
    if (changement == 0)
      return it;
  }

  return 0;
}

// static inline void compute_new_state_opt (int y, int x)
// {
//   if (table (y, x) >= 4) {
//     unsigned long int mod4 = table (y,x) % 4;
//     unsigned long int div4 = table (y, x) / 4;

//     table(y, x) = mod4;
//     table (y, x - 1) += div4;
//     table (y, x + 1) += div4;
//     table (y - 1, x) += div4;
//     table (y + 1, x) += div4;
//     table (y, x) %= 4;
//     changement = 1;
//   }
// }

// static void do_tile_opt (int x, int y, int width, int height, int who)
// {
//   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", x, x + width - 1, y,
//                y + height - 1);

//   monitoring_start_tile (who);

//   for (int i = y; i < y + height; i++)
//     for (int j = x; j < x + width; j++) {
//       compute_new_state_opt (i, j);
//     }
//   monitoring_end_tile (x, y, width, height, who);
// }


// unsigned sable_compute_seq (unsigned nb_iter)
// {

//   for (unsigned it = 1; it <= nb_iter; it++) {
//     changement = 0;
    
//     do_tile_opt (1, 1, DIM - 2, DIM - 2, 0);
//     if (changement == 0)
//       return it;
//   }
//   return 0;
// }

///////////////////////////// Version OpenMP avec opérations atomiques (omp_atomic)


// static inline void compute_new_state_omp (int y, int x)
// {
//   if (table (y, x) >= 4) {

//       unsigned long int div4 = table (y, x) / 4;
//       // #pragma omp atomic
//       table (y, x - 1) += div4;
//       // #pragma omp atomic
//       table (y, x + 1) += div4;
//       // #pragma omp atomic
//       table (y - 1, x) += div4;
//       // #pragma omp atomic
//       table (y + 1, x) += div4;
//       table (y, x) %= 4;
//       changement = 1;

//   }
// }

// static void do_tile_omp (int x, int y, int width, int height, int who)
// {
//   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", x, x + width - 1, y,
//                y + height - 1);

//   monitoring_start_tile (who);
//   #pragma omp for schedule(dynamic)
//   for (int i = y; i < y + height; i++)
//     for (int j = x; j < x + width; j++) {
//       if(i)
//         compute_new_state_omp (i, j);
//     }
//   monitoring_end_tile (x, y, width, height, who);
// }

// unsigned sable_compute_omp (unsigned nb_iter) 
// {
  
//   for (unsigned it = 1; it <= nb_iter; it++) {
//     changement = 0;
//     #pragma omp parallel
//     do_tile_omp (1, 1, DIM - 2, DIM - 2, omp_get_thread_num());
//     if (changement == 0)
//       return it;
//     #pragma omp barrier
  
//   }
//   return 0;
// }



// ///////////////////////////// Version séquentielle tuilée (tiled)

// static void compute_new_state_tiled (int x, int y)
// {
//   if (table (y, x) >= 4) {
//     unsigned long int div4 = table (y, x) / 4;
//       table (y, x - 1) += div4;
//       table (y, x + 1) += div4;
//       table (y - 1, x) += div4;
//       table (y + 1, x) += div4;
//       table (y, x) %= 4;
//       changement = 1;
//   }
  
// }

// static void compute_new_state_edge (int x, int y)
// {
  
//   if (table (y, x) >= 4) {
//     int i_d = (y > 0) ? y - 1 : y;
//     int i_f = (y < DIM - 1) ? y + 1 : y;
//     int j_d = (x > 0) ? x - 1 : x;
//     int j_f = (x < DIM - 1) ? x + 1 : x;
//     unsigned long int div4 = table (y, x) / 4;
//       // #pragma omp atomic
//       table (y, j_d) += div4;
//       // #pragma omp atomic
//       table (y, j_f) += div4;
//       // #pragma omp atomic
//       table (i_d, x) += div4;
//       // #pragma omp atomic
//       table (i_f, x) += div4;
//       table (y, x) %= 4;
//       changement = 1;
//   }
  
// }

// static void do_tile_tiled (int x, int y, int width, int height, int who)
// {
//   monitoring_start_tile (who);
//   for (int i = y; i < y + height; i++)
//     for (int j = x; j < x + width; j++) {
//       compute_new_state_tiled(i, j);
//     }
//   monitoring_end_tile (x, y, width, height, who);
// }

// static void do_tile_edge (int x, int y, int width, int height, int who)
// {
//   monitoring_start_tile (who);
//   for (int i = y; i < y + height; i++)
//     for (int j = x; j < x + width; j++) {
//       compute_new_state_edge(i, j);
//     }
//   monitoring_end_tile (x, y, width, height, who);
// }


// unsigned sable_compute_tiled (unsigned nb_iter)
// {
//   for (unsigned it = 1; it <= nb_iter; it++) {
//     changement = 0;
//     #pragma omp parallel for collapse(2) schedule(dynamic)
//     for (int y = 0; y < DIM; y += TILE_SIZE)
//       for (int x = 0; x < DIM; x += TILE_SIZE)
//       // #pragma omp critical
//       if(x > 0 && y > 0 && x > DIM - TILE_SIZE && y > DIM- TILE_SIZE){
// 		  //pas bord
//         do_tile_tiled (x + (x == 0), y + (y == 0),
//                   TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
//                   TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
//                     omp_get_thread_num());
//       }
//       else{
//         //bord
//         do_tile_edge (x + (x == 0), y + (y == 0),
//                  TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
//                  TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
//                   omp_get_thread_num());
		  
//       }
//     if (changement == 0)
//       return it;
//     #pragma omp barrier
//   }

//   return 0;
// }

// static void compute_new_state_tiled (int x, int y, int width, int height)
// {
//   for (int i = y; i < y + height; i++)
//     for (int j = x; j < x + width; j++) {

//       int i_d = (i > 0) ? i - 1 : i;
//       int i_f = (i < DIM - 1) ? i + 1 : i;
//       int j_d = (j > 0) ? j - 1 : j;
//       int j_f = (j < DIM - 1) ? j + 1 : j;

//       if (table (i, j) >= 4) {
//         unsigned long int div4 = table (i, j) / 4;
//           table (i, j_d) += div4;
//           table (i, j_f) += div4;
//           table (i_d, j) += div4;
//           table (i_f, j) += div4;
//           table (i, j) %= 4;
//           changement = 1;
//       }
//     }
      
// }
  

// static void compute_new_state_tiled_inner (int x, int y, int width, int height)
// {
//   for (int i = y; i < y + height; i++)
//     for (int j = x; j < x + width; j++) {


//        if (table (i, j) >= 4) {
//         unsigned long int div4 = table (i, j) / 4;
//           table (i, j - 1) += div4;
//           table (i, j + 1) += div4;
//           table (i - 1, j) += div4;
//           table (i + 1, j) += div4;
//           table (i, j) %= 4;
//           changement = 1;
//       }
//     }
      
// }

// static void do_tile_tiled_inner (int x, int y, int width, int height, int who)
// {
//   monitoring_start_tile (who);
//   compute_new_state_tiled_inner(y, x, width, height);
//   monitoring_end_tile (x, y, width, height, who);
// }

// static void do_tile_tiled (int x, int y, int width, int height, int who)
// {
//   monitoring_start_tile (who);
//   compute_new_state_tiled(y, x, width, height);
//   monitoring_end_tile (x, y, width, height, who);
// }


// unsigned sable_compute_tiled (unsigned nb_iter)
// {
//   for (unsigned it = 1; it <= nb_iter; it++) {
//     changement = 0;
//     #pragma omp parallel for collapse(2) schedule(dynamic)
//     for (int y = 0; y < DIM; y += TILE_SIZE)
//       for (int x = 0; x < DIM; x += TILE_SIZE)
//       #pragma omp critical
//       if(x > 0 || y > 0 || x < DIM - TILE_SIZE || y < DIM- TILE_SIZE){
//         do_tile_tiled(x + (x == 0), y + (y == 0),
//                  TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
//                  TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
//                   omp_get_thread_num());
//       }
//       else{
//         do_tile_tiled (x + (x == 0), y + (y == 0),
//                   TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
//                   TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
//                     omp_get_thread_num());
//       }
//     if (changement == 0)
//       return it;
//   }

//   return 0;
// }
