CC = g++

CFLAGS= -O3 -mavx2 -march=native -Wall -Wextra -pedantic -Wshadow

SOURCES = main.cpp

EXECUTABLE = simd_xor

benchmark: $(SOURCES)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(SOURCES)

clean:
	rm -f simd_xor



