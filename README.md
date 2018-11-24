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
Testing function : xor128
 2.28 cycles per operation
Testing function : xoro128_simd
 0.70 cycles per operation
Testing function : xoro128_simd_two
 0.62 cycles per operation
Testing function : xoro128_simd_four
 0.75 cycles per operation
Testing function : aes_dragontamer
 0.58 cycles per operation
```

### Requirements

A processor that supports AVX2. An AVX-512 version is available for processors supporting these extensions.

### References

Daniel Lemire - for the original C code - https://github.com/lemire/SIMDxorshift

Sebastiano Vigna - xorshift128+ implementation - http://xorshift.di.unimi.it/xorshift128plus.c

Melissa O'Neil - for the functions allowing seeding of the PRNGs - http://www.pcg-random.org/posts/developing-a-seed_seq-alternative.html



