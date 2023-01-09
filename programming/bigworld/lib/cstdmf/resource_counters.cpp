#include "pch.hpp"

#include "resource_counters.hpp"
#include "bw_util.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <d3d9types.h>

BW_BEGIN_NAMESPACE

/**
 *	Initialise static member variables
 */
ResourceCounters 	ResourceCounters::s_instance_;
bool				ResourceCounters::s_bValid_ = false;

/**
 * DescriptionPool constructor.
 */

ResourceCounters::DescriptionPool::DescriptionPool( const BW::string description,
                                                    uint memoryPool,
                                                    ResourceType resourceType ):
	description_( description ),
	memoryPool_( static_cast< MemoryPool >( memoryPool ) ),
	resourceType_( resourceType )
{
	assert( memoryPool < MP_MAX );
	assert( resourceType < RT_MAX );
}

/**
 * DescriptionPool operator <.
 */
bool ResourceCounters::DescriptionPool::operator < ( const DescriptionPool& Other ) const
{
	return ( description_ < Other.description_ ) ||
	       ( !( Other.description_ < description_ ) && memoryPool_ < Other.memoryPool_ ) ||
		   ( !( Other.description_ < description_ ) && !( Other.memoryPool_ < memoryPool_ ) && resourceType_ < Other.resourceType_ );
}

ResourceCounters::MemoryEntry::MemoryEntry():
	main_(0),
	gpu_(0)

{

}

ResourceCounters::MemoryEntry::MemoryEntry(int64 value):
	main_(value),
	gpu_(value)
{

}

ResourceCounters::MemoryEntry::MemoryEntry(int64 main, int64 gpu):
	main_(main),
	gpu_(gpu)
{

}

ResourceCounters::MemoryEntry& ResourceCounters::MemoryEntry::operator = (const ResourceCounters::MemoryEntry& other)
{
	main_ = other.main_;
	gpu_ = other.gpu_;
	return *this;
}

ResourceCounters::MemoryEntry& ResourceCounters::MemoryEntry::operator += (const ResourceCounters::MemoryEntry& other)
{
	main_ += other.main_;
	gpu_ += other.gpu_;
	return *this;
}

ResourceCounters::MemoryEntry& ResourceCounters::MemoryEntry::operator -= (const ResourceCounters::MemoryEntry& other)
{
	main_ -= other.main_;
	gpu_ -= other.gpu_;
	return *this;
}

ResourceCounters::MemoryEntry ResourceCounters::MemoryEntry::operator + (const ResourceCounters::MemoryEntry& other) const
{
	return MemoryEntry(main_ + other.main_, gpu_ + other.gpu_);
}

ResourceCounters::MemoryEntry ResourceCounters::MemoryEntry::operator - (const ResourceCounters::MemoryEntry& other) const
{
	return MemoryEntry(main_ - other.main_, gpu_ - other.gpu_);
}

ResourceCounters::MemoryEntry ResourceCounters::MemoryEntry::operator * (const ResourceCounters::MemoryEntry& other) const
{
	return MemoryEntry(main_ * other.main_, gpu_ * other.gpu_);
}

ResourceCounters::MemoryEntry ResourceCounters::MemoryEntry::operator / (const ResourceCounters::MemoryEntry& other) const
{
	return MemoryEntry(main_ / other.main_, gpu_ / other.gpu_);
}

void ResourceCounters::MemoryEntry::clampAboveZero()
{
	main_ = max( main_, (int64)0 );
	gpu_ = max( gpu_, (int64)0 );
}

/**
 *	Initialises a ResourceEntry with zero values.
 */
ResourceCounters::Entry::Entry():
	size_(0, 0),
	peakSize_(0, 0),
	sum_(0, 0),
	instances_(0),
	peakInstances_(0),
	numberAdds_(0),
	numberSubs_(0)
{
}

/**
 *	Returns the resource counters singleton instance.
 *
 *	@returns ResourceCounters	Singleton instance
 */
/*static*/ ResourceCounters &ResourceCounters::instance()
{
	return s_instance_;
}


/**
 *	Adds an amount to the passed resource.
 *
 *  @param descriptionPool	The resource and pool to add too.
 *  @param amount			The amount to add to the given resource.
 */
