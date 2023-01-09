#include "script/first_include.hpp"

#include "space.hpp"
#include "physical_chunk_space.hpp"
#include "physical_delegate_space.hpp"

#include "buffered_ghost_message_factory.hpp"
#include "buffered_ghost_messages.hpp"
#include "cell.hpp"
#include "cell_chunk.hpp"
#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "entity.hpp"
#include "entity_population.hpp"
#include "entity_range_list_node.hpp"
#include "range_list_node.hpp"
#include "range_trigger.hpp"
#include "real_entity.hpp"
#include "replay_data_collector.hpp"
#include "space_branch.hpp"

#include "cellappmgr/cellappmgr_interface.hpp"

#include "chunk/chunk_vlo.hpp"

#include "common/closest_triangle.hpp"
#include "common/closest_chunk_item.hpp"
#include "common/py_physics2.hpp"
#include "common/space_data_types.hpp"

#include "cstdmf/debug.hpp"

#include "network/bundle.hpp"
#include "network/event_dispatcher.hpp"

#include "pyscript/personality.hpp"

#include "server/balance_config.hpp"

#include <cmath>
#include <limits>
#include "cstdmf/bw_string.hpp"
#include "server/server_app_config.hpp"


DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Chunk Items
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Section: Constructor/Destructor
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Space::Space( SpaceID id ) :
	id_( id ),
	pCell_( NULL ),
	pPhysicalSpace_( NULL ),
	spaceDataMapping_(),
	entities_(),
	rangeList_(),
	appealRadiusList_(),
	begDataSeq_( 0 ),
	endDataSeq_( 0 ),
	initialTimeOfDay_( -1.f ),
	gameSecondsPerSecond_( 0.f ),
	pCellInfoTree_( NULL ),
	shuttingDownTimerHandle_(),
	artificialMinLoad_( 0.f ),
	shouldCheckLoadBounds_( false )
{
	if (IGameDelegate::instance() != NULL) 
	{
		pPhysicalSpace_ = PhysicalSpacePtr( new PhysicalDelegateSpace( id ) );
	}
 	else
	{
		pPhysicalSpace_ = PhysicalSpacePtr( new PhysicalChunkSpace( id, *this ) );
	}

	MF_ASSERT( pPhysicalSpace_ );
}


/**
 *	Destructor.
 */
Space::~Space()
{
	// destroy any ghosts we have
	// (real entities should be taken care of by ~Cell)
	SpaceEntities::iterator entityIter = entities_.begin();
	while( entityIter != entities_.end() )
	{
		EntityPtr & pEntity = *entityIter;

		if(!pEntity->isReal() && !pEntity->isDestroyed())
		{
			pEntity->destroy();

			entityIter = entities_.begin();
		}
		else
		{
			if (pEntity->isReal() && !pEntity->isDestroyed())
			{
				WARNING_MSG( "Space::~Space: "
						"not destroying real entity %s %u\n",
					pEntity->pType()->name(),
					pEntity->id()
				);
			}
			++entityIter;
		}
	}

	shuttingDownTimerHandle_.cancel();

	if (pCellInfoTree_ != NULL)
	{
		pCellInfoTree_->deleteTree();
		pCellInfoTree_ = NULL;
	}

	pPhysicalSpace_->clear();
}


/**
 *	This method sets this space into a state that is appropriate for reuse.
 */
void Space::reuse()
{
	// The following currently causes problems with empty space data being sent
	// to the client.
	recentData_.clear();
	begDataSeq_ = 0;
	endDataSeq_ = 0;
	initialTimeOfDay_ = -1.f;
	gameSecondsPerSecond_ = 0.f;

	if (pCellInfoTree_)
	{
		pCellInfoTree_->deleteTree();
		pCellInfoTree_ = NULL;
	}

	spaceDataMapping_.clear();
	pPhysicalSpace_->reuse();
}


/**
 *	This method sets the cell that controls real entities in this space
 *	on this cellapp.
 */
void Space::pCell( Cell * pCell )
{
	pCell_ = pCell;
}


/**
 *	ChunkSpace accessor
 */
ChunkSpacePtr Space::pChunkSpace() const
{
	if (IGameDelegate::instance() == NULL) 
	{
		return static_cast< PhysicalChunkSpace * >( pPhysicalSpace_.get()
			)->pChunkSpace();
	}

	return NULL;
}


/**
 *	This method handles the when a geometry mapping has been fully loaded.
 */
void Space::onSpaceGeometryLoaded( SpaceID spaceID, const BW::string & name )
{
	CellApp::instance().scriptEvents().triggerEvent(
			"onSpaceGeometryLoaded",
			Py_BuildValue( "is", spaceID, name.c_str() ) );
}


/**
 *	This method sets the output argument to the initial point of this Space.
 *
 *	@param result	Set to the initial point.
 *
 *	@return True if result is set to the initial point, false otherwise.
 */
bool Space::initialPoint( Vector3 & result ) const
{
	if (pCell_ == NULL)
	{
		return false;
	}

    BW::Rect cellRect = pCell_->cellInfo().rect();

	result.x = 0.5f * cellRect.xMin() + 0.5f * cellRect.xMax();
	result.y = 0.f;
	result.z = 0.5f * cellRect.yMin() + 0.5f * cellRect.yMax();

	return true;
}


// -----------------------------------------------------------------------------
// Section: Entities
// -----------------------------------------------------------------------------

/**
 *	This method creates a new ghost entity on this cell according to the
 *	parameters in 'data'.
 */
