#include "easypap.h"

#include <stdbool.h>

static long unsigned int *TABLE = NULL;

// static int changement;

static int *tab_changement = NULL;

static unsigned long int max_grains;

#define tab_changement(i, j) tab_changement[(i)*DIM + (j)]

#define table(i, j) TABLE[(i)*DIM + (j)]

#define RGB(r, v, b) (((r) << 24 | (v) << 16 | (b) << 8) | 255)



void sable_init ()
{
  TABLE = calloc (DIM * DIM, sizeof (long unsigned int));
  tab_changement = calloc (DIM * DIM, sizeof (int));
}

void sable_finalize ()
{
  free (TABLE);
  free (tab_changement);
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

static inline int compute_new_state (int y, int x)
{
  if (table (y, x) >= 4) {
    unsigned long int div4 = table (y, x) / 4;
    table (y, x - 1) += div4;
    table (y, x + 1) += div4;
    table (y - 1, x) += div4;
    table (y + 1, x) += div4;
    table (y, x) %= 4;
    return 1;
  }
  return 0;
}

static inline int compute_new_state_opt(int y, int x)
{
  if (table (y, x) >= 4) {
    unsigned long int mod4 = table (y,x) % 4;
    unsigned long int div4 = table (y, x) / 4;

    table(y, x) = mod4;
    table (y, x - 1) += div4;
    table (y, x + 1) += div4;
    table (y - 1, x) += div4;
    table (y + 1, x) += div4;
    table (y, x) %= 4;
    return 1;
  }
  return 0;
}

static inline int compute_new_state_opt_tmp (int y, int x)
{
  if (table (y, x) >= 4 || tab_changement(y,x) != 0) {
    unsigned long int div4 = table (y, x) / 4;
    table (y, x - 1) += div4;
    tab_changement (y, x - 1) = 1;
    table (y, x + 1) += div4;
    tab_changement (y, x + 1) = 1;
    table (y - 1, x) += div4;
    tab_changement (y - 1, x) = 1;
    table (y + 1, x) += div4;
    tab_changement (y + 1, x) = 1;
    table (y, x) %= 4;
    tab_changement (x,y) = 1;
    return 1;
  }
  return 0;
}



static int do_tile (int x, int y, int width, int height, int who)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", x, x + width - 1, y,
               y + height - 1);
  int changements = 0;
  monitoring_start_tile (who);

  for (int i = y; i < y + height; i++)
    for (int j = x; j < x + width; j++) {
			if(compute_new_state (i, j)) {
                changements = 1;
            }
    }
  monitoring_end_tile (x, y, width, height, who);
  return changements;
}

// Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
unsigned sable_compute_seq (unsigned nb_iter)
{

  for (unsigned it = 1; it <= nb_iter; it++) {
    int changements = 0;
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
		if(do_tile (1, 1, DIM - 2, DIM - 2, 0)) {
                changements = 1;
            }
    if (changements == 0)  
      return it;
  }
  return 0;
}

// /////////////////////////////// Version séquentielle simple mais optimisé (seq_opt)




///////////////////////////// Version OpenMP (omp_atomic)



unsigned sable_compute_omp (unsigned nb_iter) 
{
  
  for (unsigned it = 1; it <= nb_iter; it++) {
    int changements = 0;

    #pragma omp parallel for schedule(runtime)
      for (int y = 1; y < DIM - 1; y = y + 3) {
          monitoring_start_tile (omp_get_thread_num());
        for (int x = 1; x < DIM - 1; x++) {
              if(compute_new_state (y, x)) {
                changements = 1;
            }
        }
          monitoring_end_tile (1, y, DIM, 1, omp_get_thread_num());
      }

      #pragma omp parallel for schedule(runtime)
      for (int y = 2; y < DIM - 1; y = y + 3) {
          monitoring_start_tile (omp_get_thread_num());
        for (int x = 1; x < DIM - 1; x++) {
              if(compute_new_state (y, x)) {
                changements = 1;
            }
        }
          monitoring_end_tile (1, y, DIM, 1, omp_get_thread_num());
      }

      #pragma omp parallel for schedule(runtime)
      for (int y = 3; y < DIM - 1; y = y + 3) {
          monitoring_start_tile (omp_get_thread_num());
        for (int x = 1; x < DIM - 1; x++) {
              if(compute_new_state (y, x)) {
                changements = 1;
            }
        }
          monitoring_end_tile (1, y, DIM, 1, omp_get_thread_num());
      }
    
    
    if (changements == 0)
      return it;
  
  
  }
  return 0;
}

