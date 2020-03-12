#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <unordered_map>
#include <array>
#include <memory_resource>
#include <cstdlib> // for std::byte
#include "tracknew.hpp"

#pragma region Monotonic Memory Resource
void whyRegularAllocationBad() {
	TrackNew::reset();

	std::vector<std::string> coll;
	for (int i = 0; i < 1000; ++i)
		coll.emplace_back("just a non-SSO string");

	TrackNew::status();
}

void aLittleBetterWithPmr() {
	TrackNew::reset();

	// allocate some memory on the stack:
	std::array<std::byte, 200000> buf;

	// and use it as initial memory pool for a vector
	// passing its address and size:
	std::pmr::monotonic_buffer_resource pool{
		buf.data(), buf.size()
	};

	// create a pmr vector that takes the memory resource
	// for all its allocations
	std::pmr::vector<std::string> coll2{ &pool };

	for (int i = 0; i < 1000; ++i) {
		coll2.emplace_back("just a non-SSO string");
	}

	TrackNew::status();
}

void dontAllocateOnTheHeapAtAll() {
	// define the pmr::vector to be of type pmr::string to avoid allocation to the heap
	TrackNew::reset();

	// allocate some memory on the stack
	std::array<std::byte, 200000> buf;

	// and use it as initial memory pool for a vector and its strings:
	std::pmr::monotonic_buffer_resource pool{ buf.data(), buf.size() };
	std::pmr::vector<std::pmr::string> coll{ &pool };

	for (int i = 0; i < 1000; i++) {
		coll.emplace_back("just a non-SSO string");
	}

	TrackNew::status();

	// output: 0 allocations for 0 bytes

	/*
		The reason for this is that by default a pmr vector tries to propogate
		its allocator to its elements. This is not succssful when the elements
		don't use a polymorphic allocator, as is the case with type std::string.
		However, by using type std::pmr::string, which is a string using a
		polymorphic allocator, the propogation works fine.
	*/
}

void reUsingMemoryPools() {
	// allocate some memory on the stack
	std::array<std::byte, 200000> buf;

	for (int num : {1000, 2000, 3000, 4000, 5000}) {
		std::cout << "-- check with  " << num << " elements\n";
		TrackNew::reset();

		std::pmr::monotonic_buffer_resource pool{ buf.data(), buf.size() };
		std::pmr::vector<std::pmr::string> col{ &pool };

		for (int i = 0; i < num; ++i) {
			col.emplace_back("just a non-SSO string");
		}

		TrackNew::status();
	}


	static std::pmr::synchronized_pool_resource myPool;

	// set myPool as new default memory resource:
	std::pmr::memory_resource* old = std::pmr::set_default_resource(&myPool);

	// restore old default memory resource as default:
	std::pmr::set_default_resource(old);
}
#pragma endregion

#pragma region Synchronized Memory Pools
std::pmr::memory_resource* initGlobMemResource()
{
	static std::pmr::synchronized_pool_resource g_MemoryResource;

	// (same as above)
	// static std::pmr::synchronized_pool_resource myPool{ std::pmr::get_default_resource() };
	return &g_MemoryResource;
}

// one application of synchronized pools is to ensure elements in a node
// based container are located next to each other. This may also increase
// performance of the containers significantly, because then CPU caches 
// load elements together in cache lines. However, it depends on the implem-
// entation of the memory resource. If the memory resource uses a mutex to 
// synchronise memory access, performance will take a significant hit.
void exampleSyncPoolBadImpl() {
	std::map<long, std::string> coll;

	for (int i = 0; i < 10; ++i) {
		std::string s{ "Customer" + std::to_string(i) };
		coll.emplace(i, s);
	}

	// print element distance:
	for (const auto& elem : coll) {
		static long long lastVal = 0;
		long long val = reinterpret_cast<long long>(&elem);
		std::cout << "diff: " << (val - lastVal) << '\n';
		lastVal = val;
	}
}

/*
	With this function example, the elements are now located close to each
	other. Still, they are not located in one chunk of memory. When the
	pool finds out that the first chink is not big enough for all the elements,
	it allocates more memory for even more elements. Thus, the more memory
	we allocate, the larger the chunks of memory are so that more elements
	are located close to each other.
*/
void betterExampleSyncPool() {
	std::pmr::synchronized_pool_resource pool;
	std::pmr::map<long, std::pmr::string> coll{ &pool };

	for (int i = 0; i < 10; ++i) {
		std::string s{ "Customer" + std::to_string(i) };
		coll.emplace(i, s);
	}

	/*
	The largest allocation size that is required to be fulfilled using the pooling
	mechanism. Attempts to allocate a single block larger than this threshold will
	be allocated directly from the upstream std::pmr::memory_resource. If
	largest_required_pool_block is zero or is greater than an implementation-defined limit,
	that limit is used instead. The implementation may choose a pass-through threshold
	larger than specified in this field.
	*/
	std::cout << "Largest required pool block: " << pool.options().largest_required_pool_block << '\n';

	/*
	The maximum number of blocks that will be allocated at once from the upstream
	std::pmr::memory_resource to replenish the pool. If the value of max_blocks_per_chunk
	is zero or is greater than an implementation-defined limit, that limit is used instead.
	The implementation may choose to use a smaller value than is specified in this field and
	may use different values for different pools.
	*/
	std::cout << "Max blocks per chunk: " << pool.options().max_blocks_per_chunk << '\n';

	// print element distances:
	for (const auto& elem : coll) {
		static long long lastVal = 0;
		long long val = reinterpret_cast<long long>(&elem);
		std::cout << "diff: " << (val - lastVal) << '\n';
		lastVal = val;
	}
}
#pragma endregion