void Space::createGhost( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
{
	EntityID entityID;
	data >> entityID;

	CellApp & app = ServerApp::getApp< CellApp >( header );
	Entity * pExistingEntity = app.findEntity( entityID );

	BufferedGhostMessages & bufferedMessages = app.bufferedGhostMessages();

	if ((pExistingEntity != NULL) ||
			bufferedMessages.hasMessagesFor( entityID, srcAddr ))
	{
		BufferedGhostMessage * pMsg =
			BufferedGhostMessageFactory::createBufferedCreateGhostMessage(
				srcAddr, entityID, id_, data );

		if (bufferedMessages.hasMessagesFor( entityID, srcAddr ))
		{
			bufferedMessages.add( entityID, srcAddr, pMsg );
		}
		else // if pExistingEntity != NULL
		{
			TRACE_MSG( "Space::createGhost: "
					"delaying subsequence for %d from %s\n",
				entityID, srcAddr.c_str() );
			bufferedMessages.delaySubsequence( entityID, srcAddr, pMsg );
		}

		WARNING_MSG( "Space::createGhost(%u): "
					"Buffered createGhost message for entity %u from %s\n",
				id_, entityID, srcAddr.c_str() );
	}
	else
	{
		this->createGhost( entityID, data );
	}
}


/**
 *	This method creates a ghost entity in this space. This version of
 *	createGhost already has the entity's id streamed off.
 */
void Space::createGhost( const EntityID entityID, BinaryIStream & data )
{
	//ToDo: remove when load balancing is supported on Delegate types
	// of physical spaces
	if (IGameDelegate::instance() != NULL) {
		ERROR_MSG( "Space::createGhost: "
			"Currently not supported by Delegate Physical spaces" );
		return;
	}

	AUTO_SCOPED_PROFILE( "createGhost" );
	SCOPED_PROFILE( TRANSIENT_LOAD_PROFILE );

	// Build up the Entity structure
	EntityTypeID entityTypeID;

	data >> entityTypeID;

	EntityPtr pNewEntity = this->newEntity( entityID, entityTypeID );
	pNewEntity->initGhost( data );

	Entity::population().notifyObservers( *pNewEntity );
}


/**
 *	This method adds an entity to this space.
 */
void Space::addEntity( Entity * pEntity )
{
	MF_ASSERT( pEntity->removalHandle() == NO_SPACE_REMOVAL_HANDLE );

	pEntity->removalHandle( entities_.size() );
	entities_.push_back( pEntity );

	pEntity->addToRangeList( rangeList_, appealRadiusList_ );
}


/**
 *	This method removes an entity from this space.
 */
void Space::removeEntity( Entity * pEntity )
{
	SpaceRemovalHandle handle = pEntity->removalHandle();

	MF_ASSERT( handle < entities_.size() );
	MF_ASSERT( entities_[ handle ] == pEntity );

	Entity * pBack = entities_.back().get();
	pBack->removalHandle( handle );
	entities_[ handle ] = pBack;
	pEntity->removalHandle( NO_SPACE_REMOVAL_HANDLE );
	entities_.pop_back();

	// Need to remove this after pop_back() since it can set off triggers
	// which can potentially call addEntity() or removeEntity().
	pEntity->removeFromRangeList( rangeList_, appealRadiusList_ );

	if (entities_.empty())
	{
		if (pCell_ != NULL)
		{
			this->checkForShutDown();
		}
	}
}


/**
 *	This method visits all large entities in this space whose Area of Appeal
 *	contains the given position.
 */
void Space::visitLargeEntities( float x, float z, RangeTrigger & visitor )
{
	for (RangeTriggerList::iterator iter = appealRadiusList_.begin();
		iter != appealRadiusList_.end();
		++iter)
	{
		RangeTrigger * pAppealTrigger = *iter;

		if (pAppealTrigger->contains( x, z ))
		{
			visitor.triggerEnter( *pAppealTrigger->pEntity() );
		}
	}
}


/**
 *	This method calculates boundary values for the boundaries that contains
 *	the real entities of this space at different sizes based on CPU. This is
 *	used by load balancing to calculate where to move partitions.
 *
 *	@param stream		The stream to write the levels to.
 *	@param isMax		Indicates whether the lower or upper bound should be
 *						calculated.
 *	@param isY			Indicates whether the X or Y/Z bound should be
 *						calculated.
 */
void Space::writeEntityBoundsForEdge( BinaryOStream & stream,
		bool isMax, bool isY ) const
{
	const RangeListNode * pNode =
		isMax ? rangeList_.pLastNode() : rangeList_.pFirstNode();

	float currCPULoad = 0.f;
	int level = BalanceConfig::numCPUOffloadLevels();
	float currCPULimit = BalanceConfig::cpuOffloadForLevel( --level );

	float perEntityLoadShare = CellApp::instance().getPerEntityLoadShare();

	bool hasEntities = false;
	float lastEntityPosition = 0.f;

	const float FUDGE_FACTOR = isMax ? -0.1f : 0.1f;

	// Looking for the first real entity node
	while (pNode && (!pNode->isEntity() ||
				!EntityRangeListNode::getEntity( pNode )->isReal()))
	{
		pNode = pNode->getNeighbour( !isMax, isY );
	}

	// Now iterate through the real entities but counting dense bursts
	// within POS_DIFF_EPSILON range as one entity
	while (pNode && (level >= 0))
	{
		const float POS_DIFF_EPSILON = 0.01f;

		bool shouldCountThisEntity = true;

		const RangeListNode * pNextNode = pNode->getNeighbour( !isMax, isY );
		while (pNextNode && (!pNextNode->isEntity() ||
				!EntityRangeListNode::getEntity( pNextNode )->isReal()))
		{
			pNextNode = pNextNode->getNeighbour( !isMax, isY );
		}

		const Entity * pEntity = EntityRangeListNode::getEntity( pNode );
		const float entityPosition = pEntity->position()[ isY * 2 ];

		currCPULoad += pEntity->profiler().load() + perEntityLoadShare;

		const Entity * pNextEntity = NULL;
		float nextEntityPosition = 0.f;

		if (pNextNode)
		{
			pNextEntity = EntityRangeListNode::getEntity( pNextNode );
			nextEntityPosition = pNextEntity->position()[ isY * 2 ];

			if (almostEqual(entityPosition, nextEntityPosition, POS_DIFF_EPSILON))
			{
				shouldCountThisEntity = false;
			}
		}

		if (shouldCountThisEntity)
		{
			if (currCPULoad > currCPULimit)
			{
				const float limitPosition = pNextEntity ?
						(nextEntityPosition + entityPosition) / 2 :
						entityPosition + FUDGE_FACTOR;

				stream << limitPosition << currCPULoad;

				currCPULimit = BalanceConfig::cpuOffloadForLevel( --level );
			}

			hasEntities = true;
			lastEntityPosition = entityPosition;
		}

		pNode = pNextNode;
	}

	// failed to overcome the limit?
	if (level == BalanceConfig::numCPUOffloadLevels() - 1 && hasEntities)
	{
		stream << lastEntityPosition + FUDGE_FACTOR << currCPULoad;

		--level;
	}

	while (level >= 0)
	{
		if (isMax)
		{
			stream << -std::numeric_limits< float >::max();
		}
		else
		{
			stream << std::numeric_limits< float >::max();
		}
		stream << std::max( currCPULoad,
							BalanceConfig::cpuOffloadForLevel( level ) );

		--level;
	}
}


/**
 *	This method is used for debugging. It dumps out information about the range
 *	list.
 */
void Space::debugRangeList()
{
	rangeList_.debugDump();
}


/**
 *	This method returns the static Watcher associated with this class.
 */
WatcherPtr Space::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (!pWatcher)
	{
		pWatcher = new DirectoryWatcher;
		Space * pNull = NULL;
		pWatcher->addChild( "id", makeWatcher( &Space::id_ ) );

		pWatcher->addChild( "cellInfos", new MapWatcher<CellInfos>(
			pNull->cellInfos_) );
		Watcher * pCellInfoWatcher = new SmartPointerDereferenceWatcher(
			CellInfo::pWatcher() );
		pWatcher->addChild( "cellInfos/*", pCellInfoWatcher );

		pWatcher->addChild( "numEntities", makeWatcher( &Space::numEntities ) );

		pWatcher->addChild( "artificialMinLoad", makeWatcher(
								&Space::artificialMinLoad ) );
}

	return pWatcher;
}


/**
 *	This method returns a new entity. It may be the case that the entity
 *	returned is a re-initialised entity still in our population. This is
 *	possible that it could be if it was removed (via teleport or a ghost removed
 *	and added quickly, for example) and is now being re-added before it has been
 *	removed from everyone's AoI.
 */
EntityPtr Space::newEntity( EntityID id, EntityTypeID entityTypeID )
{
	// We check here that the new entity is not in our population. It is
	// possible that it could be if it was removed (via teleport or a ghost
	// added and removed quickly, for example) and is now being re-added before
	// it has been removed from everyone's AoI.
	EntityPopulation::const_iterator deadEntity =
									Entity::population().find( id );

	EntityPtr pNewEntity;

	if (deadEntity != Entity::population().end())
	{
		//WARNING_MSG( "Space::newEntity: Bringing %d back to life\n", id );

		pNewEntity = deadEntity->second;

		// We should never call through to here for non-destroyed entities.
		// Cell::createEntityInternal() checks this before calling this method.
		MF_ASSERT( pNewEntity->isDestroyed() );
		MF_ASSERT( pNewEntity->entityTypeID() == entityTypeID );
	}
	else
	{
		EntityTypePtr pType = EntityType::getType( entityTypeID );

		if (!pType || !pType->canBeOnCell())
		{
			ERROR_MSG( "Space::newEntity: "
				"Entity type %d has no cell entity class\n", entityTypeID );
			return NULL;
		}
		else
		{
			pNewEntity =
				EntityPtr( pType->newEntity(), EntityPtr::STEAL_REFERENCE );
		}
	}

	pNewEntity->setToInitialState( id, this );

	return pNewEntity;
}


/**
 *	This method find the entity nearest to the given point.
 */
Entity * Space::findNearestEntity( const Vector3 & position )
{
	// TODO: Implement this function properly.
	if (!entities_.empty()) 
	{
		return entities_.back().get();
	}

	return NULL;
}


// -----------------------------------------------------------------------------
// Section: Space Data
// -----------------------------------------------------------------------------

/**
 *	This method finds the given space data by its local sequence number
 *	(local to this CellApp). Note that even 'tho it is recent data, it
 *	might since have disappeared, in which case the returned string will
 *	be NULL.
 */
const BW::string * Space::dataBySeq( int32 seq,
	SpaceEntryID & entryID, uint16 & key ) const
{
	MF_ASSERT( seq >= begDataSeq_ && seq < endDataSeq_ );

	const RecentDataEntry & rde = recentData_[ seq - begDataSeq_ ];
	entryID = rde.entryID;
	key = rde.key;
	if (key != uint16(-1))
	{
		const SpaceDataMapping::DataValue & data =
				spaceDataMapping_.dataRetrieveSpecific( entryID );
		if (data.valid())
		{
			return &data.data();
		}
		return NULL;
	}
	else
	{
		return NULL;
	}
}


/**
 *	This method returns the recency level of the space data change at the
 *	given local sequence number (local to this CellApp)
 */
int Space::dataRecencyLevel( int32 seq ) const
{
	MF_ASSERT( seq >= begDataSeq_ && seq < endDataSeq_ );

	const RecentDataEntry & rde = recentData_[ seq - begDataSeq_ ];

	int diff = int(CellApp::instance().time() - rde.time);
	int hertz = CellAppConfig::updateHertz();

	// these values may change in the future...
	if (diff < hertz * 10) return 0;
	if (diff < hertz * 20) return 1;
	if (diff < hertz * 30) return 3;
	return 4;
}


/**
 *	Message handler for when another cellapp is telling us about some space
 *	data. If the data is news to us, then we pass it on to all other CellApps
 *	that we know contain this space.
 */
void Space::spaceData( BinaryIStream & data )
{
	// stream off args
	SpaceEntryID entryID;
	uint16 key;
	data >> entryID >> key;

	uint32 length = data.remainingLength();
	BW::string value( (char*)data.retrieve( length ), length );

	this->spaceDataEntry( entryID, key, value, DONT_UPDATE_CELL_APP_MGR );
}


