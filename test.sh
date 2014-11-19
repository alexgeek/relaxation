#!/bin/bash
START=${1:-1}
INC=${2:-1}
FINISH=${3:-8}
N=${4:-2050}
echo "Testing with threading in range $START-$FINISH in increments of $INC for $N by $N array."
OUTPUT="time"
TIMES=(real user sys)
for t in ${TIMES[*]}; do
  O="$OUTPUT.$t.log"
  >$O
  for i in `seq $START $INC $FINISH`; do
  # I refuse to give it a linebreak
  (time ./relax -n $N -t $i)  2>&1 > /dev/null | grep $t | awk '{print $2}'|sed "s/^[0-9]\+m//" | xargs printf "$i\t%s\n" >> $O
  done
done
for t in ${TIMES[*]}; do
  cat "$OUTPUT.$t.log"
done

#for t in ${TIMES[*]}; do
gnuplot <<- EOF
    set xlabel "Threads"
    set ylabel "Time (s)"
    #set term dumb
    set term png
    set output "${OUTPUT}.png"
    plot [$START:$FINISH] "${OUTPUT}.${TIMES[0]}.log" title "Speedup (${TIMES[0]})" with lines, \
    "${OUTPUT}.${TIMES[1]}.log" title "Speedup (${TIMES[1]})" with lines, \
    "${OUTPUT}.${TIMES[2]}.log" title "Speedup (${TIMES[2]})" with lines
EOF
#done
