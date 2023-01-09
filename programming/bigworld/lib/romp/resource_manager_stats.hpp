#ifndef RESOURCE_MANAGER_STATS_HPP
#define RESOURCE_MANAGER_STATS_HPP

#if ENABLE_CONSOLES

#include <d3d9types.h>
#include "moo/moo_dx.hpp"
#include "moo/device_callback.hpp"
#include "xconsole.hpp"


BW_BEGIN_NAMESPACE

const int NUM_FRAMES = 1;

/**
 * TODO: to be documented.
 */
class ResourceManagerStats : public Moo::DeviceCallback
{
public:
	static ResourceManagerStats& instance();
	static void fini();
	void displayStatistics( XConsole & console );
	void logStatistics( std::ostream& f_logFile );
	bool enabled() const
	{
		return (available_ && enabled_);
	}
	void createUnmanagedObjects();
	void deleteUnmanagedObjects();
private:
	static ResourceManagerStats* s_instance_;
	ResourceManagerStats();
	~ResourceManagerStats();
	const BW::string& resourceString( uint32 i );
	void getData();
	D3DRESOURCESTATS results_[NUM_FRAMES][D3DRTYPECOUNT];
	DX::Query*	pEventQuery_;
	bool		available_;
	bool		enabled_;
	int			frame_;
};

BW_END_NAMESPACE

#endif // ENABLE_CONSOLES
#endif // RESOURCE_MANAGER_STATS_HPP