/**
 *	This method is the message handler for when the Cell App Manager tells us of
 *	all the space data.
 */
void Space::allSpaceData( BinaryIStream & data )
{
	this->readDataFromStream( data );
}


/**
 *	This method notifies the space that there's some new space data about.
 */
bool Space::spaceDataEntry( const SpaceEntryID & entryID, uint16 key,
	const BW::string & value,
	UpdateCellAppMgr cellAppMgrAction, DataEffected effected )
{
	TRACE_MSG( "Space::spaceDataEntry: key = %hu. value.size() = %" PRIzu "\n",
			key, value.size() );

	if (cellAppMgrAction == UPDATE_CELL_APP_MGR)
	{
		// TODO:BAR Move this into CellAppMgrGateway
		CellAppMgrGateway & cellAppMgr = CellApp::instance().cellAppMgr();
		Mercury::Bundle & b = cellAppMgr.bundle();
		b.startMessage( CellAppMgrInterface::updateSpaceData );
		b << id_ << entryID << key;
		if (key != uint16(-1))
		{
			b.addBlob( (void *)value.data(), value.length() );
		}

		cellAppMgr.send();

		if (this->pCell()->pReplayData() != NULL)
		{
			this->pCell()->pReplayData()->addSpaceData( entryID, key, value );
		}
	}
	else
	{
		// make sure we are not trying to add back a recently deleted key
		if (key < SPACE_DATA_FIRST_CELL_ONLY_KEY)	// && key != uint16(-1)
		{
			// TODO: find a better way of doing this than using recent data,
			// as it shouldn't exclude cell-only space data.
			for (RecentDataEntries::reverse_iterator rit = recentData_.rbegin();
				rit != recentData_.rend();
				++rit)
			{
				if (rit->entryID == entryID &&
					rit->key == uint16(-1))
				{
					WARNING_MSG( "Space::spaceDataEntry: "
						"attempted to add back a recently deleted entry!\n" );
					return false;
				}
			}
		}
	}

	bool hasBeenAdded = false;

	SpaceDataMapping::DataValue oldValue;
	uint16 effectiveKey = uint16(-1);
	if (key != uint16(-1))
	{
		hasBeenAdded = spaceDataMapping_.addDataEntry( entryID, key, value );
		if (hasBeenAdded)
		{
			effectiveKey = key;
		}
	}
	else
	{
		bool res = spaceDataMapping_.delDataEntry( entryID, oldValue );
		if (res)
		{
			effectiveKey = oldValue.key();
		}
	}

	// see if the chunk space already has this one
	// if it was not new then effectiveKey is -1
	if (effectiveKey == uint16(-1))
	{
		WARNING_MSG( "Space::spaceDataEntry: Attempted to add or delete entry "
				"twice!\n" );
		return false;
	}

	// Let the script know that a space data value has been added,
	// or that a space data value has been deleted.
	{
		const BW::string callbackName = 
			hasBeenAdded ? "onSpaceData" : "onSpaceDataDeleted";

		const BW::string & actualValue = 
			hasBeenAdded ? value : oldValue.data();

		ScriptObject entryIDObject = ScriptObject::createFrom( entryID );
		ScriptObject keyObject = ScriptObject::createFrom( effectiveKey );

		CellApp::instance().scriptEvents().triggerEvent( callbackName.data(),
					Py_BuildValue( "iOOs#",
						this->id(),
						entryIDObject.get(), keyObject.get(),
						actualValue.data(), actualValue.size() ) );
	}

	// see if this is data that can be sent to the client
	if (effectiveKey < SPACE_DATA_FIRST_CELL_ONLY_KEY)
	{
		// add it to our recent data
		RecentDataEntry rde;
		rde.entryID = entryID;
		rde.time = CellApp::instance().time();
		rde.key = key;	// including -1 for revoked
		recentData_.push_back( rde );
		endDataSeq_++;

		// update the global space data	sequence number
		s_allSpacesDataChangeSeq_++;
	}

	// effect this change unless that has already been done
	if (effected == NEED_TO_EFFECT)
	{
		if (effectiveKey == SPACE_DATA_TOD_KEY)
		{
			const BW::string * pValue =
				spaceDataMapping_.dataRetrieveFirst( SPACE_DATA_TOD_KEY );
			if ((pValue != NULL) &&
					(pValue->size() >= sizeof( SpaceData_ToDData )))
			{
				SpaceData_ToDData & todData =
					*(SpaceData_ToDData*)pValue->data();
				initialTimeOfDay_ = todData.initialTimeOfDay;
				gameSecondsPerSecond_ = todData.gameSecondsPerSecond;
			}
			else
			{
				initialTimeOfDay_ = -1.f;
				gameSecondsPerSecond_ = 0.f;
			}
		}
		else if (effectiveKey == SPACE_DATA_ARTIFICIAL_LOAD)
		{
			const BW::string * pValue =
				spaceDataMapping_.dataRetrieveFirst( SPACE_DATA_ARTIFICIAL_LOAD );
			if ((pValue != NULL) &&
					(pValue->size() >= sizeof( SpaceData_ArtificialLoad )))
			{
				SpaceData_ArtificialLoad & loadData =
					*(SpaceData_ArtificialLoad*)pValue->data();
				artificialMinLoad_ = loadData.artificialMinLoad;
			}
			else
			{
				artificialMinLoad_ = 0.f;
			}
		}
		else if (effectiveKey == SPACE_DATA_MAPPING_KEY_CLIENT_SERVER)
		{
			// see if this mapping is being added
			if (hasBeenAdded)
			{
				MF_ASSERT( value.size() >= sizeof(SpaceData_MappingData) );
				SpaceData_MappingData & mappingData =
					*(SpaceData_MappingData*)value.data();
				BW::string path( (char*)(&mappingData+1),
					value.length() - sizeof(SpaceData_MappingData) );
				pPhysicalSpace_->loadResource( entryID, path, 
					&mappingData.matrix[0][0] );
				this->onNewGeometryMapping();

				// Someone else has mapped a geometry. If we thought we had the
				// last mapped geometry then clear it.
				// TODO: Need to handle case where two cells simultaneously
				// maps a geometry. We could end up with both of them clearing
				// their lastMappedGeometry_. What happens with normal space
				// data?
				lastMappedGeometry_.clear();
			}
			// ok it's being removed then
			else
			{
				pPhysicalSpace_->unloadResource( entryID );
			}
		}
		/*
		else if (effectiveKey == SPACE_DATA_MAPPING_KEY_CLIENT_ONLY)
		{
			// Do nothing on the server
		}
		*/
		else if (effectiveKey == SPACE_DATA_RECORDING)
		{
			if (hasBeenAdded)
			{
				BW::string name;
				uint8 shouldRecordAoIEvents = 0;

				MemoryIStream dataStream( value.data(), value.size() );
				dataStream >> name >> shouldRecordAoIEvents;
				this->pCell()->startRecording( entryID, name, 
					(shouldRecordAoIEvents != 0), /* isInitial */ false );
			}
			else
			{
				this->pCell()->stopRecording( /* isInitial*/ false );
			}
		}
	}

	return true;
}


/**
 *	This method handles a message from the server that updates geometry
 *	information.
 */
void Space::updateGeometry( BinaryIStream & data )
{
	bool wasMulticell = !this->hasSingleCell();

	// Mark them all to be deleted
	{
		// We could get rid of this step if we used a flip-flop value but it's
		// simpler this way.

		CellInfos::const_iterator iter = cellInfos_.begin();

		while (iter != cellInfos_.end())
		{
			iter->second->shouldDelete( true );

			++iter;
		}
	}

	if (pCellInfoTree_ != NULL)
	{
		pCellInfoTree_->deleteTree();
	}

	BW::Rect rect(
			-std::numeric_limits< float >::max(),
			-std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max() );

	pCellInfoTree_ = this->readTree( data, rect );

	// Delete the cells that should be
	//if(1)
	{
		CellInfos::iterator iter = cellInfos_.begin();

		while (iter != cellInfos_.end())
		{
			CellInfos::iterator oldIter = iter;
			++iter;

			if (oldIter->second->shouldDelete())
			{
				// TODO: This assertion can be triggered. I believe this can
				// occur if multiple cells are deleted at the same time. It
				// would be good to confirm that this is not an issue with
				// missing notifyOfCellRemoval calls.
				// MF_ASSERT( oldIter->second->isDeletePending() );
				cellInfos_.erase( oldIter );
			}
		}
	}

	// see if we are going to get rid of our own cell
	if (pCell_)
	{
		if (pCell_->cellInfo().shouldDelete())
		{
			INFO_MSG( "Space::updateGeometry: Cell in space %u is going\n",
					id_ );
		}
		else
		{
			pCell_->checkOffloadsAndGhosts();
		}
	}

	// see if we want to expressly shut down this space now
	if (wasMulticell)
	{
		this->checkForShutDown();
	}
}


