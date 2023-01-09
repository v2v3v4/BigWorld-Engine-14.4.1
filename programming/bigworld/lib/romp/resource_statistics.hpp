#ifndef RESOURCE_STATISTICS_HPP
#define RESOURCE_STATISTICS_HPP

#if ENABLE_CONSOLES
#include "console.hpp"
#include "cstdmf/resource_counters.hpp"

#include <iostream>
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class displays resource usage statistics to a console.
 *
 *	Most often, this console can be viewed by pressing \<Ctrl\> + F5
 */
class ResourceStatistics : public ResourceUsageConsole::Handler
{
public:
	~ResourceStatistics() {}
	static ResourceStatistics & instance();

	void cycleGranularity();

private:
	ResourceStatistics();
	ResourceStatistics( const ResourceStatistics& );
	ResourceStatistics& operator=( const ResourceStatistics& );

	void displayResourceStatistics( XConsole & console );

	void dumpToCSV(XConsole & console);

	void printGranularity(XConsole & console, uint hanging);
	void printUsageStatistics(XConsole & console);

	void printResourceTypeBreakdown(XConsole & console);

	// Member variables
private:
	static ResourceStatistics			s_instance_;
	ResourceCounters::GranularityMode	granularityMode_;
	BW::string							lastFileWritten_;
};

BW_END_NAMESPACE

#endif // ENABLE_CONSOLES
#endif // RESOURCE_STATISTICS_HPP
