#ifndef DEBUG_MESSAGE_SOURCE_HPP
#define DEBUG_MESSAGE_SOURCE_HPP

#include "bw_namespace.hpp"

BW_BEGIN_NAMESPACE

enum DebugMessageSource
{
	MESSAGE_SOURCE_CPP,
	MESSAGE_SOURCE_SCRIPT,
	NUM_MESSAGE_SOURCE
};

const char * messageSourceAsCString( DebugMessageSource sourceIndex );

BW_END_NAMESPACE

#endif // DEBUG_MESSAGE_SOURCE_HPP
