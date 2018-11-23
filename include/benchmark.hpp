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
	static const int N_rands = 50000;
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

    template <typename T>
    void benchmark_time(T& test_obj, uint32_t repeat, std::string& str)
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
         
            test_obj.fill_array(rand_arr.data(), N_rands);
            
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

    void run()
    {
    	uint32_t repeats = 500;

    	std::string my_string = "xor128";
    	benchmark_time(my_xor, repeats, my_string);

    	std::string my_string1 = "xoro128_simd";
    	benchmark_time(my_simd_xor, repeats, my_string1);

    	std::string my_string2 = "aes_dragontamer";
    	benchmark_time(my_dragon, repeats, my_string2);

    	#if defined(__AVX512F__)
    		std::string my_string3 = "AVX512 xoro128_simd";
			benchmark_time(my_512simd_xor, repeats, my_string2);
		#endif

    }
    
};

#endif