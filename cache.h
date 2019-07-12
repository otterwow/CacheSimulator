#pragma once

uint LOADUINT( uint* address );
void STOREUINT( uint* address, uint value );

class EvictionPolicy;

struct CacheLine
{
	CacheLine();
	uint tag;
	uint data[16]; // 64 bytes
	bool valid, dirty;
};

struct CacheLineFetch {
public:
	CacheLineFetch::CacheLineFetch(CacheLine* cacheLine, bool hit)
		: cacheLine(cacheLine), hit(hit) {}
	CacheLine* cacheLine;
	bool hit;
};

class Cache
{
public:
	Cache::Cache(char* name, uint sets, uint slots, Cache* lowerMemory);
	~Cache();
	uint Read32bit( uint address );
	void Write32bit( uint address, uint value );
	CacheLineFetch GetSlot(uint address);
	float GetDummyValue() const { return dummy; }
	char* name;
	Cache* lowerMemory;
	const uint sets, slots;
	CacheLine** cache;
	EvictionPolicy* policy;
	unsigned long readReq, writeReq,
		          readHit, writeHit;

private:
	uint Offset(uint address);
	uint Set(uint address);
	uint Tag(uint address);
	uint offsetBits, setBits, tagBits;
	uint offsetMask, setMask, tagMask;

	float dummy;

protected:
	void LoadLineFromMem(uint address, CacheLine& line);
	void WriteLineToMem(uint address, CacheLine& line);
	virtual void ReadCacheLine(uint address, CacheLine& line);
	virtual void WriteCacheLine(uint address, CacheLine& line);

};

class RAM : public Cache{
public:
	RAM::RAM();
	void ReadCacheLine(uint address, CacheLine & line) override;
	void WriteCacheLine(uint address, CacheLine & line) override;
};


class EvictionPolicy {
public:
	virtual uint FindSlot(uint set) = 0;
	EvictionPolicy(Cache* cache, string name);
	string name;
protected:
	Cache* cache;
	uint* metrics;
};

class RandomReplacement : public EvictionPolicy {
public:
	RandomReplacement(Cache* cache);
	uint FindSlot(uint set) override;
};

class TreePseudoLRU : public EvictionPolicy {
public:
	TreePseudoLRU(Cache* cache);
	uint FindSlot(uint set) override;
};

class BitPseudoLRU : public EvictionPolicy {
public:
	BitPseudoLRU(Cache* cache);
	uint FindSlot(uint set) override;
};

class FirstInFirstOut : public EvictionPolicy {
public:
	FirstInFirstOut(Cache* cache);
	uint FindSlot(uint set) override;
};


class Observer {
public:
	Observer(Cache* cache);
	void Draw(Surface* screen, uint size);
	void Print(uint frame, float time);
	void Update();
private:
	float readRatios[NLAYERS][300]{ 0 };
	float writeRatios[NLAYERS][300]{ 0 };
	Cache* caches[NLAYERS];
	size_t ptr = 0;
};

Cache* GetCache();
Observer* GetObserver();