#ifndef SCALEFORM_LOG_HPP
#define SCALEFORM_LOG_HPP
#if SCALEFORM_SUPPORT

#include "config.hpp"

BW_BEGIN_NAMESPACE

using namespace ::Scaleform;
namespace ScaleformBW
{
	typedef void (*LOGFN)(const char*);
	const uint32 MAXLINE = 256;

	/**
	 *	This class subclasses the GFxLog class, and provides integration
	 *	between GFx logging and BW logging services.
	 */
	class Log : public GFx::Log
	{
	public:
		Log();
		~Log();

		typedef Log This;

		void SetLogHandler(LOGFN callback);

		//GFXLog virtual fns
		void LogMessageVarg( LogMessageId messageType, const char* pfmt, va_list argList );

	private:
		LOGFN logCallback_;
	};
} //namespace ScaleformBW

BW_END_NAMESPACE

#endif // #if SCALEFORM_SUPPORT
#endif // SCALEFORM_LOG_HPP
