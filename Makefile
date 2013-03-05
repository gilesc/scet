PREFIX=/usr/local
CC=g++
CXXFLAGS=-std=c++0x -fopenmp
LDFLAGS=-lgomp

all: scet

clean:
	rm -f src/*.o
	rm scet

install:
	cp bin/* $(PREFIX)/bin

.PHONY: all clean install

src/ahocorasick.o: src/ahocorasick.cc
	$(CC) $(CXXFLAGS) -c -o $@ $^

src/scet.o: src/iridescent.cc
	$(CC) $(CXXFLAGS) -c -o $@ $^

scet: src/scet.o src/ahocorasick.o
	$(CC) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
