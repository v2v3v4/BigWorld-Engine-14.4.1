#include "script/first_include.hpp"

#include "offload_checker.hpp"

#include "cell.hpp"
#include "cell_app_channel.hpp"
#include "cell_app_channels.hpp"
#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "entity.hpp"
#include "entity_ghost_maintainer.hpp"
#include "space.hpp"

#include "cstdmf/profile.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param cell		The cell to check offloads and ghosts for.
 */
OffloadChecker::OffloadChecker( Cell & cell ) :
		cell_( cell ),
		offloadList_(),
		numGhostsDeleted_( 0 )
{

}


/**
 *	Destructor.
 */
OffloadChecker::~OffloadChecker()
{}


/**
 *	This method performs the offloads and ghosts check.
 */
void OffloadChecker::run()
{
	// If this space is shutting down, don't offload any entities so that
	// they are not lost when the cells are shut down.
	if (cell_.space().isShuttingDown())
	{
		return;
	}

	static ProfileVal localProfile( "boundaryCheck" );
	START_PROFILE( localProfile );

	Cell::Entities::iterator iEntity = cell_.realEntities().begin();
	while (iEntity != cell_.realEntities().end())
	{
		EntityPtr pEntity = *iEntity;

		MF_ASSERT( &cell_ == &(pEntity->cell()) );

		EntityGhostMaintainer entityGhostMaintainer( *this, pEntity );
		entityGhostMaintainer.check();
		++iEntity;
	}

	this->sendOffloads();

	STOP_PROFILE_WITH_DATA( localProfile, offloadList_.size() );
}


/**
 *	This method adds an entity to the offload list.
 *
 *	@param pEntity 				The entity to add.
 *	@param pOffloadDestination 	The destination channel.
 */
void OffloadChecker::addToOffloads( EntityPtr pEntity,
		CellAppChannel * pOffloadDestination )
{
	offloadList_.push_back( OffloadEntry( pEntity, pOffloadDestination ) );
}


/**
 *	This method sends all the pending offloads.
 */
void OffloadChecker::sendOffloads()
{
	OffloadList::iterator iOffload = offloadList_.begin();

	while (iOffload != offloadList_.end())
	{
		this->sendOffload( iOffload );

		++iOffload;
	}
}


/**
 *	This method sends an individual offload, subject to sufficient ghosting
 *	capacity being present on the channels and the channels being in a good
 *	state.
 *
 *	@param iOffload		The iterator pointing to the entry in the offload list
 *						to process.
 */
void OffloadChecker::sendOffload( OffloadList::const_iterator iOffload )
{
	EntityPtr pEntity = iOffload->first;
	CellAppChannel * pOffloadDestination = iOffload->second;

	cell_.offloadEntity( pEntity.get(), pOffloadDestination, 
			/* isTeleport: */ false );
}


/**
 *	This method returns whether more ghost deletions are allowed.
 */
bool OffloadChecker::canDeleteMoreGhosts() const
{
	const uint MAX_GHOSTS_TO_DELETE = CellAppConfig::maxGhostsToDelete();
	return numGhostsDeleted_ <= MAX_GHOSTS_TO_DELETE;
}

BW_END_NAMESPACE

// offload_checker.cpp
