#!/bin/bash

iterations=20
processes=4

for ((i=0;i<$iterations;i+=1))
	do
	echo test run: $i
	mpirun -np $processes ./test >> time_result.txt 
done
