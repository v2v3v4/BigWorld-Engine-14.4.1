#include "pch.hpp"

#include "error_reporter.hpp"

#include "basictypes.hpp"
#include "event_dispatcher.hpp"

#include "nub_exception.hpp"

#include "cstdmf/timestamp.hpp"

#include <sstream>

DECLARE_DEBUG_COMPONENT2( "Network", 0 )

BW_BEGIN_NAMESPACE

namespace Mercury
{

namespace
{
/**
 *	The minimum time that an exception can be reported from when it was first
 *	reported.
 */
const uint ERROR_REPORT_MIN_PERIOD_MS = 2000; // 2 seconds

/**
 *	The nominal maximum time that a report count for a Mercury address and
 *	error is kept after the last raising of the error.
 */
const uint ERROR_REPORT_COUNT_MAX_LIFETIME_MS = 10000; // 10 seconds


void outputErrorString( const AddressAndReason & addressAndReason )
{
	ERROR_MSG( "Exception occurred: '%s' on '%s'\n", 
		reasonToString( addressAndReason.second ),
		addressAndReason.first.c_str() );
}


void outputErrorString( const AddressAndReason & addressAndReason,
		const ErrorReportAndCount & reportAndCount,
		const uint64 & now )
{
	uint64 deltaStamps = now - reportAndCount.lastReportStamps;
	double deltaMillis = 1000 * stampsToSeconds( deltaStamps );

	ERROR_MSG( "%d reports of 'Exception occurred: %s' "
			"on '%s' in the last %0.00fms\n",
		reportAndCount.count, 
		addressAndReason.first.c_str(),
		reasonToString( addressAndReason.second ),
		deltaMillis );
}

} // anonymous namespace

/**
 *	Constructor.
 */
ErrorReporter::ErrorReporter( EventDispatcher & dispatcher ) :
	reportLimitTimerHandle_(),
	errorsAndCounts_()
{
	// report any pending exceptions every so often
	reportLimitTimerHandle_ = dispatcher.addTimer(
		ERROR_REPORT_MIN_PERIOD_MS * 1000, this, NULL, "ErrorReporter" );
}


/**
 *	Destructor.
 */
ErrorReporter::~ErrorReporter()
{
	reportLimitTimerHandle_.cancel();
}


/**
 *
 */
void ErrorReporter::reportException( Reason reason,
		const Address & addr )
{
	this->addReport( addr, reason );
}


/**
 *	Adds a new error message for an address to the reporter count map.
 *	Emits an error message if there has been no previous equivalent error
 *	string provider for this address.
 */
void ErrorReporter::addReport( const Address & address, Reason reason )
{
	AddressAndReason addressError( address, reason );
	ErrorReportAndCount & reportAndCount = errorsAndCounts_[ addressError ];

	uint64 now = timestamp();
	reportAndCount.lastRaisedStamps = now;

	// see if we have ever reported this error
	if (reportAndCount.lastReportStamps > 0)
	{
		uint64 millisSinceLastReport = 1000 *
			(now - reportAndCount.lastReportStamps) * stampsPerSecond();

		reportAndCount.count++;

		if (millisSinceLastReport >= ERROR_REPORT_MIN_PERIOD_MS)
		{
			outputErrorString( addressError, reportAndCount, now );
			reportAndCount.count = 0;
			reportAndCount.lastReportStamps = now;
		}

	}
	else
	{
		outputErrorString( addressError );

		reportAndCount.lastReportStamps = now;
	}
}


/**
 *	Output all exception's reports that have not yet been output.
 */
void ErrorReporter::reportPendingExceptions( bool reportBelowThreshold )
{
	uint64 now = timestamp();

	ErrorsAndCounts::iterator exceptionCountIter =
				this->errorsAndCounts_.begin();
	while (exceptionCountIter != this->errorsAndCounts_.end())
	{
		// check this iteration's last report and see if we need to output
		// anything
		const AddressAndReason & addressError =
			exceptionCountIter->first;
		ErrorReportAndCount & reportAndCount = exceptionCountIter->second;

		uint64 millisSinceLastReport = 1000 *
			(now - reportAndCount.lastReportStamps) * stampsPerSecond();
		if (reportBelowThreshold ||
				millisSinceLastReport >= ERROR_REPORT_MIN_PERIOD_MS)
		{
			if (reportAndCount.count)
			{
				outputErrorString(
						addressError,
						reportAndCount, now );
				reportAndCount.count = 0;
				reportAndCount.lastReportStamps = now;

			}
		}

		// see if we can remove this mapping if it has not been raised in a
		// while
		uint64 sinceLastRaisedMillis = 1000 * 
			(now - reportAndCount.lastRaisedStamps) * stampsPerSecond();
		if (sinceLastRaisedMillis > ERROR_REPORT_COUNT_MAX_LIFETIME_MS)
		{
			errorsAndCounts_.erase( exceptionCountIter++ );
		}
		else
		{
			++exceptionCountIter;
		}
	}
}


/**
 *	This method handles the timer event and checks to see if any delayed
 *	messages should be reported.
 */
void ErrorReporter::handleTimeout( TimerHandle handle, void * arg )
{
	MF_ASSERT( handle == reportLimitTimerHandle_ );

	this->reportPendingExceptions();
}

} // namespace Mercury

BW_END_NAMESPACE

// error_reporter.cpp
