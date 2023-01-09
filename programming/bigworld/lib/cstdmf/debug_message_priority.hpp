#ifndef DEBUG_MESSAGE_PRIORITY_HPP
#define DEBUG_MESSAGE_PRIORITY_HPP

#include <iostream>

#include "cstdmf/stdmf_minimal.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This enumeration is used to indicate the priority of a message. The higher
 *	the enumeration's value, the higher the priority.
 */
enum DebugMessagePriority
{
	MESSAGE_PRIORITY_TRACE,
	MESSAGE_PRIORITY_DEBUG,
	MESSAGE_PRIORITY_INFO,
	MESSAGE_PRIORITY_NOTICE,
	MESSAGE_PRIORITY_WARNING,
	MESSAGE_PRIORITY_ERROR,
	MESSAGE_PRIORITY_CRITICAL,
	MESSAGE_PRIORITY_HACK,
	DEPRECATED_PRIORITY_SCRIPT,
	MESSAGE_PRIORITY_ASSET,
	NUM_MESSAGE_PRIORITY,
	FORCE_32_BITS = 0x7fffffff
};

const char * messagePrefix( DebugMessagePriority p );

std::ostream& operator<<( std::ostream &s, const DebugMessagePriority &v );
std::istream& operator>>( std::istream &s, DebugMessagePriority &v );

BW_END_NAMESPACE

// We always inline these implementations
#include "debug_message_priority.ipp"

#endif // DEBUG_MESSAGE_PRIORITY_HPP
