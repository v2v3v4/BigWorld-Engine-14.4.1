#include "pch.hpp"
#include "waypoint_view.hpp"

#include "waypoint/waypoint.hpp"

#include "cstdmf/debug.hpp"

#include "resmgr/bin_section.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"

DECLARE_DEBUG_COMPONENT2( "WayPoint", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: IWaypointView
// -----------------------------------------------------------------------------

/**
 *	This method writes the nav polys (replacing legacy waypoint polygons) in 
 *  this view to the given DataSection.
 *
 *	@param pSection Datasection to write to
 */
void IWaypointView::writeToSection( DataSectionPtr pSection, 
	Chunk * pChunk, float defaultGirth, bool removeOld ) const
{
	BW_GUARD;

	// first get rid of the old ones if desired.
	int maxSetNum = 0;
	int maxPolyNum = 0;
	if (removeOld)
	{
		while (1)
		{
			DataSectionPtr pWSect = pSection->openSection( "navPolySet" );
			// no navPolySets left? then try to find waypointSets.
			if (!pWSect)
			{
				pWSect = pSection->openSection( "waypointSet" );
			}
			// no waypointsets left then done.
			if (!pWSect) 
			{
				break;
			}
			pSection->delChild( pWSect );
		}
	}
	else
	{
		// make sure NavPoly / navPolySet ids do not overlap
		BW::string wsName("waypointSet");
		BW::string wpName("waypoint");
		BW::string nsName("navPolySet");
		BW::string npName("navPoly");
		for (int i = 0; i < pSection->countChildren(); i++)
		{
			DataSectionPtr wsds = pSection->openChild( i );
			if ( !((wsds->sectionName() == nsName) ||
				   (wsds->sectionName() == wsName)) ) continue;
			maxSetNum = std::max( wsds->asInt(), maxSetNum );
			for (int j = 0; j < wsds->countChildren(); j++)
			{
				DataSectionPtr wpds = wsds->openChild( j );
				if ( !((wpds->sectionName() == npName) ||
					   (wpds->sectionName() == wpName)) ) continue;
				maxPolyNum = std::max( wpds->asInt(), maxPolyNum );
			}
		}
	}

	// remove old setless waypoints / navpolys too
	while (1)
	{
		DataSectionPtr pWSect = pSection->openSection( "waypoint" );
		if (!pWSect) 
		{
			pWSect = pSection->openSection( "navPoly" );
		}
		if (!pWSect) 
		{
			break;
		}
		pSection->delChild( pWSect );
	}

	// now add the new navPolySets / navPolys.
	BW::map<int,DataSectionPtr> navPolySetSections;

	// for each polygon...
	for (int p = 0; p<this->getPolygonCount(); p++)
	{
		// get class wrapper around current polygon.
		PolygonRef polygon( *const_cast<IWaypointView*>(this), p );
		unsigned int vertexCount = polygon.size();

		// if this navPolySetIndex isn't found in the map, then it is inserted with the
		// default value of DataSectionPtr.
		DataSectionPtr & ourNavPolySet = navPolySetSections[ polygon.navPolySetIndex() ];

		// if navPolySetIndex not in map, then make a new section
		if (!ourNavPolySet)
		{
			ourNavPolySet = pSection->newSection( "navPolySet" );
			ourNavPolySet->setInt( polygon.navPolySetIndex() + maxSetNum );
			float girth = this->getGirth( polygon.navPolySetIndex() );
			if (girth <= 0.f) girth = defaultGirth;
			ourNavPolySet->writeFloat( "girth", girth );
		}

		// now add a new nav poly to the navPoly set.
		DataSectionPtr pNavPoly = ourNavPolySet->newSection( "navPoly" );
		pNavPoly->setInt( maxPolyNum+p+1 );

		// determine heights in local coords.
		Vector3 mhv = pChunk->transformInverse().applyPoint( 
							Vector3( pChunk->centre().x, polygon.minHeight(),
								pChunk->centre().z ) );
		Vector3 hv = pChunk->transformInverse().applyPoint( 
							Vector3( pChunk->centre().x, polygon.maxHeight(),
								pChunk->centre().z ) );
		pNavPoly->writeFloat( "minHeight", mhv.y );
		pNavPoly->writeFloat( "height", hv.y );

		// calculate vertices in local coords and write out. 
		for (unsigned int v = 0; v < vertexCount; v++)
		{
			DataSectionPtr pVertex = pNavPoly->newSection( "vertex" );
			VertexRef vr = polygon[v];
			int aNavPoly = vr.adjNavPoly();

			// if valid nav poly number then add maxpolynum to it for writing out..
			if (aNavPoly> 0 && aNavPoly <= this->getPolygonCount())
			{
				aNavPoly += maxPolyNum;
			}
			// vr.adjNavPoly will be zero if next to another chunk.
			// if we are next to another chunk, set the adjacent
			// waypoint value to the magic number that says so.
			// no longer specify the chunk we are next to in a 
			// separate tag.
			else if (vr.adjToAnotherChunk()) 
			{
				aNavPoly = CHUNK_ADJACENT_CONSTANT;
			}

			// convert vertex to local coords.
			Vector3 nc = pChunk->transformInverse().applyPoint( 
							Vector3( vr.pos().x, polygon.maxHeight(),
								vr.pos().y ) );

			// and write to datasection.
			pVertex->setVector3( Vector3(
				nc.x, nc.z, float( aNavPoly ) ) );
		}
	}

	INFO_MSG( "Wrote %d polygons to data section %s\n",
		this->getPolygonCount(), pSection->sectionName().c_str() );
}



/**
 *	This method saves out this view to the given chunk
 */
void IWaypointView::saveOut( Chunk * pChunk, float defaultGirth,
	bool removeAllOld )
{
	BW_GUARD;

	// get the chunk's cdata section
	DataSectionPtr pChunkBin = BWResource::openSection( pChunk->binFileName() );

	// build new navmesh
	BinaryPtr pNewNavmesh = this->asBinary( Matrix::identity, defaultGirth );

	// combine new and old unless all are being removed or there is no binary 
	// chunk yet
	if (!removeAllOld && pChunkBin)
	{
		// get old binary navmesh 
		BinaryPtr pOldNavmesh = pChunkBin->readBinary( "worldNavmesh" );

		BW::vector<int>	oldParts;
		size_t keepSize = 0;
		const char * dataPtr = pOldNavmesh->cdata();
		const char * dataEnd = dataPtr + pOldNavmesh->len();

		// build index of old navmesh
		while (dataPtr < dataEnd)
		{
			const char * dataElt = dataPtr;

			int aVersion = *((int*&)dataPtr)++;
			float aGirth = *((float*&)dataPtr)++;
			int navPolyElts = *((int*&)dataPtr)++;
			int navPolyEdges = *((int*&)dataPtr)++;
			// navPolySetEltSize

			dataPtr += navPolyElts * navPolyEltSize;
			dataPtr += navPolyEdges * navPolyEdgeEltSize;

			size_t offset = dataPtr - pOldNavmesh->cdata();
			if (aGirth == defaultGirth) offset |= (1<<31);
			else keepSize += dataPtr - dataElt;

			MF_ASSERT( offset <= INT_MAX );
			oldParts.push_back( (int ) offset );
		}

		// append parts that we care about onto new navmesh
		if (keepSize > 0)
		{
			BinaryPtr combo =
				new BinaryBlock( NULL, pNewNavmesh->len() + keepSize, "BinaryBlock/IWaypointView" );
			char * comboPtr = combo->cdata();
			memcpy( comboPtr, pNewNavmesh->cdata(), pNewNavmesh->len() );
			comboPtr += pNewNavmesh->len();

			int curOffset = 0;
			for (uint i = 0; i < oldParts.size(); ++i)
			{
				if (!(oldParts[i] >> 31))
				{
					memcpy( comboPtr, pOldNavmesh->cdata() + curOffset,
						oldParts[i]-curOffset );
					comboPtr += oldParts[i]-curOffset;
				}
				curOffset = oldParts[i] & ~(1<<31);
			}

			// phew!
			MF_ASSERT( comboPtr == combo->cdata()+combo->len() );
			pNewNavmesh = combo;
		}
	}

	// create cdata file if it didn't already exist
	if (!pChunkBin) 
	{
		DataSectionPtr pTemp = 
			new BinSection( "", new BinaryBlock( NULL, 0, "BinaryBlock/IWaypointView" ) );
		pTemp->newSection( "worldNavmesh" );
		pTemp->save( pChunk->binFileName() );

		// make sure it is in the cache
		pChunkBin = BWResource::openSection( pChunk->binFileName() );
	}

	// save it out
	pChunkBin->writeBinary( "worldNavmesh", pNewNavmesh );
	pChunkBin->save();

	// clear old style navmesh datasections from chunk file
	DataSectionPtr pChunkSect =
		BWResource::openSection( pChunk->resourceID() );

	const char* oldSects[] =
		{ "waypoint", "waypointSet", "navPoly", "navPolySet", "navmesh" };

	for (int i = 0; i < sizeof( oldSects ) / sizeof( *oldSects ); i++)
	{

		DataSectionPtr pOldData;
		while ((pOldData = pChunkSect->openSection( oldSects[i] )))
			pChunkSect->delChild( pOldData );
	}

	// put in record of (latest :) new style navmesh
	pChunkSect->writeString( "worldNavmesh/resource",
		pChunk->identifier() + ".cdata/worldNavmesh" );

	pChunkSect->deleteSections( "boundary" );

	// and save that too
	pChunkSect->save();
}


#include "cstdmf/memory_stream.hpp"

// Helper struct for writing navpolys out as binary
struct PairOfStreams
{
	PairOfStreams() :
		pCtrl( new MemoryOStream() ),
		pData( new MemoryOStream() ),
		girth( 0.f ),
		count( 0 )
	{ }

	MemoryOStream * pCtrl;
	MemoryOStream * pData;
	float			girth;
	int				count;
	BW::map<int,int>	indexMap;
};
typedef BW::map<int,PairOfStreams> NavPolySets_asBinary_map;

/**
 *	This method returns a binary serialisation of this view.
 */
BinaryPtr IWaypointView::asBinary( const Matrix & transformToLocal,
	float defaultGirth )
{
	BW_GUARD;

	NavPolySets_asBinary_map	navPolySets;

	// preflight polygon indices for each nav poly set
	for (int p = 0; p < this->getPolygonCount(); p++)
	{
		PolygonRef polygon( *const_cast<IWaypointView*>(this), p );

		PairOfStreams & pos = navPolySets[ polygon.navPolySetIndex() ];

		if (pos.girth <= 0.f)
		{
			pos.girth = this->getGirth( polygon.navPolySetIndex() );
			if (pos.girth <= 0.f)
				pos.girth = defaultGirth;
		}

		pos.indexMap[ p+1 ] = pos.count++;	// we are p+1 in adjacency field
	}

	// build the pairs of streams simultaneously for each polygon
	for (int p = 0; p < this->getPolygonCount(); p++)
	{
		PolygonRef polygon( *const_cast<IWaypointView*>(this), p );
		unsigned int vertexCount = polygon.size();

		// find the stream for its set
		PairOfStreams & pos = navPolySets[ polygon.navPolySetIndex() ];
		MemoryOStream & cmos = *pos.pCtrl;
		MemoryOStream & dmos = *pos.pData;

		// determine heights in local coords.
		Vector3 mhv = transformToLocal.applyPoint( Vector3(
			polygon[0].pos().x, polygon.minHeight(), polygon[0].pos().y ) );
		Vector3 Mhv = transformToLocal.applyPoint( Vector3(
			polygon[0].pos().x, polygon.maxHeight(), polygon[0].pos().y ) );

		// add it to the control stream
		cmos << mhv.y << Mhv.y << vertexCount;	// adds up to navPolyEltSize

		// add it to the data stream
		for (unsigned int v = 0; v < vertexCount; v++)
		{
			VertexRef vr = polygon[v];

			Vector3 nc = transformToLocal.applyPoint( Vector3(
				vr.pos().x, polygon.maxHeight(), vr.pos().y ) );
			dmos << nc.x << nc.z;

			int aNavPoly = vr.adjNavPoly();
			if (aNavPoly > 0 && aNavPoly <= this->getPolygonCount())
				aNavPoly = pos.indexMap[ aNavPoly ];	// adj to another poly
			else if (vr.adjToAnotherChunk())
				aNavPoly = 65535;						// adj to another chunk
			else if (aNavPoly <= 0)
				aNavPoly = ~-aNavPoly;					// adj to just a vista
			dmos << aNavPoly;
			// adds up to navPolyEdgEltSize

			// note it would be possible to halve the size of the navpoly vertex
			// by using grid co-ords and uint16s, but probably preferable to
			// avoid the conversion to floating point on load instead
		}
	}

	// figure out how big the data should be
	uint32	dataSize = 0;
	for (NavPolySets_asBinary_map::iterator it = navPolySets.begin();
		it != navPolySets.end();
		++it)
	{
		dataSize += navPolySetEltSize +
			it->second.pCtrl->size() + it->second.pData->size();
	}

	// allocate and build the data block
	BinaryPtr dataBlock = 
		new BinaryBlock( NULL, dataSize, "BinaryBlock/IWaypointView" );
	char * dataPtr = dataBlock->cdata();
	for (NavPolySets_asBinary_map::iterator it = navPolySets.begin();
		it != navPolySets.end();
		++it)
	{
		PairOfStreams & pos = it->second;
		*((int*&)dataPtr)++ = 0;	// version 0
		*((float*&)dataPtr)++ = pos.girth;
		*((int*&)dataPtr)++ = pos.pCtrl->size()/navPolyEltSize;
		*((int*&)dataPtr)++ = pos.pData->size()/navPolyEdgeEltSize;
		// adds up to navPolySetEltSize

		memcpy( dataPtr, pos.pCtrl->data(), pos.pCtrl->size() );
		dataPtr += pos.pCtrl->size();

		memcpy( dataPtr, pos.pData->data(), pos.pData->size() );
		dataPtr += pos.pData->size();

		// clean up the map
		bw_safe_delete(pos.pCtrl);
		bw_safe_delete(pos.pData);
	}

	// and we're done
	return dataBlock;
}

/**
 *	This method returns whether or not the given polygon index belongs
 *	to a set with girth equivalent to the given one.
 */
bool IWaypointView::equivGirth( int polygon, float girth ) const
{
	BW_GUARD;

	int set = this->getSet( polygon );
	if (set <= 0) return true;
	float g = this->getGirth( set );
	if (g == 0 || g == girth) return true;
	return false;
}

BW_END_NAMESPACE

// waypoint_view.cpp
