#!/bin/sh

PROG=./run

THREADS=12
PROCS=4

OMP_NUM_THREADS=$THREADS ./run -s 1024 -k lifec -v omp_task -m -g 32 -r 4

OMP_NUM_THREADS=3 ./run -s 1024 -k life -v mpi_omp -mpi "-np $PROCS" -m -g 32 -r 4 -d M