/**
 *	This method checks whether we should request for this space to shut down.
 *	If we have no entities and we're the only cell, request a shutdown.
 *	We won't actually be deleted however until we've unloaded all our chunks.
 */
void Space::checkForShutDown()
{
	if (this->hasSingleCell() &&
			entities_.empty() && CellApp::instance().hasStarted() &&
			!this->isShuttingDown() &&
			!CellApp::instance().mainDispatcher().processingBroken() &&
			!(CellAppConfig::useDefaultSpace() && id_ == 1)) // Not for the default space.
	{
		INFO_MSG( "Space::checkForShutDown: Space %u is now empty.\n", id_ );
		this->requestShutDown();
	}
}


/**
 *	This method returns whether or not this space contains only one cell.
 */
bool Space::hasSingleCell() const
{
	return pCell_ && ((SpaceNode*)&pCell_->cellInfo() == pCellInfoTree_);
}


/**
 *	This method handles a message from the CellAppMgr telling us that all the
 *	cells in this newly created space have loaded all the chunks that they need.
 */
void Space::spaceGeometryLoaded( BinaryIStream & data )
{
	uint8 flags;
	data >> flags;

	BW::string lastMappedGeometry;
	data >> lastMappedGeometry;

	if (lastMappedGeometry == lastMappedGeometry_)
		lastMappedGeometry_.clear();

	bool isBootStrap = (flags & SPACE_GEOMETRY_LOADED_BOOTSTRAP_FLAG) != 0;

	// Call script to let them know.
	CellApp::instance().scriptEvents().triggerEvent( "onAllSpaceGeometryLoaded",
			Py_BuildValue( "iis#", this->id(), int(isBootStrap),
					lastMappedGeometry.data(), lastMappedGeometry.length() ) );
}


/**
 *	This method handles a message from the CellAppMgr telling us that the space
 *	has been destroyed. It may take some time before all the cells are removed.
 */
void Space::shutDownSpace( BinaryIStream & data )
{
	if (!shuttingDownTimerHandle_.isSet())
	{
		// Register a timer to go off in one second.
		shuttingDownTimerHandle_ =
			CellApp::instance().mainDispatcher().addTimer( 1000000, this, NULL,
			"ShutdownSpace" );
	}
	else
	{
		INFO_MSG( "Space::shutDownSpace: Already shutting down.\n" );
	}
}


/**
 *	This method handles the timer associated with the space.
 *	Currently it is only used for the shutting down timer.
 */
void Space::handleTimeout( TimerHandle handle, void * arg )
{
	if (pCell_)
	{
		pCell_->onSpaceGone();

		if (this->hasSingleCell() && entities_.empty())
		{
			CellApp::instance().destroyCell( pCell_ );
			// when the cell is destructed it will clear our ptr to it
			MF_ASSERT( pCell_ == NULL );
		}
	}
}


/**
 *  This method updates space data entries with existing loadable bounds information.
 *  It deletes the 'load bounds' entries for which no loadable bounds exist.
 */
void Space::updateLoadBounds()
{
	IPhysicalSpace::BoundsList loadableRects;
	BW::vector< SpaceEntryID > entriesToDelete;

	if (!this->pPhysicalSpace()->getLoadableRects( loadableRects ))
	{
		return;
	}

	for (SpaceDataMapping::const_iterator iter = spaceDataMapping_.begin();
			iter != spaceDataMapping_.end();
			++iter)
	{
		const SpaceEntryID & entryID = iter->first;
		const SpaceDataMapping::DataValue & entryData = iter->second;

		if (entryData.key() != SPACE_DATA_SERVER_LOAD_BOUNDS)
		{
			continue;
		}

		const BW::string & value = entryData.data();
		SpaceData_LoadBounds * pBounds;
		MF_ASSERT( SpaceData_LoadBounds::castFromString( value, pBounds ) && pBounds );
		
		IPhysicalSpace::BoundsList::iterator boundsIter =
			std::find( loadableRects.begin(), loadableRects.end(), pBounds->loadRect );

		if (boundsIter != loadableRects.end())
		{
			loadableRects.erase( boundsIter );
		}
		else
		{
			entriesToDelete.push_back( entryID );
		}
	}

	for (IPhysicalSpace::BoundsList::iterator iter = loadableRects.begin();
			iter != loadableRects.end();
			++iter)
	{
		BW::string value;
		SpaceData_LoadBounds bounds;

		bounds.loadRect = *iter;
		SpaceData_LoadBounds::copyToString( bounds, value );
		SpaceEntryID newEntryID = CellApp::instance().nextSpaceEntryID();

		this->spaceDataEntry( newEntryID, SPACE_DATA_SERVER_LOAD_BOUNDS, value,
				Space::UPDATE_CELL_APP_MGR, Space::ALREADY_EFFECTED );
	}

	BW::string emptyString;
	for (BW::vector< SpaceEntryID >::iterator iter = entriesToDelete.begin();
			iter != entriesToDelete.end();
			++iter)
	{
		this->spaceDataEntry( *iter, uint16(-1), emptyString );
	}
	shouldCheckLoadBounds_ = false;
}


/**
 *	This method sends a request to the CellAppMgr to shut this space down.
 */
void Space::requestShutDown()
{
	if ( CellAppConfig::useDefaultSpace() && this->id() == 1 )
	{
		ERROR_MSG( "Space::requestShutDown: Requesting shut down for "
			"the default space\n" );
	}
	CellApp::instance().cellAppMgr().shutDownSpace( this->id() );
}


/**
 *	This method progresses with loading and/or unloading chunks, to cover
 *	the area we serve.
 */
void Space::chunkTick()
{
	bool unloadOnly = (pCell_ == NULL);
	BW::Rect cellRect;
	if (unloadOnly)
	{
		// retreat to origin if no cell
		cellRect = BW::Rect( 0.f, 0.f, 0.f, 0.f );
	}
	else
	{
		// Inflate boundary of rect to load, but don't go outside +-(nearIntMax);
		static const float nearIntMax = 1000000000.f;

		const float inflateBy = CellAppConfig::ghostDistance()*2; // or something

		cellRect = pCell_->cellInfo().rect();

		cellRect.safeInflateBy( inflateBy );

		cellRect.xMin( Math::clamp( -nearIntMax, cellRect.xMin(), nearIntMax ) );
		cellRect.yMin( Math::clamp( -nearIntMax, cellRect.yMin(), nearIntMax ) );
		cellRect.xMax( Math::clamp( -nearIntMax, cellRect.xMax(), nearIntMax ) );
		cellRect.yMax( Math::clamp( -nearIntMax, cellRect.yMax(), nearIntMax ) );

		// Sometimes the CellAppMgr will give us an inverted cellRect with
		// FLT_MAX on the min side
		cellRect.xMin( std::min( cellRect.xMin(), cellRect.xMax() ) );
		cellRect.yMin( std::min( cellRect.yMin(), cellRect.yMax() ) );

		if (shouldCheckLoadBounds_)
		{
			this->updateLoadBounds();
		}
	}

	bool anyColumnsLoaded = pPhysicalSpace_->update( cellRect, unloadOnly );

	if (this->pChunkSpace()) 
	{
		// TODO: Entities as dynamic chunk items that get moved into the
		// correct chunk when one loads underneath them. Of course, that
		// should never happen if the load balancing all does its thing,
		// but we know it's going to anyway.

		if (anyColumnsLoaded)
		{
			// TODO: A separate linked list of entities without a chunk could be
			// kept (using their pNextInChunk/pPrevInChunk pointers.
			for (SpaceEntities::iterator it = entities_.begin();
					it != entities_.end();
					it++)
			{
				Entity * pEntity = it->get();
				if (pEntity->pChunk() == NULL)
				{
					pEntity->checkChunkCrossing();
				}
			}
		}
	}
}


/**
 *	This function goes through any chunks that were submitted to the loading
 *	thread and prepares them for deletion if the loading thread has finished
 *	loading them. Also changes the loading flag to false for those that were
 *	submitted but not loaded.
 */
void Space::prepareNewlyLoadedChunksForDelete()
{
	pPhysicalSpace_->cancelCurrentlyLoading();
}


/**
 *	This private method does the best it can to determine an axis-aligned
 *	rectangle that has been loaded. If there is no geometry mapped into the
 *	space, then a very big rectangle is returned.
 */
void Space::calcLoadedRect( BW::Rect & loadedRect ) const
{
	pPhysicalSpace_->getLoadedRect( loadedRect );
}


/**
 *	This method returns whether or not the space is fully unloaded.
 */
bool Space::isFullyUnloaded() const
{
	// if some (ghost) entities have been given to us since killCell was called,
	// then 1. that's bad, and 2. we're not unloaded (really a sanity check).
	if (!entities_.empty())
	{
		return false;
	}

	bool result = pPhysicalSpace_->isFullyUnloaded();


	if (result)
	{
		INFO_MSG( "Space::isFullyUnloaded: ChunkSpace unloading complete.\n" );
	}

	return result;
}


