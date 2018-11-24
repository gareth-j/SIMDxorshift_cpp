#ifndef SIMD512XORSHIFT128PLUS_H
#define SIMD512XORSHIFT128PLUS_H

#include <cstring>
#include <iostream>
#include <array>
#include <immintrin.h>

#include "randutils.hpp"

// Creates two 512-bit seed variables for use by the PRNG
class simd_avx512_xorshift128plus_key 
{
protected:    
    // The non-intrinsics version of the RNG
    // Used by xorshift128plus_jump_onkeys
    static void xorshift128plus_onkeys(uint64_t* ps0, uint64_t* ps1) 
    {
        uint64_t s1 = *ps0;
        const uint64_t s0 = *ps1;
        *ps0 = s0;
        s1 ^= s1 << 23; // a
        *ps1 = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5); // b, c
    }

    // This is the jump function from the original code, same as 2^64 calls to next
    static void xorshift128plus_jump_onkeys(uint64_t in1, uint64_t in2, uint64_t * output1, uint64_t * output2) 
    {
        /* todo: streamline */
        static const uint64_t JUMP[] = { 0x8a5cd789635d2dff, 0x121fd2155c472f96 };
        uint64_t s0 = 0;
        uint64_t s1 = 0;
        for (unsigned int i = 0; i < sizeof(JUMP) / sizeof(*JUMP); i++)
        {
            for (int b = 0; b < 64; b++) 
            {
                if (JUMP[i] & 1ULL << b) 
                {
                    s0 ^= in1;
                    s1 ^= in2;
                }
                xorshift128plus_onkeys(&in1, &in2);
            }
        }

        output1[0] = s0;
        output2[0] = s1;
    };

public:
    simd_avx512_xorshift128plus_key()
    {
        // Do the seeding here
    	std::array<uint64_t, 4> seed_array;
        randutils::auto_seed_128 seeder;
        seeder.generate(seed_array.begin(), seed_array.end());

		uint64_t S0[8];
		uint64_t S1[8];

		uint64_t seed_part1 = seed_array[0];
		uint64_t seed_part2 = seed_array[1];
		uint64_t seed_part3 = seed_array[2];
		uint64_t seed_part4 = seed_array[3];

		// Create some 64-bit numbers
		uint64_t seed1 = seed_part1 << 32 | seed_part2;
		uint64_t seed2 = seed_part3 << 32 | seed_part4;
		
		S0[0] = seed1;
		S1[0] = seed2;

	    // todo: fix this so that the init is correct - Fixed

		// GJ - fixed for full array initialization
		xorshift128plus_jump_onkeys(*S0, *S1, S0 + 1, S1 + 1);
		xorshift128plus_jump_onkeys(*(S0 + 1), *(S1 + 1), S0 + 2, S1 + 2);
		xorshift128plus_jump_onkeys(*(S0 + 2), *(S1 + 2), S0 + 3, S1 + 3);
		xorshift128plus_jump_onkeys(*(S0 + 3), *(S1 + 3), S0 + 4, S1 + 4);
		xorshift128plus_jump_onkeys(*(S0 + 4), *(S1 + 4), S0 + 5, S1 + 5);
		xorshift128plus_jump_onkeys(*(S0 + 5), *(S1 + 5), S0 + 6, S1 + 6);
		xorshift128plus_jump_onkeys(*(S0 + 6), *(S1 + 6), S0 + 7, S1 + 7);

		part1 = _mm512_loadu_si512((const __m512i *) S0);
		part2 = _mm512_loadu_si512((const __m512i *) S1);             
       
    }

    __m512i part1;
    __m512i part2;
};


class simd_avx512_xorshift128plus
{
protected:
    __m512i simd_avx512_xorshift128plus_rand(simd_avx512_xorshift128plus_key& key) 
    {
		__m512i s1 = key.part1;

		const __m512i s0 = key.part2;

		key.part1 = key.part2;

		s1 = _mm512_xor_si512(key.part2, _mm512_slli_epi64(key.part2, 23));

		key.part2 = _mm512_xor_si512(_mm512_xor_si512(_mm512_xor_si512(s1, s0),	_mm512_srli_epi64(s1, 18)), _mm512_srli_epi64(s0, 5));

		return _mm512_add_epi64(key.part2, s0);
	}

