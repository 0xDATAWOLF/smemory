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
	int* intarr2 = (int*)smemory::alloc(sizeof(int)*numints);
	void* largearr = smemory::alloc(smemory::page_size() * 2);
	void* largearr2 = smemory::alloc(smemory::page_size() * 2);
	void* largearr3 = smemory::alloc(smemory::page_size() * 2);

	// Insert values into the array of integers.
	for (int i = 0; i < numints; ++i) intarr[i] = i+1;
	for (int i = 0; i < numints; ++i) intarr2[i] = i+1;

	// Free those values.
	smemory::free(intarr);

	// Perform a reclaim.
	smemory::reclaim();

	// Free the second array.
	smemory::free(intarr2);

	// Attempt another reclaim. This should reclaim the region.
	smemory::reclaim();
	
}


