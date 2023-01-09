#ifndef CELL_APP_DEATH_HANDLER_HPP
#define CELL_APP_DEATH_HANDLER_HPP

#include "cstdmf/memory_stream.hpp"
#include "network/basictypes.hpp"

#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used while handling a dead cell app. We want to wait until all
 *	CellApp have handled the message before we send it to any BaseApps. This
 *	class handles this.
 */
class CellAppDeathHandler
{
public:
	CellAppDeathHandler( const Mercury::Address & deadAddr,
		   int ticksToWait );

	void addWaiting( const Mercury::Address & addr );
	void clearWaiting( const Mercury::Address & waitingAddr,
			const Mercury::Address & deadAddr );

	void tick();

	void finish();

	MemoryOStream & stream()					{ return stream_; }
	const Mercury::Address & deadAddr() const	{ return deadAddr_; }

private:
	const Mercury::Address deadAddr_;
	MemoryOStream stream_;
	typedef BW::set< Mercury::Address > WaitingForSet;
	WaitingForSet waitingFor_;

	int ticksToWait_;
};

BW_END_NAMESPACE

#endif // CELL_APP_DEATH_HANDLER_HPP