    void populateRandom_avx512_xorshift128plus(uint32_t* rand_arr, const uint32_t size)
    {
        uint32_t i = 0;

        // Automatically seeded 512-bit state variables
        simd_avx512_xorshift128plus_key mykey;

		// This should be 16
		const uint32_t block = sizeof(__m512i) / sizeof(uint32_t);

        while (i + block <= size) 
        {
            // Fill the array with random numbers
            // Store the output of the RNG into an unaligned memory location
            // Where simd_avx512_xorshift128plus_rand returns a __m256i
            _mm512_storeu_si512((__m512i *)(rand_arr + i), simd_avx512_xorshift128plus_rand(my_key1));

            i += block;
        }

        // If the array isn't full because of a block size mis-match fill the rest of the elements
		if (i != size) 
		{
			uint32_t buffer[sizeof(__m512i) / sizeof(uint32_t)];

			_mm512_storeu_si512((__m512i *)buffer, simd_avx512_xorshift128plus_rand(my_key1));

			memcpy(rand_arr + i, buffer, sizeof(uint32_t) * (size - i));
		}
    }	    


	void populateRandom_avx512_xorshift128plus_two(uint32_t* rand_arr, const uint32_t size) 
	{
		uint32_t i = 0;

		simd_avx512_xorshift128plus_key my_key1;
		simd_avx512_xorshift128plus_key my_key2;

		// This should be 16
		const uint32_t block = sizeof(__m512i) / sizeof(uint32_t);

		while (i + 2 * block <= size) 
		{
			_mm512_storeu_si512((__m512i *)(rand_arr + i), simd_avx512_xorshift128plus_rand(my_key1));			
			_mm512_storeu_si512((__m512i *)(rand_arr + i + block), simd_avx512_xorshift128plus_rand(my_key2));
			
			i += 2 * block;
		}
		while (i + block <= size) 
		{
			_mm512_storeu_si512((__m512i *)(rand_arr + i), simd_avx512_xorshift128plus_rand(my_key1));

			i += block;
		}
		if (i != size) 
		{
			uint32_t buffer[sizeof(__m512i) / sizeof(uint32_t)];

			_mm512_storeu_si512((__m512i *)buffer, simd_avx512_xorshift128plus_rand(my_key1));

			memcpy(rand_arr + i, buffer, sizeof(uint32_t) * (size - i));
		}
	}

	void populateRandom_avx512_xorshift128plus_four(uint32_t* rand_arr, const uint32_t size) 
	{

		uint32_t i = 0;

		// Key objects self-seed
		simd_avx512_xorshift128plus_key my_key1;
		simd_avx512_xorshift128plus_key my_key2;
		simd_avx512_xorshift128plus_key my_key3;
		simd_avx512_xorshift128plus_key my_key4;

		const uint32_t block = sizeof(__m512i) / sizeof(uint32_t); // 16
		while (i + 4 * block <= size) 
		{
			_mm512_storeu_si512((__m512i *)(rand_arr + i), simd_avx512_xorshift128plus_rand(my_key1));
			_mm512_storeu_si512((__m512i *)(rand_arr + i + block), simd_avx512_xorshift128plus_rand(my_key2));
			_mm512_storeu_si512((__m512i *)(rand_arr + i + 2 * block), simd_avx512_xorshift128plus_rand(my_key3));
			_mm512_storeu_si512((__m512i *)(rand_arr + i + 3 * block), simd_avx512_xorshift128plus_rand(my_key4));

			i += 4 * block;
		}
		while (i + 2 * block <= size) 
		{
			_mm512_storeu_si512((__m512i *)(rand_arr + i), simd_avx512_xorshift128plus_rand(my_key1));
			_mm512_storeu_si512((__m512i *)(rand_arr + i + block), simd_avx512_xorshift128plus_rand(my_key2));

			i += 2 * block;
		}

		while (i + block <= size) 
		{
			_mm512_storeu_si512((__m512i *)(rand_arr + i), simd_avx512_xorshift128plus_rand(my_key1));
			i += block;
		}

		if (i != size) 
		{
			uint32_t buffer[sizeof(__m512i) / sizeof(uint32_t)];

			_mm512_storeu_si512((__m512i *)buffer, simd_avx512_xorshift128plus_rand(my_key1));

			std::memcpy(rand_arr + i, buffer, sizeof(uint32_t) * (size - i));
		}
	}


public:
    // Do nothing at the moment
    simd_avx512_xorshift128plus(){}
    
    void fill_array(uint32_t* rand_arr, uint32_t N_rands)
    {
    	return populateRandom_avx512_xorshift128plus(rand_arr, N_rands);
    }

    void fill_array_two(uint32_t* rand_arr, uint32_t N_rands)
    {
    	return populateRandom_avx512_xorshift128plus_two(rand_arr, N_rands);
    }

    void fill_array_four(uint32_t* rand_arr, uint32_t N_rands)
    {
    	return populateRandom_avx512_xorshift128plus_four(rand_arr, N_rands);
    }

    __m512i get_rand(simd_avx512_xorshift128plus_key& key)
    {
    	return simd_avx512_xorshift128plus_rand(key);
    }

    __m512i operator()(simd_avx512_xorshift128plus_key& key)
    {
    	return simd_avx512_xorshift128plus_rand(key);
    }
};


#endif