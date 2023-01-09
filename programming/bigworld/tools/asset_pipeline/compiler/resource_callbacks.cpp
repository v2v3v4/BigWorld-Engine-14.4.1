#include "resource_callbacks.hpp"
#include "resmgr/bwresource.hpp"

BW_BEGIN_NAMESPACE

void ResourceCallbacks::purgeResource( const StringRef & resourceId )
{
	BWResource::instance().purge( resourceId );
}

BW_END_NAMESPACE