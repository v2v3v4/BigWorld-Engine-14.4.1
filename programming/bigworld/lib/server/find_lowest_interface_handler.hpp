#ifndef FIND_LOWEST_INTERFACE_HANDLER
#define FIND_LOWEST_INTERFACE_HANDLER

#include "cstdmf/bw_map.hpp"

#include "network/basictypes.hpp"
#include "network/machined_utils.hpp"

BW_BEGIN_NAMESPACE

class ProcessStatsMessage;

/**
 *	Finds the Interface with the lowest ID.
 */
class FindLowestInterfaceHandler:
	public Mercury::MachineDaemon::IFindInterfaceHandler
{
public:
	FindLowestInterfaceHandler();

	bool onProcessMatched( Mercury::Address & addr,
			const ProcessStatsMessage & psm ); /* override */

	Mercury::Address result() const; /* override */

private:
	typedef BW::map< DBAppID, Mercury::Address > Map;
	Map map_;

};

BW_END_NAMESPACE

#endif // FIND_LOWEST_INTERFACE_HANDLER
