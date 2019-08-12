#include "tips.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w);

/* return random int from 0..x-1 */
int randomint(int x);
int getLRU(int index);
/*
This function allows the lfu information to be displayed

assoc_index - the cache unit that contains the block to be modified
block_index - the index of the block to be modified

returns a string representation of the lfu information
*/
char* lfu_to_string(int assoc_index, int block_index)
{
	/* Buffer to print lfu information -- increase size as needed. */
	static char buffer[9];
	sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

	return buffer;
}

/*
This function allows the lru information to be displayed

assoc_index - the cache unit that contains the block to be modified
block_index - the index of the block to be modified

returns a string representation of the lru information
*/
char* lru_to_string(int assoc_index, int block_index)
{
	/* Buffer to print lru information -- increase size as needed. */
	static char buffer[9];
	sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

	return buffer;
}

/*
This function initializes the lfu information

assoc_index - the cache unit that contains the block to be modified
block_number - the index of the block to be modified

*/
void init_lfu(int assoc_index, int block_index)
{
	cache[assoc_index].block[block_index].accessCount = 0;
}

/*
This function initializes the lru information

assoc_index - the cache unit that contains the block to be modified
block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index)
{
	cache[assoc_index].block[block_index].lru.value = 0;
}

/*
This is the primary function you are filling out,
You are free to add helper functions if you need them

@param addr 32-bit byte address
@param data a pointer to a SINGLE word (32-bits of data)
@param we if we == READ, then data used to return
information back to CPU

if we == WRITE, then data used to
update Cache/DRAM
*/
//we need to first find the lru before anything else
int getLRU(int index) {
	int max = 0;
	int block = 0;
	for (int i = 0; i < assoc; i++) {
		if (cache[index].block[i].lru.value > max)
		{

			max = cache[index].block[i].lru.value;
			block = i;
		}

	}
	return block;
}
int getLFU(int index) {
	unsigned int low = 0xFFFFFFFF;
	unsigned int block = 0;
	for (int i = 0; i < assoc; i++) {
		if (cache[index].block[i].accessCount < low) {
			block = i;
			low = cache[index].block[i].accessCount;
		}
	}
	return block;

}
//void dFunc(address addr, word *data, WriteEnable we) {
//	//for (int i = 0; i < block_size; i += 4)
//	//{
//	int i = 0;
//	while (i < block_size) {
//		accessDRAM(addr + i, (byte*)data + i, WORD_SIZE, we);
//		i += 4;
//	}
//}
//need a function to replace dirty bits

