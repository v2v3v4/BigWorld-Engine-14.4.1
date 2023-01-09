#include "pch.hpp"

#include "chunk_waypoint_set.hpp"

#include "chunk_navigator.hpp"
#include "navigator_find_result.hpp"
#include "waypoint_stats.hpp"

#include "chunk/chunk_space.hpp"
#include "chunk/chunk_overlapper.hpp"

#if !defined( MF_SERVER )
#include "moo/debug_draw.hpp"
#include "moo/geometrics.hpp"
#endif // !defined( MF_SERVER )

DECLARE_DEBUG_COMPONENT2( "WayPoint", 0 )


BW_BEGIN_NAMESPACE

#if !defined( MF_SERVER )
/*static*/ bool ChunkWaypointSet::s_drawArrows_ = false;
/*static*/ bool ChunkWaypointSet::s_drawConnections_ = false;
#endif // !defined( MF_SERVER )

int ChunkWaypointSet_token = 1;


/**
 *	This is the ChunkWaypointSet constructor.
 */
ChunkWaypointSet::ChunkWaypointSet( ChunkWaypointSetDataPtr pSetData ):
	neighbourEdges_(),
	pSetData_( pSetData ),
	connections_(),
	edgeLabels_(),
	backlinks_()
{
	ChunkWaypoints::const_iterator wit;
	ChunkWaypoint::Edges::const_iterator eit;
	for (wit = pSetData_->waypoints().begin();
		wit != pSetData_->waypoints().end();
		++wit)
	{
		float wymin = wit->minHeight_;
		float wymax = wit->maxHeight_;
		float wyavg = (wymin + wymax) * 0.5f + 0.1f;
		for (eit = wit->edges_.begin(); eit != wit->edges_.end(); ++eit)
		{
			if (!eit->adjacentToChunk()) 
			{
				continue;
			}

			MF_ASSERT( neighbourEdges_.find(
				pSetData_->getAbsoluteEdgeIndex( *eit ) ) ==
					neighbourEdges_.end() );

			NeighbourEdge & rNewEdge =
				neighbourEdges_[ pSetData_->getAbsoluteEdgeIndex( *eit ) ];

			rNewEdge.isBound = false;

			rNewEdge.yValues[ 0 ] = wyavg;
			rNewEdge.yValues[ 1 ] =
				wymax + ChunkWaypoint::HEIGHT_RANGE_TOLERANCE;
			rNewEdge.yValues[ 2 ] =
				wymin - ChunkWaypoint::HEIGHT_RANGE_TOLERANCE;

			ChunkWaypoint::Edges::const_iterator nextEdgeIter = eit + 1;
			if (nextEdgeIter == wit->edges_.end())
			{
				nextEdgeIter = wit->edges_.begin();
			}

			const Vector2 & start = pSetData_->vertexByIndex( 
				eit->vertexIndex_ );
			const Vector2 & end = pSetData_->vertexByIndex( 
				nextEdgeIter->vertexIndex_ );

			rNewEdge.v = WaypointSpaceVector3( (start.x + end.x) / 2.f, 0.f,
				(start.y + end.y) / 2.f );
		}
	}

#ifndef MF_SERVER
	static bool firstTime = true;
	if (firstTime)
	{
		firstTime = false;
		MF_WATCH( "Client Settings/Physics/debug draw waypoints with arrows",
			s_drawArrows_,
			Watcher::WT_READ_WRITE,
			"Use arrows or lines to draw waypoint navpolys or connections" );
		MF_WATCH( "Client Settings/Physics/debug draw waypoint connections",
			s_drawConnections_,
			Watcher::WT_READ_WRITE,
			"Draw connections from the centre of the navpoly to the centre "
			"of other connected navpolys. Connections to another chunk are "
			"darker." );
	}
#endif // MF_SERVER
}


/**
 *	This is the ChunkWaypointSet destructor.
 */
ChunkWaypointSet::~ChunkWaypointSet()
{
}


