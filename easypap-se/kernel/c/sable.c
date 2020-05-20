#include "easypap.h"
#include <omp.h>
#include <stdbool.h>

static long unsigned int *TABLE = NULL;

// static int changement;

static int *tab_current_instable = NULL;


static unsigned long int max_grains;

#define tab_current_instable(i, j) tab_current_instable[(i)*GRAIN + (j)]

#define table(i, j) TABLE[(i)*DIM + (j)]

#define RGB(r, v, b) (((r) << 24 | (v) << 16 | (b) << 8) | 255)



void sable_init ()
{
  TABLE = calloc (DIM * DIM, sizeof (long unsigned int));
  tab_current_instable = malloc (GRAIN * GRAIN * sizeof (int));

    for (int i = 0; i < GRAIN; i++) {
      for (int j = 0; j < GRAIN; j++) {
        tab_current_instable(i, j) = 1;
      }
    }
}

void sable_finalize ()
{
  free (TABLE);
  free (tab_current_instable);
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


static int do_tile (int x, int y, int width, int height, int who)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", x, x + width - 1, y,
               y + height - 1);
  int changements = 0;
  // printf("DANS LE DOTILE, width = %d ; height = %d; proc =%d\n",width, height, who);
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



unsigned sable_compute_seq_opt (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {
    int changements = 0;
    
    monitoring_start_tile (0);
    for (int y = 0; y < DIM; y += TILE_SIZE)
			for (int x = 0; x < DIM; x += TILE_SIZE) {
        int tile_x = x/TILE_SIZE;
        int tile_y = y/TILE_SIZE;

				if (tab_current_instable(tile_y, tile_x)) {
					if (do_tile(
                x + (x == 0),
                y + (y == 0),
                TILE_SIZE - ((x + TILE_SIZE == DIM) + (x == 0)),
                TILE_SIZE - ((y + TILE_SIZE == DIM) + (y == 0)),
								0)) {
            changements = 1;

            if (tile_x > 0) tab_current_instable(tile_y, tile_x - 1) = 1;
            if (tile_y > 0) tab_current_instable(tile_y - 1, tile_x) = 1;
            if (tile_x < GRAIN - 1) tab_current_instable(tile_y, tile_x + 1) = 1;
            if (tile_x < GRAIN - 1) tab_current_instable(tile_y + 1, tile_x) = 1;
					} else {
						tab_current_instable(tile_y, tile_x) = 0;
					}
				}
			}
		
    monitoring_end_tile (1, 1, DIM - 2, DIM - 2, 0);

    if (changements == 0)  
      return it;
  }
  return 0;
}

///////////////////////////// Version OpenMP (omp)



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


////////////////////////////// Version MPI

// unsigned sable_compute_mpi (unsigned nb_iter){
//   static int rank, size;
//   MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
//   MPI_Comm_size(MPI_COMM_WORLD, &size);
//   MPI_Status status;
//   //j'envoie à chacun une partie de TABLE 
//   //je récup dans table les résultats de mes enfants
//   //chaque enfant envoie sa partie de TABLE, qui est de la taille de buf

//   long unsigned int**buf = malloc(DIM/size*sizeof(long unsigned int*));
//   for(int i = 0; i < DIM/size; i++)
//     buf[i]=malloc(DIM*sizeof(long unsigned int));

//   for (unsigned it = 1; it <= nb_iter; it++) {
//     int changement = 0;

//     MPI_Bcast(TABLE, DIM*DIM, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
//       if (rank == 0){
//         if (do_tile(1, 1, DIM, ( DIM/size), 0))
//           changement = 1;
//         MPI_Gather(MPI_IN_PLACE,DIM*(DIM/size),MPI_UNSIGNED_LONG,TABLE,DIM*(DIM/size),MPI_UNSIGNED_LONG,0,MPI_COMM_WORLD);
//       }
          
