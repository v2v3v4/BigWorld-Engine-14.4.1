
#include "pch.hpp"

#include "adaptive_lod_controller.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "adaptive_lod_controller.ipp"
#endif

AdaptiveLODController::AdaptiveLODController( int smoothingPeriod )
:minimumFPS_( 10.f ),
 filterIdx_( 0 ),
 maxFilterIdx_( smoothingPeriod ),
 filterSum_( 0.f ),
 numSamples_( 0 ),
 effectiveFPS_( 0.f )
{
	BW_GUARD;
	framesPerSecond_ = new float[ maxFilterIdx_ ];
	memset( framesPerSecond_, 0, sizeof( float ) * maxFilterIdx_ );
}


AdaptiveLODController::~AdaptiveLODController()
{
	BW_GUARD;
	bw_safe_delete_array(framesPerSecond_);
}


///Call fpsTick to update the level of detail of the application
void
AdaptiveLODController::fpsTick( float fps )
{
	BW_GUARD;
	effectiveFPS_ = filter( fps );

	LodControllerVector::iterator it = lodControllers_.begin();
	LodControllerVector::iterator end = lodControllers_.end();

	while( it != end )
	{
		LODController & l = *it++;

		if ( effectiveFPS_ < minimumFPS_ )
		{
			//head towards worst_
			l.current_ -= l.dValue_;
		}
		else if ( effectiveFPS_ > minimumFPS_ + 5.f )
		{
			//head towards default_
			l.current_ += l.dValue_;
		}

		//clamp
		if ( l.dValue_ > 0.f )
		{
			if ( l.current_ < l.worst() )
				l.current_ = l.worst();
			if ( l.current_ > l.defaultValue() )
				l.current_ = l.defaultValue();
		}
		else
		{
			if ( l.current_ > l.worst() )
				l.current_ = l.worst();
			if ( l.current_ < l.defaultValue() )
				l.current_ = l.defaultValue();
		}
	}
}


///filter performs a simple moving average of the instantaneous fps values
float
AdaptiveLODController::filter( float instantaneousFPS )
{
	BW_GUARD;
	//remove the effect of the previous value from the circular buffer
	filterSum_ -= framesPerSecond_[ filterIdx_ ];
	//add the effect of the new value
	framesPerSecond_[ filterIdx_++ ] = instantaneousFPS;
	filterSum_ += instantaneousFPS;

	if ( numSamples_ < maxFilterIdx_ )
		numSamples_ ++;

	if ( filterIdx_ == maxFilterIdx_ )
		filterIdx_ = 0;

	return ( filterSum_ / (float)numSamples_ );
}


std::ostream& operator<<(std::ostream& o, const AdaptiveLODController& t)
{
	BW_GUARD;
	o << "AdaptiveLODController\n";
	return o;
}

BW_END_NAMESPACE

/*adaptive_lod_controller.cpp*/
