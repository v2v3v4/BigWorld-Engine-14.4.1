#ifndef __ENTITY_GHOST_MAINTAINER__HEADER__
#define __ENTITY_GHOST_MAINTAINER__HEADER__ 

#include "cell_info.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/smartpointer.hpp"

#include "math/rectt.hpp"


BW_BEGIN_NAMESPACE

class Cell;
class CellAppChannel;
class Entity;
class OffloadChecker;

namespace Mercury 
{
	class Address;
}

typedef SmartPointer< Entity > EntityPtr;

/**
 *	This class is used to visit all the cells of a space, and create new ghosts
 *	for those that are required for a particular entity, as well as unmark
 *	those that are are still required to exist.
 */
class EntityGhostMaintainer : public CellInfoVisitor
{
public:
	EntityGhostMaintainer( OffloadChecker & offloadChecker,
			EntityPtr pEntity );
	virtual ~EntityGhostMaintainer();

	void check();

	// Override from CellInfoVisitor.
	virtual void visit( CellInfo & cellInfo );

private:
	Cell & cell();

	void checkEntityForOffload();
	bool markHaunts();
	void createOrUnmarkRequiredHaunts();
	void deleteMarkedHaunts();

	OffloadChecker & 			offloadChecker_;
	EntityPtr 					pEntity_;
	const Mercury::Address & 	ownAddress_;
	BW::Rect 					hysteresisArea_;
	CellAppChannel * 			pOffloadDestination_;
	uint						numGhostsCreated_;
};

BW_END_NAMESPACE

#endif // __ENTITY_GHOST_MAINTAINER__HEADER__

