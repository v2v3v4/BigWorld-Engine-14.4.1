#include "pch.hpp"
#if SCALEFORM_SUPPORT
#include "log.hpp"

DECLARE_DEBUG_COMPONENT2( "Scaleform", 0 )


BW_BEGIN_NAMESPACE

using namespace ::Scaleform;
namespace ScaleformBW
{
	Log::Log():
logCallback_( NULL )
{
	BW_GUARD;
}


Log::~Log()
{

}


void Log::SetLogHandler(LOGFN callback)
{
	BW_GUARD;
	logCallback_ = callback;
}


void Log::LogMessageVarg( LogMessageId messageId, const char* pfmt, va_list argList )
{
	BW_GUARD;
	if ( messageId & Log_Warning )
		WARNING_MSG( pfmt, argList );
	else if ( messageId & Log_Message )
		INFO_MSG( pfmt, argList );
	else
		ERROR_MSG( pfmt, argList );
}
} // namespace ScaleformBW

BW_END_NAMESPACE

#endif //#if SCALEFORM_SUPPORT
