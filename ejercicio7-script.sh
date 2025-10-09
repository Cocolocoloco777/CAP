#!/bin/bash

sbatch -n 1 -o ejercicio7-1-cores.stdout ejercicio7-sbatch-script.sh 1
sbatch -n 2 -N 1 -o ejercicio7-2-cores.stdout ejercicio7-sbatch-script.sh 2
sbatch -n 4 -N 1 -o ejercicio7-4-cores.stdout ejercicio7-sbatch-script.sh 4
sbatch -n 8 -N 3 -o ejercicio7-8-cores.stdout ejercicio7-sbatch-script.sh 8
sbatch -n 16 -N 6 -o ejercicio7-16-cores.stdout ejercicio7-sbatch-script.sh 16
