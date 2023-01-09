#include "network/common_interface_macros.hpp"

#undef MF_EMPTY_PROXY_MSG
#define MF_EMPTY_PROXY_MSG( NAME )											\
	MERCURY_HANDLED_EMPTY_MESSAGE( NAME,									\
			NoBlockProxyMessageHandler< void >,								\
			&Proxy::NAME )													\

#undef MF_BEGIN_PROXY_MSG
#define MF_BEGIN_PROXY_MSG( NAME )											\
	BEGIN_HANDLED_STRUCT_MESSAGE( NAME,										\
			NoBlockProxyMessageHandler< INTERFACE_NAME::NAME##Args >,	\
			&Proxy::NAME )													\

#undef MF_BEGIN_UNBLOCKABLE_PROXY_MSG
#define MF_BEGIN_UNBLOCKABLE_PROXY_MSG MF_BEGIN_PROXY_MSG

#undef MF_EMPTY_BLOCKABLE_PROXY_MSG
#define MF_EMPTY_BLOCKABLE_PROXY_MSG( NAME )								\
	MERCURY_HANDLED_EMPTY_MESSAGE( NAME,									\
			ProxyMessageHandler< void >,									\
			&Proxy::NAME )													\

#undef MF_BEGIN_BLOCKABLE_PROXY_MSG
#define MF_BEGIN_BLOCKABLE_PROXY_MSG( NAME )								\
	BEGIN_HANDLED_STRUCT_MESSAGE( NAME,										\
			ProxyMessageHandler< INTERFACE_NAME::NAME##Args >,			\
			&Proxy::NAME )													\

#define MF_VARLEN_PROXY_MSG( NAME ) 										\
	MERCURY_HANDLED_VARIABLE_MESSAGE( NAME, 2, 								\
			ProxyVarLenMessageHandler<false>, &Proxy::NAME )

#define MF_VARLEN_UNBLOCKABLE_PROXY_MSG MF_VARLEN_PROXY_MSG

#undef MF_VARLEN_BLOCKABLE_PROXY_MSG
#define MF_VARLEN_BLOCKABLE_PROXY_MSG( NAME )								\
	MERCURY_HANDLED_VARIABLE_MESSAGE( NAME, 2,								\
			ProxyVarLenMessageHandler< true >,								\
			&Proxy::NAME )

// TODO: Should the message be blockable?
#undef MF_METHOD_RANGE_MSG
#define MF_METHOD_RANGE_MSG( NAME, RANGE_PORTION )							\
	MF_VARLEN_PROXY_MSG( NAME )												\
	MERCURY_METHOD_RANGE_MSG( NAME, RANGE_PORTION )

// common_baseapp_interface.hpp
