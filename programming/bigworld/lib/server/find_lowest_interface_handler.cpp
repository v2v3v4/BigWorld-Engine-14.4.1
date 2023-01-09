#include "find_lowest_interface_handler.hpp"

#include "network/machine_guard.hpp"

BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
FindLowestInterfaceHandler::FindLowestInterfaceHandler() :
	Mercury::MachineDaemon::IFindInterfaceHandler(),
	map_()
{
}


/*
 * Override from Mercury::IFindInterfaceHandler
 */
bool FindLowestInterfaceHandler::onProcessMatched( Mercury::Address & addr,
		const ProcessStatsMessage & psm )
{
	map_[psm.id_] = addr;
	return /* shouldContinue */ true;
}


/*
 * Override from IFindInterfaceHandler
 */
Mercury::Address FindLowestInterfaceHandler::result() const
{
	return map_.empty() ? Mercury::Address::NONE : map_.begin()->second;
}


BW_END_NAMESPACE

// find_lowest_interface_handler.cpp
