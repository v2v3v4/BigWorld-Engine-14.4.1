#ifndef SPACES_HPP
#define SPACES_HPP

#include "cstdmf/watcher.hpp"

#include "network/basictypes.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class BinaryOStream;
class Space;
class CellAppChannel;

class Spaces
{
public:
	~Spaces();

	Space * find( SpaceID id ) const;
	Space * create( SpaceID id );

	void prepareNewlyLoadedChunksForDelete();
	void tickChunks();
	void deleteOldSpaces();
	void writeRecoveryData( BinaryOStream & stream );

	size_t size() const		{ return container_.size(); }

	WatcherPtr pWatcher();

	bool haveOtherCellsIn( const CellAppChannel & remoteChannel ) const;

private:
	typedef BW::map< SpaceID, Space * > Container;
	Container container_;
};

BW_END_NAMESPACE

#endif // SPACES_HPP
