#!/bin/bash

#SBATCH -J ejercicio7 # Job name
#SBATCH -t 01:00:00 # Run time (hh:mm:ss)
# Launch MPI-based executable

n=20  # Number of iterations
sizes=(100 1000 10000 100000 1000000)
output_table="ejercicio7-time-table-${1}-cores.dat"

# Clear the files 
> "$output_table"

for size in "${sizes[@]}"; do

    printf "%s\t" "$size" >> "$output_table"
    echo $size
    sum_1=0
    sum_2=0
    sum_3=0

    for i in $(seq 1 $n); do
        value_1=$(prun ./ejercicio7-1 $size | grep "Time:" | awk '{print $4}' | tr -d ',') 
        sum_1=$(awk -v a="$sum_1" -v b="$value_1" 'BEGIN {print a + b}')
        value_2=$(prun ./ejercicio7-2 $size | grep "Time:" | awk '{print $4}' | tr -d ',') 
        sum_2=$(awk -v a="$sum_2" -v b="$value_2" 'BEGIN {print a + b}')
        value_3=$(prun ./ejercicio7-3 $size | grep "Time:" | awk '{print $4}' | tr -d ',') 
        sum_3=$(awk -v a="$sum_3" -v b="$value_3" 'BEGIN {print a + b}') 
    done
    time_ej1=$(awk -v a="$sum_1" -v b="$n" 'BEGIN {print a / b}')
    time_ej2=$(awk -v a="$sum_2" -v b="$n" 'BEGIN {print a / b}')
    time_ej3=$(awk -v a="$sum_3" -v b="$n" 'BEGIN {print a / b}')
    
    printf "%s\t" "$time_ej1" >> "$output_table"
    printf "%s\t" "$time_ej2" >> "$output_table"
    printf "%s\t" "$time_ej3" >> "$output_table"

    echo >> "$output_table"
done