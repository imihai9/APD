#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include "genetic_algorithm.h"
#include "arg.h"

int main(int argc, char *argv[]) {
	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	// number of threads
	int threads_count = 0;

	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, &threads_count, argc, argv)) {
		return 0;
	}

	run_genetic_algorithm(threads_count, objects, object_count, generations_count, sack_capacity);

	free(objects);
	
	return 0;
}
