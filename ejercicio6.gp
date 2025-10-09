set title "Test Ancho de banda"
set ylabel "Tiempo (ms)"
set xlabel "Tamano bloque (B)"
set key left top
set grid
set term png
set output 'ejercicio6-time.png'
set logscale x 2 
set terminal png size 1000,1000

plot    "mean_time_table.dat" using 1:2 with lines lw 2 title "Mismo nodo",\
        "mean_time_table.dat" using 1:3 with lines lw 2 title "Mismo rack",\
        "mean_time_table.dat" using 1:4 with lines lw 2 title "Distinto rack"

replot

set title "Test Ancho de banda"
set ylabel "Tiempo (ms)"
set xlabel "Tamano bloque (B)"
set key left top
set grid
set term png
set output 'ejercicio6-time-rack-only.png'
set logscale x 2 
set terminal png size 1000,1000

plot    "mean_time_table.dat" using 1:3 with lines lw 2 title "Mismo rack",\
        "mean_time_table.dat" using 1:4 with lines lw 2 title "Distinto rack"

replot

set title "Test Ancho de banda"
set ylabel "Tiempo (ms)"
set xlabel "Tamano bloque (B)"
set yrange [0:*]
set key left top
set grid
set term png
set output 'ejercicio6-time-truncated.png'
set logscale x 2 
set terminal png size 1000,1000

plot    "mean_time_table.dat" every ::7::20 using 1:2 with lines lw 2 title "Mismo nodo",\
        "mean_time_table.dat" every ::7::20 using 1:3 with lines lw 2 title "Mismo rack",\
        "mean_time_table.dat" every ::7::20 using 1:4 with lines lw 2 title "Distinto rack"

replot

set title "Test Ancho de banda"
set ylabel "Ancho de banda (B/s)"
set xlabel "Tamano bloque (B)"
set key left top
set grid
set term png
set output 'ejercicio6-bw.png'
set terminal png size 1000,1000

plot    "mean_bw_table.dat" using 1:2 with lines lw 2 title "Mismo nodo",\
        "mean_bw_table.dat" using 1:3 with lines lw 2 title "Mismo rack",\
        "mean_bw_table.dat" using 1:4 with lines lw 2 title "Distinto rack"

replot

set title "Test Ancho de banda"
set ylabel "Ancho de banda (B/s)"
set xlabel "Tamano bloque (B)"
set key left top
set grid
set term png
set output 'ejercicio6-bw-rack-only.png'
set terminal png size 1000,1000

plot    "mean_bw_table.dat" using 1:3 with lines lw 2 title "Mismo rack",\
        "mean_bw_table.dat" using 1:4 with lines lw 2 title "Distinto rack"

replot