/**
 *	If we have any waypoints with any edges that link to other waypont sets,
 *	remove our labels.  Remove us from the backlinks in the waypointSets we
 *	linked to, then delete our connections list.
 */
void ChunkWaypointSet::removeOurConnections()
{
	// first set all our external edge labels back to "other chunk".
	edgeLabels_.clear();

	// now remove us from all the backlinks of the chunks we put
	// ourselves in when we set up those connections.
	ChunkWaypointConns::iterator i = connections_.begin();
	for (; i != connections_.end(); ++i)
	{
		i->first->removeBacklink( this );
	}

	connections_.clear();

	// Mark all our neighbour edges as unbound
	for (NeighbourEdges::iterator iEdge = neighbourEdges_.begin();
		iEdge != neighbourEdges_.end(); ++iEdge)
	{
		iEdge->second.isBound = false;
	}
}


/**
 *	Remove the connection with index conNum from connections and set all edges
 *	corresponding to this connection to "other chunk".  Also remove us from the
 *	backlinks list in the waypointSet that the connection was to.
 *
 *	@param pSet			The ChunkWaypointSet to remove the connection to.
 */
void ChunkWaypointSet::deleteConnection( ChunkWaypointSetPtr pSet )
{
	ChunkWaypointConns::iterator found;
	if ((found = connections_.find( pSet )) == connections_.end())
	{
		ERROR_MSG( "ChunkWaypointSet::deleteConnection: connection from %u/%s "
				"to %u/%s does not exist\n",
			this->chunk()->space()->id(),
			this->chunk()->identifier().c_str(),
			pSet->chunk()->space()->id(),
			pSet->chunk()->identifier().c_str() );
		return;
	}

	// (1) remove our edge labels for this chunk waypoint set
	BW::vector< EdgeIndex > removingEdges;
	ChunkWaypointEdgeLabels::iterator edgeLabel = edgeLabels_.begin();
	for (;edgeLabel != edgeLabels_.end(); ++edgeLabel)
	{
		if (edgeLabel->second.pSet_ == pSet)
		{
			// add to removingEdges to remove it from the map after loop
			removingEdges.push_back( edgeLabel->first );
		}
	}

	while (removingEdges.size())
	{
		edgeLabels_.erase( removingEdges.back() );
		neighbourEdges_[ removingEdges.back() ].isBound = false;
		removingEdges.pop_back();
	}

	// (2) now remove from backlinks list of chunk we connected to.
	pSet->removeBacklink( this );


	// (3) now remove from us, and all trace of connection is gone!
	connections_.erase( pSet );

}


/**
 *	This goes through all waypoint sets that link to us, and tells them not to
 *	link to us anymore.
 */
void ChunkWaypointSet::removeOthersConnections()
{
	// loop around all our backlinks_ [waypoint sets that contain
	// edges that link back to us].

	while (backlinks_.size())
	{
		ChunkWaypointSetPtr pSet = backlinks_.back();

		// loop around the connections in this waypointSet with
		// a backlink to us looking for it..
		if (pSet->connections_.find( this ) == pSet->connectionsEnd())
		{
			ERROR_MSG( "ChunkWaypointSet::removeOthersConnections: "
				"Back connection not found.\n" );
			backlinks_.pop_back();
			continue;
		}

		// will delete the current backlink from our list.
		pSet->deleteConnection( ChunkWaypointSetPtr( this ) );
	}
}


/**
 *	This adds a waypointSet to our backlink list.
 *
 *	@param pWaypointSet		The ChunkWaypointSet to backlink to.
 */
void ChunkWaypointSet::addBacklink( ChunkWaypointSetPtr pWaypointSet )
{
	backlinks_.push_back( pWaypointSet );
}


/**
 *	This removes a waypointSet from our backlink list.
 *
 *	@param pWaypointSet		The ChunkWaypointSet to remove the backlink for.
 */
