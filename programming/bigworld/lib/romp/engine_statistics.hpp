#ifndef ENGINE_STATISTICS_HPP
#define ENGINE_STATISTICS_HPP
#if ENABLE_CONSOLES

#include <iostream>

#include "romp/console.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class implements a console displaying various
 *	engine statistics.  The main displays are :
 *
 *	memory and performance indications from the Moo library
 *	timing displays from DogWatches throughout the system.
 *
 *	Most often, this console can be viewed by pressing F5
 */
class EngineStatistics : public StatisticsConsole::Handler
{
public:
	~EngineStatistics();

	static EngineStatistics & instance();

	// Accessors
	void tick( float dTime );

	static bool logSlowFrames_;

private:
	EngineStatistics();

	virtual void displayStatistics( XConsole & console );

	void dogWatchDisplay( XConsole& console );

	EngineStatistics( const EngineStatistics& );
	EngineStatistics& operator=( const EngineStatistics& );

	friend std::ostream& operator<<( std::ostream&, const EngineStatistics& );

	float lastFrameTime_;
	float timeToNextUpdate_;

	static EngineStatistics instance_;
};

#ifdef CODE_INLINE
#include "engine_statistics.ipp"
#endif

BW_END_NAMESPACE

#endif // ENABLE_CONSOLES
#endif // ENGINE_STATISTICS_HPP