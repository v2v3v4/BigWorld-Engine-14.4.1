#ifndef CLIENT_METHOD_CALLING_FLAGS_HPP
#define CLIENT_METHOD_CALLING_FLAGS_HPP

#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE


enum ClientMethodCallingFlags
{
	MSG_FOR_OWN_CLIENT		= 0x01,		///< Send to own client
	MSG_FOR_OTHER_CLIENTS	= 0x02,		///< Send to other clients
	MSG_FOR_REPLAY			= 0x04		///< Record for replay
};


BW_END_NAMESPACE


#endif // CLIENT_METHOD_CALLING_FLAGS_HPP
