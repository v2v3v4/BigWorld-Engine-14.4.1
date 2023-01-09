#include "pch.hpp"
#if ENABLE_CONSOLES

#include "resource_statistics.hpp"
#include "cstdmf/resource_counters.hpp"
#include <sstream>
#include <iomanip>
#include <Psapi.h>


BW_BEGIN_NAMESPACE


/**
 *	Initalise the singleton instance.
 */
ResourceStatistics ResourceStatistics::s_instance_;


/**
 *	Default constructor
 */
ResourceStatistics::ResourceStatistics() :
	granularityMode_(ResourceCounters::GM_MEMORY)
{
	BW_GUARD;
}


/**
 *	Returns the ResourceStatistics singleton instance
 *
 *	@return	Singleton instance
 */
ResourceStatistics& ResourceStatistics::instance()
{
	BW_GUARD;
	return s_instance_;
}


/**
 *	Displays resource usage statistics in the passed console.
 *
 *	@param console	The console the statistics will be displayed in.
 */
void ResourceStatistics::displayResourceStatistics(XConsole & console)
{
	BW_GUARD;
	static const size_t STR_SIZE = 64;
	char statString[STR_SIZE];
	statString[STR_SIZE - 1] = 0;

	// Console title
	console.setCursor(0, 0);
	console.print("Resource Usage Console\n\n");

	// Display last file written
	if (!lastFileWritten_.empty())
		console.print("Last file written: " + lastFileWritten_ + "\n\n");
	else
		console.print("Press the 'c' key to save to a .CSV file\n\n");

	// Display granularity
	console.print("Granularity (SPACE) - ");
	printGranularity(console, 22);

	// Display resource usage statistics
	printUsageStatistics(console);

	// if we want a breakdown, grab gpu info too.
	if(granularityMode_ == ResourceCounters::GM_BREAKDOWN_RESOURCE_TYPE)
	{
		// NOTE: This is done in here as GPU info comes from Moo, and ResourceCounters
		//       is in cstdmf.
		printResourceTypeBreakdown(console);
	}
}


/**
 *	This dumps the current resource statistics to a CSV file.
 */
void ResourceStatistics::dumpToCSV(XConsole & console)
{
	BW_GUARD;
	// Find a free filename:
	size_t cnt = 0;
	BW::string filename;
	while (true)
	{
		BW::ostringstream out;
		out << "resource_stats_" << (int)cnt++ << ".csv";
		filename = out.str();
		FILE *file = bw_fopen(filename.c_str(), "rb");
		bool newFile = file == NULL;
		if (file != NULL)
			fclose(file);
		if (newFile)
			break;
	}

	ResourceCounters::instance().toCSV(filename);
	lastFileWritten_ = filename;
}


/**
 *	Cycles through the granularity modes.
 */
void ResourceStatistics::cycleGranularity()
{
	BW_GUARD;
	granularityMode_ = static_cast<ResourceCounters::GranularityMode>((granularityMode_ + 1) % ResourceCounters::GM_MAX);
}


/**
 *	Prints the current granularity to the passed console using the passed hanging.
 *
 *	@param console	The output console
 *	@param hanging	The hanging to use when outputting
 */
void ResourceStatistics::printGranularity(XConsole & console, uint hanging)
{
	BW_GUARD;
	static BW::string spaces = "                                            ";

	switch (granularityMode_)
	{
	case ResourceCounters::GM_MEMORY:
		{
			console.print("One pool\n");
			console.print(spaces.substr(0, hanging));
			console.print("1 - Memory\n\n\n");
		} break;
	case ResourceCounters::GM_SYSTEM_VIDEO_MISC:
		{
			console.print("Three pools\n");
			console.print(spaces.substr(0, hanging));
			console.print("1 - System Memory\n");
			console.print(spaces.substr(0, hanging));
			console.print("2 - Video Memory\n");
			console.print(spaces.substr(0, hanging));
			console.print("3 - Misc. Memory\n\n\n");
		} break;
	case ResourceCounters::GM_SYSTEM_DEFAULT_MANAGED_MISC:
		{
			console.print("Four pools\n");
			console.print(spaces.substr(0, hanging));
			console.print("1 - System Memory\n");
			console.print(spaces.substr(0, hanging));
			console.print("2 - Default Video Memory\n");
			console.print(spaces.substr(0, hanging));
			console.print("3 - Managed Video Memory\n");
			console.print(spaces.substr(0, hanging));
			console.print("4 - Misc. Memory\n\n\n");
		} break;
	case ResourceCounters::GM_BREAKDOWN_RESOURCE_TYPE:
		{
			console.print("Breakdown by Resource Type\n");
		} break;
	default:
		{
			ERROR_MSG("ResourceStatistics::getGranularity:"
					"Unknown granularity type - '%d'\n",
					granularityMode_ );
		}
	}
}


/**
 *	Prints the current granularity to the passed console using the passed hanging.
 *
 *	@param console	The output console
 */
void ResourceStatistics::printUsageStatistics(XConsole & console)
{
	BW_GUARD;
	console.print(ResourceCounters::instance().getUsageDescription(granularityMode_));
}

/**
 *	Prints a breakdown of resource usage by type.
 *
 *	@param console	The output console
 */