ResourceCounters::MemoryEntry ResourceCounters::add(const DescriptionPool& descriptionPool, size_t amount, size_t gpuAmount)
{
	SimpleMutexHolder smh( accessMutex_ );

	Entry &entry = resourceCounts_[descriptionPool];
	
	if (amount != 0)
	{	
		++entry.numberAdds_;
		
		++entry.instances_;
		entry.instances_ > entry.peakInstances_ ? entry.peakInstances_ = entry.instances_ : 0;
		
		entry.size_.main_ += amount;
		if(entry.size_.main_ > entry.peakSize_.main_)
		{
			entry.peakSize_.main_ = entry.size_.main_;
		}

		entry.size_.gpu_ += gpuAmount;
		if(entry.size_.gpu_ > entry.peakSize_.gpu_)
		{
			entry.peakSize_.gpu_ = entry.size_.gpu_;
		}

		entry.sum_.main_ += amount;
		entry.sum_.gpu_ += gpuAmount;
	}
	
	return entry.size_;
}


/**
 *	Subtracts an amount from the passed resource.
 *
 *  @param descriptionPool	The resource and pool to subtract from.
 *  @param amount			The amount to subtract from the given resource.
  */
ResourceCounters::MemoryEntry ResourceCounters::subtract(const DescriptionPool& descriptionPool, size_t amount, size_t gpuAmount)
{
	SimpleMutexHolder smh( accessMutex_ );

	Entry &entry = resourceCounts_[descriptionPool];
	
	if (amount != 0)
	{
		++entry.numberSubs_;

		// Shouldn't get negative instances
		if (entry.instances_ > 0)
			--entry.instances_;

		// Shouldn't get negative memory allocated
		if ((int64)amount <= entry.size_.main_)
		{
			entry.size_.main_ -= amount;
		}

		if ((int64)gpuAmount <= entry.size_.gpu_)
		{
			entry.size_.gpu_ -= gpuAmount;
		}
	}
	
	return entry.size_;
}


bool ResourceCounters::printPoolContents( uint pool )
{
	ResourceCountMap::const_iterator it = this->resourceCounts_.begin();
	DEBUG_MSG("Resource pool contents: (pool id %d)\n", pool);
	for (; it != this->resourceCounts_.end(); it++)
	{
		const DescriptionPool& descPool = (*it).first;
		const Entry& entry = (*it).second;
		if (descPool.memoryPool_ == pool)
		{
			if (entry.instances_ > 0)
			{
				WARNING_MSG("\tResource (%d): %s\n",
								entry.instances_,
								descPool.description_.c_str());
			}
		}
	}
	return true;
}

/**
 *	Returns the current allocation for the given resource.
 *
 *  @param descriptionPool	The resource to get.
 *  @returns				The current allocation for the given resource.
 */
ResourceCounters::MemoryEntry ResourceCounters::operator[](const DescriptionPool& descriptionPool) const
{
	ResourceCounters *myself = const_cast<ResourceCounters *>(this);
	if (myself->resourceCounts_.find(descriptionPool) == myself->resourceCounts_.end())
	{
		return ResourceCounters::MemoryEntry(0, 0);
	}
	else
	{
		return myself->resourceCounts_[descriptionPool].size_;
	}
}


/**
 *	This allows iteration over the resources being counted.
 *
 *  @returns				An iterator at the beginning of the resources
 *							being counted.
 */
ResourceCounters::Iterator ResourceCounters::begin() const
{
	return resourceCounts_.begin();
}


/**
 *	This allows iteration over the resources being counted.
 *
 *  @returns				An iterator at the end of the resources
 *							being counted.
 */
ResourceCounters::Iterator ResourceCounters::end() const
{
	return resourceCounts_.end();
}


/**
 *	Returns a map of the descriptions while ignoring pool.
 *
 *  @returns				A map of descriptions.
 */
BW::map<int, BW::string> ResourceCounters::getDescriptionsMap()
{
	uint count = 0;
	BW::map<int, BW::string> descriptions;
	Iterator it = resourceCounts_.begin();

	// Loop through the resources
	while (it != resourceCounts_.end())
	{
		const DescriptionPool& descPool = it->first;
		BW::map<int, BW::string>::iterator it2 = descriptions.begin();
		bool found = false;

		// Search for the description in the descriptions map
		while (it2 != descriptions.end())
		{
			if (it2->second == descPool.description_)
			{
				found = true;
				break;
			}

			it2++;
		}

		// Check if the description is already in the description map
		if (!found)
			descriptions[count++] = descPool.description_;

		it++;
	}

	return descriptions;
}


/**
 *	Returns a string description of the current memory usage.
 *
 *	@param granularityMode	The granularity mode to be used to categorise the resource usage.
 *  @returns				String describing the current memory usage.
 */
