#ifndef CELLS_HPP
#define CELLS_HPP

#include "cstdmf/watcher.hpp"

#include "network/basictypes.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class Cell;
class BinaryOStream;

namespace Mercury
{
class Address;
}


/**
 *	This class handles a collection of Cell instances.
 */
class Cells
{
public:
	Cells();

	bool empty() const 				{ return container_.empty(); }
	size_t size() const 			{ return container_.size(); }

	int numRealEntities() const;

	void handleCellAppDeath( const Mercury::Address & addr );
	void checkOffloads();
	void backup( int index, int period );

	void add( Cell * pCell );
	void destroy( Cell * pCell );

	void writeBounds( BinaryOStream & stream ) const;
	void destroyAll();

	void debugDump();

	void tickRecordings();

	// Section: profiling
	void tickProfilers( uint64 tickDtInStamps, float smoothingFactor );

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif // ENABLE_WATCHERS

private:
	typedef BW::map< SpaceID, Cell * > Container;
	Container container_;
};

BW_END_NAMESPACE

#endif // CELLS_HPP
