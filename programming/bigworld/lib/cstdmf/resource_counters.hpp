#ifndef RESOURCE_COUNTER_HPP
#define RESOURCE_COUNTER_HPP

#include "config.hpp"
#include "stdmf.hpp"
#include "concurrency.hpp"

#include "cstdmf/bw_map.hpp"
#include <utility>
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

/**
 *	The ResourceCounters class is a simple class that tracks the memory usage of a component
 *	based on a description string (the component allocating/deallocating the memory) and a
 *	memory pool enumeration (the memory pool to be used, i.e. system memory, managed memory,
 *	or default memory). The main two worker methods of this class are add and subtract. These
 *	methods track memory allocations and deallocations of a component using the description-pool.
 *	The ResourceCounters class maintains a map of the components that are being memory tracked.
 *	Rather than calling the methods of ResourceCounters directly, the exposed MACROS below should
 *	be used.
 */
class ResourceCounters
{
public:
	typedef unsigned int							Handle;

	/**
	 * Granularity mode. Will show breakdown of memory usage in different ways.
	 */
	enum GranularityMode
	{
		GM_MEMORY,							// All memory.
		GM_SYSTEM_VIDEO_MISC,				// System, video (default + managed combined), misc.
		GM_SYSTEM_DEFAULT_MANAGED_MISC,		// System, default, managed, misc.
		GM_BREAKDOWN_RESOURCE_TYPE,			// Breakdown by resource type.

		GM_MAX
	};

	/**
	 * Memory pool that the resource belongs in.
	 * NOTE: Maps to D3DPOOL to save converting pool type from D3D resources.
	 */
	enum MemoryPool	
	{
		MP_DEFAULT = 0,
		MP_MANAGED = 1,
		MP_SYSTEM = 2,
		MP_SCRATCH = 3,			// Unused, but added since it is part of D3DPOOL.

		MP_MAX
	};

	/**
	 *  Type of resource this is.
	 */
	enum ResourceType
	{
		RT_TEXTURE = 0,
		RT_INDEX_BUFFER,
		RT_VERTEX_BUFFER,
		RT_OTHER,

		RT_MAX,
		RT_INVALID = 0x7fffffff
	};

	/**
	 * Memory pool masks, used for enabling/disabling in reporting.
	 */
	enum MemoryPoolMask
	{
		MPM_DEFAULT_MASK =		1 << MP_DEFAULT,
		MPM_MANAGED_MASK =		1 << MP_MANAGED,
		MPM_SYSTEM_MASK =		1 << MP_SYSTEM,
		MPM_SCRATCH_MASK =		1 << MP_SCRATCH,

		MPM_MISC_MASK =			1 << MP_MAX
	};

	enum MemoryType
	{
		MT_MAIN = 0,
		MT_GPU,

		MT_MAX
	};

	struct CSTDMF_DLL MemoryEntry
	{
		int64			main_;
		int64			gpu_;

		MemoryEntry();
		MemoryEntry(int64 value);
		MemoryEntry(int64 main, int64 gpu);
		MemoryEntry& operator = (const MemoryEntry& other);
		MemoryEntry& operator += (const MemoryEntry& other);
		MemoryEntry& operator -= (const MemoryEntry& other);
		MemoryEntry operator + (const MemoryEntry& other) const;
		MemoryEntry operator - (const MemoryEntry& other) const;
		MemoryEntry operator / (const MemoryEntry& other) const;
		MemoryEntry operator * (const MemoryEntry& other) const;
		void clampAboveZero();
	};

	struct CSTDMF_DLL Entry
	{
		MemoryEntry		size_;
		MemoryEntry		peakSize_;
		MemoryEntry		sum_;
		size_t			instances_;
		size_t			peakInstances_;
		size_t			numberAdds_;
		size_t			numberSubs_;

		Entry();
	};


	struct DescriptionPool
	{
		DescriptionPool( const BW::string description,
		                 uint memoryPool,							// Converts from D3DPOOL too, so take as uint and handle it.
		                 ResourceType resourceType );