BW::string ResourceCounters::getUsageDescription(GranularityMode granularityMode)
{
	// Create an output file stream
	BW::stringstream ss(BW::stringstream::in | BW::stringstream::out);

	// Print the file header to the stream
	addHeader(ss);

	// Get descriptions map
	BW::map<int, BW::string> descriptionsMap = getDescriptionsMap();

	// Add total usage
	addTotalUsage(ss, descriptionsMap, MPM_SYSTEM_MASK | MPM_DEFAULT_MASK | MPM_MANAGED_MASK | MPM_MISC_MASK);	

	// Create the appropriate subtrees based on granularity
	switch (granularityMode)
	{
	case GM_MEMORY:
		{
			addMemorySubtree(ss, descriptionsMap, MPM_SYSTEM_MASK | MPM_DEFAULT_MASK | MPM_MANAGED_MASK | MPM_MISC_MASK);
		} break;
	case GM_SYSTEM_VIDEO_MISC:
		{
			addTotalUsage(ss, descriptionsMap, MPM_SYSTEM_MASK);
			addMemorySubtree(ss, descriptionsMap, MPM_SYSTEM_MASK);
			ss << std::endl;
			addTotalUsage(ss, descriptionsMap, MPM_DEFAULT_MASK | MPM_MANAGED_MASK);
			addMemorySubtree(ss, descriptionsMap, MPM_DEFAULT_MASK | MPM_MANAGED_MASK);
			ss << std::endl;
			addTotalUsage(ss, descriptionsMap, MPM_MISC_MASK);
			addMemorySubtree(ss, descriptionsMap, MPM_MISC_MASK);
		} break;
	case GM_SYSTEM_DEFAULT_MANAGED_MISC:
		{
			addTotalUsage(ss, descriptionsMap, MPM_SYSTEM_MASK);
			addMemorySubtree(ss, descriptionsMap, MPM_SYSTEM_MASK);
			ss << std::endl;
			addTotalUsage(ss, descriptionsMap, MPM_DEFAULT_MASK);
			addMemorySubtree(ss, descriptionsMap, MPM_DEFAULT_MASK);
			ss << std::endl;
			addTotalUsage(ss, descriptionsMap, MPM_MANAGED_MASK);
			addMemorySubtree(ss, descriptionsMap, MPM_MANAGED_MASK);
			ss << std::endl;
			addTotalUsage(ss, descriptionsMap, MPM_MISC_MASK);
			addMemorySubtree(ss, descriptionsMap, MPM_MISC_MASK);
		} break;
	case GM_BREAKDOWN_RESOURCE_TYPE: 
		{
		// NOTE: Handled in resource_statistics.cpp, it knows about Moo.
		} break;
	default:
		{
			ERROR_MSG(	"ResourceCounters::getUsageDescription : "
						"Unknown granularity type - '%d'\n",
						granularityMode );
		}
	}

	return ss.str();
}


/**
 *	Prints the file header to fileName.
 *
 *  @param out			The stream the header is written to.
 */
void ResourceCounters::addHeader(std::ostream& out)
{
	out << "                                    |Usage         |Peak          |Insts  |Peak   |Allocs  |De-     |Average     " << std::endl;
	out << "Name                                |              |Usage         |       |Insts  |        |allocs  |            " << std::endl;
	out << "-----------------------------------------------------------------------------------------------------------------" << std::endl;
}


/**
 *	Adds the total usage for the passed memory pool to the passed stringstream.
 *
 *  @param ss			The stream to write to.
 *  @param descriptions	The map of descriptions.
 *  @param pool			The usage memory pool.
 */