void ChunkWaypointSet::removeBacklink( ChunkWaypointSetPtr pWaypointSet )
{
	ChunkWaypointSets::iterator blIter =
		std::find( backlinks_.begin(), backlinks_.end(), pWaypointSet );

	if (blIter != backlinks_.end())
	{
		backlinks_.erase( blIter );
		return;
	}

	ERROR_MSG( "ChunkWaypointSet::removeBacklink: "
		"trying to remove backlink that doesn't exist\n" );
}


/**
 *	Print some debugging information for this ChunkWaypointSet.
 */
void ChunkWaypointSet::print() const
{
	DEBUG_MSG( "ChunkWayPointSet: 0x%p - %s\tWayPointCount: %" PRIzu "\n",
		this, pChunk_->identifier().c_str(), this->waypointCount() );

	for (ChunkWaypoints::size_type i = 0; i < waypointCount(); ++i)
	{
		waypoint( i ).print( *this );
	}

	for (ChunkWaypointConns::const_iterator iter = this->connectionsBegin();
			iter != this->connectionsEnd(); 
			++iter)
	{
		DEBUG_MSG( "**** connecting to 0x%p %s", 
			iter->first.get(), 
			iter->first->pChunk_->identifier().c_str() );
	}
}


/**
 *	 Connect edge in the current waypointSet to pWaypointSet.
 *
 *	(1)	If the connection doesn't exist in connections, this is created together
 *		with a backlink to us in pWaypointSet.
 *
 *	(2)	Update the edge Label.
 *
 *	@param pWaypointSet		The ChunkWaypointSet to connect to.
 *	@param otherWaypoint	The waypoint which this edge connects to
 *	@param pPortal			The connection portal.
 *	@param edge				The edge along which the ChunkWaypointSets are
 *							connected.
 */
void ChunkWaypointSet::connect(
						ChunkWaypointSetPtr pWaypointSet,
						WaypointIndex otherWaypoint,
						ChunkBoundary::Portal * pPortal,
						const EdgeIndex & rEdgeIndex,
						const Vector3 & entryPoint )
{
	ChunkWaypointConns::iterator found = connections_.find( pWaypointSet );

	if (found == connections_.end())
	{
		connections_[pWaypointSet] = pPortal;

		// now add backlink to chunk connected to.
		pWaypointSet->addBacklink( this );

	}
	else if (found->second != pPortal)
	{
		NOTICE_MSG( "ChunkWaypointSet::connect: "
				"Chunk %s is connected to chunk %s via two different portals. "
				"This may cause computed paths to be sub-optimal.\n"
				"Fix by changing the shell to make sure there is no more than "
				"one portal between adjacent shells.\n",
			chunk()->identifier().c_str(), 
			pWaypointSet->chunk()->identifier().c_str() );
	}

	EdgeLabel edgeLabel = { pWaypointSet, otherWaypoint, entryPoint };
	edgeLabels_[rEdgeIndex] = edgeLabel;
}


/**
 *	This adds or removes us from the given chunk.
 *
 *	@param pChunk			The chunk that the ChunkWaypointSet is being added.
 *							If this is NULL then the ChunkWaypointSet is being
 *							removed.
 */
void ChunkWaypointSet::toss( Chunk * pChunk )
{
	if (pChunk == pChunk_) return;

	// out with the old
	if (pChunk_ != NULL)
	{
		// Note: program flow arrives here when pChunk_ is
		// being ejected (with pChunk = NULL)

		//this->toWorldCoords();
		this->removeOthersConnections();
		this->removeOurConnections();

		ChunkNavigator::instance( *pChunk_ ).del( this );
	}

	this->ChunkItem::toss( pChunk );

	// and in with the new
	if (pChunk_ != NULL)
	{
		if (pChunk_->isBound())
		{
			CRITICAL_MSG( "ChunkWaypointSet::toss: "
				"Tossing after loading is not supported\n" );
		}

		//this->toLocalCoords() // ... now already there

		// now that we are in local co-ords we can add ourselves to the
		// cache maintained by ChunkNavigator
		ChunkNavigator::instance( *pChunk_ ).add( this );
	}
}