/**
 *	This method returns the current time of day in this space
 */
float Space::timeOfDay() const
{
	CellApp & ca = CellApp::instance();
	double serverTime = double(ca.time()) / CellAppConfig::updateHertz();
	return float(initialTimeOfDay_ + serverTime * gameSecondsPerSecond_);
}


/**
 *	This method writes data that is to be sent
 */
void Space::writeRecoveryData( BinaryOStream & stream ) const
{
	// TODO: Do we need to worry about cell'less spaces?
	if (pCell_)
	{
		// Write the space data
		this->writeDataToStream( stream );

		// Write the BSP tree
		if (pCellInfoTree_)
		{
			pCellInfoTree_->addToStream( stream );
		}
	}
}


/**
 *	This method writes the data associated with this space to the input stream.
 */
void Space::writeDataToStream( BinaryOStream & stream ) const
{
	stream << id_;
	stream << lastMappedGeometry_;

	SpaceDataMapping::const_iterator iter = spaceDataMapping_.begin();
	SpaceDataMapping::const_iterator endIter = spaceDataMapping_.end();

	uint32 size = spaceDataMapping_.size();
	stream << size;

	DEBUG_MSG( "Space::writeDataToStream: size = %u\n", size );

	while (iter != endIter)
	{
		stream << iter->first << iter->second.key() << iter->second.data();
		DEBUG_MSG( "SpaceData: %s %d %s\n", iter->first.c_str(),
				iter->second.key(), iter->second.data().c_str() );
		++iter;
	}
}


/**
 *	This method reads space data from the input stream.
 */
void Space::readDataFromStream( BinaryIStream & stream )
{
	int size;
	stream >> size;

	for (int i = 0; i < size; i++)
	{
		SpaceEntryID entryID;
		uint16 key;
		BW::string value;
		stream >> entryID >> key >> value;

		this->spaceDataEntry( entryID, key, value, DONT_UPDATE_CELL_APP_MGR );
	}
}


/**
 *	This method streams on various boundaries to inform the CellAppMgr for the
 *	purposes of load balancing.
 *
 */
void Space::writeBounds( BinaryOStream & stream ) const
{
	if (this->isShuttingDown())
	{
		return;
	}

	stream << this->id();

	this->writeEntityBounds( stream );
	this->writeChunkBounds( stream );

	// Number of entities including ghosts.
	stream << uint32( this->spaceEntities().size() );
}


/**
 *	This method writes the entity bounds to inform the CellAppMgr. These are
 *	used in load balancing.
 */
void Space::writeEntityBounds( BinaryOStream & stream ) const
{
	// This needs to match CellAppMgr's CellData::updateEntityBounds

	// Args are isMax and isY
	this->writeEntityBoundsForEdge( stream, false, false ); // Left
	this->writeEntityBoundsForEdge( stream, false, true  ); // Bottom
	this->writeEntityBoundsForEdge( stream, true,  false ); // Right
	this->writeEntityBoundsForEdge( stream, true,  true  ); // Top
}


/**
 *	This method writes the chunk bounds to inform the CellAppMgr. These are
 *	used in load balancing.
 */
void Space::writeChunkBounds( BinaryOStream & stream ) const
{
	BW::Rect loadedBoundary( -FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX );
	this->calcLoadedRect( loadedBoundary );

	BoundingBox bbox = this->pPhysicalSpace_->bounds();
	BW::Rect spaceBoundary( bbox.minBounds().x, bbox.minBounds().z,
								bbox.maxBounds().x, bbox.maxBounds().z );

	float gridSize;
	
	if ( this->pChunkSpace() )
	{
		gridSize = this->pChunkSpace()->gridSize();	
	}
	else 
	{
		gridSize = std::min( spaceBoundary.range1D( false ).length(),
							 spaceBoundary.range1D( true ).length() );
	}
	
	stream << loadedBoundary;
	stream << spaceBoundary;
	stream << gridSize;
}


/// Sequence number that is incremented when any data in any space changes
uint32 Space::s_allSpacesDataChangeSeq_ = 0;


// -----------------------------------------------------------------------------
// Section: Script functions
// -----------------------------------------------------------------------------



BW_END_NAMESPACE

#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_manager.hpp"

#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"

BW_BEGIN_NAMESPACE



/*~ function BigWorld addSpaceData
 *  @components{ cell }
 *  Adds a key/value pair (space data) to the given space.
 *  Old values associated with the input key are kept.
 *  Consider using BigWorld.setSpaceData if old values are no
 *  longer required.
 *
 *	If the given key is not in the valid user range of 256 to 32767, or if the
 *	spaceID does not refer to a valid space, then a ValueError is raised.
 *
 *	@see BigWorld.setSpaceData
 *	@see BigWorld.getSpaceDataFirstForKey
 *	@see BigWorld.delSpaceDataForKey
 *	@see BigWorld.delSpaceData
 *	@see BigWorld.getSpaceDataForKey
 *	@see BigWorld.getSpaceData
 *
 *  @param spaceID	the space in which to add the data
 *  @param key		the key under which to add the space data (between 256 and
 *						32767)
 *  @param value	the string data to add for this key
 *  @return			the new space data entry id for the added data
 */
/**
 *	This function allows scripts to add their own space data
 */
static PyObject * addSpaceData( SpaceID spaceID, uint16 key,
	const BW::string & value, bool userOnly = true )
{
	if ((key < SPACE_DATA_FIRST_USER_KEY || key > SPACE_DATA_FINAL_USER_KEY) &&
		userOnly)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.addSpaceData: "
			"User space data key must be between %d and %d (not %d)",
			SPACE_DATA_FIRST_USER_KEY, SPACE_DATA_FINAL_USER_KEY, int(key) );
		return NULL;
	}

	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (pSpace == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.addSpaceData: "
			"No space ID %d", int(spaceID) );
		return NULL;
	}

	SpaceEntryID entryID = CellApp::instance().nextSpaceEntryID();

	// and add it
	pSpace->spaceDataEntry( entryID, key, value, Space::UPDATE_CELL_APP_MGR );

	// everything's hunky dory, so return the entry ID
	return Script::getData( entryID );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, addSpaceData, ARG( SpaceID,
	ARG( uint16, ARG( BW::string, END ) ) ), BigWorld )


/*~ function BigWorld delSpaceData
 *  @components{ cell }
 *
 *  Deletes the space data entry of the given id in the given space. Note that
 *	only the data known by the current CellApp is deleted from other CellApps.
 *
 *	A ValueError is raised if the space ID is invalid, or the entry ID does not
 *	refer to a mapped entry.
 *
 *  @see BigWorld.setSpaceData
 *  @see BigWorld.getSpaceDataFirstForKey
 *	@see BigWorld.delSpaceDataForKey
 *	@see BigWorld.addSpaceData
 *	@see BigWorld.getSpaceDataForKey
 *	@see BigWorld.getSpaceData
 *
 *  @param spaceID	the space in which to operate
 *  @param entryID	the entry to delete
 */
/**
 *	This function deletes space data from the given space.
 */
static bool delSpaceData( SpaceID spaceID, const SpaceEntryID & entryID )
{
	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (pSpace == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.delSpaceData() "
			"No space ID %d", int(spaceID) );
		return false;
	}

	BW::string emptyString;
	bool ok = pSpace->spaceDataEntry( entryID, uint16(-1), emptyString );
	if (!ok)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.delSpaceData(): "
			"Could not unmap entry id %s from space ID %d (no such entry)",
			entryID.c_str(), int( spaceID ) );
		return false;
	}

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, delSpaceData,
	ARG( SpaceID, ARG( SpaceEntryID, END ) ), BigWorld )

/*~ function BigWorld delSpaceDataForKey
 *  @components{ cell }
 *  Deletes all space data for a given key in the given space.
 *
 *  @see BigWorld.setSpaceData
 *  @see BigWorld.getSpaceDataFirstForKey
 *	@see BigWorld.addSpaceData
 *	@see BigWorld.delSpaceData
 *	@see BigWorld.getSpaceDataForKey
 *	@see BigWorld.getSpaceData
 *
 *  @param spaceID the space in which to operate
 *  @param key the key to delete space data for
 */
/**
 *	This function deletes all space data under the given key
 */
static bool delSpaceDataForKey( SpaceID spaceID, uint16 key )
{
	if (key == uint16(-1))
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.delSpaceDataForKey: "
			"Illegal key %d", int(key) );
		return false;
	}

	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (pSpace == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.delSpaceDataForKey: "
			"No space ID %d", (int)spaceID );
		return false;
	}

	// find all the existing entries with the same key
	BW::vector<SpaceEntryID> deadEntries;
	SpaceDataMapping::const_iterator it =
			pSpace->spaceDataMapping().begin();
	SpaceDataMapping::const_iterator endIter =
			pSpace->spaceDataMapping().end();
	for (; it != endIter; ++it)
	{
		if (it->second.key() == key)
		{
			deadEntries.push_back( it->first );
		}
	}

	// and remove them
	BW::string emptyString;
	for (uint i = 0; i < deadEntries.size(); i++)
	{
		MF_VERIFY( pSpace->spaceDataEntry(
			deadEntries[i], uint16(-1), emptyString ) );
	}

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, delSpaceDataForKey,
	ARG( SpaceID, ARG( uint16, END ) ), BigWorld )

