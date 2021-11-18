#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "genetic_algorithm.h"
#include "arg.h"

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int *threads_count,
				int argc, char *argv[])
{
	FILE *fp;

	if (argc < 4) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count thread_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	
	if (*generations_count == 0) {
		free(tmp_objects);
		return 0;
	}

	*objects = tmp_objects;

	*threads_count = (int) strtol(argv[3], NULL, 10);

	if (*threads_count == 0) {
		free(tmp_objects);
		return 0;
	}

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int sack_capacity,
							  int start, int end)
{
	for (int i = start; i < end; ++i) {
		int weight = 0;
		int profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	//int i;
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = first->set_chromosomes_count;
		int second_count = second->set_chromosomes_count;

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

void *run_genetic_algorithm_parallel(void *_arg);
void run_genetic_algorithm(int threads_count, sack_object *objects, int object_count, 
						   int generations_count, int sack_capacity)
{
	individual *current_generation = (individual*) calloc(object_count, sizeof(individual));
	if (!current_generation)
		exit(-1);
	individual *next_generation = (individual*) calloc(object_count, sizeof(individual));
	if (!next_generation)
		exit(-1);

	// threads creation ----------------------------------------------
	pthread_t *threads = (pthread_t*) malloc(threads_count * sizeof(pthread_t));
	if (!threads)
		exit(-1);

	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier, NULL, threads_count);

	Arg *args = (Arg*) malloc(threads_count * sizeof(Arg));

	for (int i = 0; i < threads_count; i++) {
		args[i].id = i;
		args[i].threads_count = threads_count;
		args[i].objects = objects;
		args[i].object_count = object_count;
		args[i].generations_count = generations_count;
		args[i].sack_capacity = sack_capacity;
		args[i].current_generation = current_generation;
		args[i].next_generation = next_generation;
		args[i].barrier = &barrier;

		int r = pthread_create(&threads[i], NULL, run_genetic_algorithm_parallel, &args[i]);
		if (r != 0) {
			printf("Eroare la crearea thread-ului %d\n", i);
			exit(-1);
		}
	}

	for (int i = 0; i < threads_count; i++) {
		int r = pthread_join(threads[i], NULL);
		if (r) {
			printf("Eroare la asteptarea thread-ului %d\n", i);
			exit(-1);
		}
	}

	// final steps -----------
	qsort(current_generation, object_count, sizeof(individual), cmpfunc);
	print_best_fitness(current_generation);

	// free resources for old generation
	free_generation(current_generation);
	free_generation(next_generation);

	// free resources
	free(current_generation);
	free(next_generation);
	pthread_barrier_destroy(&barrier);
}

// Returns start and end indices of the interval corresponding to arg->id
// after division of [0, N] interval
void generate_thread_indices(Arg *arg, int N, int *start, int *end) {
	*start = arg->id * (double)N / arg->threads_count;
	*end = min((arg->id + 1) * (double)N / arg->threads_count, N);
}

void *run_genetic_algorithm_parallel(void *_arg) {
	Arg *arg = (Arg*)_arg;
	int count, cursor;
	individual *tmp = NULL;

	// for better code comprehension
	sack_object *objects = arg->objects;
	int object_count = arg->object_count;
	int generations_count = arg->generations_count;
	int sack_capacity = arg->sack_capacity;
	individual *current_generation = arg->current_generation;
	individual *next_generation = arg->next_generation;

	int start_obj_cnt, end_obj_cnt;
	generate_thread_indices(arg, object_count, &start_obj_cnt, &end_obj_cnt);
	
	// initializations
	for (int i = start_obj_cnt; i < end_obj_cnt; ++i) {
		current_generation[i].fitness = 0;
		current_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		current_generation[i].chromosomes[i] = 1;
		current_generation[i].index = i;
		current_generation[i].chromosome_length = object_count;

		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].chromosome_length = object_count;
	}

	// iterate for each generation
	for (int k = 0; k < generations_count; ++k) {
		cursor = 0;

		// compute fitness and sort by it
		compute_fitness_function(objects, current_generation, sack_capacity, start_obj_cnt, end_obj_cnt);
		
		// Updates the "set_chromosome_count" field in current_generation arrays.
		// Function will be called by the thread operating on the [start, end)
		// portion of the current_generation array
		for (int i = start_obj_cnt; i < end_obj_cnt; i++) { // individuals
			current_generation[i].set_chromosomes_count = 0;
			for (int j = 0; j < current_generation[i].chromosome_length; j++) // chromozomes
				current_generation[i].set_chromosomes_count += current_generation[i].chromosomes[j];
		}

		pthread_barrier_wait(arg->barrier);

		if (arg->id == 0)
			qsort(current_generation, object_count, sizeof(individual), cmpfunc);
		

		// keep first 30% children (elite children selection)
		count = object_count * 3 / 10;
		int start_curr_cnt, end_curr_cnt;
		generate_thread_indices(arg, count, &start_curr_cnt, &end_curr_cnt);

		pthread_barrier_wait(arg->barrier);

		for (int i = start_curr_cnt; i < end_curr_cnt; ++i) {
			copy_individual(current_generation + i, next_generation + i);
		}
		cursor = count;

		// mutate first 20% children with the first version of bit string mutation
		count = object_count * 2 / 10;
		generate_thread_indices(arg, count, &start_curr_cnt, &end_curr_cnt);

		for (int i = start_curr_cnt; i < end_curr_cnt; ++i) {
			copy_individual(current_generation + i, next_generation + cursor + i);
			mutate_bit_string_1(next_generation + cursor + i, k);
		}
		cursor += count;

		// mutate next 20% children with the second version of bit string mutation
		count = object_count * 2 / 10;
		generate_thread_indices(arg, count, &start_curr_cnt, &end_curr_cnt);

		for (int i = start_curr_cnt; i < end_curr_cnt; ++i) {
			copy_individual(current_generation + i + count, next_generation + cursor + i);
			mutate_bit_string_2(next_generation + cursor + i, k);
		}
		cursor += count;
		
		// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		count = object_count * 3 / 10;

		if (count % 2 == 1) {
			if (arg->id == 0) // memcpy only on one thread
				copy_individual(current_generation + object_count - 1, next_generation + cursor + count - 1);
			count--;
		}

		// Divide [0, count] interval between threads_count indices
		// that start with an even index
		generate_thread_indices(arg, count, &start_curr_cnt, &end_curr_cnt);
		if (start_curr_cnt % 2 == 1 && start_curr_cnt < count - 1)
			start_curr_cnt++; 

		for (int i = start_curr_cnt; i < end_curr_cnt; i += 2) {
			crossover(current_generation + i, next_generation + cursor + i, k);
		}
		
		// switch to new generation
		tmp = current_generation;
		current_generation = next_generation;
		next_generation = tmp;

		for (int i = start_obj_cnt; i < end_obj_cnt; ++i) {
			current_generation[i].index = i;
		}

		if (arg->id == 0 && k % 5 == 0) {
			print_best_fitness(current_generation);
		}

		pthread_barrier_wait(arg->barrier);
	}

	compute_fitness_function(objects, current_generation, sack_capacity, start_obj_cnt, end_obj_cnt);

	for (int i = start_obj_cnt; i < end_obj_cnt; i++) { // individuals
		current_generation[i].set_chromosomes_count = 0;
		for (int j = 0; j < current_generation[i].chromosome_length; j++) // chromozomes
			current_generation[i].set_chromosomes_count += current_generation[i].chromosomes[j];
	}

	pthread_exit(NULL);
}