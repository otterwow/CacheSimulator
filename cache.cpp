#include	"precomp.h"

// instantiate the cache
#if NLAYERS == 1
Cache cache("L1$", 128, 4, new RAM());

#else
Cache cache("L1$", 128, 4,         //128  sets,  4 slots =>  32KB
	new Cache("L2$", 1024, 8,      //1024 sets,  8 slots => 256KB
		new Cache("L3$", 8192, 16, //8192 sets, 16 slots =>   2MB
			new RAM()
		)
	)
);
#endif

// instantiate the observer (draws and prints logging)
Observer observer(&cache);

// returns a reference to the l1 cache
Cache* GetCache() {
	return &cache;
}

// returns a reference to the observer
Observer * GetObserver()
{
	return &observer;
}

// helper functions; forward all requests to the cache
uint LOADUINT( uint* address ) { return cache.Read32bit( (uint)address ); }
void STOREUINT(uint* address, uint value) { cache.Write32bit((uint)address, value); }


// artificial delay to feel some pain on cache misses
void Delay() { }//static int* x = new int[20000]; memcpy(x + 10000, x, 40000); }

// ============================================================================
// CACHE SIMULATOR IMPLEMENTATION
// ============================================================================

// cache constructor
Cache::Cache(char* name, uint sets, uint slots, Cache* lowerMemory) : name(name), sets(sets), slots(slots), lowerMemory(lowerMemory)
{
	// precompute relevant information regarding bit shifts for addressing
	this->offsetBits = 6;
	this->setBits = (uint)log2(sets);
	this->tagBits = 32 - (setBits + offsetBits);

	this->offsetMask = 63u;
	this->setMask = UINT_MAX >> (32 - setBits - offsetBits) & ~offsetMask;
	this->tagMask = UINT_MAX << (setBits + offsetBits);

	// initialize the cache
	this->cache = new CacheLine*[sets];
	for (size_t i = 0; i < sets; i++)
		cache[i] = new CacheLine[slots];

	// initialize the logging statistics to 0
	this->readReq = 0;
	this->writeReq = 0;
	this->readHit = 0;
	this->writeHit = 0;

	// choose the desired policy
	if (POLICY == 0)
		this->policy = new RandomReplacement(this);
	if (POLICY == 1)
		this->policy = new TreePseudoLRU(this);
	if (POLICY == 2)
		this->policy = new BitPseudoLRU(this);
	if (POLICY == 3)
		this->policy = new FirstInFirstOut(this);
}

// setup a dummy cache to appear as ram
RAM::RAM() : Cache("RAM", 0, 0, nullptr) {}

// cache destructor
Cache::~Cache()
{
	printf( "%f\n", dummy ); // prevent dead code elimination
}

// reading 32-bit values
uint Cache::Read32bit( uint address )
{
	this->readReq++; // logging
	auto fetch = GetSlot(address); // get a cacheline to read from
	this->readHit += fetch.hit; // logging
	return fetch.cacheLine->data[Offset(address)]; // read from the cacheline
}

// writing 32-bit values
void Cache::Write32bit( uint address, uint value )
{
	this->writeReq++; // logging
	auto fetch = GetSlot(address); // get a cacheline to write to
	this->writeHit += fetch.hit; // logging
	fetch.cacheLine->data[Offset(address)] = value; // write to the cacheline
	fetch.cacheLine->dirty = 1; // reflect that the cacheline data does not reflect lower memory
}

// read a cacheline (64B) into line
void Cache::ReadCacheLine(uint address, CacheLine& line)
{
	this->readReq++; // logging
	auto fetch = GetSlot(address); // get a cacheline to read from
	this->readHit += fetch.hit; // logging
	memcpy(line.data, fetch.cacheLine->data, 64); // transfer the data to the line
}

// write a cacheline (64B) into the cache
void Cache::WriteCacheLine(uint address, CacheLine& line)
{
	this->writeReq++; // logging
	auto fetch = GetSlot(address); // get a cacheline to write to
	this->writeHit += fetch.hit; // logging
	memcpy(fetch.cacheLine->data, line.data, 64); // transfer the data from the line
	fetch.cacheLine->dirty = 1; // reflect that the cacheline data does not reflect lower memory
}

// read a cacheline (64B) into a line from RAM
void RAM::ReadCacheLine(uint address, CacheLine& line) {
	LoadLineFromMem(address, line);
}

