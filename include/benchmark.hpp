#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <cstring>
#include <iostream>
#include <array>
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


	
public:
	static const std::size_t N_rands = 50000;
	const std::size_t shuffle_size = 10000;

	// Use a vector here in case a lot of rands are requested
	std::vector<uint32_t> rand_arr;

	benchmark() {rand_arr.resize(N_rands);} // Do nothing here at the moment

    void RDTSC_start(uint64_t* cycles)
    {
        // (Hopefully) place these in a register
        register unsigned cyc_high, cyc_low;

        // Blackmagic ASM
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

        // Blackmagic ASM
        __asm volatile("rdtscp\n\t"                                               
                       "mov %%edx, %0\n\t"                                        
                       "mov %%eax, %1\n\t"                                        
                       "cpuid\n\t"                                                
                       : "=r"(cyc_high), "=r"(cyc_low)::"%rax", "%rbx", "%rcx",   
                         "%rdx");

        *cycles = (uint64_t(cyc_high) << 32) | cyc_low;                          
    }

 
	// template <typename T>
	// void benchmark_time(T& test_obj, uint32_t repeat, std::string& str)
	
	// TODO - there seems to be a bit of overhead with this method, need to look into it.


    // Would it just be better passing a function object here instead?
    // Look for pass function pointer vs pass function object
    // template<typename TEST_CLASS>
    // using pass_fn = void (TEST_CLASS::*)(uint32_t*, uint32_t);
    
	template <typename TEST_CLASS>
    void benchmark_shuffle(void (TEST_CLASS::*test_fn)(uint32_t*, uint32_t), TEST_CLASS& class_obj, uint32_t* test_array, const uint32_t repeat, const std::string& str)
    {
    	std::fflush(nullptr);

        uint64_t cycles_start{0}, cycles_final{0}, cycles_diff{0};

        // Get max value of a uin64_t by rolling it over
        uint64_t min_diff = (uint64_t)-1;

        std::cout << "Testing function : " << str << "\n"; 

        for(uint32_t i = 0; i < repeat; i++)
        {
        	array_cache_prefetch(test_array, shuffle_size);

            // Pretend to clobber memory 
            __asm volatile("" ::: "memory");
            
            RDTSC_start(&cycles_start);
         
            // test_obj.fill_array(rand_arr.data(), N_rands);

            (class_obj.*test_fn)(test_array, shuffle_size);
            
            RDTSC_final(&cycles_final);

            cycles_diff = (cycles_final - cycles_start);   
            
            if (cycles_diff < min_diff)
                min_diff = cycles_diff;
        }  

        uint64_t S = N_rands;

        // Calculate the number of cycles per operation
        float cycles_per_op = min_diff / float(S);

        std::printf(" %.2f cycles per operation", cycles_per_op);
        std::printf("\n");
        std::fflush(nullptr);
    }

    template <typename TEST_CLASS>
    void benchmark_generators(void (TEST_CLASS::*test_fn)(uint32_t*, uint32_t), TEST_CLASS& class_obj, const uint32_t repeat, const std::string& str)
    {
        std::fflush(nullptr);

        uint64_t cycles_start{0}, cycles_final{0}, cycles_diff{0};

        // Get max value of a uin64_t by rolling it over
        uint64_t min_diff = (uint64_t)-1;

        std::cout << "Testing function : " << str << "\n"; 

        for(uint32_t i = 0; i < repeat; i++)
        {
            // Pretend to clobber memory 
            __asm volatile("" ::: "memory");
            
            RDTSC_start(&cycles_start);
         
            // test_obj.fill_array(rand_arr.data(), N_rands);

            (class_obj.*test_fn)(rand_arr.data(), N_rands);
            
            RDTSC_final(&cycles_final);

            cycles_diff = (cycles_final - cycles_start);   
            
            if (cycles_diff < min_diff)
                min_diff = cycles_diff;
        }  

        uint64_t S = N_rands;

        // Calculate the number of cycles per operation
        float cycles_per_op = min_diff / float(S);

        std::printf(" %.2f cycles per operation", cycles_per_op);
        std::printf("\n");
        std::fflush(nullptr);
    }

    //===========================
	// Sorting stuff
	//===========================
	

	// int qsort_compare_uint32_t(const void *a, const void *b) 
	// {
	//     return ( *(uint32_t *)a - *(uint32_t *)b );
	// }



	static int qsort_compare_uint32_t(const uint32_t a, const uint32_t b) 
	{
	    return a - b;
	}
	

	// Unused
	// uint32_t* create_sorted_array(uint32_t length) 
	// {
	//     uint32_t *array = malloc(length * sizeof(uint32_t));

	//     for (uint32_t i = 0; i < length; i++) array[i] = (uint32_t) rand();

	//     qsort(array, length, sizeof(*array), qsort_compare_uint32_t);

	//     return array;
	// }

	// // Create an array filled with random numbers and return a pointer to it
	// uint32_t *create_random_array(uint32_t count) 
	// {
	// 	uint32_t* targets = malloc(count * sizeof(uint32_t));


	//     for (uint32_t i = 0; i < count; i++) targets[i] = (uint32_t) rand();
	//     return targets;
	// }



	// flushes the array from cache
	// Unused
	// void array_cache_flush(uint32_t* B, int32_t length) 
	// {
	//     const int32_t CACHELINESIZE = 64;// 64 bytes per cache line
	//     for(int32_t  k = 0; k < length; k += CACHELINESIZE/sizeof(uint32_t)) {
	//         __builtin_ia32_clflush(B + k);
	//     }
	// }

	// int qsort_compare_uint32_t(const uint32_t a, const uint32_t b)
	// {
	// 	return a-b;
	// }

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

	bool sort_compare(std::vector<uint32_t>& shuf, std::vector<uint32_t>& orig)//, const uint32_t size) 
	{
		// auto qsort_compare_uint32_t = [] (const uint32_t a, const uint32_t b){return a-b;};

		// std::sort(shuf, shuf+size, qsort_compare_uint32_t);
		std::cout << "sort comp 1.\n";
		std::sort(shuf.begin(), shuf.end(), qsort_compare_uint32_t);
		std::cout << "sort comp 2.\n";
		std::sort(orig.begin(), orig.end(), qsort_compare_uint32_t);


		bool are_same;
		if(shuf == orig)
		{
			are_same = true;
		}
		else
		{
			are_same = false;			
		}
		std::cout << "sort comp 3.\n";

		return are_same;

	}

	void run_shuffle()
	{
		const uint32_t repeats = 500;

	    std::cout << "Shuffling arrays of size << " << shuffle_size << "\n";
	    std::cout << "Time reported in number of cycles per array element\n"; 
	    std::cout << "Tests assume that array is in cache as much as possible\n"; 

	    // To fill the vector
	    xorshift128plus_key my_normal_key;

	    std::cout << "We get here 1.\n";
	    std::vector<uint32_t> test_array(shuffle_size);

	    auto rand_fn = [&](auto){return my_xor.get_rand(my_normal_key);};

	    std::cout << "We get here 2.\n";
	    std::transform(test_array.begin(), test_array.end(), test_array.begin(), rand_fn);

	    auto pristine_array = test_array;
	    

	    std::cout << "We get here 3.\n";
	    // Make sure they're identical
	    if(!sort_compare(test_array, pristine_array))
	    	return;

	    std::string fn_name = "Normal xoroshiro128plus";

	    // void benchmark_shuffle(void (TEST_CLASS::*test_fn)(uint32_t*, uint32_t), TEST_CLASS& class_obj, uint32_t* test_array, const uint32_t repeat, const std::string& str)

	    std::cout << "We get here 4.\n";
	    benchmark_shuffle(&xorshift128plus::xorshift128plus_shuffle32, my_xor, test_array.data(), repeats, fn_name);


	    std::cout << "We get here 5.\n";
   	    if(!sort_compare(test_array, pristine_array))
	    	return;

		std::cout << "We get here 6.\n";	
	    fn_name = "SIMD xoroshiro128plus";

	    benchmark_shuffle(&simd_xorshift128plus::simd_xorshift128plus_shuffle32, my_simd_xor, test_array.data(), repeats, fn_name);

	    std::cout << "We get here 7.\n";
		if(!sort_compare(test_array, pristine_array))
			return;


		std::cout << "\n";

	}

    void run_generators()
    {
    	uint32_t repeats = 500;

    	std::string fn_name = "xor128";
    	// benchmark_generators(my_xor, repeats, fn_name);
    	benchmark_generators(&xorshift128plus::fill_array, my_xor, repeats, fn_name);

    	fn_name = "xor128_simd";
    	benchmark_generators(&simd_xorshift128plus::fill_array, my_simd_xor, repeats, fn_name);

		fn_name = "xor128_simd_two";
		benchmark_generators(&simd_xorshift128plus::fill_array_two, my_simd_xor, repeats, fn_name);

		fn_name = "xor128_simd_four";
		benchmark_generators(&simd_xorshift128plus::fill_array_four, my_simd_xor, repeats, fn_name);

    	fn_name = "aes_dragontamer";
		benchmark_generators(&aes_dragontamer::fill_array, my_dragon, repeats, fn_name);
  		// benchmark_generators(my_dragon, repeats, fn_name);

    	#if defined(__AVX512F__)
    		fn_name = "AVX512 xor128_simd";
    		benchmark_generators(&simd_avx512_xorshift128plus::fill_array, my_dragon, repeats, fn_name);
			// benchmark_generators(my_512simd_xor, repeats, fn_name);
		#endif

    }
    
};

#endif