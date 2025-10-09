#!/bin/bash

# Output file
table="latency_table.dat"


# Clear the files
> "$table"

# Define types and values
types=(sn sr dr)

# Loop over types

for type in "${types[@]}"; do
    file="ejercicio6-lat-${type}.stdout"

    # Read the values and do the mean
    if [[ -f "$file" ]]; then
        result=$(awk '$1 ~ /^[0-9]+$/ {
            time += $2
            bw += $3
            count++
        } END {
            if (count > 0) {
                printf "%.6f", time / count
            } else {
                printf "NA"
            }
        }' "$file")

        mean_time=$(echo "$result" | cut -d' ' -f1)
    else
        mean_time="NA"
    fi

    # Write the values in the tables
    printf "%s\t" "$mean_time" >> "$table"
done

