#!/bin/bash
START=1
FINISH=10
OUTPUT="time"
TIMES=(real user sys)
for t in ${TIMES[*]}; do
  O="$OUTPUT.$t.log"
  >$O
  for i in `seq $START $FINISH`; do
  # I refuse to give it a linebreak
  (time ./relax -n 10 -t $i)  2>&1 > /dev/null | grep $t | awk '{print $2}'|sed "s/^[0-9]\+m//" | xargs printf "$i\t%s\n" >> $O
  done
done
for t in ${TIMES[*]}; do
  cat time.log
done

for t in ${TIMES[*]}; do
gnuplot <<- EOF
    set xlabel "Threads"
    set ylabel "Time (ms)"
    set term dumb
    #set term png
    #set output "${OUTPUT}.${t}.png"
    plot "${OUTPUT}.${t}.log" title "Speedup Graph"
EOF
done
