
#define CHUNK_SIZE 32	/* How much length will be a single chunk in bitmap */


/*
 * @return success: 0, fail: 1
 */

int bitmap_set(int *bitmap, int bitmap_size, int digit)
{
	int chunk, rest;
	
	if (bitmap_size <= digit) {
		return 1;
	}

	chunk = digit / CHUNK_SIZE;
	rest = digit % CHUNK_SIZE;

	bitmap += chunk;
	(*bitmap) |= 1 << rest;

	return 0;
}

int bitmap_clear(int *bitmap, int bitmap_size, int digit)
{
	int chunk, rest;
	
	if (bitmap_size <= digit) {
		return 1;
	}

	chunk = digit / CHUNK_SIZE;
	rest = digit % CHUNK_SIZE;

	bitmap += chunk;
	(*bitmap) &= ~(1 << rest);

	return 0;
}


/*
 * @return success: value on digit, fail: -1
 */

int bitmap_get(int *bitmap, int bitmap_size, int digit)
{
	int chunk, rest;

	if (bitmap_size <= digit) {
		return -1;
	}

	chunk = digit / CHUNK_SIZE;
	rest  = digit % CHUNK_SIZE;

	bitmap += chunk;
	return ((*bitmap) & (1 << rest)) == 0 ? 0 : 1;
}
