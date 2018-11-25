
#include <cstdint>

class xorshift128plus_key
{
public:
	xorshift128plus_key()
	{
		std::array<uint32_t, 4> seed_array;

	    randutils::auto_seed_128 seeder;
		
		seeder.generate(seed_array.begin(), seed_array.end());

		uint64_t seed_part1 = seed_array[0];
		uint64_t seed_part2 = seed_array[1];
		uint64_t seed_part3 = seed_array[2];
		uint64_t seed_part4 = seed_array[3];

		// Create some 64-bit numbers
		seed1 = seed_part1 << 32 | seed_part2;
		seed2 = seed_part3 << 32 | seed_part4;
	}

	uint64_t seed1 = 0;
    uint64_t seed2 = 0;
};

class xorshift128plus
{
protected:
	uint64_t xorshift128plus_rand(xorshift128plus_key& key) 
	{
		uint64_t s1 = key.seed1;
		const uint64_t s0 = key.seed2;
		key.seed1 = s0;
		s1 ^= s1 << 23; // a
		key.seed2 = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5); // b, c
		return key.seed2 + s0;
	}

	void populateRandom_xorshift128plus(uint32_t *rand_arr, const uint32_t size) 
	{
		xorshift128plus_key mykey;

		uint32_t i = size;
		
		while (i > 2) 
		{
			*(uint64_t *)(rand_arr + size - i) = xorshift128plus_rand(mykey);
			i -= 2;
		}
		if (i != 0)
		{
			rand_arr[size - i] = (uint32_t)xorshift128plus_rand(mykey);
		}
	}


	
	// Compute two random number in [0,bound1) and [0,bound2)
	// has a slight bias, but that's probably the last of your concerns given
	// that xorshift128plus is not perfect to begin with.

	void xorshift128plus_bounded_two_by_two(xorshift128plus_key& key, uint32_t bound1, uint32_t bound2, uint32_t* bounded1, uint32_t* bounded2) 
	{
		uint64_t rand = xorshift128plus_rand(key);
		*bounded1 = ((rand & UINT64_C(0xFFFFFFFF)) * bound1) >> 32;
		*bounded2 = ((rand >> 32) * bound2) >> 32;
	}

	uint32_t xorshift128plus_bounded(xorshift128plus_key& key, uint32_t bound) 
	{
		uint64_t rand = xorshift128plus_rand(key);
		return ((rand & UINT64_C(0xFFFFFFFF)) * bound ) >> 32;
	}







public:

	xorshift128plus(){} // Do nothing here at the moment

	// For benchmarking
	void fill_array(uint32_t* rand_arr, uint32_t N_rands)
    {
    	return populateRandom_xorshift128plus(rand_arr, N_rands);
    }

    uint64_t get_rand(xorshift128plus_key& key)
    {
    	return xorshift128plus_rand(key);
    }

    uint64_t operator()(xorshift128plus_key& key)
    {
    	return xorshift128plus_rand(key);
    }


    	// Fisher-Yates shuffle, shuffling an array of integers, uses the provided key
	void xorshift128plus_shuffle32(uint32_t* storage, const uint32_t size) 
	{
		// Create the key here?
		xorshift128plus_key key;

		uint32_t i;
		
		uint32_t nextpos1, nextpos2;

		for (i=size; i>2; i-=2) 
		{
			xorshift128plus_bounded_two_by_two(key,i,i-1,&nextpos1,&nextpos2);

			const uint32_t tmp1 = storage[i-1];// likely in cache
			
			const uint32_t val1 = storage[nextpos1]; // could be costly
			
			storage[i - 1] = val1;
			
			storage[nextpos1] = tmp1; // you might have to read this store later

			uint32_t tmp2 = storage[i-2];// likely in cache
			
			uint32_t val2 = storage[nextpos2]; // could be costly
			
			storage[i - 2] = val2;
			
			storage[nextpos2] = tmp2; // you might have to read this store later
		}

		if(i>1) 
		{
			const uint32_t nextpos = xorshift128plus_bounded(key,i);
			const uint32_t tmp = storage[i-1];// likely in cache
			const uint32_t val = storage[nextpos]; // could be costly
			storage[i - 1] = val;
			storage[nextpos] = tmp; // you might have to read this store later
		}
	}
	
};