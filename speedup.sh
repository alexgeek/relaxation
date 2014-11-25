#!/bin/bash

>"speedup.tmp"
S=$(head -1 time.real.log | awk '{print $1}')
ST=$(grep "^$S[^0-9]" 'time.real.log' | awk '{ sum += $2; n++ } END { if (n > 0) print sum / n; }')
echo "T(1) = $ST"
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