/*
 *	Check if ready to bind
 */
bool ChunkWaypointSet::readyToBind() const
{
	if (pChunk_->isOutsideChunk())
	{
		ChunkOverlappers::Overlappers& overlappers = 
			ChunkOverlappers::instance( *pChunk_ ).overlappers();

		for (ChunkOverlappers::Overlappers::iterator iter = overlappers.begin();
			iter != overlappers.end(); ++iter)
		{
			if (!(*iter)->pOverlappingChunk()->loaded())
			{
				return false;
			}
		}
	}

	for (ChunkBoundaries::iterator iter = pChunk_->joints().begin();
		iter != pChunk_->joints().end(); ++iter)
	{
		ChunkBoundary::Portals::iterator iPortal;
		for (iPortal = (*iter)->unboundPortals_.begin();
			iPortal != (*iter)->unboundPortals_.end();
			++iPortal)
		{
			// Heaven, Earth and Extern portals aren't a problem
			// for binding waypoint sets
			if ((*iPortal)->hasChunkOrIsNothing() || (*iPortal)->isInvasive())
			{
				return false;
			}
		}
	}

	for (Chunk::piterator iter = pChunk_->pbegin();
		iter != pChunk_->pend(); iter++)
	{
		if (iter->hasChunk() && !iter->pChunk->loaded())
		{
			return false;
		}
	}

	return true;
}


/**
 *	Bind any unbound waypoints
 */
void ChunkWaypointSet::bind()
{
	if (!this->readyToBind())
	{
		return;
	}

	// We would like to first make sure that all our existing connections
	// still exist.  Unfortunately we can't tell what is being unbound,
	// so we have to delay this until the set we are connected to is tossed
	// out of its chunk.

	// now make new connections

	NeighbourEdges::iterator eit;

	for (eit = neighbourEdges_.begin(); eit != neighbourEdges_.end(); ++eit )
	{
		if (eit->second.isBound)
		{
			continue;
		}


		const EdgeIndex & edgeIndex = eit->first;
		// http://www.cplusplus.com/forum/general/4125/#msg18297
		const float (& yValues)[3] = eit->second.yValues;
		const WaypointSpaceVector3 & v = eit->second.v;

		for (size_t i = 0; i < sizeof( yValues ) / sizeof( *yValues ); ++i)
		{

			WaypointSpaceVector3 wv = v;
			wv.y = yValues[ i ];
			Vector3 lwv = MappedVector3( wv, pChunk_ ).asChunkSpace();

			Chunk::piterator pit = pChunk_->pbegin();
			Chunk::piterator pnd = pChunk_->pend();
			ChunkBoundary::Portal * cp = NULL;
			const WorldSpaceVector3 wsv = MappedVector3( wv, pChunk_ );

			while (pit != pnd)
			{
				if (pit->hasChunk() && 
					pit->pChunk->boundingBox().intersects( wsv ))
				{
					// only use test for minimum distance from portal plane
					// for inside chunks
					float minDist = pit->pChunk->isOutsideChunk() ?
						0.f : 1.f;
					if (Chunk::findBetterPortal( cp, minDist, &*pit, lwv ))
					{
						cp = &*pit;
					}
				}
				pit++;
			}

			if (cp == NULL)
			{
				continue;
			}

			Chunk* pConn = cp->pChunk;

			// make sure we have a corresponding waypoint on the other side
			// of the portal.

			NavigatorFindResult navigatorFindResult;
			Vector3 ltpv = cp->origin + cp->uAxis*cp->uAxis.dotProduct( lwv ) +
				cp->vAxis*cp->vAxis.dotProduct( lwv );
			MappedVector3 tpwv( ltpv, pChunk_, MappedVector3::CHUNK_SPACE );

			if (!ChunkNavigator::instance( *pConn ).findExact(
				tpwv, this->girth(), navigatorFindResult ))
			{
				continue;
			}

			this->connect( navigatorFindResult.pSet().get(),
				navigatorFindResult.waypoint(), cp, edgeIndex, wsv );

			eit->second.isBound = true;

			break;
		}
	}
}


