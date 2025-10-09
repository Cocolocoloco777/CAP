#!/bin/bash

#SBATCH -J ejercicio6-lat # Job name
#SBATCH -t 00:02:00 # Run time (hh:mm:ss)
# Launch MPI-based executable

n=100  # Number of iterations

for i in $(seq 1 $n); do
  prun ./latencia_ida_vuelta.out
done