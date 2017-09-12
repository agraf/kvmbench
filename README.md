# kvmbench

A small collection of tools to benchmark KVM guests in comparison with their hosts.

## Building

```
$ make -j
```

## invtsc

A small benchmark that compares performance get gettimeofday() with native
rdtsc() performance. The rationale is that applications are only going to use
native rdtsc() if they see the invtsc cpuid flag.

It shows the number of timer reads it was able to perform within 1 second.

### Example output

```
$ ./invtsc
Timer loops finished with gettimeofday(): 42818657
Timer loops finished with optimized gettimeofday(): 43119668
Timer loops finished with TSC: 130291178
```

## l3cache

WIP (not working correctly)

### Example output

```
$ ./l3cache
Number of sleeps: 257917
```

## numa

This test sends a lot of data through pipes between 2 processes each. It allows
for different tunings via command line parameters

It shows the amount of MB/s it was able to transfer through the pipes.

### Command line arguments

```
$ ./numa -h
Syntax: ./numa

  -u    Misalign NUMA nodes for reader/writer
  -N    Disable NUMA awareness
  -0    Always use NUMA node 0
```

### Example output

```
$ ./numa
Benchmarking message passing on 2 NUMA nodes ...
  NUMA aware:        yes
  NUMA aligned:      yes
  NUMA nodes used:   0-1

Total MB/s read: 37520
$ ./numa -u
Benchmarking message passing on 2 NUMA nodes ...
  NUMA aware:        yes
  NUMA aligned:      no
  NUMA nodes used:   0-1

Total MB/s read: 18500
```

## ht

With this benchmark you can see how running different workloads on hyperthreads
is superior to running the same workload across them. So exposing the HT
topology can be important, depending on how smart the workload inside the guest
is.

It shows the number of operations it was able to execute within 1 second.

### Command line arguments

```
$ ./ht -h
Syntax: ./ht

  -c    Pin the threads to collide
  -P    Don't pin the threads
  -s    Run store test on all threads
  -m    Run mul test on all threads
```

### Example output

```
$ ./ht
Running  MUL  test on CPUs 0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 42 44 46 48 50 52 54 56 58 60 62 64 66 68 70 72 74 76 78 80 82 84 86 88 90 92 94
Running STORE test on CPUs 1 3 5 7 9 11 13 15 17 19 21 23 25 27 29 31 33 35 37 39 41 43 45 47 49 51 53 55 57 59 61 63 65 67 69 71 73 75 77 79 81 83 85 87 89 91 93 95
Total MMuls: 54031
Total MStore: 151489
$ ./ht -P
Running STORE and MUL tests without pinning ...
Total MMuls: 53996
Total MStore: 119016
```

## cpuid

This benchmark does a checksum over 2MB of data, once with a generic compiler
optimized version, once with a compiler optimized avx2 version and once with
a hand tuned avx2 implementation.

It shows the number of checksums it was able to create within 1 second.

### Example output

```
$ ./cpuid
Generic loops: 4433
AVX2 loops: 6219
AVX2 (optimized) loops: 9317
```
