#include "pch.hpp"

#include "cstdmf/log_msg.hpp"

#include "cstdmf/debug_message_priority.hpp"

//#include "cstdmf/bw_string.hpp"
//#include "cstdmf/memory_stream.hpp"
//#include "cstdmf/stdmf_minimal.hpp"
//
//#include <float.h>
//#include <limits.h>

BW_BEGIN_NAMESPACE


#define CONSTRUCT_LOG_MSG( PRIORITY ) 			\
{												\
	LogMsg logMsg( PRIORITY );					\
	LogMsg * pLogMsg = new LogMsg( PRIORITY );	\
	delete pLogMsg;								\
}
	

TEST( BasicLogMsgConstruction )
{
		CONSTRUCT_LOG_MSG( MESSAGE_PRIORITY_TRACE );
		CONSTRUCT_LOG_MSG( MESSAGE_PRIORITY_DEBUG );
		CONSTRUCT_LOG_MSG( MESSAGE_PRIORITY_INFO );
		CONSTRUCT_LOG_MSG( MESSAGE_PRIORITY_NOTICE );
		CONSTRUCT_LOG_MSG( MESSAGE_PRIORITY_WARNING );
		CONSTRUCT_LOG_MSG( MESSAGE_PRIORITY_ERROR );
		CONSTRUCT_LOG_MSG( MESSAGE_PRIORITY_CRITICAL );
		CONSTRUCT_LOG_MSG( MESSAGE_PRIORITY_HACK );
		CONSTRUCT_LOG_MSG( MESSAGE_PRIORITY_ASSET );
}

BW_END_NAMESPACE

// test_log_msg.cpp
