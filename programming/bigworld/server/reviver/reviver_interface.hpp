#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef REVIVER_INTERFACE_HPP
#endif

#ifndef REVIVER_INTERFACE_HPP
#define REVIVER_INTERFACE_HPP

#include "network/interface_macros.hpp"


// -----------------------------------------------------------------------------
// Section: Helper macro
// -----------------------------------------------------------------------------

#define BW_REVIVER_MSGS( COMPONENT )										\
	MERCURY_FIXED_MESSAGE( handle##COMPONENT##Birth,						\
							sizeof( Mercury::Address ),						\
							&g_reviverOf##COMPONENT )						\
	MERCURY_FIXED_MESSAGE( handle##COMPONENT##Death,						\
							sizeof( Mercury::Address ),						\
							&g_reviverOf##COMPONENT )						\


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Reviver interface
// -----------------------------------------------------------------------------

#pragma pack(push, 1)
BEGIN_MERCURY_INTERFACE( ReviverInterface )

	BW_REVIVER_MSGS( CellAppMgr )
	BW_REVIVER_MSGS( BaseAppMgr )
	BW_REVIVER_MSGS( DBAppMgr )
	BW_REVIVER_MSGS( DBApp )
	BW_REVIVER_MSGS( Login )

END_MERCURY_INTERFACE()
#pragma pack(pop)

// Cleanup
#undef BEGIN_REVIVER_MSG
#undef BW_REVIVER_MSGS

BW_END_NAMESPACE

#endif // REVIVER_INTERFACE_HPP