		bool operator < ( const DescriptionPool& Other ) const;

		BW::string description_;
		MemoryPool memoryPool_;
		ResourceType resourceType_;
	};

	CSTDMF_DLL static ResourceCounters& instance();
	~ResourceCounters() { s_bValid_ = false; }

	MemoryEntry add(const DescriptionPool& descriptionPool, size_t amount, size_t gpuAmount);
	MemoryEntry subtract(const DescriptionPool& descriptionPool, size_t amount, size_t gpuAmount);

	CSTDMF_DLL BW::string getUsageDescription(GranularityMode granularityMode);

	CSTDMF_DLL bool toCSV(const BW::string &filename);

	Handle newHandle(const BW::string& description);
	BW::string description(Handle handle) const;

	static bool isValid() { return s_bValid_; }

	bool printPoolContents(uint pool);

	CSTDMF_DLL void addEntry(BW::stringstream& ss,
	              BW::string name,
	              uint64 size,
	              uint64 peakSize,
	              uint64 instances,
	              uint64 peakInstances,
	              uint64 numberAdds,
	              uint64 numberSubs,
	              uint64 average);

	CSTDMF_DLL void getResourceTypeMemoryUsage(ResourceType resourceType, Entry* entry);

private:


	typedef BW::map< DescriptionPool, Entry >		ResourceCountMap;

	static ResourceCounters		s_instance_;
	static bool					s_bValid_;
	ResourceCountMap			resourceCounts_;

	typedef ResourceCountMap::const_iterator		Iterator;

	ResourceCounters();

	MemoryEntry operator[](const DescriptionPool& descriptionPool) const;

	Iterator begin() const;
	Iterator end() const;

	BW::map<int, BW::string> getDescriptionsMap();

	void addHeader(std::ostream& out);
	void addTotalUsage(BW::stringstream& ss, BW::map<int, BW::string>& descriptions, uint pool);
	void addMemorySubtree(BW::stringstream& ss, BW::map<int, BW::string>& descriptions, uint pool);

	BW::string setIndentation(uint pool, uint depth);
	BW::string parseDescription(BW::string desc, uint depth);

	typedef BW::map<Handle, BW::string>				ResourceHandles;
	typedef ResourceHandles::const_iterator				ResourceHandlesConstIter;

	ResourceHandles	resourceHandles_;
	Handle			lastHandle_;
	SimpleMutex		accessMutex_;
};


#if ENABLE_RESOURCE_COUNTERS

#define RESOURCE_COUNTER_ADD(DESCRIPTION_POOL, AMOUNT, GPUAMOUNT)			  \
	if (ResourceCounters::instance().isValid())								  \
		ResourceCounters::instance().add(DESCRIPTION_POOL, (size_t)(AMOUNT), (size_t)(GPUAMOUNT)); 

#define RESOURCE_COUNTER_SUB(DESCRIPTION_POOL, AMOUNT, GPUAMOUNT)			  \
	if (ResourceCounters::instance().isValid())								  \
		ResourceCounters::instance().subtract(DESCRIPTION_POOL, (size_t)(AMOUNT), (size_t)(GPUAMOUNT)); 

#define RESOURCE_COUNTER_NEWHANDLE(DESCRIPTION)								  \
	((ResourceCounters::instance().isValid())								  \
		? ResourceCounters::instance().newHandle(DESCRIPTION)				  \
		: (unsigned int)-1);

#else

#define RESOURCE_COUNTER_ADD(DESCRIPTION_POOL, AMOUNT, GPUAMOUNT)
#define RESOURCE_COUNTER_SUB(DESCRIPTION_POOL, AMOUNT, GPUAMOUNT)
#define RESOURCE_COUNTER_NEWHANDLE(DESCRIPTION)

#endif // ENABLE_RESOURCE_COUNTERS

BW_END_NAMESPACE

#endif // RESOURCE_COUNTER_HPP