void ResourceStatistics::printResourceTypeBreakdown(XConsole & console)
{
	BW::stringstream ss(BW::stringstream::in | BW::stringstream::out);

	// NOTE: Dedicated & Shared are roughly equivelent to System, and since System is
	//       what we could consistently track for GPU memory, this is what will be used
	//       in the breakdown. This is still not 100% as the driver internally moves
	//       stuff around, but it gives us a good idea of how much GPU System memory
	//       is being consumed.
	Moo::GpuInfo::MemInfo memInfo;
	Moo::rc().getGpuMemoryInfo( &memInfo );

	// Grab current main (private bytes) usage & GPU system mem usage, calculate unaccounted from that.
	// Unaccounted memory is memory which is used, but ResourceCounters has not been
	// told about with a call to 'add'.
	ResourceCounters::MemoryEntry usedMemory(memInfo.privateUsage_, memInfo.systemMemUsed_);
	ResourceCounters::MemoryEntry unaccountedMemory(usedMemory);

	ResourceCounters::Entry entries[ResourceCounters::RT_MAX];


	bw_zero_memory(&entries[0], sizeof(entries));
	const BW::string entryNames[] = 
	{
		"- Texture",
		"- Index Buffer",
		"- Vertex Buffer",
		"- Other"
	};

	BW_STATIC_ASSERT(ARRAY_SIZE(entryNames) == ResourceCounters::RT_MAX, resource_type_size_and_entrynames_mismatch);

	// Grab entries for each resource type.
	for (int resourceTypeIdx = 0; resourceTypeIdx < ResourceCounters::RT_MAX; ++resourceTypeIdx)
	{
		ResourceCounters::Entry& entry(entries[resourceTypeIdx]);
		ResourceCounters::instance().getResourceTypeMemoryUsage((ResourceCounters::ResourceType)resourceTypeIdx, &entry);

		unaccountedMemory -= entry.size_;
	}

	// Clamp unaccounted memory above zero. Sometimes the GPU system usage may not actually
	// use as much as we think. D3D can move data to/from GPU system memory as it pleases.
	unaccountedMemory.clampAboveZero();

	// Main (private set) memory usage.
	ResourceCounters::instance().addEntry( ss, "Virtual Address Space Total",
		memInfo.virtualAddressSpaceTotal_,
		0, 0, 0, 0, 0, 0 );
	ResourceCounters::instance().addEntry( ss, "Virtual Address Space Used",
		memInfo.virtualAddressSpaceUsage_,
		0, 0, 0, 0, 0, 0 );
	ResourceCounters::instance().addEntry( ss, "Main (Private Bytes) Used",
		usedMemory.main_,
		0, 0, 0, 0, 0, 0 );
	ResourceCounters::instance().addEntry( ss, "- Unaccounted",
		unaccountedMemory.main_,
		0, 0, 0, 0, 0, 0 );
	for(int resourceTypeIdx = 0; resourceTypeIdx < ResourceCounters::RT_MAX; ++resourceTypeIdx)
	{
		ResourceCounters::Entry entry(entries[resourceTypeIdx]);
		ResourceCounters::instance().addEntry( ss, entryNames[resourceTypeIdx],
			entry.size_.main_,
			entry.peakSize_.main_, 
			entry.instances_, 
			entry.peakInstances_,
			entry.numberAdds_,
			entry.numberSubs_,
			0);
	}

	// GPU (system) memory usage
	ResourceCounters::instance().addEntry( ss, "GPU (Shared Memory) Total",
		memInfo.sharedMemTotal_,
		0, 0, 0, 0, 0, 0 );
	ResourceCounters::instance().addEntry( ss, "GPU (Shared Memory) Committed",
		memInfo.sharedMemCommitted_,
		0, 0, 0, 0, 0, 0 );
	ResourceCounters::instance().addEntry( ss, "GPU (Dedicated Memory) Total",
		memInfo.dedicatedMemTotal_,
		0, 0, 0, 0, 0, 0 );
	ResourceCounters::instance().addEntry( ss, "GPU (Dedicated Memory) Committed",
		memInfo.dedicatedMemCommitted_,
		0, 0, 0, 0, 0, 0 );
	ResourceCounters::instance().addEntry( ss, "GPU (System Memory) Used",
		usedMemory.gpu_,
		0, 0, 0, 0, 0, 0 );
	ResourceCounters::instance().addEntry( ss, "- Unaccounted",
		unaccountedMemory.gpu_,
		0, 0, 0, 0, 0, 0 );
	for(int resourceTypeIdx = 0; resourceTypeIdx < ResourceCounters::RT_MAX; ++resourceTypeIdx)
	{
		ResourceCounters::Entry entry(entries[resourceTypeIdx]);
		ResourceCounters::instance().addEntry( ss, entryNames[resourceTypeIdx],
			entry.size_.gpu_,
			entry.peakSize_.gpu_, 
			entry.instances_, 
			entry.peakInstances_,
			entry.numberAdds_,
			entry.numberSubs_,
			0);
	}


	console.print(ss.str());
}

BW_END_NAMESPACE
#endif // ENABLE_CONSOLES
// resource_staticstics.cpp

