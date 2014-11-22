#!/bin/bash

# Params {arg:-default}
START=${1:-1}
INC=${2:-1}
FINISH=${3:-8}
N=${4:-10}

echo "Testing with threading in range $START-$FINISH in increments of $INC for $N by $N array."

TIMES=(real user sys)
OUTPUT="time.log"
TMP="time.tmp"

# clear logs
>$OUTPUT
>$TMP
for t in ${TIMES[*]}; do
  >"time.$t.log"
done

for i in `seq $START $INC $FINISH`; do
  for r in `seq 1 5`; do
    (time ./relax -n $N -t $i)  2>&1 > /dev/null | grep -E "real|user|sys" > $TMP
    for t in ${TIMES[*]}; do
      grep $t $TMP | awk '{print $2}'|sed "s/^[0-9]\+m//" |sed "s/s$//" | xargs printf "$i\t%s\n" >> "time.$t.log"
    done
  done
done

gnuplot <<- EOF
    set xlabel "Threads"
    set ylabel "Time (s)"
    set xrange [$START:$FINISH]
    #set term dumb
    set term png
    set output "${OUTPUT}.png"
    plot "time.${TIMES[0]}.log" title "Speedup (${TIMES[0]})" smooth unique, \
      "time.${TIMES[1]}.log" title "Speedup (${TIMES[1]})" smooth unique, \
      "time.${TIMES[2]}.log" title "Speedup (${TIMES[2]})" smooth unique,
EOF

rm -rf $TMP
