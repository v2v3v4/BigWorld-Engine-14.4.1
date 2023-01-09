#ifndef CELLS_HPP
#define CELLS_HPP

#include "cstdmf/watcher.hpp"
#include "network/basictypes.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class BinaryOStream;
class CellData;

/**
 *	This class is used to store a collection of CellData.
 */
class Cells
{
private:
	typedef BW::vector< CellData * > Container;

public:
	Cells() {}
	~Cells();

	void add( CellData * pData )		{ cells_.push_back( pData ); }
	void erase( CellData * pData );

	CellData * findFromSpaceID( SpaceID spaceID ) const;
	CellData * findFromCellAppAddr( const Mercury::Address & addr ) const;

	void notifyOfCellRemoval( SpaceID spaceID, CellData & removedCell ) const;

	void deleteAll();
	void retireAll();
	void cancelRetiring();

	CellData * front() const	{ return cells_.front(); }

	size_t size() const	{ return cells_.size(); }
	bool empty() const	{ return cells_.empty(); }

	// TODO: Look to remove these
	typedef Container::iterator iterator;
	typedef Container::const_iterator const_iterator;


	iterator begin()				{ return cells_.begin(); }
	const_iterator begin() const	{ return cells_.begin(); }
	iterator end()					{ return cells_.end(); }
	const_iterator end() const		{ return cells_.end(); }

	static WatcherPtr pWatcher();

private:
	Container cells_;
};

BW_END_NAMESPACE

#endif // CELLS_HPP
