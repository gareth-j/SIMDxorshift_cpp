#ifndef SIMDXORSHIFT128PLUS_H
#define SIMDXORSHIFT128PLUS_H

#include <cstring>
#include <iostream>
#include <array>
#include <immintrin.h>

#include "randutils.hpp"

// Creates two 256-bit seed variables for use by the PRNG
class simd_xorshift128plus_key 
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
    simd_xorshift128plus_key()
    {
        // Do the seeding here
    	std::array<uint64_t, 4> seed_array;
        randutils::auto_seed_128 seeder;
        seeder.generate(seed_array.begin(), seed_array.end());

        uint64_t S0[4];
        uint64_t S1[4];

        uint64_t seed_part1 = seed_array[0];
		uint64_t seed_part2 = seed_array[1];
		uint64_t seed_part3 = seed_array[2];
		uint64_t seed_part4 = seed_array[3];

		// Create some 64-bit numbers
		uint64_t seed1 = seed_part1 << 32 | seed_part2;
		uint64_t seed2 = seed_part3 << 32 | seed_part4;
                
        S0[0] = seed1;
        S1[0] = seed2;
        
        xorshift128plus_jump_onkeys(*S0, *S1, S0 + 1, S1 + 1);
        xorshift128plus_jump_onkeys(*(S0 + 1), *(S1 + 1), S0 + 2, S1 + 2);
        xorshift128plus_jump_onkeys(*(S0 + 2), *(S1 + 2), S0 + 3, S1 + 3);

        part1 = _mm256_loadu_si256((const __m256i *) S0);
        part2 = _mm256_loadu_si256((const __m256i *) S1);
    }

    __m256i part1;
    __m256i part2;
};


// A C++ implementation of Lemire's SIMD xor RNG
class simd_xorshift128plus
{
protected:
	// Return a 256-bit random "number"
    __m256i simd_xorshift128plus_rand(simd_xorshift128plus_key& key)
    {
        __m256i s1 = key.part1;

        const __m256i s0 = key.part2;

        key.part1 = key.part2;

        s1 = _mm256_xor_si256(key.part2, _mm256_slli_epi64(key.part2, 23));

        key.part2 = _mm256_xor_si256(_mm256_xor_si256(_mm256_xor_si256(s1, s0),_mm256_srli_epi64(s1, 18)), _mm256_srli_epi64(s0, 5));

        return _mm256_add_epi64(key.part2, s0);
    }

    void populate_array_simd_xorshift128plus(uint32_t* rand_arr, const uint32_t size)
    {
        uint32_t i = 0;

        // Get two 256-bit seeds
        simd_xorshift128plus_key mykey;

        // The number of variables we're operating on - should be 8 here
        const uint32_t block = sizeof(__m256i) / sizeof(uint32_t); 

        while (i + block <= size) 
        {
            // Fill the array with random numbers
            // Store the output of the RNG into an unaligned memory location
            // Where simd_xorshift128plus_rand returns a __m256i
            _mm256_storeu_si256((__m256i *)(rand_arr + i), simd_xorshift128plus_rand(mykey));

            i += block;
        }

        // If the array isn't full because of a block size mis-match fill the rest of the elements
        if (i != size) 
        {
            // Create a buffer for the number of vars we can operate on at once
            uint32_t buffer[sizeof(__m256i) / sizeof(uint32_t)];

            // Moves values returned by the axv_xorshift fn into the buffer
            _mm256_storeu_si256((__m256i *)buffer, simd_xorshift128plus_rand(mykey));

            // Copy the buffer to the array
            std::memcpy(rand_arr + i, buffer, sizeof(uint32_t) * (size - i));
        }
    }

    // Uses two generators (states really) to fill an array
    void populate_array_simd_xorshift128plus_two(uint32_t* rand_arr, const uint32_t size) 
    {
        uint32_t i = 0;

        // Two seed objects
        simd_xorshift128plus_key my_key1;   
        simd_xorshift128plus_key my_key2;

        const uint32_t block = sizeof(__m256i) / sizeof(uint32_t);
        
        while (i + 2 * block <= size) 
        {
            _mm256_storeu_si256((__m256i *)(rand_arr + i), simd_xorshift128plus_rand(my_key1));
            _mm256_storeu_si256((__m256i *)(rand_arr + i + block),simd_xorshift128plus_rand(my_key2));
            i += 2 * block;
        }

        while (i + block <= size) 
        {
            _mm256_storeu_si256((__m256i *)(rand_arr + i), simd_xorshift128plus_rand(my_key1));
            i += block;
        }

        // This just make sure the array is full
        if (i != size) 
        {
            uint32_t buffer[sizeof(__m256i) / sizeof(uint32_t)];

            _mm256_storeu_si256((__m256i *)buffer, simd_xorshift128plus_rand(my_key1));

            std::memcpy(rand_arr + i, buffer, sizeof(uint32_t) * (size - i));
        }
    }

    // Uses four generators (states really) to fill an array
	void populateRandom_avx_xorshift128plus_four(uint32_t *rand_arr, uint32_t size) 
	{
		uint32_t i = 0;

		// Key objects self seed
		simd_xorshift128plus_key my_key1;
		simd_xorshift128plus_key my_key2;
		simd_xorshift128plus_key my_key3;
		simd_xorshift128plus_key my_key4;

		const uint32_t block = sizeof(__m256i) / sizeof(uint32_t); // 8

		while (i + 4 * block <= size) 
		{
			_mm256_storeu_si256((__m256i *)(rand_arr + i), simd_xorshift128plus_rand(my_key1));
			_mm256_storeu_si256((__m256i *)(rand_arr + i + block), simd_xorshift128plus_rand(my_key2));
			_mm256_storeu_si256((__m256i *)(rand_arr + i + 2 * block), simd_xorshift128plus_rand(my_key3));
			_mm256_storeu_si256((__m256i *)(rand_arr + i + 3 * block), simd_xorshift128plus_rand(my_key4));
			
			i += 4 * block;
		}

		while (i + 2 * block <= size) 
		{
			_mm256_storeu_si256((__m256i *)(rand_arr + i), simd_xorshift128plus_rand(my_key1));
			_mm256_storeu_si256((__m256i *)(rand_arr + i + block), simd_xorshift128plus_rand(my_key2));

			i += 2 * block;
		}
		while (i + block <= size) 
		{
			_mm256_storeu_si256((__m256i *)(rand_arr + i), simd_xorshift128plus_rand(my_key1));
			i += block;
		}
		if (i != size) 
		{
			uint32_t buffer[sizeof(__m256i) / sizeof(uint32_t)];

			_mm256_storeu_si256((__m256i *)buffer, simd_xorshift128plus_rand(my_key1));

			std::memcpy(rand_arr + i, buffer, sizeof(uint32_t) * (size - i));
		}
	}


public:
    // Do nothing at the moment
    simd_xorshift128plus(){}
    
    void fill_array(uint32_t* rand_arr, uint32_t N_rands)
    {
    	return populate_array_simd_xorshift128plus(rand_arr, N_rands);
    }

    __m256i get_rand(simd_xorshift128plus_key& key)
    {
    	return simd_xorshift128plus_rand(key);
    }

    __m256i operator()(simd_xorshift128plus_key& key)
    {
    	return simd_xorshift128plus_rand(key);
    }
};



#endif