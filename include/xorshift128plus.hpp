
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
	
};