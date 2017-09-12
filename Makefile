CC=gcc
CFLAGS=-Wall -O3
TARGETS=invtsc l3cache numa ht
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M),x86_64)
    TARGETS += cpuid
    CFLAGS += -mavx -mavx2 -std=gnu99
endif

all: $(TARGETS)

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

cpuid: cpuid.o tsc.o tsc.h
	$(CC) -o cpuid -mavx2 -mavx cpuid.o tsc.o

clean:
	rm -f *.o $(TARGETS)