#ifndef MF_SERVER
/**
 *	Draw the nav-polys and connections between nav-polys (if enabled).
 */
void ChunkWaypointSet::drawDebug() const
{
	BW_GUARD;
	if (s_drawConnections_)
	{
		this->drawConnections();
	}
	this->drawNavPolys();
}


/**
 *	This method draws a visualisation of the chunk waypoint set.
 *	It draws the connections between waypoints as lines or arrows, from the
 *	centre of the given waypoint to the centre of the other waypoint.
 *	It draws a line from the waypoint's min height to max height.
 *	It draws outside chunks with different colours based on their
 *	position, inside chunks are always green. Connections to other
 *	chunks are drawn darker than connections within a chunk.
 *	Edge labels are drawn as orange lines.
 */
void ChunkWaypointSet::drawConnections() const
{
	BW_GUARD;
	
	MF_ASSERT( pChunk_ != NULL );

	// Line colours
	Vector3 colourValue( 0.0f, 0.0f, 0.0f );

	if (pChunk_->isOutsideChunk())
	{
		colourValue.x = static_cast< float >( pChunk_->x() ) * 0.5f + 0.5f;
		colourValue.y = 0.5f;
		colourValue.z = static_cast< float >( pChunk_->z() ) * 0.5f + 0.5f;
	}
	else
	{
		colourValue.y = 1.0f;
	}

	const Moo::Colour regularColour(
		colourValue.x, colourValue.y, colourValue.z, 1.0f );

	const Vector3 adjacentToChunkColourValue( 0.5f, 0.5f, 0.5f );

	Moo::Colour darkerColour = regularColour;
	darkerColour.r = Math::clamp( 0.f,
		regularColour.r - adjacentToChunkColourValue.x,
		1.0f );
	darkerColour.g = Math::clamp( 0.f,
		regularColour.g - adjacentToChunkColourValue.y,
		1.0f );
	darkerColour.b = Math::clamp( 0.f,
		regularColour.b - adjacentToChunkColourValue.z,
		1.0f );

	// Get each waypoint and draw the edges
	for (ChunkWaypoints::size_type i = 0; i != this->waypointCount(); ++i)
	{
		const ChunkWaypoint & wp = this->waypoint( i );

		const Vector3 waypointBottom( wp.centre_.x, wp.minHeight_, wp.centre_.y );
		const Vector3 waypointTop( wp.centre_.x, wp.maxHeight_, wp.centre_.y );

		const Vector3 start( waypointTop.x, waypointTop.y, waypointTop.z );

		const Moo::Colour* drawColour = &regularColour;

		// Edges
		for (ChunkWaypoint::Edges::const_iterator edgeItr = wp.edges_.begin();
			edgeItr != wp.edges_.end();
			++edgeItr)
		{
			const ChunkWaypoint::Edge& edge = (*edgeItr);
			
			if (edge.adjacentToChunk())
			{
				drawColour = &darkerColour;

				const int neighbourIndex = edge.neighbouringWaypoint();
				MF_ASSERT( neighbourIndex == -1 );
				
				const EdgeLabel & edgeLabel = this->edgeLabel ( edge );

				if (!edgeLabel.pSet_ || !edgeLabel.pSet_->chunk())
				{
					continue;
				}

				MF_ASSERT( edgeLabel.waypoint_ >= 0 );
				MF_ASSERT(
					edgeLabel.waypoint_ <
					static_cast< WaypointIndex >(
					edgeLabel.pSet_->waypointCount() ) );

				const ChunkWaypoint& neighbourWaypoint =
					edgeLabel.pSet_->waypoint( edgeLabel.waypoint_ );

				const Vector3 neighbourTop( neighbourWaypoint.centre_.x, 
					neighbourWaypoint.maxHeight_,
					neighbourWaypoint.centre_.y );

				const Vector3 end(
					neighbourTop.x, neighbourTop.y, neighbourTop.z );

				// Line between waypoints
				if (s_drawArrows_)
				{
					DebugDraw::arrowAdd( start, end, *drawColour );
				}
				else
				{
					Geometrics::drawLine( start, end, *drawColour );
				}
			}
			else
			{
				drawColour = &regularColour;

				const WaypointIndex neighbourIndex =
					edge.neighbouringWaypoint();
				MF_ASSERT( neighbourIndex >= 0 );
				MF_ASSERT( neighbourIndex <
					static_cast< WaypointIndex >( this->waypointCount() ) );

				const ChunkWaypoint& neighbourWaypoint =
					this->waypoint( neighbourIndex );

				const Vector3 neighbourTop( neighbourWaypoint.centre_.x, 
					neighbourWaypoint.maxHeight_,
					neighbourWaypoint.centre_.y );

				const Vector3 end(
					neighbourTop.x, neighbourTop.y, neighbourTop.z );

				// Line between waypoints
				if (s_drawArrows_)
				{
					DebugDraw::arrowAdd( start, end, *drawColour );
				}
				else
				{
					Geometrics::drawLine( start, end, *drawColour );
				}
			}
		}
	}
}