#pragma region Monotonic Memory Resource Cont.
// You should prefer this resource if you know you will have no deletes
// or you have memory to waste (don't worry, you never will).
void skipDeallocations() {
	// use default memory resource but skip deallocations as long as the pool lives.
	std::pmr::monotonic_buffer_resource pool;
	std::pmr::vector<std::pmr::string> coll{ &pool };

	for (int i = 0; i < 100; i++) {
		coll.emplace_back("just a non-SSO string");
	}
	coll.clear(); // destruction but no deallocation
}

void chainMemRes() {
	/*
	Create a pool for all of our memory, which never deallocates
	as long as it lives and is initialized by starting to allocate
	10000 bytes (allocated using the default memory resource)
	*/
	std::pmr::monotonic_buffer_resource keepAllocatedPool{ 10000 };
	// Create another pool that uses this non-deallocating pool to allocate chunks of memory
	std::pmr::synchronized_pool_resource pool{ &keepAllocatedPool };

	/*
	Combined effect is that we have a pool for all our memory, allocates more
	memory with little fragmentation if neccessary, and can be used by all
	pmr objects using the pool.
	*/

	for (int j = 0; j < 100; ++j) {
		std::pmr::vector<std::pmr::string> coll{ &pool };
		for (int i = 0; i < 100; i++)
		{
			coll.emplace_back("just a non-SSO string");
		}
	} // deallocations are given back to the pool, but not deallocated
	// so far nothing was deallocated
} // deallocates all allocated memory
#pragma endregion

#pragma region Null_Memory_Resource
void exampleNMR() {
	/*
	Null_memory_resource handles allocations in a way that each 
	allocation throws a bad_alloc exception. The most important
	application is to ensure that a memory pool using memory
	allocated on the stack does not suddenly allocate on the heap
	if it needs more.
	*/

	// use memory on the stack without fallback on the heap:
	std::array<std::byte, 200000> buf;
	std::pmr::monotonic_buffer_resource pool{ buf.data(), buf.size(), std::pmr::null_memory_resource() };

	// and allocate too much memory
	std::pmr::unordered_map<long, std::pmr::string> coll{ &pool };
	try {
		for (int i = 0; i < buf.size(); ++i) {
			std::string a{ "Customer" + std::to_string(i) };
			coll.emplace(i, a);
		}
	}
	catch (const std::bad_alloc & e) {
		std::cerr << "BAD ALLOC EXCEPTION: " << e.what() << '\n';
	}
	std::cout << "size: " << coll.size() << '\n';
}
#pragma endregion

#pragma region Custom_Memory_Resources
class Tracker : public std::pmr::memory_resource
{
private:
	std::pmr::memory_resource* upstream;
	std::string prefix{};

public:
	// we wrap the passed or default resource
	explicit Tracker(std::pmr::memory_resource* us
		= std::pmr::get_default_resource())
		: upstream{ us } {
	}

	explicit Tracker(std::string p,
		             std::pmr::memory_resource* us = std::pmr::get_default_resource())
		: prefix{ std::move(p) }, upstream{ us } {
	}

private:
	void* do_allocate(size_t bytes, size_t alignment) override {
		std::cout << prefix << " allocate " << bytes << " Bytes\n";
		void* ret = upstream->allocate(bytes, alignment);
		return ret;
	}

	void do_deallocate(void* ptr, size_t bytes, size_t alignment) override {
		std::cout << prefix << " deallocate " << bytes << " Bytes\n";
		upstream->deallocate(ptr, bytes, alignment);
	}

	/* Determines if one polymorphic memory resource object can deallocate 
	   memory allocated by another */
	bool do_is_equal(const std::pmr::memory_resource& other) const noexcept
		override {
		if (this == &other) return true;
		auto op = dynamic_cast<const Tracker*>(&other);
		return op != nullptr && op->prefix == prefix
			&& upstream->is_equal(other);
	}
};
#pragma endregion


int main() {
	{
		// track allocating chunks of memory (starting with 10k) without deallocating:
		Tracker track1{ "keeppool:" };
		std::pmr::monotonic_buffer_resource keeppool{ 10000, &track1 };
		{
			Tracker track2{ "  syncpool", &keeppool };
			std::pmr::synchronized_pool_resource pool{ &track2 };

			for (int j = 0; j < 100; ++j) {
				std::pmr::vector<std::pmr::string> coll{ &pool };
				coll.reserve(100);
				for (int i = 0; i < 100; ++i) {
					coll.emplace_back("just a non-SSO string");
				}
				if (j == 2) std::cout << "--- third iteration done\n";
			}// deallocations are given back to the pool, but not deallocated
			std::cout << "--- leave scope of pool\n";
		} // so far nothing was deallocated
		std::cout << "--- leave scope of keeppool\n";
	} // deallocates all allocated memory
	return 0;
}