/*~ function BigWorld getSpaceData
 *  @components{ cell }
 *  Gets space data for a given entry id in the given space.
 *
 *  @see BigWorld.setSpaceData
 *  @see BigWorld.getSpaceDataFirstForKey
 *	@see BigWorld.delSpaceDataForKey
 *	@see BigWorld.addSpaceData
 *	@see BigWorld.delSpaceData
 *	@see BigWorld.getSpaceDataForKey
 *
 *  @param spaceID the space in which to operate
 *  @param entryID the entry to get data for
 *  @param key the key of the entry. (deprecated: the key value is ignored).
 *  @return a tuple of (key, value) for the entry if found
 */
/**
 *	This function allows script to retrieve space data given an entry id.
 *	Pass the key in if you know it to improve lookup time.
 */
static PyObject * getSpaceData( SpaceID spaceID, const SpaceEntryID & entryID,
	uint16 key = uint16(-1) )
{
	/*
	if (key == uint16(-1))
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.getSpaceData: "
			"Illegal key %d", int(key) );
		return NULL;
	}
	*/

	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (pSpace == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.getSpaceData: "
			"No space ID %d", int(spaceID) );
		return NULL;
	}

	SpaceDataMapping::DataValue ret =
		pSpace->spaceDataMapping().dataRetrieveSpecific( entryID );
	if (!ret.valid())
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.getSpaceData(): "
			"Could not retrieve entry id %s key hint %d "
			"from space ID %d (no such entry)",
			entryID.c_str(), int( key ), int( spaceID ) );
		return NULL;
	}

	PyObject * pTuple = PyTuple_New( 2 );
	PyTuple_SET_ITEM( pTuple, 0, Script::getData( ret.key() ) );
	PyTuple_SET_ITEM( pTuple, 1, Script::getData( ret.data() ) );
	return pTuple;
}
PY_AUTO_MODULE_FUNCTION( RETOWN, getSpaceData,
	ARG( SpaceID, ARG( SpaceEntryID, OPTARG( uint16, uint16(-1), END ) ) ),
	BigWorld )


/*~ function BigWorld getSpaceDataForKey
 *  @components{ cell }
 *  Gets all space data for a key in the given space.
 *
 *  @see BigWorld.setSpaceData
 *  @see BigWorld.getSpaceDataFirstForKey
 *	@see BigWorld.delSpaceDataForKey
 *	@see BigWorld.addSpaceData
 *	@see BigWorld.delSpaceData
 *	@see BigWorld.getSpaceData
 *
 *  @param spaceID the space in which to operate
 *  @param key the key to get data for
 *  @return a tuple of (entryID, value) for each entry under the given key
 */
/**
 *	This function allows script to retrieve space data from a key.
 *	A tuple of (entry id, value) is returned for each entry.
 *	No entries => empty tuple.
 */
static PyObject * getSpaceDataForKey( SpaceID spaceID, uint16 key )
{
	if (key == uint16(-1))
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.getSpaceDataForKey: "
			"Illegal key %d", int(key) );
		return NULL;
	}

	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (pSpace == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.getSpaceDataForKey: "
			"No space ID %d", int(spaceID) );
		return NULL;
	}

	// first count them up
	SpaceDataMapping::const_iterator it;
	BW::vector< SpaceDataMapping::const_iterator > mappings;

	SpaceDataMapping::const_iterator beginIter =
			pSpace->spaceDataMapping().begin();
	SpaceDataMapping::const_iterator endIter =
			pSpace->spaceDataMapping().end();
	for (it = beginIter; it != endIter; ++it)
	{
		if (it->second.key() == key)
		{
			mappings.push_back( it );
		}
	}

	// now build the tuple
	PyObject * pTuple = PyTuple_New( mappings.size() );
	int idx = 0;
	BW::vector< SpaceDataMapping::const_iterator >::const_iterator vecIter;
	for (vecIter = mappings.begin(); vecIter != mappings.end(); ++vecIter)
	{
		it = *vecIter;
		PyObject * pElt = PyTuple_New( 2 );
		PyTuple_SET_ITEM( pElt, 0, Script::getData( it->first ) );
		PyTuple_SET_ITEM( pElt, 1, Script::getData( it->second.data() ) );
		PyTuple_SET_ITEM( pTuple, idx, pElt );
		idx++;
	}

	// and that is it
	return pTuple;
}
PY_AUTO_MODULE_FUNCTION( RETOWN, getSpaceDataForKey,
	ARG( SpaceID, ARG( uint16, END ) ), BigWorld )


/*~ function BigWorld getSpaceDataFirstForKey
 *  @components{ cell }
 *  Gets space data for a key in the given space. Only appropriate for keys that
 *	should have only one value.
 *
 *  @see BigWorld.setSpaceData
 *  @see BigWorld.delSpaceDataForKey
 *	@see BigWorld.addSpaceData
 *	@see BigWorld.delSpaceData
 *	@see BigWorld.getSpaceDataForKey
 *	@see BigWorld.getSpaceData
 *
 *  @param spaceID the space in which to operate
 *  @param key the key to get data for
 *  @return the value of the first entry for the key
 */
/**
 *	This function allows script to retrieve space data from a key.
 *	The value of the first entry for the key is returned.
 *	This is useful for semantics that desire only one entry per key.
 *	This function returns that most relevant entry (if one exists)
 *	according to the canonical sort order.
 */
static PyObject * getSpaceDataFirstForKey( SpaceID spaceID, uint16 key )
{
	if (key == uint16(-1))
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.getSpaceDataForKey: "
			"Illegal key %d", int(key) );
		return NULL;
	}

	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (pSpace == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.getSpaceDataForKey: "
			"No space ID %d", int(spaceID) );
		return NULL;
	}

	const BW::string * value =
			pSpace->spaceDataMapping().dataRetrieveFirst( key );
	if (value == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.getSpaceData(): "
			"Could not retrieve any values for key %d from space ID %d",
			int(key), int(spaceID) );
		return NULL;
	}
	return Script::getData( *value );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, getSpaceDataFirstForKey,
	ARG( SpaceID, ARG( uint16, END ) ), BigWorld )



/*~ function BigWorld setSpaceData
 *  @components{ cell }
 *  Sets space data for a key in the given space. This is a utility method that
 *	calls delSpaceDataForKey then addSpaceData. Only appropriate for keys that
 *	should have only one value.
 *
 *  @see BigWorld.getSpaceDataFirstForKey
 *	@see BigWorld.delSpaceDataForKey
 *	@see BigWorld.addSpaceData
 *	@see BigWorld.delSpaceData
 *	@see BigWorld.getSpaceDataForKey
 *	@see BigWorld.getSpaceData
 *
 *  @param spaceID the space in which to add the data
 *  @param key the key under which to add the space data (between 256 and
 *	32767). Keys between 256 and 16383 are sent to the client. Keys between
 *	16384 and 32767 are not sent to the client; they are only available to the
 *	CellApps. Keys 0 to 255 are reserved for internal use. 
 *  @param value the string data to add for this key
 *  @return the new space data entry id for the added data
 */
/**
 *	This function allows scripts to set their own space data
 */
static PyObject * setSpaceData( SpaceID spaceID, uint16 key,
	const BW::string & value, bool userOnly = true )
{
	// check this before we go and delete anything!
	if ((key < SPACE_DATA_FIRST_USER_KEY || key > SPACE_DATA_FINAL_USER_KEY) &&
		userOnly)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.setSpaceData: "
			"user space data key must be between %d and %d (not %d)",
			SPACE_DATA_FIRST_USER_KEY, SPACE_DATA_FINAL_USER_KEY, int(key) );
		return NULL;
	}

	if (!delSpaceDataForKey( spaceID, key )) return NULL;
	return addSpaceData( spaceID, key, value, userOnly );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, setSpaceData, ARG( SpaceID,
	ARG( uint16, ARG( BW::string, END ) ) ), BigWorld )


/*~ function BigWorld setSpaceTimeOfDay
 *  @components{ cell }
 *  Sets the factors for the time of day equation in the given space.
 *	Time of day is measured in seconds since midnight.
 *  @param spaceID the space in which to operate
 *  @param initialTimeOfDay the time of day at BigWorld.time() 0
 *  @param gameSecondsPerSecond the game seconds that pass for each real second
 *  @return the new space data entry id for the added data
 */
/**
 *	This function sets the time in the given space.
 */
