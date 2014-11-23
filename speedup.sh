#!/bin/bash

>"speedup.tmp"
S=$(head -1 time.real.log | awk '{print $1}')
ST=$(grep "^$S\s" "time.real.log" | awk '{sum+=$2} END { print sum/NR}')
gnuplot <<- EOF
    set xlabel "Threads"
    set ylabel "Speedup"
    set title "Speedup Results"
    set xrange [$START:$FINISH]
    #set term dumb
    set term png
    set output "speedup.png"
    plot "time.real.log" using 1:($ST/\$2) title "(real)" smooth unique lw 3
EOF
