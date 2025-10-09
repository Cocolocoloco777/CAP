#!/bin/bash

# Output files
time_table="mean_time_table.dat"
bw_table="mean_bw_table.dat"

# Clear the files 
> "$time_table"
> "$bw_table"

# Define types and values
types=(sn sr dr)
vals=(2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576)

# Loop over types and vals
for val in "${vals[@]}"; do

    # Insert block size
    printf "%s\t" "$val" >> "$time_table"
    printf "%s\t" "$val" >> "$bw_table"

    for type in "${types[@]}"; do
        file="ejercicio6-${type}-${val}.stdout"

        # Read the values and do the mean
        if [[ -f "$file" ]]; then
            result=$(awk '$1 ~ /^[0-9]+$/ {
                time += $2
                bw += $3
                count++
            } END {
                if (count > 0) {
                    printf "%.6f %.2f", time / count, bw / count
                } else {
                    printf "NA NA"
                }
            }' "$file")

            mean_time=$(echo "$result" | cut -d' ' -f1)
            mean_bw=$(echo "$result" | cut -d' ' -f2)
        else
            mean_time="NA"
            mean_bw="NA"
        fi

        # Write the values in the tables
        printf "%s\t" "$mean_time" >> "$time_table"
        printf "%s\t" "$mean_bw" >> "$bw_table"
    done

    echo >> "$time_table"
    echo >> "$bw_table"
done
