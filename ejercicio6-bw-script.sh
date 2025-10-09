#!/bin/bash

for val in 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576; do
     
    sbatch -w compute-small-09-01 -n 2 -o ejercicio6-sn-$val.stdout ejercicio6-sbatch-script.sh $val
    sbatch -w compute-small-09-01,compute-big-09-01 -o ejercicio6-sr-$val.stdout ejercicio6-sbatch-script.sh $val
    sbatch -w compute-small-09-01,compute-big-08-01 -o ejercicio6-dr-$val.stdout ejercicio6-sbatch-script.sh $val
    
done
