#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    int rank, size, taille;
    char str[4];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    taille = (size - 1) * 2;
    int tranche = taille / (size - 1);


    if (rank == 0)
    {
    //     MPI_Send("hi",3,MPI_CHAR,1,0,MPI_COMM_WORLD);
        int in[taille], out[taille]; // init in
    //     printf("HEllo world from process %d of %d\n", rank, size);
        for(int i = 0; i < taille; i++){
            in[i] = 1;
        }
        MPI_Status etat;
        for(int k = 1; k < size; k++){
            printf("J'envoie la tranche de %d à %d et je suis rank %d\n",((k-1) * tranche),((k-1) * tranche + tranche),rank);
            MPI_Send(&in[(k-1) * tranche], tranche, MPI_INT, k,0,MPI_COMM_WORLD);
        }
        for(int k = 1; k < size; k++)
        {

            printf("Je recois la tranche %d et je suis rank %d\n",k,rank);
            MPI_Recv(&out[(k-1) * tranche], tranche, MPI_INT, k,0,MPI_COMM_WORLD, &etat);
        }
        for(int i = 0; i < taille; i++){
            printf("%d = %d |",i,out[i]);
        }
    } else { // esclave
        MPI_Status etat;
        MPI_Probe(0, 0, MPI_COMM_WORLD, &etat);
        MPI_Get_count(&etat, MPI_INT, &tranche);
        int in[tranche], out[tranche];
        // int* in = malloc(tranche * sizeof(int));
        //int* out = malloc(tranche * sizeof(int));
        MPI_Recv(&in, tranche, MPI_INT, 0,0,MPI_COMM_WORLD, &etat);
        // printf("in[0] = %d\n",out[0]);
        for(int j = 0; j < tranche; j++){
            out[j] = in[j]*rank;
        }
        printf("je suis l'esclave de rang %d\n",rank);
        MPI_Send(&out, tranche, MPI_INT, 0,0,MPI_COMM_WORLD);
    }
    // if(rank == 1)
    // {
    //     MPI_Recv(str,4,MPI_CHAR,0,0,MPI_COMM_WORLD, &etat);
    //     printf("%d recoit: %s\n",rank,str);
    // }



    MPI_Finalize();
    return 0;

}