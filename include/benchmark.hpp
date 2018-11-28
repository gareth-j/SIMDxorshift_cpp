#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <cstring>
#include <iostream>
#include <array>
#include <random>
#include <iomanip>
#include <immintrin.h>

#include "randutils.hpp"

#include "simd_xorshift128plus.hpp"
#include "xorshift128plus.hpp"
#include "aes_dragontamer.hpp"

#if defined(__AVX512F__)
	#include "simd_avx512_xorshift128plus.hpp"
#endif

class benchmark
{
protected:
	// Create some objects to work with
	xorshift128plus my_xor;

	simd_xorshift128plus my_simd_xor;	

	aes_dragontamer my_dragon;

#if defined(__AVX512F__)
	simd_avx512_xorshift128plus my_512simd_xor;
#endif

	// Number of random numbers to generate
	const std::size_t N_rands = 50000;
	
	// The number of  random numbers used in the shuffling
	// benchmakr
	const std::size_t N_shuffle = 10000;
	
	// The number of times to repeat each function benchmark
	const std::size_t repeats = 500;
	
	// Use a vector here in case a lot of rands are requested
	std::vector<uint32_t> rand_arr;

	// Benchmarking functions
    void RDTSC_start(uint64_t* cycles)
    {
        // (Hopefully) place these in a register
        register unsigned cyc_high, cyc_low;

        // Use the read time-stamp counter so we can count the 
        // number of cycles
        __asm volatile("cpuid\n\t"                                               
                       "rdtsc\n\t"                                               
                       "mov %%edx, %0\n\t"                                       
                       "mov %%eax, %1\n\t"                                       
                       : "=r"(cyc_high), "=r"(cyc_low)::"%rax", "%rbx", "%rcx",  
                         "%rdx");    

        // Update the number of cycles
        *cycles = (uint64_t(cyc_high) << 32) | cyc_low; 
    }

    void RDTSC_final(uint64_t* cycles)
    {
        // (Hopefully) place these in a register
        register unsigned cyc_high, cyc_low;

        // Use the read time-stamp counter so we can count the 
        // number of cycles
        __asm volatile("rdtscp\n\t"                                               
                       "mov %%edx, %0\n\t"
                       "mov %%eax, %1\n\t"
                       "cpuid\n\t"
                       : "=r"(cyc_high), "=r"(cyc_low)::"%rax", "%rbx", "%rcx",
                         "%rdx");

        *cycles = (uint64_t(cyc_high) << 32) | cyc_low;                          
    }


    // This function can be passed a member function and an array for testing 
    template <typename TEST_CLASS>
    void benchmark_fn(void (TEST_CLASS::*test_fn)(uint32_t*, uint32_t), TEST_CLASS& class_obj, uint32_t* test_array, const std::string& str,
    																						const std::size_t size, bool prefetch = false)
    {
    	std::fflush(nullptr);

        uint64_t cycles_start{0}, cycles_final{0}, cycles_diff{0};

        // Get max value of a uin64_t by rolling it over
        uint64_t min_diff = (uint64_t)-1;

        std::cout << "Testing function : " << str << "\n"; 

        for(uint32_t i = 0; i < repeats; i++)
        {
        	if(prefetch)
        		array_cache_prefetch(test_array, size);

            // Pretend to clobber memory 
            // Don't allow reordering of memory access instructions
            __asm volatile("" ::: "memory");
            
            RDTSC_start(&cycles_start);
         
            (class_obj.*test_fn)(test_array, size);
            
            RDTSC_final(&cycles_final);

            cycles_diff = (cycles_final - cycles_start);   
            
            if (cycles_diff < min_diff)
                min_diff = cycles_diff;
        }  

        uint64_t S = N_rands;

        // Calculate the number of cycles per operation
        float cycles_per_op = min_diff / float(S);

        // std::printf(" %.2f cycles per operation", cycles_per_op);
        std::cout << std::setprecision(2) << cycles_per_op << " cycles per operation\n";
        std::fflush(nullptr);
    }


	// Sorting functions    

    // Check what this does with big numbers, it must roll over

	static int qsort_compare_uint32_t(const void *a, const void *b) 
	{
	    return ( *(uint32_t *)a - *(uint32_t *)b );
	}

