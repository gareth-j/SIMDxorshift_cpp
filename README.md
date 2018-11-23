# SIMDxorshift_cpp

A C++ implementation of Daniel Lemire's SIMD (single instruction multiple data) xorshift128+ implementation.

## Getting Started

A version of Lemire's benchmarking function is included to allow testing of each implementation.

All you should need to do is

```
make
./simd_xor
```

And you should get something like

```
Testing function : xorshift128plus
 1.83 cycles per operation
Testing function : simd_xorshift128plus
 0.66 cycles per operation
Testing function : aes_dragontamer
 0.61 cycles per operation
```

### Requirements

A processor that supports AVX2. An AVX-512 version is available for processors supporting these extensions.

### References

Daniel Lemire - for the original C code - https://github.com/lemire/SIMDxorshift

Sebastiano Vigna - xorshift128+ implementation - http://xorshift.di.unimi.it/xorshift128plus.c

Melissa O'Neil - for the functions allowing seeding of the PRNGs - http://www.pcg-random.org/posts/developing-a-seed_seq-alternative.html



