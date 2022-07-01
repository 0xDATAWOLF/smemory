/**
 * A test application for the smemory framework.
 */
#include <iostream>
#include "smemory.h"

int main(int argc, char** argv)
{
	
	// Initialize smemory. We will create a journal with room to store 1 page at
	// initialization time.
	SMEMORY_CONFIG smemory_config = {};
	smemory_config.journal_create_journal = 1;
	smemory::init(&smemory_config);

	int numints = 34;

	// Allocate an array of integers.
	int* intarr = (int*)smemory::alloc(sizeof(int)*numints);

	// Insert values into the array of integers.
	for (int i = 0; i < numints; ++i) intarr[i] = i+1;

	// Free those values.
	smemory::free(intarr);
	
}


