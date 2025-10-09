#!/bin/bash

#SBATCH -J ejercicio6-bw # Job name
#SBATCH -t 00:02:00 # Run time (hh:mm:ss) - 1.5 hours
# Launch MPI-based executable


n=20  # Number of iterations

for i in $(seq 1 $n); do
  prun ./bw.out $1
done