//////////////////////////////// Version séquentielle tuilée (tiled)

unsigned sable_compute_tiled (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {
    int changements = 0;

    for (int y = 0; y < DIM; y += TILE_SIZE)
      for (int x = 0; x < DIM; x += TILE_SIZE)
        if( do_tile (x + (x == 0), y + (y == 0),
                      TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                      TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                      omp_get_thread_num())) {
                changements = 1;
				}				
    if (changements == 0)
      return it;
  }

  return 0;
}

//////////////////////////////// Version tuilée OpenMP Damier (tileddb)

unsigned sable_compute_tileddb (unsigned nb_inter)
{
  for(unsigned it = 1;it <= nb_inter; it++){
    int changements = 0;
    #pragma omp parallel for collapse(2) schedule(runtime)
    for (int y = 0; y < DIM; y += 2 * TILE_SIZE) 
        for (int x = 0; x < DIM; x += 2 * TILE_SIZE) 
          if( do_tile (x + (x == 0), y + (y == 0),
                      TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                      TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                      omp_get_thread_num())) {
                changements = 1;
            }
            
    #pragma omp parallel for collapse(2) schedule(runtime)
    for (int y = TILE_SIZE; y < DIM; y += 2 * TILE_SIZE)
        for (int x = 0; x < DIM; x += 2 * TILE_SIZE)
          if( do_tile (x + (x == 0), y + (y == 0),
                      TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                      TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                      omp_get_thread_num())) {
                changements = 1;
            }
            
    #pragma omp parallel for collapse(2) schedule(runtime)
    for (int y = 0; y < DIM; y += 2 * TILE_SIZE) 
        for (int x = TILE_SIZE; x < DIM; x += 2 * TILE_SIZE) 
          if( do_tile (x + (x == 0), y + (y == 0),
                      TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                      TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                      omp_get_thread_num())) {
                changements = 1;
            }

    #pragma omp parallel for collapse(2) schedule(runtime)
    for (int y = TILE_SIZE; y < DIM; y += 2 * TILE_SIZE) 
        for (int x = TILE_SIZE; x < DIM; x += 2 * TILE_SIZE)
          if( do_tile (x + (x == 0), y + (y == 0),
                      TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                      TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                      omp_get_thread_num())) {
                changements = 1;
            }
      
        if (changements == 0)
      return it;
  }

  return 0;
}



//////////////////////////////// Version tuilée OpenMP Damier bis (tiledsharedy) 


unsigned sable_compute_tiledsharedy (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {
    int changements = 0;
    int y = 0;
    #pragma omp parallel for shared(y) schedule(runtime)
    for (y = 0; y < DIM; y += TILE_SIZE)
      for (int x = (y % (TILE_SIZE * 2) == 0) ? 0:TILE_SIZE; x < DIM; x += TILE_SIZE* 2)
      {
        #pragma omp critical
        if(           (x == 0 && y == 0) || 
          (x == 0 && y % TILE_SIZE == 0) || 
          (x % TILE_SIZE == 0 && y == 0) || 
          (x % TILE_SIZE == 0 && y % TILE_SIZE == 0))
          {
          if( do_tile (x + (x == 0), y + (y == 0),
                      TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                      TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                      omp_get_thread_num())) {
                changements = 1;
            }
        }
        else{
          if( do_tile (x + (x == 0), y + (y == 0),
                      TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                      TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                      omp_get_thread_num())) {
                changements = 1;
            }
        }
      }

    #pragma omp parallel for shared(y) schedule(runtime)
    for (int y = 0; y < DIM; y += TILE_SIZE)
      for (int x = (y % (TILE_SIZE * 2) == 0) ? TILE_SIZE:0; x < DIM; x += TILE_SIZE  * 2)
      {
        #pragma omp critical
        if(           (x == 0 && y == 0) || 
          (x == 0 && y % TILE_SIZE == 0) || 
          (x % TILE_SIZE == 0 && y == 0) || 
          (x % TILE_SIZE == 0 && y % TILE_SIZE == 0))
          {
            if( do_tile (x + (x == 0), y + (y == 0),
                      TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                      TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                      omp_get_thread_num())) {
                changements = 1;
            }
        }
        else{
          if( do_tile (x + (x == 0), y + (y == 0),
                      TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                      TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
                      omp_get_thread_num())) {
                changements = 1;
            }
        }
      }

    if (changements == 0)
      return it;
  }

  return 0;
}