/**
 *	This method draws a visualisation of the chunk waypoint set's navpolys.
 *	It draws the connections between waypoints as lines or arrows, from the
 *	centre of the given waypoint to the centre of the other waypoint.
 *	It draws a line from the waypoint's min height to max height.
 *	It draws outside chunks with different colours based on their
 *	position, inside chunks are always green. Connections to other
 *	chunks are drawn darker than connections within a chunk.
 *	Edge labels are drawn as orange lines.
 */
void ChunkWaypointSet::drawNavPolys() const
{
	BW_GUARD;
	
	MF_ASSERT( pChunk_ != NULL );

	// Line colours
	Vector3 colourValue( 0.0f, 0.0f, 0.0f );

	if (pChunk_->isOutsideChunk())
	{
		colourValue.x = static_cast< float >( pChunk_->x() ) * 0.5f + 0.5f;
		colourValue.y = 0.5f;
		colourValue.z = static_cast< float >( pChunk_->z() ) * 0.5f + 0.5f;
	}
	else
	{
		// Green
		colourValue.y = 1.0f;
	}

	const Moo::Colour regularColour(
		colourValue.x, colourValue.y, colourValue.z, 1.0f );

	const Vector3 adjacentToChunkColourValue( 0.5f, 0.5f, 0.5f );

	Moo::Colour darkerColour = regularColour;
	darkerColour.r = Math::clamp( 0.f,
		regularColour.r - adjacentToChunkColourValue.x,
		1.0f );
	darkerColour.g = Math::clamp( 0.f,
		regularColour.g - adjacentToChunkColourValue.y,
		1.0f );
	darkerColour.b = Math::clamp( 0.f,
		regularColour.b - adjacentToChunkColourValue.z,
		1.0f );

	// Get each waypoint and draw the edges
	for (ChunkWaypoints::size_type i = 0; i != this->waypointCount(); ++i)
	{
		const ChunkWaypoint & waypoint = this->waypoint( i );

		const Vector3 navPolyBottom(
			waypoint.centre_.x, waypoint.minHeight_, waypoint.centre_.y );
		const Vector3 navPolyTop(
			waypoint.centre_.x, waypoint.maxHeight_, waypoint.centre_.y );

		const Vector3 navPolyCentre( navPolyTop.x, navPolyTop.y, navPolyTop.z );

		// Draw edges of navpoly
		ChunkWaypoint::Edges::const_iterator edgeItr = waypoint.edges_.begin();
		MF_ASSERT( edgeItr != waypoint.edges_.end() );
		const ChunkWaypoint::Edge* lastEdge = &(*edgeItr);
		++edgeItr;

		for (;
			edgeItr != waypoint.edges_.end();
			lastEdge = &(*edgeItr), ++edgeItr)
		{
			this->drawNavPolyEdge( (*edgeItr),
				*lastEdge,
				navPolyTop.y,
				regularColour,
				darkerColour );
		}
		this->drawNavPolyEdge( *lastEdge,
			*(waypoint.edges_.begin()),
			navPolyTop.y,
			regularColour,
			darkerColour );

		// WP Height
		if (!almostEqual( navPolyBottom.y, navPolyTop.y ))
		{
			if (s_drawArrows_)
			{
				DebugDraw::arrowAdd( navPolyBottom, navPolyTop, regularColour );
			}
			else
			{
				Geometrics::drawLine( navPolyBottom, navPolyTop, regularColour );
			}
		}
	}

	this->drawEdgeLabels();
}