	// Tries to put the array in cache
	void array_cache_prefetch(uint32_t* B, const int32_t length) 
	{
		// 64 bytes per cache line
	    const int32_t cache_linesize = 64;

	    for(int32_t  k = 0; k < length; k += cache_linesize/sizeof(uint32_t)) 
	    {
	    	// Try and move data into the cache before it is accessed
	        __builtin_prefetch(B + k);
	    }
	}

	// Compare the arrays
	bool sort_compare(uint32_t* shuf, uint32_t* orig, const uint32_t size) 
	{
		std::qsort(shuf, size, sizeof(uint32_t), qsort_compare_uint32_t);
		std::qsort(orig, size, sizeof(uint32_t), qsort_compare_uint32_t);

		for(uint32_t k = 0; k < size; ++k)
		{
			if(shuf[k] != orig[k]) 
			{
				std::cout << "[bug - mismatch]\n";
				return false;
			}
		}

		return true;
	}


public:
	
	benchmark() {rand_arr.resize(N_rands);}     

	void run_shuffle()
	{
		std::cout << "\n==========================\n" <<
					   		"\tShuffle test" 			<<
					"\n==========================\n\n";
		std::cout << "Shuffling arrays of size " << N_shuffle << "\n";
	    std::cout << "Time reported in number of cycles per array element\n"; 
	    std::cout << "Tests assume that array is in cache as much as possible\n\n";

	    // To fill the vector
	    xorshift128plus_key my_normal_key;

	    std::vector<uint32_t> test_array(N_shuffle);

	    auto rand_fn = [&](auto)
	    {
	    	uint64_t rand64 = my_xor.get_rand(my_normal_key);
	    	
	    	// Shift it over to get a 32-bit number
	    	uint32_t rand32 = rand64 << 32;

	    	return rand32;
	    };

		// Fill the array with random numbers from our xorshift generator
	    std::transform(test_array.begin(), test_array.end(), test_array.begin(), rand_fn);

	    // Create another array identical to the test array
	    auto pristine_array = test_array;
	    
	    // Make sure they're identical
	    if(!sort_compare(test_array.data(), pristine_array.data(), N_shuffle))
	    	return;

	    // Try and prefetch data in the benchmarking function
	    bool prefetch = true;

	    std::string fn_name = "xorshift128plus_shuffle32";

	    benchmark_fn(&xorshift128plus::xorshift128plus_shuffle32, my_xor, test_array.data(), fn_name, N_shuffle, prefetch);

   	    if(!sort_compare(test_array.data(), pristine_array.data(), N_shuffle))
	    	return;

	    fn_name = "simd_xorshift128plus_shuffle32";

	    benchmark_fn(&simd_xorshift128plus::simd_xorshift128plus_shuffle32, my_simd_xor, test_array.data(), fn_name, N_shuffle, prefetch);

		if(!sort_compare(test_array.data(), pristine_array.data(), N_shuffle))
			return;

		std::cout << "\n";

	}

    void run_generators()
    {
		std::cout << "==========================\n" <<
					   		"\tGenerators" 			<<
					"\n==========================\n\n";

    	std::string fn_name = "xor128";
    	benchmark_fn(&xorshift128plus::fill_array, my_xor, rand_arr.data(), fn_name, N_rands);

    	fn_name = "xor128_simd";
    	benchmark_fn(&simd_xorshift128plus::fill_array, my_simd_xor, rand_arr.data(), fn_name, N_rands);

		fn_name = "xor128_simd_two";
		benchmark_fn(&simd_xorshift128plus::fill_array_two, my_simd_xor, rand_arr.data(),  fn_name, N_rands);

		fn_name = "xor128_simd_four";
		benchmark_fn(&simd_xorshift128plus::fill_array_four, my_simd_xor, rand_arr.data(),  fn_name, N_rands);

    	fn_name = "aes_dragontamer";
		benchmark_fn(&aes_dragontamer::fill_array, my_dragon, rand_arr.data(), fn_name, N_rands);

    	#if defined(__AVX512F__)
    		fn_name = "AVX512 xor128_simd";
    		benchmark_fn(&simd_avx512_xorshift128plus::fill_array, simd_avx512_xorshift128plus, rand_arr.data(), fn_name, N_rands);
			// benchmark_generators(my_512simd_xor, fn_name);
		#endif

    }
    
};

#endif