// write a cacheline (64) into RAM
void RAM::WriteCacheLine(uint address, CacheLine& line) {
	WriteLineToMem(address, line);
}

// get a cacheline from cache, or lower memory
CacheLineFetch Cache::GetSlot(uint address)
{
	// extract data location from address
	uint offset = Offset(address);
	uint set = Set(address);
	uint tag = Tag(address);

	// check if address is already in cache
	for (uint i = 0; i < this->slots; i++)
		// cache hit
		if (cache[set][i].valid && cache[set][i].tag == tag) {
			return CacheLineFetch(&(cache[set][i]), true);
		}

	// cache miss
	// locate a cacheline to use
	uint slot = this->policy->FindSlot(set);

	// if cacheline is dirty, write its previous data back to the lower memory
	if (cache[set][slot].valid && cache[set][slot].dirty) {
		auto prevAddress = (cache[set][slot].tag << (offsetBits + setBits)) | (set << offsetBits);
		lowerMemory->WriteCacheLine(prevAddress, cache[set][slot]);
	}

	// read data from lower memory into the cache block
	lowerMemory->ReadCacheLine(address, cache[set][slot]);

	// set metadata for cacheline
	cache[set][slot].tag = tag;
	cache[set][slot].valid = 1;
	cache[set][slot].dirty = 0;

	return CacheLineFetch(&(cache[set][slot]), false);
}

// ============================================================================
// CACHE SIMULATOR LOW LEVEL MEMORY ACCESS WITH LATENCY SIMULATION
// ============================================================================

uint Cache::Offset(uint address)
{
	return (address & offsetMask) >> 2;
}

uint Cache::Set(uint address)
{
	return (address & setMask) >> offsetBits;
}

uint Cache::Tag(uint address)
{
	return (address & tagMask) >> (setBits + offsetBits);
}

// load a cache line from memory; simulate RAM latency
void Cache::LoadLineFromMem( uint address, CacheLine& line )
{
    uint lineAddress = address & 0xFFFFFFC0; // set last six bit to 0
    memcpy( line.data, (void*)lineAddress, 64 ); // fetch 64 bytes into line
    Delay();
}

// write a cache line to memory; simulate RAM latency
void Cache::WriteLineToMem( uint address, CacheLine& line )
{
    uint lineAddress = address & 0xFFFFFFC0; // set last six bit to 0
    memcpy( (void*)lineAddress, line.data, 64 ); // fetch 64 bytes into line
    Delay();
}

// cacheline constructor
CacheLine::CacheLine()
{
	this->valid = 0;
	this->dirty = 1;
}

RandomReplacement::RandomReplacement(Cache * cache) : EvictionPolicy(cache, "Random Replacement") {}
TreePseudoLRU::TreePseudoLRU(Cache * cache) : EvictionPolicy(cache, "Tree Pseudo Least Recently Used") {}
BitPseudoLRU::BitPseudoLRU(Cache * cache) : EvictionPolicy(cache, "Bit Pseudo Least Recently Used") {}
FirstInFirstOut::FirstInFirstOut(Cache * cache) : EvictionPolicy(cache, "First In First Out") {}


EvictionPolicy::EvictionPolicy(Cache * cache, string name)
{
	this->cache = cache;
	this->name = name;
	this->metrics = new uint[cache->sets]{ 0 };
}

uint RandomReplacement::FindSlot(uint set)
{
	return rand() % cache->slots;
}

uint TreePseudoLRU::FindSlot(uint set)
{
	uint treeDepth = (uint)log2(cache->slots); // The depth of the bit tree (does not include the actual bits, only the tree that points to it)
	int i = 0; // Indicates the node that we are currently at
	uint traverselDepth = 0; // The depth that we are currently at
	while (traverselDepth < treeDepth) // While we are not done traversing (we're not at the bottom of the tree yet)
	{
		metrics[set] = metrics[set] ^ (1 << i); // Flip the bit of that node

		if ((((metrics[set] ^ (1 << i)) >> i) & 1) == 0) { i = 2 * i + 1; } // If the original value of the nodebit points to the left
		else { i = 2 * i + 2; } // If the original value of the nodebit points to the right
		traverselDepth++; // We need to go deeper
	}
	return i - ((1 << treeDepth) - 1); // We end up somewhere next/outside of the bit tree, so we shift to the right to get our index
}

