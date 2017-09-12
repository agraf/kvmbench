CC=gcc
CFLAGS=-Wall -O3
TARGETS=invtsc l3cache numa ht
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M),x86_64)
    TARGETS += cpuid
endif

all: $(TARGETS)

cpuid_csum_avx2.o: cpuid_csum.c
	$(CC) -c -o $@ $< $(CFLAGS) -mavx -mavx2 -std=gnu99

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

invtsc: invtsc.o tsc.o tsc.h
	$(CC) -o invtsc invtsc.o tsc.o

l3cache: l3cache.o
	$(CC) -o l3cache l3cache.o -lpthread

numa: numa.o
	$(CC) -o numa numa.o -lnuma

ht: ht.o
	$(CC) -o ht ht.o -lpthread

cpuid: cpuid.o cpuid_csum.o cpuid_csum_avx2.o tsc.o tsc.h
	$(CC) -o cpuid cpuid.o cpuid_csum.o cpuid_csum_avx2.o tsc.o

clean:
	rm -f *.o $(TARGETS)
