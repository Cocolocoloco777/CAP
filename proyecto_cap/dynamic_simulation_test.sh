#!/bin/bash

#SBATCH -J dynamic-simulation-scheduler # Job name
#SBATCH -t 00:00:10 # Run time (hh:mm:ss)
# Launch MPI-based executable

n_tasks=100
type_tasks="random"
scheduler="chunk"

prun ./dynamic_simulation.out $n_tasks $type_tasks $scheduler
