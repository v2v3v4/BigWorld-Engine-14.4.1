#ifndef MESSAGE_HANDLERS_HPP
#define MESSAGE_HANDLERS_HPP

#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
	class InputMessageHandler;
	class InterfaceTable;
}


namespace InternalInterfaceHandlers
{
	void init( Mercury::InterfaceTable & interfaceTable );
}

namespace ExternalInterfaceHandlers
{
	void init( Mercury::InterfaceTable & interfaceTable );
}

BW_END_NAMESPACE

#endif // MESSAGE_HANDLERS_HPP
