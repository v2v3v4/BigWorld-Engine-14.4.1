#include "script/first_include.hpp"

#include "entity_ghost_maintainer.hpp"

#include "cell.hpp"
#include "cell_app_channel.hpp"
#include "cell_app_channels.hpp"
#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "entity.hpp"
#include "offload_checker.hpp"
#include "real_entity.hpp"
#include "space.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *	
 *	@param offloadChecker 	The associated OffloadChecker instance.
 *	@param pEntity			The entity to check ghosts and offloads for.
 */
EntityGhostMaintainer::EntityGhostMaintainer( OffloadChecker & offloadChecker, 
			EntityPtr pEntity ) :
		offloadChecker_( offloadChecker ),
		pEntity_( pEntity ),
		ownAddress_( CellApp::instance().interface().address() ),
		hysteresisArea_(),
		pOffloadDestination_( NULL ),
		numGhostsCreated_( 0 )
{}


/**
 *	Destructor.
 */
EntityGhostMaintainer::~EntityGhostMaintainer()
{}


/**
 *	This method checks through this entity's ghosts and checks whether the real
 *	entity needs to be offloaded elsewhere.
 */
void EntityGhostMaintainer::check()
{
	if (this->cell().shouldOffload())
	{
		this->checkEntityForOffload();
	}

	// We mark all the haunts for this entity, and then we unmark all the valid
	// ones. The invalid ghosts are left marked and are deleted.

	bool doesOffloadDestinationHaveGhost = this->markHaunts();

	this->createOrUnmarkRequiredHaunts();

	MF_ASSERT( (pOffloadDestination_ == NULL) ||
			doesOffloadDestinationHaveGhost ||
			(numGhostsCreated_ == 1) );

	this->deleteMarkedHaunts();
}


/**
 *	This method retrieves our entity's current cell.
 */
Cell & EntityGhostMaintainer::cell()
{
	return pEntity_->cell();
}


/**
 *	This method checks whether the given entity requires offloading to another
 *	cell.
 */
void EntityGhostMaintainer::checkEntityForOffload()
{
	// Find out where we really want to live.
	const Vector3 & position = pEntity_->position();
	const CellInfo * pHomeCell = pEntity_->space().pCellAt( 
		position.x, position.z );

	if ((pHomeCell == NULL) || (pHomeCell == &(this->cell().cellInfo())))
	{
		// Don't offload to ourselves.
		return;
	}

	if (pHomeCell->isDeletePending())
	{
		// Don't offload to a cell that is being deleted.
		return;
	}

	CellAppChannel * pOffloadDestination = 
		CellAppChannels::instance().get( pHomeCell->addr() );

	if (!pOffloadDestination || !pOffloadDestination->isGood())
	{
		// Don't offload if other cell has failed or died.
		return;
	}

	// OK to offload now.
	pOffloadDestination_ = pOffloadDestination;

	offloadChecker_.addToOffloads( pEntity_, pOffloadDestination_ );
}


/**
 *	This method marks each of the channels belonging to all the entity's
 *	haunts. An entity's required haunts are then unmarked during
 *	createOrUnmarkRequiredHaunts(), leaving those haunts that are not required
 *	to be removed during deleteMarkedHaunts().
 *
 *	@return true if the offload destination has an existing haunt, otherwise
 *			false.
 */
bool EntityGhostMaintainer::markHaunts()
{
	RealEntity * pReal = pEntity_->pReal();

	bool doesOffloadDestinationHaveGhost = false;

	RealEntity::Haunts::iterator iHaunt = pReal->hauntsBegin();
	while (iHaunt != pReal->hauntsEnd())
	{
		if (pOffloadDestination_ == &(iHaunt->channel()))
		{
			doesOffloadDestinationHaveGhost = true;
		}

		iHaunt->channel().mark( 1 );
		++iHaunt;
	}

	return doesOffloadDestinationHaveGhost;
}


/**
 *	This method evaluates each cell in the space's tree for suitability for
 *	adding a ghost to that cell for the given entity.  It creates a ghost if
 *	one is required but not present, and leaves the channel unmarked.
 *
 *	If a ghost exists and the cell is still a suitable haunt for the entity,
 *	then the haunt's channel is unmarked.
 *
 *	Those cells that no longer require a ghost for the given entity are left
 *	alone (they should be marked for removal).
 *
 */
