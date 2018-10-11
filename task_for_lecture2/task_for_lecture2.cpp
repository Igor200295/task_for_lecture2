#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cilk/reducer_max.h>
#include <cilk/reducer_min.h>
#include <cilk/reducer_vector.h>
#include <chrono>
#include <iostream>

using namespace std::chrono;

/// Функция ReducerMaxTest() определяет максимальный элемент массива,
/// переданного ей в качестве аргумента, и его позицию
/// mass_pointer - указатель исходный массив целых чисел
/// size - количество элементов в массиве
void ReducerMaxTest(int *mass_pointer, const long size)
{
	cilk::reducer<cilk::op_max_index<long, int>> maximum;
	cilk_for(long i = 0; i < size; ++i)
	{
		maximum->calc_max(i, mass_pointer[i]);
	}
	printf("Maximal element = %d has index = %d\n\n",
		maximum->get_reference(), maximum->get_index_reference());
}

/// Функция ReducerMinTest() определяет минимальный элемент массива,
/// переданного ей в качестве аргумента, и его позицию
/// mass_pointer - указатель исходный массив целых чисел
/// size - количество элементов в массиве
void ReducerMinTest(int *mass_pointer, const long size)
{
	cilk::reducer<cilk::op_min_index<long, int>> minimum;
	cilk_for(long i = 0; i < size; ++i)
	{
		minimum->calc_min(i, mass_pointer[i]);
	}
	printf("Minimal element = %d has index = %d\n\n",
		minimum->get_reference(), minimum->get_index_reference());
}

/// Функция ParallelSort() сортирует массив в порядке возрастания
/// begin - указатель на первый элемент исходного массива
/// end - указатель на последний элемент исходного массива
/// возвращает время выполнения сортировки
double ParallelSort(int *begin, int *end)
{
	high_resolution_clock::time_point t_start = high_resolution_clock::now();

	if (begin != end)
	{
		--end;
		int *middle = std::partition(begin, end, std::bind2nd(std::less<int>(), *end));
		std::swap(*end, *middle);
		cilk_spawn ParallelSort(begin, middle);
		ParallelSort(++middle, ++end);
		cilk_sync;
	}

	high_resolution_clock::time_point t_end = high_resolution_clock::now();

	duration<double> duration = (t_end - t_start);
	return duration.count();
}

void CompareForAndCilk_For(size_t sz)
{
	int M = 1000000 / sz; // число M необходимо для более точного вычисления времени работы кода
	std::cout << "Size is: " << sz << std::endl;
	std::cout << std::scientific;

	high_resolution_clock::time_point t_start_for = high_resolution_clock::now();

	for (int i = 0; i < M; ++i)
	{
		std::vector<int> vec(sz);
		for (size_t i = 0; i < sz; ++i)
			vec.push_back(rand() % 20000 + 1);
	}

	high_resolution_clock::time_point t_end_for = high_resolution_clock::now();
	duration<long double> duration_for = (t_end_for - t_start_for) / M;
	std::cout << "Duration in \"for\"      is: " << duration_for.count() << " seconds" << std::endl;

	high_resolution_clock::time_point t_start_cilk_for = high_resolution_clock::now();

	for (int i = 0; i < M; ++i)
	{
		cilk::reducer<cilk::op_vector<int>> red_vec;
		cilk_for(size_t i = 0; i < sz; ++i)
			red_vec->push_back(rand() % 20000 + 1);
	}

	high_resolution_clock::time_point t_end_cilk_for = high_resolution_clock::now();
	duration<long double> duration_cilk_for = (t_end_cilk_for - t_start_cilk_for) / M;
	std::cout << "Duration in \"cilk_for\" is: " << duration_cilk_for.count() << " seconds" << std::endl;
	std::cout << std::endl;
}

int main()
{
	srand((unsigned)time(0));

	// устанавливаем количество работающих потоков = 4
	__cilkrts_set_param("nworkers", "4");

	long i;
	const long mass_size = 10000;
	int *mass_begin, *mass_end;
	int *mass = new int[mass_size];

	std::cout << "Size is: " << mass_size << std::endl << std::endl;

	for (i = 0; i < mass_size; ++i)
	{
		mass[i] = (rand() % 25000) + 1;
	}

	mass_begin = mass;
	mass_end = mass_begin + mass_size;
	ReducerMaxTest(mass, mass_size);
	ReducerMinTest(mass, mass_size);

	double t = ParallelSort(mass_begin, mass_end);
	std::cout << "Duration of sorting is: " << t << " seconds" << std::endl << std::endl;
	ReducerMaxTest(mass, mass_size);
	ReducerMinTest(mass, mass_size);

	delete[]mass;

	std::cout << "Comparing \"for\" and \"cilk_for\":" << std::endl << std::endl;
	std::vector<long> sizes = { 1000000, 100000, 1000, 500, 100, 50, 10 };
	for each (long sz in sizes)
	{
		CompareForAndCilk_For(sz);
	}

	return 0;
}