uint BitPseudoLRU::FindSlot(uint set)
{
	for (uint i = 0; i < cache->slots; i++)
	{
		if (((metrics[set] >> i) & 1) == 0) // If we have found a zero bit (which is garanteed to happen at some point)
		{
			metrics[set] = metrics[set] ^ (1 << i); // Flip that bit
			if (metrics[set] == pow(2, cache->slots) - 1) // If all bits are now set to 1
			{
				metrics[set] = 0; // We reset all bits to 0
				metrics[set] = 0 ^ (1 << i); // Except the one we just flipped
			}
			return i;
		}
	}
}

uint FirstInFirstOut::FindSlot(uint set)
{
	return (++metrics[set]) % cache->slots;
}

Observer::Observer(Cache* cache)
{
	for (int i = 0;i < NLAYERS; i++) {
		caches[i] = cache;
		cache = cache->lowerMemory;
	}
}

// draws bars that display cache hit ratio over time
void Observer::Draw(Surface * screen, uint size)
{
	//draw black background for the graph
	screen->Bar(0, SCRHEIGHT - NLAYERS * size - 1, SCRWIDTH, SCRHEIGHT, 0);
	uint color;
	float y;

	//for each measurement
	for (uint m = 0; m < 300; m++) {
		color = 255;
		y = SCRHEIGHT;
		//for each cache level
		for (int i = 0; i < NLAYERS; i++) {

			// draw a read ratio bar for the current measurement
			float x = m + 50;
			float dy = readRatios[i][(m + ptr + 1) % 300] * size;
			screen->Line(x, y - dy, x, y, color);

			// draw a write ratio bar for the current measurement
			x += SCRWIDTH / 2;
			dy = writeRatios[i][(m + ptr + 1) % 300] * size;
			screen->Line(x, y - dy, x, y, color);

			// update y and color for next cache
			y -= size;
			color <<= 8;
		}
	}

	y = SCRHEIGHT;
	//draw 50% and 100% bar
	for (int i = 0; i < NLAYERS; i++) {
		screen->Line(50, y - (size >> 1), 350, y - (size >> 1), 0xFFFFFFFF);
		screen->Line(450, y - (size >> 1), 750, y - (size >> 1), 0xFFFFFFFF);
		screen->Line(50, y - size, 350, y - size, 0xFFFFFFFF);
		screen->Line(450, y - size, 750, y - size, 0xFFFFFFFF);

		y -= size;
	}

	//draw text
	for (int i = 0; i < NLAYERS; i++) {
		screen->Print(caches[i]->name, 10, SCRHEIGHT - i * size - size / 2, 0xFFFFFFFF);
		screen->Print("R", 10, SCRHEIGHT - i * size - 64 / 2 , 0xFFFFFFFF);
		screen->Print(caches[i]->name, SCRWIDTH / 2 + 10, SCRHEIGHT - i * size - size / 2, 0xFFFFFFFF);
		screen->Print("W", SCRWIDTH / 2 + 10, SCRHEIGHT - i * size - 64 / 2, 0xFFFFFFFF);
	}
}

// write information about the current frame to the console
void Observer::Print(uint frame, float time)
{
	printf("FRAME: %i TIME: %fms POLICY: %s\n", frame, time, caches[0]->policy->name);
	for (int c = 0; c < NLAYERS; c++) {
		printf("%s\n", caches[c]->name);
		printf("Read  hits: %i \t requests: %i \t %f%%\n",
			caches[c]->readHit,
			caches[c]->readReq,
			(float)caches[c]->readHit / caches[c]->readReq
		);
		printf("Write hits: %i \t requests: %i \t %f%%\n",
			caches[c]->writeHit,
			caches[c]->writeReq,
			(float)caches[c]->writeHit / caches[c]->writeReq
		);
	}
	printf("\n");
}

// update the measurements of the observer and reset the statistics of the caches
void Observer::Update()
{
	ptr = (ptr + 1) % 300;
	for (int c = 0; c < NLAYERS; c++) {
		readRatios[c][ptr] = (float)caches[c]->readHit / caches[c]->readReq;
		writeRatios[c][ptr] = (float)caches[c]->writeHit / caches[c]->writeReq;
		caches[c]->readReq = 0;
		caches[c]->writeReq = 0;
		caches[c]->readHit = 0;
		caches[c]->writeHit = 0;
	}
}
