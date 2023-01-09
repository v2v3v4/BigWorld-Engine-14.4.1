#ifndef CRITICAL_HANDLER_HPP
#define CRITICAL_HANDLER_HPP

#include "cstdmf/debug_message_callbacks.hpp"
#include "cstdmf/dprintf.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This functor is called when a critical message is printed.
 */
struct CriticalHandler : public CriticalMessageCallback
{
	void handleCritical( const char * msg );
};

BW_END_NAMESPACE

#endif // CRITICAL_HANDLER_HPP

// critical_handler.hpp
