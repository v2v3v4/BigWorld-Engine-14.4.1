#include "pch.hpp"

#include "unpacked_message_header.hpp"

#include "interface_table.hpp"
#include "network_interface.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: UnpackedMessageHeader
// -----------------------------------------------------------------------------

/**
 *	This method returns the name of the message.
 */
const char * UnpackedMessageHeader::msgName() const
{
	return pInterfaceElement ? pInterfaceElement->name() : "";
}

}

BW_END_NAMESPACE

// unpacked_message_header.hpp
