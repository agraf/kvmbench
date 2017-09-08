#!/bin/sh

gcc invtsc.c -o invtsc -O3 -Wall
gcc l3cache.c -o l3cache -O3 -Wall -lpthread
gcc numa.c -o numa -O3 -Wall -lnuma
