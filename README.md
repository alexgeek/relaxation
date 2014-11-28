parallel-relaxation
===================

Relaxation technique in parallel using POSIX threads, barriers and condition variables.

# Compiling

First make sure you have the dependencies:

```
sudo apt-get install gcc gnuplot
```

Then to compile and run:

```
make
./relax
``` 

# Test Scripts

The test scripts record execution time of the program for various parameters. 
`time.sh` varies the number of threads used and produces a graph of the execution times.
Running `speedup.sh` after `time.sh` produces a speedup a graph:

![speedup graph](http://i.imgur.com/qOw07Fv.png)
