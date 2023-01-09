#ifndef EMERGENCY_THROTTLE_HPP
#define EMERGENCY_THROTTLE_HPP

#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

class Watcher;
typedef SmartPointer< Watcher > WatcherPtr;

class EmergencyThrottle
{
public:
	EmergencyThrottle();

	void update( float numSecondsBehind, float spareTime, float tickPeriod );
	float value() const	{ return value_; }
	float estimatePersistentLoadTime( float persistentLoadTime,
										float throttledLoadTime) const;

	static WatcherPtr pWatcher();

private:
	bool shouldScaleBack( float timeToNextTick, float spareTime ) const;

	bool scaleBack( float fraction );
	bool scaleForward( float fraction );

	float value_;

	bool shouldPrintScaleForward_;
	uint64 startTime_;
	float maxTimeBehind_;
};

BW_END_NAMESPACE

#endif // EMERGENCY_THROTTLE_HPP
