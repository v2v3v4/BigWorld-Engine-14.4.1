#ifndef CONSOLIDATE_DBS__SLUGGISH_PROGRESS_REPORTER_HPP
#define CONSOLIDATE_DBS__SLUGGISH_PROGRESS_REPORTER_HPP

#include "dbapp_status_reporter.hpp"

#include "cstdmf/timestamp.hpp"


BW_BEGIN_NAMESPACE

/**
 * 	This is a base class for progress reporters. reportProgress() can be called
 * 	very often, but reportProgressNow() will only be called once every
 * 	reportInterval.
 */
class SluggishProgressReporter
{
public:
	/**
	 *	Constructor.
	 */
	SluggishProgressReporter( DBAppStatusReporter & reporter,
			float reportInterval = 0.5 ) : // Half a second
		reporter_( reporter ),
		reportInterval_( uint64( reportInterval * stampsPerSecondD() ) ),
		lastReportTime_( timestamp() )
	{}

	void reportProgress()
	{
		uint64 now = timestamp();
		if ((now - lastReportTime_) > reportInterval_)
		{
			this->reportProgressNow();
			lastReportTime_ = now;
		}
	}

	// Should be overridden by derived class
	virtual void reportProgressNow() = 0;

protected:
	DBAppStatusReporter & reporter()
		{ return reporter_; }

private:
	DBAppStatusReporter & reporter_;

	uint64	reportInterval_;
	uint64	lastReportTime_;
};

BW_END_NAMESPACE

#endif // CONSOLIDATE_DBS__SLUGGISH_PROGRESS_REPORTER_HPP
