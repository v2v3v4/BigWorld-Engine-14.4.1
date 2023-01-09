#ifndef SCRIPT_BIGWORLD_HPP
#define SCRIPT_BIGWORLD_HPP

#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE

class Bases;

namespace Mercury
{
class EventDispatcher;
class NetworkInterface;
}

// TODO: This is not the right place for these.
enum LogOnAttemptResult
{
	LOG_ON_REJECT = 0,
	LOG_ON_ACCEPT = 1,
	LOG_ON_WAIT_FOR_DESTROY = 2,
};


namespace BigWorldBaseAppScript
{

bool init( const Bases & bases,
		const Bases * pServices,
		Mercury::EventDispatcher & dispatcher,
		Mercury::NetworkInterface & intInterface,
		bool isServiceApp );

}

BW_END_NAMESPACE

#endif // SCRIPT_BIGWORLD_HPP