void accessMemory(address addr, word* data, WriteEnable we)
{
	/* Declare variables here */
	TransferUnit t;
	int offsetBits = 0;
	switch (block_size) {
	case 0:
		t = BYTE_SIZE;
		break;
	case 4:
		t = WORD_SIZE;
		break;
	case 2:
		t = HALF_WORD_SIZE;
		break;
	case 8:
		t = DOUBLEWORD_SIZE;
		break;
	case 16:
		t = QUADWORD_SIZE;
		break;
	case OCTWORD_SIZE:
		t = OCTWORD_SIZE;
		break;
	}
	 offsetBits = uint_log2(block_size);
	int indexBits;
	address db;

	indexBits = uint_log2(set_count);
	int tagBits = 32 - offsetBits - indexBits;
	//find values for offset,
	unsigned int offset = addr << (tagBits + indexBits);
	offset = offset >> (tagBits + indexBits);
	unsigned int index;
	index = addr << (tagBits);
	index = index >> (tagBits + offsetBits);
	unsigned int tag = addr >> (offsetBits + indexBits);
	int H = assoc;

	/* handle the case of no cache at all - leave this in */
	if (assoc == 0) {
		accessDRAM(addr, (byte*)data, WORD_SIZE, we);
		return;
	}

	/*
	You need to read/write between memory (via the accessDRAM() function) and
	the cache (via the cache[] global structure defined in tips.h)

	Remember to read tips.h for all the global variables that tell you the
	cache parameters

	The same code should handle random, LFU, and LRU policies. Test the policy
	variable (see tips.h) to decide which policy to execute. The LRU policy
	should be written such that no two blocks (when their valid bit is VALID)
	will ever be a candidate for replacement. In the case of a tie in the
	least number of accesses for LFU, you use the LRU information to determine
	which block to replace.

	Your cache should be able to support write-through mode (any writes to
	the cache get immediately copied to main memory also) and write-back mode
	(and writes to the cache only gets copied to main memory when the block
	is kicked out of the cache.

	Also, cache should do allocate-on-write. This means, a write operation
	will bring in an entire block if the block is not already in the cache.

	To properly work with the GUI, the code needs to tell the GUI code
	when to redraw and when to flash things. Descriptions of the animation
	//functions can be found in tips.h
	*/

	/* Start adding code here */
	int i = 0;
	

	
	while (i < assoc) {
		if (cache[index].block[i].tag == tag && cache[index].block[i].valid == 1) {
			H = i;
			highlight_offset(index, H, offset, HIT);
			break;
		}
		i++;
	}
	if (we == READ) {
		if (H != assoc) {
			//goto hit_copy; this doesnt work for some reason so i just copied the block normally
			memcpy((void *)data, (void *)cache[index].block[H].data + offset, block_size);
		}
		else {

			//dFunc(addr, data, READ);
			accessDRAM(addr, (void*)data, t, READ);
			int bigLRU;
			
			switch (policy) {
			case LRU:
				bigLRU = getLRU(index);
				//now i need to check if anthing is dirty
				if (cache[index].block[bigLRU].dirty == DIRTY)
				{
					// i need the address
					db = cache[index].block[bigLRU].tag << ((indexBits + offsetBits) + (index << offsetBits));
					accessDRAM(db, (cache[index].block[bigLRU].data), t, WRITE);
				}
				memcpy((void *)cache[index].block[bigLRU].data, (void *)data, block_size);
				cache[index].block[bigLRU].dirty = VIRGIN;
				cache[index].block[bigLRU].valid = VALID;
				cache[index].block[bigLRU].tag = tag;
				cache[index].block[bigLRU].accessCount = 0;
				cache[index].block[bigLRU].lru.value = 1;
				break;
			case LFU:
				bigLRU = getLFU(index);
				//now i need to check if anthing is dirty
				if (cache[index].block[bigLRU].dirty == DIRTY)
				{
					// i need the address
					db = cache[index].block[bigLRU].tag << ((indexBits + offsetBits) + (index << offsetBits));
					accessDRAM(db, (cache[index].block[bigLRU].data), t, WRITE);
				}
				memcpy((void *)cache[index].block[bigLRU].data, (void *)data, block_size);
				cache[index].block[bigLRU].dirty = VIRGIN;
				cache[index].block[bigLRU].valid = VALID;
				cache[index].block[bigLRU].tag = tag;
				cache[index].block[bigLRU].accessCount = 0;
				cache[index].block[bigLRU].lru.value = 1;
				break;
			case RANDOM:
				
				bigLRU = randomint(assoc);

				if (cache[index].block[bigLRU].dirty == DIRTY)
				{

					// i need the address
					db = cache[index].block[bigLRU].tag << ((indexBits + offsetBits) + (index << offsetBits));
					accessDRAM(db, (cache[index].block[bigLRU].data), t, WRITE);

				}

				memcpy((void *)cache[index].block[bigLRU].data, (void *)data, block_size);

				//reset block data

				cache[index].block[bigLRU].dirty = VIRGIN;

				cache[index].block[bigLRU].valid = VALID;
				cache[index].block[bigLRU].tag = tag;
				cache[index].block[bigLRU].accessCount = 0;
				cache[index].block[bigLRU].lru.value = 1;
				break;
			}
			highlight_block(index, bigLRU);
			highlight_offset(index, bigLRU, offset, MISS);

			//goto hit_copy;
		}
	}
	else {
		int bigLRU2;

		switch (memory_sync_policy) {
		case WRITE_BACK:
			if (policy == LRU) {
				bigLRU2 = getLRU(index);
			}
			else if(policy==RANDOM) {
				bigLRU2 = randomint(assoc);

			}
			else {
				bigLRU2 = getLFU(index);
				
			}
			if (cache[index].block[bigLRU2].dirty == DIRTY)
			{
				// i need the address
				db = cache[index].block[bigLRU2].tag << ((indexBits + offsetBits) + (index << offsetBits));
				accessDRAM(db, (cache[index].block[bigLRU2].data), t, WRITE);
			}
			memcpy((void *)cache[index].block[bigLRU2].data, (void *)data, block_size);
			cache[index].block[bigLRU2].dirty = VIRGIN;
			cache[index].block[bigLRU2].valid = VALID;
			cache[index].block[bigLRU2].tag = tag;
			cache[index].block[bigLRU2].accessCount = 0;
			cache[index].block[bigLRU2].lru.value = 1;

			break;
		case WRITE_THROUGH:
			if (policy == LRU) {
				bigLRU2 = getLRU(index);
			}

			else if(policy==RANDOM) {
				bigLRU2 = randomint(assoc);

			}
			else{
				bigLRU2 = getLFU(index);

			}
			if (cache[index].block[bigLRU2].dirty == DIRTY)
			{
				// i need the address
				db = cache[index].block[bigLRU2].tag << ((indexBits + offsetBits) + (index << offsetBits));
				accessDRAM(db, (cache[index].block[bigLRU2].data), t, WRITE);
			}
			//dFunc(addr, data, WRITE);
			accessDRAM(addr, (void *)cache[index].block[bigLRU2].data, t, WRITE);
			memcpy((void *)cache[index].block[bigLRU2].data, (void *)data, block_size);
			cache[index].block[bigLRU2].dirty = VIRGIN;
			cache[index].block[bigLRU2].valid = VALID;
			cache[index].block[bigLRU2].tag = tag;
			cache[index].block[bigLRU2].accessCount = 0;
			cache[index].block[bigLRU2].lru.value = 1;
			break;
		}
	}

	/*hit_copy:
	memcpy((void *)data, (void *)cache[index].block[H].data + offset, block_size);
*/
}
