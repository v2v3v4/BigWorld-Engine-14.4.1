#ifndef SCRIPT_TIMERS_HPP
#define SCRIPT_TIMERS_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_map.hpp"

#include "cstdmf/timer_handler.hpp"
#include "cstdmf/time_queue.hpp"

// TODO: There is nothing script or server the following classes.
// 1) Remove 'Script' prefix
// 2) Move from lib/server to lib/cstdmf

BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;


/**
 *	This TimeQueue subclass encapsulates the queue and time for easy
 *	ScriptTimers access.
 */
class ScriptTimeQueue : public TimeQueue
{
public:
	ScriptTimeQueue( int updateHertz ) :
		updateHertz_( updateHertz ) {}

	int updateHertz() const	{ return updateHertz_; }

	virtual uint32 time() const = 0;

private:
	int updateHertz_;
};


/**
 *	This class stores a collection of timers that have an associated script id.
 */
class ScriptTimers
{
public:
	typedef int32 ScriptID;

	static void init( ScriptTimeQueue & timeQueue );
	static void fini( ScriptTimeQueue & timeQueue );

	ScriptID addTimer( float initialOffset, float repeatOffset, int32 userArg,
			TimerHandler * pHandler );
	bool delTimer( ScriptID timerID );

	void releaseTimer( TimerHandle handle );

	void cancelAll();

	void writeToStream( BinaryOStream & stream ) const;
	void readFromStream( BinaryIStream & stream,
			uint32 numTimers, TimerHandler * pHandler );

	ScriptID getIDForHandle( TimerHandle handle ) const;

	bool isEmpty() const	{ return map_.empty(); }

private:
	typedef BW::map< ScriptID, TimerHandle > Map;

	ScriptID getNewID();
	Map::const_iterator findTimer( TimerHandle handle ) const;
	Map::iterator findTimer( TimerHandle handle );

	Map map_;
};


/**
 *	This namespace contains helper methods so that ScriptTimers can be a pointer
 *	instead of an instance. These functions handle the creation and destruction
 *	of the object.
 */
namespace ScriptTimersUtil
{
	ScriptTimers::ScriptID addTimer( ScriptTimers ** ppTimers,
		float initialOffset, float repeatOffset, int32 userArg,
		TimerHandler * pHandler );
	bool delTimer( ScriptTimers * pTimers, ScriptTimers::ScriptID timerID );

	void releaseTimer( ScriptTimers ** ppTimers, TimerHandle handle );

	void cancelAll( ScriptTimers * pTimers );

	void writeToStream( ScriptTimers * pTimers, BinaryOStream & stream );
	void readFromStream( ScriptTimers ** ppTimers, BinaryIStream & stream,
			TimerHandler * pHandler );

	ScriptTimers::ScriptID getIDForHandle( ScriptTimers * pTimers,
			TimerHandle handle );
}

BW_END_NAMESPACE

#endif // SCRIPT_TIMERS_HPP
