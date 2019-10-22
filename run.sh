#!/bin/bash
make clean
export OMP_NUM_THREADS=4
make
mpirun -np 2 ./test
