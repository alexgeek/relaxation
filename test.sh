#!/bin/bash
START=${1:-1}
INC=${2:-1}
FINISH=${3:-8}
N=${4:-2050}
echo "Testing with threading in range $START-$FINISH in increments of $INC for $N by $N array."
OUTPUT="time.log"
>$OUTPUT
for i in `seq $START $INC $FINISH`; do
  for r in `seq 1 5`; do
    (time ./relax -n $N -t $i)  2>&1 > /dev/null | grep real | awk '{print $2}'|sed "s/^[0-9]\+m//" | xargs printf "$i\t%s\n" >> $OUTPUT
  done
done

gnuplot <<- EOF
    set xlabel "Threads"
    set ylabel "Time (s)"
    #set term dumb
    set term png
    set output "${OUTPUT}.png"
    plot [$START:$FINISH] "${OUTPUT}" title "Speedup (real)" with lines
EOF