static PyObject * setSpaceTimeOfDay( SpaceID spaceID,
	float initialTimeOfDay, float gameSecondsPerSecond )
{
	// just call setSpaceData with the appropriate data
	SpaceData_ToDData todData;
	todData.initialTimeOfDay = initialTimeOfDay;
	todData.gameSecondsPerSecond = gameSecondsPerSecond;
	BW::string value( (char*)&todData, sizeof(todData) );

	return setSpaceData( spaceID, SPACE_DATA_TOD_KEY, value,
		/*userOnly:*/ false );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, setSpaceTimeOfDay, ARG( SpaceID,
	ARG( float, ARG( float, END ) ) ), BigWorld )


/*~ function BigWorld setSpaceArtificialMinLoad
 *  @components{ cell }
 *  Sets the artificial minimal load for the given space.
 *  This will then be used in load balancing to avoid congestions of
 *  empty spaces on the same CellApp.
 *  @param spaceID the space in which to operate
 *  @param artificialMinLoad artificial minimal load for the given space.
 *  	Zero value will effectively disable the artificial load.
 *	If the spaceID does not refer to a valid space, then a ValueError
 *	is raised.
  */
/**
 *	This function sets the artificial min load for the given space.
 */
static bool setSpaceArtificialMinLoad( SpaceID spaceID,
	float artificialMinLoad )
{
	if (!delSpaceDataForKey( spaceID, SPACE_DATA_ARTIFICIAL_LOAD ))
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.setSpaceArtificialMinLoad: "
			"No space ID %d", int(spaceID) );
		// delSpaceDataForKey only returns false on a bad key or
		// non-existing space, not on not found key
		return false;
	}

	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (pSpace == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.setSpaceArtificialMinLoad: "
			"No space ID %d", int(spaceID) );
		return false;
	}

	SpaceData_ArtificialLoad loadData;
	loadData.artificialMinLoad = artificialMinLoad;
	BW::string value( (char*)&loadData, sizeof(loadData) );

	SpaceEntryID entryID = CellApp::instance().nextSpaceEntryID();

	bool res = pSpace->spaceDataEntry( entryID, SPACE_DATA_ARTIFICIAL_LOAD,
				value, Space::UPDATE_CELL_APP_MGR );
	if (!res)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.setSpaceArtificialMinLoad: "
			"failed to set space data entry for space ID %d", int(spaceID) );

		return false;
	}

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, setSpaceArtificialMinLoad, ARG( SpaceID,
	ARG( float, END ) ), BigWorld )


/*~	function BigWorld addSpaceGeometryMapping
 *	@components{ cell }
 *	This method associates a geometry mapping to the given space. This causes
 *	geometry to be loaded on the client and server.
 *
 *	On the server, the geometry from all chunks in the given directory is loaded
 *	into the specified space. The chunk files are loaded asynchronously,
 *	and when the geometry is fully loaded the following notification method is
 *	called on the personality module:
 *	@{
 *		def onAllSpaceGeometryLoaded( self, spaceID, mappingName ):
 *	@}
 *
 *	The server only loads the data it needs. For example, it only loads the BSP
 *	for each model, it does not load textures or the model's full mesh.
 *
 *	Terrain height data is loaded, but not terrain texturing data.
 *	To save memory, the server can load lower resolutions of the terrain height
 *	data.  This is specified in the space.settings file in the space directory.
 *	The tag <terrain/lodInfo/server/heightMapLod> governs which LOD level is
 *	chosen. The tag value can be set from 0 to 6.  Each level is half the
 *	resolution of the previous one, so 0 is the full mesh resolution, 1 is
 *	half the resolution, etc.
 *
 *	There is a possibility that onAllSpaceGeometryLoaded() will not be
 *	called if the CellAppMgr crashes at the precise moment multiple CellApps
 *	simultaneously call this method to add geometry to the same space.
 *
 *	The given transform must be aligned to the chunk grid. That is, it should
 *	be a translation matrix whose position is in multiples of 100 on the
 *	X and Z axis. Any other transform will result in undefined behaviour.
 *
 *	Any extra space mapped in must use the same terrain system as the first,
 *	with the same settings, the behaviour of anything else is undefined.
 *
 *	@param spaceID the space in which to operate
 *	@param mapper the matrix to map the chunks in at (or None = identity)
 *	@param path the path to the directory that contains the chunks
 *	@param shouldLoadOnServer Optional boolean argument indicates whether to
 *		load the geometry on the server. Defaults to True.
 *
 *	@return the new space data entry id for the added data
 */
/**
 *	This function maps geometry into the given space.
 *
 *	It returns a reference which can later be used to unmap it.
 */
static PyObject * addSpaceGeometryMapping( SpaceID spaceID,
	MatrixProviderPtr pMapper, const BW::string & path,
	bool shouldLoadOnServer )
{
	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (pSpace == NULL || pSpace->pCell() == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.addSpaceGeometryMapping() "
			"No space ID %d", int(spaceID) );
		return NULL;
	}

	SpaceEntryID entryID = CellApp::instance().nextSpaceEntryID();

	// get the matrix
	Matrix m = Matrix::identity;
	if (pMapper && pSpace->pChunkSpace())
	{
		ChunkSpacePtr pChunkSpace = pSpace->pChunkSpace();

		pMapper->matrix( m );

		Vector3 translation = m.applyToOrigin();

		if (!almostZero( fmod( translation.x, pChunkSpace->gridSize() ) ) ||
			!almostZero( fmod( translation.z, pChunkSpace->gridSize() ) ))
		{
			ERROR_MSG( "addSpaceGeometryMapping '%s'. Translation needs to be "
					"aligned to %.1f metre grid but is (%.1f, %.1f, %.1f)\n",
				path.c_str(), pChunkSpace->gridSize(),
				translation.x, translation.y, translation.z );
		}

		float yaw = m.yaw();
		float pitch = m.pitch();
		float roll = m.roll();

		if (!almostZero( fmod( yaw, MATH_PI/2.f ) ) ||
			!almostZero( pitch ) ||
			!almostZero( roll ))
		{
			ERROR_MSG( "addSpaceGeometryMapping '%s'. Rotation needs to be "
					"axis aligned. yaw = %.2f. pitch = %.2f. roll = %.2f\n",
				path.c_str(),
				RAD_TO_DEG( yaw ), RAD_TO_DEG( pitch ), RAD_TO_DEG( roll ) );
		}
	}

	if (shouldLoadOnServer)
	{
		pSpace->pPhysicalSpace()->loadResource( entryID, path, m );	
		pSpace->onNewGeometryMapping();
		DEBUG_MSG( "addSpaceGeometryMapping: Added mapping %s\n",
				path.c_str() );
	}

	/*
	float x1 = (float)pChunkSpace->minGridX() * pChunkSpace->gridSize();
	float y1 = (float)pChunkSpace->minGridY() * pChunkSpace->gridSize();
	float x2 = (float)(pChunkSpace->maxGridX()+1) * pChunkSpace->gridSize();
	float y2 = (float)(pChunkSpace->maxGridY()+1) * pChunkSpace->gridSize();
	DEBUG_MSG( "add Mapping bounding rectangle: %f %f %f %f\n", x1, y1, x2, y2 );
	*/

	// yes, let's tell the world!
	BW::string value( (char*)(float*)m, sizeof(Matrix) );
	value.append( path );
	pSpace->setLastMappedGeometry( path );
	pSpace->spaceDataEntry( entryID,
			shouldLoadOnServer ?
				SPACE_DATA_MAPPING_KEY_CLIENT_SERVER :
				SPACE_DATA_MAPPING_KEY_CLIENT_ONLY,
			value, Space::UPDATE_CELL_APP_MGR, Space::ALREADY_EFFECTED );

	// everything's hunky dory, so return the entry ID
	return Script::getData( entryID );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, addSpaceGeometryMapping, ARG( SpaceID,
	ARG( MatrixProviderPtr,
	ARG( BW::string,
	OPTARG( bool, true, END ) ) ) ), BigWorld )


/*
 *	This is a helper function for getSpaceGeometryMappings.
 */
void collectSpaceGeometryMappings( Space * pSpace,
	bool isClientOnly, PyObject * pResultList )
{
	uint16 key = isClientOnly ?
		SPACE_DATA_MAPPING_KEY_CLIENT_ONLY :
		SPACE_DATA_MAPPING_KEY_CLIENT_SERVER;

	SpaceDataMapping::const_iterator iter =
			pSpace->spaceDataMapping().begin();
	SpaceDataMapping::const_iterator endIter =
			pSpace->spaceDataMapping().end();

	for (; iter != endIter; ++iter)
	{
		if (iter->second.key() != key)
		{
			continue;
		}
		MF_ASSERT( iter->second.data().size() >= sizeof( Matrix ) );

		ScriptTuple tuple = ScriptTuple::create( 3 );
		MF_VERIFY( tuple.setItem( 0, ScriptObject::createFrom( iter->second.data().data() + sizeof( Matrix ) ) ) );
		MF_VERIFY( tuple.setItem( 1, ScriptObject::createFrom( *(Matrix *)iter->second.data().data() ) ) );
		MF_VERIFY( tuple.setItem( 2, ScriptObject::createFrom( isClientOnly ) ) );
		PyList_Append( pResultList, tuple.get() );
	}
}


