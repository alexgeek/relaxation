#!/bin/bash

# Params {arg:-default}
START=${1:-1}
INC=${2:-1}
FINISH=${3:-16}
N=${4:-2050}

echo "Thread testing in range $START-$FINISH in increments of $INC for $N by $N array."

TIMES=(real user sys)
TMP="time.tmp"

# clear logs
>$TMP
for t in ${TIMES[*]}; do
  >"time.$t.log"
done

# loop varying thread count
for i in `seq $START $INC $FINISH`; do
  # 5 trials
  for r in `seq 1 5`; do
    # write time data into tmp
    (time ./relax -n $N -t $i)  2>&1 > /dev/null | grep -E "real|user|sys" > $TMP
    # write time data into separate logs with thread count
    for t in ${TIMES[*]}; do
      # grab, get second field, remove prefix, remove suffix, format, append
      grep $t $TMP | awk '{print $2}'|sed "s/^[0-9]\+m//" |sed "s/s$//" | xargs printf "$i\t%s\n" >> "time.$t.log"
    done
  done
done

gnuplot <<- EOF
    set xlabel "Threads"
    set ylabel "Time (s)"
    set title "Relaxing $N by $N Array"
    set xrange [$START:$FINISH]
    #set term dumb
    set term png
    set output "time.png"
    plot "time.${TIMES[0]}.log" title "Speedup (${TIMES[0]})" smooth unique, \
      "time.${TIMES[1]}.log" title "Speedup (${TIMES[1]})" smooth unique, \
      "time.${TIMES[2]}.log" title "Speedup (${TIMES[2]})" smooth unique,
EOF

rm -rf $TMP