//       if (rank !=0) {
//         if(do_tile (1, rank*(DIM/size) - 1, DIM, (DIM/size), 0))
//           changement = 1;
//         for(int i=1;i<DIM/size;i++)
//           for(int j=1;j<DIM;j++)
//             buf[i][j] = table(i,j);
//         MPI_Gather(buf,DIM*(DIM/size),MPI_UNSIGNED_LONG,TABLE,DIM*(DIM/size),MPI_UNSIGNED_LONG,0,MPI_COMM_WORLD);
//       }

//       MPI_Barrier(MPI_COMM_WORLD);
//     if (changement == 0)
//       return it;
//   }
//   return 0;
// }

unsigned sable_compute_mpi (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {
    int changements = 0;
    int rank, size;
    MPI_Status etat;

    
    long unsigned int** buf = (long unsigned int**) malloc (DIM * sizeof (long unsigned int*));

    for (int i = 0; i < DIM; i++){
      buf[i] = (long unsigned int*) malloc(DIM * sizeof(long unsigned int));
      for(int j = 0; j < DIM; j++){
        buf[i][j]=table(i,j);
      }
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(rank % 2 == 0 ){
      if(rank != 0){
        MPI_Send(buf[rank * (DIM/size) + 1],DIM, MPI_UNSIGNED_LONG, rank - 1,0,MPI_COMM_WORLD);


        MPI_Recv(buf[(rank * (DIM/size))],DIM, MPI_UNSIGNED_LONG, (rank - 1),0,MPI_COMM_WORLD, &etat); 
        for(int i = 0; i < DIM; i++){
          table(rank * (DIM/size) + 1,i) = buf[(rank * (DIM/size))][i];
        }
      }
      MPI_Send(buf[rank + 1 * (DIM/size)],DIM, MPI_UNSIGNED_LONG, rank + 1,0,MPI_COMM_WORLD);
      MPI_Recv(buf[((rank + 1) * (DIM/size)) + 1],DIM, MPI_UNSIGNED_LONG, rank + 1 ,0,MPI_COMM_WORLD, &etat);
      for(int i = 0; i < DIM; i++){
          table(((rank + 1) * (DIM/size)) + 1,i) = buf[((rank + 1) * (DIM/size)) + 1][i];
        }
      
    }else {
        if(rank != size - 1){
          MPI_Recv(buf[((rank + 1)* (DIM/size)) + 1],DIM, MPI_UNSIGNED_LONG, (rank + 1),0,MPI_COMM_WORLD, &etat);

          for(int i = 0; i < DIM; i++){
            table(((rank + 1)* (DIM/size)) + 1,i) = buf[((rank + 1)* (DIM/size)) + 1][i];
          }
          MPI_Send(buf[(rank + 1 )* (DIM/size)],DIM, MPI_UNSIGNED_LONG, rank + 1,0,MPI_COMM_WORLD);
        }
        MPI_Recv(buf[rank * (DIM/size) - 1],DIM, MPI_UNSIGNED_LONG, (rank - 1),0,MPI_COMM_WORLD, &etat);

        for(int i = 0; i < DIM; i++){
            table(rank * (DIM/size) - 1,i) = buf[rank * (DIM/size) - 1][i];
          }
          MPI_Send(buf[rank * (DIM/size)],DIM, MPI_UNSIGNED_LONG, (rank - 1),0,MPI_COMM_WORLD);
    }
    if(rank == 0){
      if(do_tile (1, 1, DIM,(DIM/size), rank)) {
                changements = 1;
      }
    }
    else if(rank == size - 1){
      if(do_tile (1, rank*(DIM/size), DIM, DIM/size, rank)) {
                changements = 1;
      }
    }
    else{
      if(do_tile (1, rank*(DIM/size) - 1, DIM, (DIM/size), rank)) {
                changements = 1;
      }

    }
    
    if (changements == 0)
      return it;
  }
  return 0;

}


