#ifndef __OFFLOAD_CHECKER__HEADER__
#define __OFFLOAD_CHECKER__HEADER__

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"
#include "math/rectt.hpp"

#include <utility>
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class Cell;
class CellAppChannel;
class CellInfo;
class Entity;

typedef SmartPointer< Entity > EntityPtr;

/**
 *	This class is responsible for offloading entities in a cell to their most
 *	appropriate remote cell, and checking each of the cell's real entities'
 *	ghosts to make sure that nearby cells that need knowledge of that entity
 *	have ghosts created, as well as removing them if they are no longer needed.
 */
class OffloadChecker
{
public:
	OffloadChecker( Cell & cell );
	~OffloadChecker();

	void run();

	void addToOffloads( EntityPtr pEntity, 
		CellAppChannel * pOffloadDestination );

	bool canDeleteMoreGhosts() const;

	/**
	 *	This method increments the count for deleted ghosts.
	 */
	void addDeletedGhost()
		{ ++numGhostsDeleted_; }

private:
	typedef std::pair< EntityPtr, CellAppChannel * > OffloadEntry;
	typedef BW::vector< OffloadEntry > OffloadList;

	void sendOffloads();

	void sendOffload( OffloadList::const_iterator iOffload );

	// Member data
	Cell & 			cell_;
	OffloadList 	offloadList_;
	uint 			numGhostsDeleted_;
};

BW_END_NAMESPACE

#endif // __OFFLOAD_CHECKER__HEADER__
