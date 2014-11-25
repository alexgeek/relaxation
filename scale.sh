#!/bin/bash

# Params {arg:-default}
START=${1:-10}
INC=${2:-100}
FINISH=${3:-1000}
T=${4:-8}
P=${5:-0.01}

echo "Scale testing on $T threads for $START-$FINISH in increments of $INC."

TIMES=(real user sys)
TMP="time.tmp"

# clear logs
>$TMP
for t in ${TIMES[*]}; do
  >"time.$t.log"
done

# loop varying n
for i in `seq $START $INC $FINISH`; do
  # 5 trials
  for r in `seq 1 5`; do
    # write time data into tmp
    (time ./relax -n $i -t $T -p $P)  2>&1 > /dev/null | grep -E "real|user|sys" > $TMP
    # write time data into separate logs with thread count
    for t in ${TIMES[*]}; do
      # grab, get second field, remove prefix, remove suffix, format, append
      grep $t $TMP | awk '{print $2}'|sed "s/^[0-9]\+m//" |sed "s/s$//" | xargs printf "$i\t%s\n" >> "time.$t.log"
    done
  done
done

gnuplot <<- EOF
    set xlabel "Dimension"
    set ylabel "Time (s)"
    set title "Scalability Testing ($T threads)"
    set xrange [$START:$FINISH]
    set term png
    set output "time.png"
    plot "time.${TIMES[0]}.log" title "(${TIMES[0]})" smooth unique lw 3, \
      "time.${TIMES[1]}.log" title "(${TIMES[1]})" smooth unique lw 2, \
      "time.${TIMES[2]}.log" title "(${TIMES[2]})" smooth unique lw 2
EOF

rm -rf $TMP
