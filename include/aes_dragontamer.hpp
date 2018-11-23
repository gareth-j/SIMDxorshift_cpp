#ifndef AESDRAGONTAMER_H
#define AESDRAGONTAMER_H

#include <cstring>
#include <iostream>
#include <array>
#include <immintrin.h>

#include "randutils.hpp"

class aes_dragontamer_key
{
protected:

public:
	aes_dragontamer_key()
	{
		std::array<uint32_t, 4> seed_array;

	    randutils::auto_seed_128 seeder;
		
		seeder.generate(seed_array.begin(), seed_array.end());
		  
		uint64_t seed_part1 = seed_array[0];
		uint64_t seed_part2 = seed_array[1];
		uint64_t seed_part3 = seed_array[2];
		uint64_t seed_part4 = seed_array[3];

		// Create some 64-bit numbers
		uint64_t seed1 = seed_part1 << 32 | seed_part2;
		uint64_t seed2 = seed_part3 << 32 | seed_part4;

		// Create a __m128i with 16 8-bit numbers
		increment = _mm_set_epi8(0x2f, 0x2b, 0x29, 0x25, 0x1f, 0x1d, 0x17, 0x13, 
								0x11, 0x0D, 0x0B, 0x07, 0x05, 0x03, 0x02, 0x01);

		// Create __m128i with 2 64-bit numbers
		state = _mm_set_epi64x(seed1, seed2);
	}

	__m128i state;
	__m128i increment;
};


class aes_dragontamer
{
protected:
	inline __m256i aesdragontamer_rand(aes_dragontamer_key& key) 
	{
		key.state = _mm_add_epi64(key.state, key.increment);

		// Perform rounds of AES 
		__m128i penultimate = _mm_aesenc_si128(key.state, key.increment); 
		__m128i penultimate1 = _mm_aesenc_si128(penultimate, key.increment); 
		__m128i penultimate2 = _mm_aesdec_si128(penultimate, key.increment);

		// Create a m256i var from two m128i vars
		return _mm256_set_m128i(penultimate1,penultimate2);
	}

	void populateRandom_avx_aesdragontamer(uint32_t* rand_arr, const uint32_t size) 
	{
	    uint32_t i = 0;

		// Create a seed object for the rng
		aes_dragontamer_key my_key;

        // The number of variables we're operating on
        // Should be 8 here
	    const uint32_t block = sizeof(__m256i) / sizeof(uint32_t); // 8
	    
	    while (i + block <= size) 
	    {
	        _mm256_storeu_si256((__m256i *)(rand_arr + i), aesdragontamer_rand(my_key));

	        i += block;
	    }

	    if (i != size) 
	    {
	        uint32_t buffer[sizeof(__m256i) / sizeof(uint32_t)];

	        _mm256_storeu_si256((__m256i *)buffer, aesdragontamer_rand(my_key));

	        memcpy(rand_arr + i, buffer, sizeof(uint32_t) * (size - i));
	    }
	}

public:
	aes_dragontamer() {} // Do nothing here at the moment

	// For benchmarking
	void fill_array(uint32_t* rand_arr, uint32_t N_rands)
    {
    	return populateRandom_avx_aesdragontamer(rand_arr, N_rands);
    }

	inline __m256i get_rand(aes_dragontamer_key& key)
	{
		return aesdragontamer_rand(key);
	}

	inline __m256i operator()(aes_dragontamer_key& key)
	{
		return aesdragontamer_rand(key);
	}	
};

#endif