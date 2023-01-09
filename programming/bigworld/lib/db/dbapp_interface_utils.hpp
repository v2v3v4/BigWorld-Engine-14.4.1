#ifndef DB_APP_INTERFACE_UTILS_HPP
#define DB_APP_INTERFACE_UTILS_HPP

#include "pyscript/script.hpp"
#include "network/channel.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
	class NetworkInterface;
}
class BinaryIStream;
class BinaryOStream;

namespace DBAppInterfaceUtils
{
	bool executeRawDatabaseCommand( const BW::string & command,
		PyObjectPtr pResultHandler, Mercury::Channel & channel );

	bool executeRawDatabaseCommand( const BW::string & command,
		PyObjectPtr pResultHandler, Mercury::NetworkInterface & interface,
		const Mercury::Address & dbAppAddr );
}

BW_END_NAMESPACE

#endif // DB_APP_INTERFACE_UTILS_HPP
