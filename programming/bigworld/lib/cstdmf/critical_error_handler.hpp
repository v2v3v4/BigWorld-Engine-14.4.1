#ifndef CRITICAL_ERROR_HANDLER_HPP
#define CRITICAL_ERROR_HANDLER_HPP

#include "stdmf.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class serves as base class for classes that want to handle critical
 *  messages in different ways, and also keeps track of the current critical
 *  message handler through its static methods.
 */
class CriticalErrorHandler
{
private:
	static CriticalErrorHandler * handler_;

public:

	enum Result
	{
		ENTERDEBUGGER = 0,
		EXITDIRECTLY
	};

	static CriticalErrorHandler * get();
	static CriticalErrorHandler * set( CriticalErrorHandler * );

	virtual ~CriticalErrorHandler() {}
	virtual Result ask( const char * msg ) = 0;
	virtual void recordInfo( bool willExit ) = 0;
};

void RaiseEngineException(const char* msg);

BW_END_NAMESPACE

#endif // CRITICAL_ERROR_HANDLER_HPP