/*~ function BigWorld.getSpaceGeometryMappings
 *	@components{ cell }
 *
 *	This function returns a list of the client-server geometry mappings of the
 *	space with the given space ID. Each list element consists of a tuple of the
 *	mapping, its transform matrix and a boolean indicating whether it is client
 *	only.
 *
 *	@param spaceID The id of the space to query.
 */
PyObject * getSpaceGeometryMappings( SpaceID spaceID )
{
	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (!pSpace)
	{
		PyErr_Format( PyExc_ValueError,
				"BigWorld.getSpaceGeometryMappings(): "
				"Space ID %d does not exist",
				int(spaceID));
		return NULL;
	}

	PyObject * pResultList = PyList_New( 0 );

	collectSpaceGeometryMappings( pSpace, false, pResultList );
	collectSpaceGeometryMappings( pSpace, true, pResultList );

	return pResultList;
}


PY_AUTO_MODULE_FUNCTION( RETOWN, getSpaceGeometryMappings, 
	ARG( SpaceID, END ), BigWorld )


/*~ function BigWorld timeOfDay
 *  @components{ cell }
 *	This function returns the current time of day in the given space
 *	@param spaceID the space to get the time of day for
 *	@return the time of day in seconds since midnight, or -1 if unset
 */
/**
 *	This function returns the current time of day in a space.
 */
float timeOfDay( SpaceID spaceID )
{
	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (pSpace == NULL)
	{
		return -1.f;
	}

	return pSpace->timeOfDay();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, timeOfDay, ARG( SpaceID, END ), BigWorld )


/*~ function BigWorld.isUnderWater
 *	@components{ cell }
 *
 *	This function tests if a point is under a water plane. It performs the test
 *	by casting a ray, from a given point in direction (0, 1, 0), through the
 *	collision scene. The test returns True if the ray intersects a water plane,
 *	otherwise False.
 *
 *	For example:
 *	@{
 *		>>> BigWorld.isUnderWater( spaceID, (0, -10, 0) )
 *		True
 *	@}
 *
 *	@param spaceID	The space in which to perform the test.
 *	@param point	The point to test.
 *
 *	@return			True if the point is under water, otherwise False.
 */
static PyObject * isUnderWater( SpaceID spaceID, const Vector3 & point )
{
	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (!pSpace)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.isUnderWater(): "
			"No space ID %d", int(spaceID) );
		return NULL;
	}

	//ToDo: remove the following if/when collide() is supported
	//by other (non-chunk) Physical Space implementations
	if (!pSpace->pChunkSpace())
	{
		PyErr_Format( PyExc_ValueError,
				"BigWorld.isUnderWater(): Space ID %d "
				"does not support collision queries",
			int(spaceID) );
		return NULL;
	}

	ClosestWater callback;
	Vector3 src( point.x, point.y, point.z );
	Vector3 dst( point.x, point.y + 1000000.f, point.z );
	pSpace->pChunkSpace()->collide( src, dst, callback );

	return Script::getData( callback.pWater() != NULL );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, isUnderWater, ARG( SpaceID,
	ARG( Vector3, END ) ), BigWorld )


/*~ function BigWorld.startRecording
 *	@components{ cell }
 *
 *	This function starts a space recording for the specified space.
 *	
 *	There can only be a single recording for a given space at any given time.
 *
 *	Callbacks on the personality script are called back during recording.
 *
 *	When recording starts, the callback onRecordingStarted() is called, though
 *	only once for the CellApp where the call to startRecording() was made. It
 *	is passed (spaceID, name) as arguments. 
 *
 *	During each tick of the recording duration, each CellApp with a cell in the
 *	recorded space will have onRecordingTickData() called, with arguments
 *	(spaceID, gameTime, name, numCells, data ).
 *
 *	@param spaceID 	The space ID.
 *	@param name 	The name of the recording. This is passed to the script
 *					callbacks and can be used to identify the recording by
 *					name.
 *	@param shouldRecordAoIEvents
					Whether AoI events should be recorded in the replay or not.
 *
 */
static bool startRecording( SpaceID spaceID, const BW::string & name, 
		bool shouldRecordAoIEvents )
{
	Space * pSpace = CellApp::instance().findSpace( spaceID );

	if ((pSpace == NULL) || (pSpace->pCell() == NULL))
	{
		PyErr_Format( PyExc_ValueError, "Invalid space ID: %d", spaceID ); 
		return false;
	}

	SpaceEntryID entryID = CellApp::instance().nextSpaceEntryID();

	if (!pSpace->pCell()->startRecording( entryID, name, 
			shouldRecordAoIEvents ))
	{
		PyErr_Format( PyExc_ValueError, "Space %d is already recording",
			spaceID );
		return false;
	}

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, startRecording, 
	ARG( SpaceID, ARG( BW::string, OPTARG( bool, false, END ) ) ), BigWorld )


/*~ function BigWorld.stopRecording
 *	@components{ cell }
 *
 *	This function stops any existing recording for the specified space.
 *
 *	After the next tick's data is recorded, onRecordingStopped() is called for
 *	the CellApp where the call to stopRecording() was made. It is passed
 *	(spaceID, name) as arguments.
 *
 *	@param spaceID 		The ID of the space to stop.
 */
static bool stopRecording( SpaceID spaceID )
{
	Space * pSpace = CellApp::instance().findSpace( spaceID );

	if ((pSpace == NULL) || (pSpace->pCell() == NULL))
	{
		PyErr_Format( PyExc_ValueError, "Invalid space ID: %d", spaceID ); 
		return false;
	}

	pSpace->pCell()->stopRecording( /* isInitial */ true );

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, stopRecording, 
	ARG( SpaceID, END ), BigWorld )


// -----------------------------------------------------------------------------
// Section: BSP
// -----------------------------------------------------------------------------

/**
 *	This method return the CellInfo at the input point.
 */
const CellInfo * Space::pCellAt( float x, float z ) const
{
	if (pCellInfoTree_ == NULL)
	{
		return NULL;
	}

	return pCellInfoTree_->pCellAt( x, z );
}


/**
 *	This method visits the cells that overlaps the input rectangle.
 */
void Space::visitRect( const BW::Rect & rect, CellInfoVisitor & visitRect )
{
	if (pCellInfoTree_ != NULL)
	{
		pCellInfoTree_->visitRect( rect, visitRect );
	}
}

/**
 *	This method reads a BSP from the stream.
 */
SpaceNode * Space::readTree( BinaryIStream & stream,
		const BW::Rect & rect )
{
	SpaceNode * pResult = NULL;
	uint8 type = 0xFF;
	stream >> type;

	switch (type)
	{
		case 0:
		case 1:
			pResult =
				new SpaceBranch( *this, rect,
						stream, type == 0 /*isHorizontal*/ );
			break;

		case 2:
		{
			Mercury::Address addr;
			stream >> addr;
			CellInfo * pCellInfo = this->findCell( addr );

			if (pCellInfo)
			{
				pCellInfo->shouldDelete( false );
				pCellInfo->rect( rect );
				pCellInfo->updateFromStream( stream );
				pResult = pCellInfo;
			}
			else
			{
				pCellInfo = new CellInfo( id_, rect, addr, stream );
				pResult = pCellInfo;
				cellInfos_[ addr ] = pCellInfo;
			}
			break;
		}

		default:
			ERROR_MSG( "Space::readTree: stream.error = %d. type = %d\n",
					stream.error(), type );
			MF_ASSERT( 0 );
			break;
	}

	return pResult;
}


/**
 *	This static method returns the CellInfo associated with the input address.
 */
CellInfo * Space::findCell( const Mercury::Address & addr ) const
{
	CellInfos::const_iterator iter = cellInfos_.find( addr );

	if (iter != cellInfos_.end())
	{
		return iter->second.get();
	}

	return NULL;
}


/**
 *	This method is called prior to a cell being deleted. This cell is then
 *	marked. Ghost entities should no longer be created on that cell.
 */
void Space::setPendingCellDelete( const Mercury::Address & addr )
{
	CellInfos::iterator iter = cellInfos_.find( addr );

	MF_ASSERT( iter != cellInfos_.end() );

	iter->second->setPendingDelete();
}


/**
 *	This method is called upon resource loading failed.
 */
void Space::onLoadResourceFailed( const SpaceEntryID & entryID )
{
	this->spaceDataEntry( entryID, uint16(-1), BW::string(),
			Space::UPDATE_CELL_APP_MGR, Space::NEED_TO_EFFECT );
}


BW_END_NAMESPACE

// space.cpp