void ResourceCounters::addTotalUsage(	BW::stringstream& ss,
										BW::map<int, BW::string>& descriptions,
										uint pool)
{
	MemoryEntry size;
	MemoryEntry peakSize;
	MemoryEntry sum;
	size_t instances = 0;
	size_t peakInstances = 0;
	size_t numberAdds = 0;
	size_t numberSubs = 0;

	// First level is total memory usage
	Iterator it = resourceCounts_.begin();

	// Loop through the resources adding up total memory usage statistics
	while (it != resourceCounts_.end())
	{
		const DescriptionPool& descPool = it->first;
		Entry const &entry = it->second;

		if (	((pool & MPM_SYSTEM_MASK) && descPool.memoryPool_ == MP_SYSTEM) ||
				((pool & MPM_DEFAULT_MASK) && descPool.memoryPool_ == MP_DEFAULT) ||
				((pool & MPM_MANAGED_MASK) && descPool.memoryPool_ == MP_MANAGED) ||
				((pool & MPM_SCRATCH_MASK) && descPool.memoryPool_ == MP_SCRATCH) ||
				(	(pool & MPM_MISC_MASK) &&
					(descPool.memoryPool_ != MP_SYSTEM) &&
					(descPool.memoryPool_ != MP_DEFAULT) &&
					(descPool.memoryPool_ != MP_MANAGED) &&
					(descPool.memoryPool_ != MP_SCRATCH)
				)
			)
		{
			size          += entry.size_;
			peakSize      += entry.peakSize_;
			instances     += entry.instances_;
			peakInstances += entry.peakInstances_;
			numberAdds    += entry.numberAdds_;
			numberSubs    += entry.numberSubs_;
			sum           += entry.sum_;
		}

		it++;
	}
	MemoryEntry average;
	if (numberAdds != 0)
	{
		average = sum / numberAdds;
	}

	BW::string desc;
	if ((pool & MPM_DEFAULT_MASK) &&
		(pool & MPM_MANAGED_MASK) &&
		(pool & MPM_SYSTEM_MASK) &&
		(pool & MPM_MISC_MASK))
		desc = "Total usage";
	else if (	(pool & MPM_DEFAULT_MASK) &&
				(pool & MPM_MANAGED_MASK))
		desc = "   Video";
	else if (pool & MPM_SYSTEM_MASK)
		desc = "   System";
	else if (pool & MPM_DEFAULT_MASK)
		desc = "   Default";
	else if (pool & MPM_MANAGED_MASK)
		desc = "   Managed";
	else if (pool & MPM_MISC_MASK)
		desc = "   Misc";
	else
		ERROR_MSG("ResourceCounters::addTotalUsage : Unknown memory pool!");

	// Output statistics
	std::ios_base::fmtflags old_options = ss.flags(std::ios_base::left);
	ss << std::setw(36) << std::setfill(' ') << desc << " ";
	ss.flags(old_options);
	
	old_options = ss.flags(std::ios_base::right);

	ss	<< formatNumber(size.main_              , 14) << ' '
		<< formatNumber(peakSize.main_          , 14) << ' '
		<< formatNumber(instances               ,  7) << ' '
		<< formatNumber(peakInstances           ,  7) << ' '
		<< formatNumber(numberAdds              ,  8) << ' '
		<< formatNumber(numberSubs              ,  8) << ' '
		<< formatNumber(average.main_           , 12) << std::endl;

	ss.flags(old_options);
}


/**
 *	Adds the usage subtree for the passed memory pool to the passed stringstream.
 *
 *  @param ss			The stream to write to.
 *  @param descriptions	The map of descriptions.
 *  @param pool			The usage memory pool.
 */
