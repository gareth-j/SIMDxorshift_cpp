#include "include/benchmark.hpp"

int main()
{
	benchmark my_bench;
	
	my_bench.run_generators();

	my_bench.run_shuffle();
}