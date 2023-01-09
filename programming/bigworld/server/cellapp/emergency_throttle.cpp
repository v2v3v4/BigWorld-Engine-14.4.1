#include "emergency_throttle.hpp"

#include "throttle_config.hpp"

#include "cstdmf/timestamp.hpp"
#include "cstdmf/watcher.hpp"

#include "math/mathdef.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
EmergencyThrottle::EmergencyThrottle() :
	value_( 1.f ),

	shouldPrintScaleForward_( false ),
	startTime_( 0 ),
	maxTimeBehind_( 0.f )
{
}


/**
 *	This method updates the throttling value.
 */
void EmergencyThrottle::update( float numSecondsBehind,
		float spareTime, float tickPeriod )
{
	maxTimeBehind_ = std::max( maxTimeBehind_, numSecondsBehind );

	if (this->shouldScaleBack( numSecondsBehind, spareTime ))
	{
		if (this->scaleBack( tickPeriod/ThrottleConfig::scaleBackTime() ))
		{
			ERROR_MSG( "EmergencyThrottle::update: Fully scaled back. "
					"spareTime %f seconds. numSecondsBehind %0.3f seconds\n",
				spareTime, numSecondsBehind );
			shouldPrintScaleForward_ = true;
		}
	}
	else
	{
		if (this->scaleForward( tickPeriod/ThrottleConfig::scaleForwardTime() ))
		{
			if (shouldPrintScaleForward_)
			{
				NOTICE_MSG( "EmergencyThrottle::update: Fully scaled forward. "
						"Scaled back for %.2f seconds. "
						"Max behind: %.3f seconds\n",
					stampsToSeconds( timestamp() - startTime_ ),
					maxTimeBehind_ );

				shouldPrintScaleForward_ = false;
			}

			maxTimeBehind_ = 0.f;
		}
	}
}


/**
 *	This method returns whether or not scaling back should occur.
 */
bool EmergencyThrottle::shouldScaleBack( float numSecondsBehind,
		float spareTime ) const
{
	return (numSecondsBehind > ThrottleConfig::behindThreshold()) ||
			(spareTime < ThrottleConfig::spareTimeThreshold());
}


/**
 *
 */
bool EmergencyThrottle::scaleBack( float fraction )
{
	if (value_ <= ThrottleConfig::min())
	{
		// Already fully scaled back.
		return false;
	}

	if (isEqual( value_, 1.f ))
	{
		// Starting to scale back.
		startTime_ = timestamp();
	}

	float minVal = ThrottleConfig::min();

	value_ = std::max( minVal, value_ - (1.f - minVal) * fraction );

	return isEqual( value_, minVal );
}


/**
 *
 */
bool EmergencyThrottle::scaleForward( float fraction )
{
	if (isEqual( value_, 1.f ))
	{
		// Already fully scaled forward.
		return false;
	}


	value_ = std::min( 1.f,
			value_ + (1.f - ThrottleConfig::min()) * fraction );

	return isEqual( value_, 1.f );
}


/**
 *	This method estimates what the persistent load would be if there was no
 *	throttling.
 */
float EmergencyThrottle::estimatePersistentLoadTime(
		float persistentLoadTime, float throttledLoadTime) const
{
	return persistentLoadTime + (1.f/value_ - 1.f) * throttledLoadTime;
}


/**
 *	This method returns a Watcher that can inspect this object.
 */
WatcherPtr EmergencyThrottle::pWatcher()
{
	DirectoryWatcherPtr pWatcher = new DirectoryWatcher();

#define MY_WATCH( VALUE ) 													\
	pWatcher->addChild( #VALUE,												\
		makeWatcher( &EmergencyThrottle::VALUE##_, Watcher::WT_READ_WRITE ) );

	MY_WATCH( value );

	return pWatcher;
}

BW_END_NAMESPACE

// emergency_throttle.cpp
