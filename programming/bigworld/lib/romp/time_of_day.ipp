// timeofday.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


INLINE float TimeOfDay::startTime() const
{
	return startTime_;
}

INLINE void TimeOfDay::startTime( float time )
{
	startTime_ = time;
}


INLINE float TimeOfDay::secondsPerGameHour() const
{
	return secondsPerGameHour_;
}

INLINE void TimeOfDay::secondsPerGameHour( float t )
{
	secondsPerGameHour_ = t;
	this->timeConfigChanged();
}


INLINE float TimeOfDay::gameTime() const
{
	return time_;
}

INLINE void TimeOfDay::gameTime( float t )
{
	time_ = fmodf( t + 2400.f, 24.f );
	this->timeConfigChanged();
}


INLINE float TimeOfDay::sunAngle() const
{
	return sunAngle_;
}

INLINE void TimeOfDay::sunAngle( float angle )
{
	sunAngle_ = angle;
}

INLINE Vector3 TimeOfDay::sunColor() const
{
	return sunColor_;
}


INLINE void TimeOfDay::sunColor(Vector3 color)
{
	sunColor_ = color;
	sunAndMoonNeedUpdate_ = true;
}


INLINE Vector3 TimeOfDay::moonColor() const
{
	return moonColor_;
}


INLINE void TimeOfDay::moonColor(Vector3 color)
{
	moonColor_ = color;
	sunAndMoonNeedUpdate_ = true;
}

INLINE float TimeOfDay::moonAngle() const
{
	return moonAngle_;
}

INLINE void TimeOfDay::moonAngle( float angle )
{
	moonAngle_ = angle;
}


INLINE const OutsideLighting & TimeOfDay::lighting() const
{
	return now_;
}

INLINE OutsideLighting & TimeOfDay::lighting()
{
	return now_;
}

INLINE void TimeOfDay::updateNotifiersOn( bool on )
{
	updateNotifiersOn_ = on;
}


// time_of_day.ipp