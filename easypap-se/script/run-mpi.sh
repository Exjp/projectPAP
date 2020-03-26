#!/bin/sh

PROG=./run

PROC=4
THREADS=2

# Normal mode: only the Master window is displayed
OMP_SCHEDULE=dynamic OMP_NUM_THREADS=$THREADS ./run -k spin -v mpi_omp -mpi "-np $PROC" -m -g 16

# Debug mode: display all windows!
OMP_SCHEDULE=dynamic OMP_NUM_THREADS=$THREADS ./run -k spin -v mpi_omp -mpi "-np $PROC" -m -g 16 -d M