void ResourceCounters::addMemorySubtree(BW::stringstream& ss,
										BW::map<int, BW::string>& descriptions,
										uint pool)
{
	uint depth = 1;
	uint lastDrawnDepth = 1;
	bool hasChildren = false;
	BW::string::size_type delimiter = 0;
	BW::string::size_type delimiter2 = 0;
	BW::string desc = "";
	BW::string previousDesc = "";
	MemoryEntry size;
	MemoryEntry peakSize;
	MemoryEntry sum;
	size_t instances = 0;
	size_t peakInstances = 0;
	size_t numberAdds = 0;
	size_t numberSubs = 0;
	
	// Loop through the descriptions
	BW::map<int, BW::string>::iterator it = descriptions.begin();
	while (it != descriptions.end())
	{
		delimiter = 0;
		delimiter2 = 0;

		previousDesc = desc;
		desc = it->second;

		if (!hasChildren && depth > 1)
		{
			depth = 1;

			while (true)
			{
				delimiter = desc.find("/");
				delimiter2 = previousDesc.find("/");
					
				if (delimiter != BW::string::npos &&
					delimiter2 != BW::string::npos)
				{
					// Compare substrings
					if (desc.substr(0, delimiter) == 
						previousDesc.substr(0, delimiter2))
					{
						desc = desc.substr(delimiter+1, desc.size());
						previousDesc = previousDesc.substr(delimiter+1, previousDesc.size());
						depth++;
					}
					else
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
		}
		
		delimiter2 = 0;
		hasChildren = false;
		desc = it->second;

		// Parse the depth-th token from the description
		for (uint i = 0; i < depth; i++)
		{
			delimiter = desc.find("/");
			 if (delimiter != BW::string::npos)
			{
				desc = desc.substr(delimiter+1, desc.size());
				delimiter2 += delimiter;
			}
			else
			{
				break;
			}
		}

		if (delimiter != BW::string::npos)
		{
			desc = it->second.substr(0, delimiter2 + 1);
		}
		else
		{
			desc = it->second;
		}

		size = 0;
		peakSize = 0;
		instances = 0;
		peakInstances = 0;
		numberAdds = 0;
		numberSubs = 0;
		sum = 0;

		// Loop through the resources adding all statistics that match this root
		Iterator res_it = resourceCounts_.begin();

		while (res_it != resourceCounts_.end())
		{
			const DescriptionPool& descPool = res_it->first;
			Entry const &entry = res_it->second;

			if ((res_it->first.description_.find(desc) == 0 && delimiter != BW::string::npos) ||
				(res_it->first.description_.find(desc) == 0 && delimiter == BW::string::npos && res_it->first.description_.size() == it->second.size()))
			{
				if (res_it->first.description_ != desc)
					hasChildren = true;

				if (	((pool & MPM_SYSTEM_MASK) && descPool.memoryPool_ == MP_SYSTEM) ||
						((pool & MPM_DEFAULT_MASK) && descPool.memoryPool_ == MP_DEFAULT) ||
						((pool & MPM_MANAGED_MASK) && descPool.memoryPool_ == MP_MANAGED) ||
						((pool & MPM_SCRATCH_MASK) && descPool.memoryPool_ == MP_SCRATCH) ||
						(	(pool & MPM_MISC_MASK) &&
							(descPool.memoryPool_ != MP_SYSTEM) &&
							(descPool.memoryPool_ != MP_DEFAULT) &&
							(descPool.memoryPool_ != MP_MANAGED) &&
							(descPool.memoryPool_ != MP_SCRATCH)
						)
					)
				{
					size += entry.size_;
					peakSize += entry.peakSize_;
					sum += entry.sum_;
					instances += entry.instances_;
					peakInstances += entry.peakInstances_;
					numberAdds += entry.numberAdds_;
					numberSubs += entry.numberSubs_;
				}
			}

			res_it++;
		}

		//if (size)  // Commented out to prevent the list jumping
		{
			if (lastDrawnDepth > depth)
			{
				ss << std::endl;
			}

			lastDrawnDepth = depth;

			// Output statistics
			BW::string name = setIndentation(pool, depth) + parseDescription(desc, depth);
			MemoryEntry average = 0;
			if (numberAdds != 0)
			{
				average = sum / MemoryEntry(numberAdds);
			}

			addEntry(ss,
			         name,
			         size.main_,
			         peakSize.main_,
			         instances,
			         peakInstances,
			         numberAdds,
			         numberSubs,
			         average.main_);
		}

		if (hasChildren)
		{
			depth++;
			continue;
		}

		it++;
	}
}

/**
 * Get total resource type memory usage.
 */
void ResourceCounters::getResourceTypeMemoryUsage(ResourceType resourceType, Entry* entry)
{
	assert(entry != NULL);

	entry->size_ = MemoryEntry(0, 0);
	entry->peakSize_ = MemoryEntry(0, 0);
	entry->sum_ = MemoryEntry(0, 0);
	entry->instances_ = 0;
	entry->peakInstances_ = 0;
	entry->numberAdds_ = 0;
	entry->numberSubs_ = 0;

	// Loop through the resources adding all statistics that match this root
	Iterator res_it = resourceCounts_.begin();

	while (res_it != resourceCounts_.end())
	{
		const DescriptionPool& descriptionPool = res_it->first;
		Entry const &resourceEntry = res_it->second;

		if (descriptionPool.resourceType_ == resourceType)
		{
			entry->size_ += resourceEntry.size_;
			entry->peakSize_ += resourceEntry.peakSize_;
			entry->sum_ += resourceEntry.sum_;
			entry->instances_ += resourceEntry.instances_;
			entry->peakInstances_ += resourceEntry.peakInstances_;
			entry->numberAdds_ += resourceEntry.numberAdds_;
			entry->numberSubs_ += resourceEntry.numberSubs_;
		}

		res_it++;
	}
}

/**
 *	Add entry to the console. Hides away formatting.
 *  @param ss String stream to write into.
 *	@param name Name of entry.
 *	@param size Size of resource.
 *	@param peakSize Peak size of resource.
 *	@param instances Instances.
 *	@param peakInstances Peak instance.
 *	@param numberAdds Number of instances added to this resource total. (confirm this)
 *	@param numberSubs Number of instances removed from this resource total. (confirm this)
 *	@param average Average size of instance size.
 */
void ResourceCounters::addEntry(BW::stringstream& ss,
			                    BW::string name,
			                    uint64 size,
			                    uint64 peakSize,
			                    uint64 instances,
			                    uint64 peakInstances,
			                    uint64 numberAdds,
			                    uint64 numberSubs,
			                    uint64 average)
{
	std::ios_base::fmtflags old_options = ss.flags(std::ios_base::left);
	ss << std::setw(36) << std::setfill(' ') << name << " ";
	ss.flags(old_options);

	old_options = ss.flags(std::ios_base::right);

	ss	<< formatNumber(size           , 14) << ' '
		<< formatNumber(peakSize       , 14) << ' '
		<< formatNumber(instances      ,  7) << ' '
		<< formatNumber(peakInstances  ,  7) << ' '
		<< formatNumber(numberAdds     ,  8) << ' '
		<< formatNumber(numberSubs     ,  8) << ' '
		<< formatNumber(average        , 12) << std::endl;

	ss.flags(old_options);
}

/**
 *	Sets the indentation for the current pool and depth.
 *
 *  @param pool			The usage memory pool.
 *  @param depth		The current depth.
 */
BW::string ResourceCounters::setIndentation(uint pool, uint depth)
{
	BW::string indent;
	if ((pool & MPM_DEFAULT_MASK) &&
		(pool & MPM_MANAGED_MASK) &&
		(pool & MPM_SYSTEM_MASK) &&
		(pool & MPM_MISC_MASK))
		indent = "";
	else
		indent = "   ";
	
	for (uint i = 0; i < depth; i++)
	{
		indent += "   ";
	}

	return indent;
}

/**
 *	Parses the description for the current depth.
 *
 *  @param desc			The current description.
 *  @param depth		The current depth.
 */
BW::string ResourceCounters::parseDescription(BW::string desc, uint depth)
{
	BW::string::size_type delimiter = 0;
	BW::string result = desc;

	for (uint i = 0; i < depth; i++)
	{
		delimiter = desc.find("/");
		if (delimiter != BW::string::npos)
		{
			result = desc.substr(0, delimiter);
			desc = desc.substr(delimiter+1, desc.size());			
		}
		else
		{
			result = desc;
			break;
		}
	}

	return result;
}


bool ResourceCounters::toCSV(const BW::string &filename)
{
	std::ofstream out(filename.c_str());

	out << "Name, Size, Peak size, Instances, Peak Instances, ";
	out << "Number adds, Number subs, Sum" << std::endl;

	for 
	(
		ResourceCountMap::iterator it = resourceCounts_.begin();
		it != resourceCounts_.end();
		++it
	)
	{
		const DescriptionPool& descPool = (*it).first;
		const Entry& entry = (*it).second;

		out << descPool.description_    << ',';
		out << entry.size_.main_		<< ',';
		out << entry.peakSize_.main_	<< ',';
		out << entry.instances_			<< ',';
		out << entry.peakInstances_		<< ',';
		out << entry.numberAdds_		<< ',';
		out << entry.numberSubs_		<< ',';
		out << entry.sum_.main_			<< std::endl;
	}

	out.close();

	return true;
}


/**
 *	This is the default constructor for the ResourceCounters class.
 */
ResourceCounters::ResourceCounters():
	lastHandle_(0)
{
	resourceHandles_[lastHandle_] = "Unknown memory";

	s_bValid_ = true;
}


/**
 *	Returns a handle for the passed description.
 *
 *  @param description		The description to get a handle for.
 *  @returns				A handle to the passed description.
 */
ResourceCounters::Handle ResourceCounters::newHandle(const BW::string& description)
{
	// Iterate through the map searching for the descriptionPool value
	for (uint i = 0; i <= lastHandle_; i++)
	{
		if (resourceHandles_[i] == description)
			return i;
	}

	// This is a new description pool
	Handle result = ++lastHandle_;
	resourceHandles_[result] = description;
	return result;
}


/**
 *	Returns a handle for the passed description.
 *
 *  @param handle			The handle to get a description for.
 *  @returns				The description of the passed handle.
 */
BW::string ResourceCounters::description(Handle handle) const
{
	ResourceHandlesConstIter iter = resourceHandles_.find(handle);
	if (iter == resourceHandles_.end())
		return BW::string();
	else
		return iter->second;
}

BW_END_NAMESPACE