void EntityGhostMaintainer::createOrUnmarkRequiredHaunts()
{
	// TODO: Make this configurable.
	static const float GHOST_FUDGE = 20.f;

	const Vector3 & position = pEntity_->position();

	// Find all the haunts that we should have.
	BW::Rect interestArea( position.x, position.z, position.x, position.z );

	// Entities with an appeal raidus have to ghost more
	interestArea.inflateBy( CellAppConfig::ghostDistance() +
			pEntity_->pType()->description().appealRadius() );

	hysteresisArea_ = interestArea;

	interestArea.inflateBy( GHOST_FUDGE );

	pEntity_->space().visitRect( interestArea, *this );
}


/**
 *	Override from CellInfoVisitor.
 */
void EntityGhostMaintainer::visit( CellInfo & cellInfo )
{
	const Mercury::Address & remoteAddress = cellInfo.addr();

	// discard it if it is ourself
	if (remoteAddress == ownAddress_)
	{
		return;
	}

	if (cellInfo.isDeletePending())
	{
		// Do not have ghosts on cells that are about to be deleted.
		return;
	}

	// If it has been marked as an existing haunt then unmark it and bail.
	CellAppChannel & channel = *CellAppChannels::instance().get( 
		remoteAddress );

	if (channel.mark() == 1)
	{
		channel.mark( 0 );
		return;
	}

	// Do not create a ghost if we are about to be offloaded. Let the
	// destination do this. This helps with not creating CellAppChannels
	// unnecessarily and also helps prevent race conditions. We still create
	// the ghost on the destination cell.
	if (pOffloadDestination_ && pOffloadDestination_->addr() != remoteAddress)
	{
		return;
	}

	// and if we are not far enough in then toss it too (hysteresis check)
	if (!cellInfo.rect().intersects( hysteresisArea_ ))
	{
		return;
	}

	// Otherwise we should create a new ghost.

	pEntity_->pReal()->addHaunt( channel );
	pEntity_->createGhost( channel.bundle() );

	++numGhostsCreated_;
}


/**
 *	This method removes ghosts on haunts that have their channels marked. All
 *	the required haunts would have had their channels unmakred in
 *	createOrUnmarkRequiredHaunts().
 *
 *	There are criteria for when a ghost should not be deleted:
 *
 *	* Each iteration of the offload checker has a maximum number of ghosts that
 *    can be deleted (configurable).
 *	* A ghost will not be deleted if it is a new ghost (configurable). 
 *	* A ghost will not be deleted if the real entity has been created recently
 *    (2 seconds).
 *
 */
void EntityGhostMaintainer::deleteMarkedHaunts()
{
	static const int NEW_REAL_KEEP_GHOST_PERIOD_IN_SECONDS = 2;
	const GameTime NEW_REAL_KEEP_GHOST_PERIOD =
		NEW_REAL_KEEP_GHOST_PERIOD_IN_SECONDS * CellAppConfig::updateHertz();

	const GameTime MINIMUM_GHOST_LIFESPAN =
		CellAppConfig::minGhostLifespanInTicks();

	const GameTime gameTime = CellApp::instance().time();

	RealEntity * pReal = pEntity_->pReal();
	RealEntity::Haunts::iterator iHaunt = pReal->hauntsBegin();

	while (iHaunt != pReal->hauntsEnd())
	{
		RealEntity::Haunt & haunt = *iHaunt;
		CellAppChannel & channel = haunt.channel();

		const bool shouldDelGhost = 
			// Too many ghosts deleted in this iteration.
			offloadChecker_.canDeleteMoreGhosts() && 

			// Keep the ghost if we're offloading there.
			(&channel != pOffloadDestination_) &&

			// Keep the ghost if the real entity is new.
			(gameTime - pReal->creationTime() > NEW_REAL_KEEP_GHOST_PERIOD) &&

			// Keep the ghost if the ghost is new.
			(gameTime - haunt.creationTime() > MINIMUM_GHOST_LIFESPAN);

		if (channel.mark() && shouldDelGhost)
		{
			// only bother telling it if it hasn't failed
			if (channel.isGood())
			{
				pReal->addDelGhostMessage( channel.bundle() );
				offloadChecker_.addDeletedGhost();
			}

			iHaunt = pReal->delHaunt( iHaunt );
		}
		else
		{
			++iHaunt;
		}

		// always clear the mark for the next user
		channel.mark( 0 );
	}
}

BW_END_NAMESPACE

// entity_ghost_maintainer.cpp

