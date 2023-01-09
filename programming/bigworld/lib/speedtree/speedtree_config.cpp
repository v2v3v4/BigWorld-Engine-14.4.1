#include "pch.hpp"

// Module Interface
#include "speedtree_config.hpp"

// BW Tech Headers
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"

DECLARE_DEBUG_COMPONENT2("SpeedTree", 0)


BW_BEGIN_NAMESPACE

namespace speedtree {

namespace { // anonymous
ErrorCallbackFunc f_errorCallback = NULL;
} // namespace anonymous

// -------------------------------------------------------------- Error Callback

void setErrorCallback(ErrorCallbackFunc callback)
{
	BW_GUARD;
	f_errorCallback = callback;
}


void speedtreeError(const char * treeFilename, const char * errorMsg)
{
	BW_GUARD;
	if (f_errorCallback != NULL)
	{
		f_errorCallback(treeFilename, errorMsg);
	}
	else
	{
		ERROR_MSG("%s: %s\n", treeFilename, errorMsg);
	}
}

} // namespace speedtree

BW_END_NAMESPACE

// speedtree_config.cpp
