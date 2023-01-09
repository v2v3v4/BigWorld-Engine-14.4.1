#ifndef BASEAPPMGR_UTIL_HPP
#define BASEAPPMGR_UTIL_HPP

#include "cstdmf/shared_ptr.hpp"
#include "network/basictypes.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class BaseApp;

typedef shared_ptr< BaseApp > BaseAppPtr;
typedef BW::map< Mercury::Address, BaseAppPtr > BaseApps;

enum AdjustBackupLocationsOp
{
	ADJUST_BACKUP_LOCATIONS_OP_ADD,
	ADJUST_BACKUP_LOCATIONS_OP_RETIRE,
	ADJUST_BACKUP_LOCATIONS_OP_CRASH
};

BW_END_NAMESPACE

#endif // BASEAPPMGR_UTIL_HPP
