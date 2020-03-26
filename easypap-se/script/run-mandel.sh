#!/bin/sh

PROG=./run

THREADS=12

$PROG -s 1024 -k mandel -v seq

$PROG -s 1024 -k mandel -v vec

OMP_NUM_THREADS=$THREADS $PROG -s 1024 -k mandel -v thread_block -m

OMP_NUM_THREADS=$THREADS $PROG -s 1024 -k mandel -v thread_dyn_tiled -g 32 -m

DEVICE=1 $PROG -s 1024 -k mandel -o