/**
 *	Draw a single nav poly.
 *	@param waypoint the navpoly to draw.
 *	@param regularColour un-darkened colour to draw.
 *		Colour gets darker when edge is adjacent to a chunk.
 */
void ChunkWaypointSet::drawNavPolyEdge( const ChunkWaypoint::Edge& edge,
	const ChunkWaypoint::Edge& lastEdge,
	float height,
	const Moo::Colour& regularColour,
	const Moo::Colour& darkerColour ) const
{
	BW_GUARD;

	// Amount to darken poly
	const Moo::Colour* drawColour = &regularColour;

	const Vector2 & edgeStart =
		this->vertexByIndex( lastEdge.vertexIndex_ );
	const Vector2 & edgeEnd =
		this->vertexByIndex( edge.vertexIndex_ );

	const Vector3 start( edgeStart.x, height, edgeStart.y );
	const Vector3 end( edgeEnd.x, height, edgeEnd.y );

	if (lastEdge.adjacentToChunk() || edge.adjacentToChunk())
	{
		if (lastEdge.adjacentToChunk())
		{
			MF_ASSERT( lastEdge.neighbouringWaypoint() == -1 );
		}
		if (edge.adjacentToChunk())
		{
			MF_ASSERT( edge.neighbouringWaypoint() == -1 );
		}

		drawColour = &darkerColour;

		// Line between waypoints
		if (s_drawArrows_)
		{
			DebugDraw::arrowAdd( start, end, *drawColour );
		}
		else
		{
			Geometrics::drawLine( start, end, *drawColour );
		}
	}
	else
	{
		drawColour = &regularColour;

		//const int neighbourIndex =
		//	edge.neighbouringWaypoint();
		//MF_ASSERT( neighbourIndex >= 0 );
		//MF_ASSERT( neighbourIndex < this->waypointCount() );

		// Line between waypoints
		if (s_drawArrows_)
		{
			DebugDraw::arrowAdd( start, end, *drawColour );
		}
		else
		{
			Geometrics::drawLine( start, end, *drawColour );
		}
	}
}


/**
 *	Draw an orange line at each edge label.
 */
void ChunkWaypointSet::drawEdgeLabels() const
{
	BW_GUARD;
	for (ChunkWaypointEdgeLabels::const_iterator itr = this->edgeLabelsBegin();
		itr != this->edgeLabelsEnd();
		++itr)
	{
		const EdgeIndex& index = (*itr).first;
		const EdgeLabel& edgeLabel = (*itr).second;

		const Vector3 start( edgeLabel.entryPoint_ );
		const Vector3 end( start.x, start.y + 1.0f, start.z );

		// Orange
		if (s_drawArrows_)
		{
			DebugDraw::arrowAdd( start, end, 0xaaffaa00 );
		}
		else
		{
			Geometrics::drawLine( start, end, 0xaaffaa00 );
		}
	}
}
#endif // MF_SERVER

BW_END_NAMESPACE

// chunk_waypoint_set.cpp
