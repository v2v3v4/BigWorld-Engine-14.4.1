#ifndef ACCESS_MONITOR_HPP
#define ACCESS_MONITOR_HPP

// Standard Library Headers.
#include "cstdmf/bw_string.hpp"

// Standard MF Library Headers.
#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE

/**
 *	The class is a singleton that accepts commands to dump file names in
 *	a format that is easily recognisable by a resource monitoring script.
 *	It presents a single point of change for formatting, and its behaviour
 *	can be turned on or off with a single flag - useful for command-line
 *	changes to its behaviour.
 *
 *	By default, it is inactive.
 */
class AccessMonitor
{
public:
	///	@name Constructor.
	//@{
	AccessMonitor();
	//@}

	///	@name Control Methods.
	//@{
	void record( const BW::string &fileName );

	void active( bool flag );

	static AccessMonitor &instance();
	//@}

private:
	bool active_;
};


#ifdef CODE_INLINE
#include "access_monitor.ipp"
#endif

BW_END_NAMESPACE

#endif // ACCESS_MONITOR_HPP

// access_monitor.cpp

