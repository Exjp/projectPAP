#!/bin/bash

rm plots/data/perf_data.csv

OMP_NUM_THREADS=1 ./run -v seq -a 4partout -s 480 -g 8  -k sable -n
OMP_NUM_THREADS=1 ./run -v omp -a 4partout -s 480 -g 8  -k sable -n
# SIZE="1""2"

# OMP_NUM_THREADS=1 ./run -v seq -a 4partout -s 480 -g 8  -k sable -n
# for i in "4 in 8";
# do
# 	OMP_NUM_THREADS=i ./run -v omp -a 4partout -s 480 -g 8  -k sable -n
# 	OMP_NUM_THREADS=i ./run -v omp -a 4partout -s 480 -g 16  -k sable -n
# 	OMP_NUM_THREADS=i ./run -v omp -a 4partout -s 480 -g 32  -k sable -n 
# done

