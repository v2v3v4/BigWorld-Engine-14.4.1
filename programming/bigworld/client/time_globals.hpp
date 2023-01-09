#ifndef TIME_GLOBALS_HPP
#define TIME_GLOBALS_HPP


BW_BEGIN_NAMESPACE

/*
 Wraps up the "Client Settings/Time of Day" and "Client Settings/Secs Per
 Hour" watcher values in a Singleton class. This way, they can reflect the
 current camera space.
 */
class TimeGlobals
{
private:
	TimeGlobals();

public:
	static TimeGlobals & instance();
	void setupWatchersFirstTimeOnly();
	TimeOfDay * getTimeOfDay() const;
	BW::string timeOfDayAsString() const;
	void timeOfDayAsString( BW::string newTime );
	float secondsPerGameHour() const;
	void secondsPerGameHour( float t );
};

BW_END_NAMESPACE

#endif // TIME_GLOBALS_HPP

// time_globals.hpp
