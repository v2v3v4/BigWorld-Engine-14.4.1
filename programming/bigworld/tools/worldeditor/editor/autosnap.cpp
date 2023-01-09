#include "pch.hpp"
#include "worldeditor/editor/autosnap.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "appmgr/options.hpp"

BW_BEGIN_NAMESPACE

typedef ChunkBoundary::Portal Portal;
typedef ChunkBoundary::Portals Portals;


typedef BW::vector< std::pair<Portal*, Portal*> > SnapHistory;
static SnapHistory s_snapHistory;
static Chunk* s_snapChunk = NULL;


void clearSnapHistory()
{
	BW_GUARD;

	s_snapHistory.clear();
	s_snapChunk = NULL;
}


SnappedChunkSet::SnappedChunkSet( const BW::set<Chunk*>& chunks )
	: chunks_( chunks.begin(), chunks.end() )
{
	BW_GUARD;

	for( BW::vector<Chunk*>::iterator iter = chunks_.begin();
		iter != chunks_.end(); ++iter )
	{
		for (ChunkBoundaries::iterator bit = (*iter)->joints().begin();
			bit != (*iter)->joints().end(); ++bit)
		{
			for( uint i=0; i < (*bit)->unboundPortals_.size(); ++i )
			{
				Portal* portal = (*bit)->unboundPortals_[i];

				if( !portal->hasChunk() && !portal->isInvasive() )
				{
					snapPortals_.push_back( portal );
					portalTransforms_.push_back( (*iter)->transform() );
				}
			}
			for( uint i=0; i < (*bit)->boundPortals_.size(); ++i )
			{
				Portal* portal = (*bit)->boundPortals_[i];

				if( portal->hasChunk() && !portal->isInvasive() &&
					chunks.find( portal->pChunk ) == chunks.end() )
				{
					snapPortals_.push_back( portal );
					portalTransforms_.push_back( (*iter)->transform() );
				}
			}
		}
	}
}

bool SnappedChunkSetSet::isConnected( Chunk* chunk1, Chunk* chunk2 ) const
{
	BW_GUARD;

	for (ChunkBoundaries::iterator bit = chunk1->joints().begin();
		bit != chunk1->joints().end(); ++bit)
	{
		for( uint i=0; i < (*bit)->boundPortals_.size(); ++i )
		{
			Portal* portal = (*bit)->boundPortals_[i];

			if( portal->hasChunk() && portal->pChunk == chunk2 )
				return true;
		}
	}
	return false;
}

SnappedChunkSetSet::SnappedChunkSetSet( const BW::vector<Chunk*>& chunks )
{
	BW_GUARD;

	BW::set<Chunk*> chunkSet( chunks.begin(), chunks.end() );
	init( chunkSet );
}

SnappedChunkSetSet::SnappedChunkSetSet( const BW::set<Chunk*>& chunks )
{
	BW_GUARD;

	init( chunks );
}

void SnappedChunkSetSet::init( BW::set<Chunk*> chunks )
{
	BW_GUARD;

	while( !chunks.empty() )
	{
		BW::set<Chunk*> set;
		set.insert( *chunks.begin() );
		chunks.erase( chunks.begin() );
		bool modified = true;
		while( modified )
		{
			modified = false;
			for( BW::set<Chunk*>::iterator iter = chunks.begin();
				iter != chunks.end() && !modified; ++iter )
			{
				for( BW::set<Chunk*>::iterator siter = set.begin();
					siter != set.end(); ++siter )
				{
					if( isConnected( *iter, *siter ) )
					{
						set.insert( *iter );
						chunks.erase( iter );
						modified = true;
						break;
					}
				}
			}
		}
		chunkSets_.push_back( SnappedChunkSet( set ) );
	}
}


/** Get all unbound portals for the given chunk */
void gatherUnboundPortals(Portals* portals, Chunk* chunk, Chunk* boundChunk = NULL )
{
	BW_GUARD;

	for (ChunkBoundaries::iterator bit = chunk->joints().begin();
		bit != chunk->joints().end();
		++bit)
	{
		for (uint i=0; i < (*bit)->unboundPortals_.size(); i++)
		{
			// get the portal
			Portal *& pp = (*bit)->unboundPortals_[i];
			Portal & p = *pp;

			// if it's not connected and it's not marked as invasive (ie, we
			// don't look at exit portals)
			if (!p.hasChunk() && !p.isInvasive())
				portals->push_back(&p);
		}
	}
	if( boundChunk )
	{
		for (ChunkBoundaries::iterator bit = chunk->joints().begin();
			bit != chunk->joints().end();
			++bit)
		{
			for (uint i=0; i < (*bit)->boundPortals_.size(); i++)
			{
				// get the portal
				Portal *& pp = (*bit)->boundPortals_[i];
				Portal & p = *pp;

				if (p.hasChunk() && p.pChunk == boundChunk )
					portals->push_back(&p);
			}
		}
	}
}

/**
 * Get all bound portals for the given chunk, unless they conned to a chunk
 * in excludeChunks
 */
void gatherBoundPortals(Portals* portals, Chunk* chunk,
						BW::vector<Chunk*> excludeChunks = BW::vector<Chunk*>())
{
	BW_GUARD;

	for (ChunkBoundaries::iterator bit = chunk->joints().begin();
		bit != chunk->joints().end();
		++bit)
	{
		for (uint i=0; i < (*bit)->boundPortals_.size(); i++)
		{
			// get the portal
			Portal *& pp = (*bit)->boundPortals_[i];
			Portal & p = *pp;

			// If the portal isn't connected to an outside chunk, we only deal
			// with internal portals here
			if (p.hasChunk() && !p.pChunk->isOutsideChunk())
				// If the bound portal isn't in excludeChunks
				if (std::find( excludeChunks.begin(), excludeChunks.end(), p.pChunk )
						== excludeChunks.end() )
					portals->push_back(&p);
		}
	}
}

/**
 * Check if the two portals could be matched from the edges
 * marked by startIndex
 * on return, n1 and n2 will contain the 2 normalised vectors of
 * the start edge of the portals, factor is an indicator for how
 * good the match is the smaller the better
 * if portal matches, the return value is true, otherwise false.
 */
static bool matchPortal( Portal* p1, int startIndex1,
						Portal* p2, int startIndex2,
						bool reverse, float* factor = NULL,
						Vector3* n1 = NULL, Vector3* n2  = NULL )
{
	BW_GUARD;

	if( factor )
		*factor = 0.f;
	unsigned int pointNum = static_cast<uint>(p1->points.size());
	for( unsigned int i = 0; i < pointNum; ++i )
	{
		Vector3 points1[3], points2[3];
		p1->objectSpacePoint( ( startIndex1 + i + pointNum ) % pointNum, points1[0] );
		p1->objectSpacePoint( ( startIndex1 + i + 1 + pointNum ) % pointNum, points1[1] );
		p1->objectSpacePoint( ( startIndex1 + i + 2 + pointNum ) % pointNum, points1[2] );

		if (reverse)
		{
			p2->objectSpacePoint( ( startIndex2 - i + 2 * pointNum ) % pointNum, points2[0] );
			p2->objectSpacePoint( ( startIndex2 - i - 1 + 2 * pointNum ) % pointNum, points2[1] );
			p2->objectSpacePoint( ( startIndex2 - i - 2 + 2 * pointNum ) % pointNum, points2[2] );
		}
		else
		{
			p2->objectSpacePoint( ( startIndex2 + i + 2 * pointNum ) % pointNum, points2[0] );
			p2->objectSpacePoint( ( startIndex2 + i + 1 + 2 * pointNum ) % pointNum, points2[1] );
			p2->objectSpacePoint( ( startIndex2 + i + 2 + 2 * pointNum ) % pointNum, points2[2] );
		}

		Vector3 edge11 = points1[1] - points1[0];
		Vector3 edge12 = points1[2] - points1[1];
		Vector3 edge21 = points2[1] - points2[0];
		Vector3 edge22 = points2[2] - points2[1];

		float length1 = edge11.length();
		float length2 = edge21.length();
		if( !almostEqual( length1, length2 ) )
			return false;

		edge11.normalise();
		edge21.normalise();
		edge12.normalise();
		edge22.normalise();
		float cos1 = edge11.dotProduct( edge12 );
		float cos2 = edge21.dotProduct( edge22 );
		if( !almostEqual( cos1, cos2 ) )
			return false;

		if( factor )
			*factor += edge11.dotProduct( edge21 );
		if( i == 0 )
		{
			if( n1 )
				*n1 = edge11;
			if( n2 )
				*n2 = edge21;
		}
	}
	return true;
}

/**
 * Check that the two given portals face opposite ways, and are the same size
 */
bool portalsMatch(Portal* p1, Portal* p2, Chunk* c1, Chunk* c2, unsigned int& targetIndex )
{
	BW_GUARD;

	if( p1->points.size() < 3 )
		return false;

	if( p1->points.size() != p2->points.size() )
		return false;

	for( targetIndex = 0; targetIndex < p2->points.size(); ++targetIndex )
	{
		if( matchPortal( p1, 0, p2, targetIndex, true ) )
			return true;
	}

	return false;
}

Matrix getRotatedTransform( Portal* source, Portal* target, Chunk* targetChunk )
{
	BW_GUARD;

	unsigned int pointSize = static_cast<uint>(source->points.size());

	Vector3 normal1 = source->plane.normal();
	Vector3 normal2 = targetChunk->transform().applyVector( -target->plane.normal() );

	normal1.normalise();
	normal2.normalise();

	Matrix rotation;
	Vector3 binormal;
	if( !almostEqual( fabs( normal1.dotProduct( normal2 ) ), 1 ) )
	{
		binormal = normal1.crossProduct( normal2 );
		binormal.normalise();

		float angle = acosf( Math::clamp(-1.0f, normal1.dotProduct( normal2 ), +1.0f) );

		Quaternion q;
		q.fromAngleAxis( angle, binormal );
		q.normalise();
		rotation.setRotate( q );
	}
	else
	{
		binormal = normal1.crossProduct( Vector3( 1, 0, 0 ) );
		if( almostZero( binormal.length() ) )
			binormal = normal1.crossProduct( Vector3( 0, 0, 1 ) );
		binormal.normalise();

		float angle = normal1.dotProduct( normal2 ) > 0 ? 0 : D3DX_PI;

		Quaternion q;
		q.fromAngleAxis( angle, binormal );
		q.normalise();
		rotation.setRotate( q );
	}

	Vector3 p0Src = source->objectSpacePoint( 0 );
	Matrix transform = Matrix::identity;

	transform.postTranslateBy( -p0Src );
	transform.postMultiply( rotation );

	Vector3 p1o0, p1o1, p2o0, p2o1;
	Vector3 portalOffset;

	float bestNormal = -2.f;

	unsigned int pointNum = static_cast<uint>(source->points.size());
	for( unsigned int startIndex2 = 0; startIndex2 < pointNum; ++startIndex2 )
	{
		float normal;
		Vector3 n1, n2;

		if (matchPortal( source, 0, target, startIndex2, true, &normal, &n1, &n2 ))
		{

			if (almostEqual( normal, bestNormal ))
			{
				n1 = transform.applyVector( n1 );
				n2 = targetChunk->transform().applyVector( n2 );
				n1.normalise();
				n2.normalise();

				if (n1.dotProduct( n2 ) > normal1.dotProduct( normal2 ))
				{
					normal1 = n1;
					normal2 = n2;
					bestNormal = normal;
					portalOffset = source->objectSpacePoint( 0 ) -
						targetChunk->transform().applyPoint( target->objectSpacePoint( startIndex2 ) );
				}
			}
			else if (normal > bestNormal)
			{
				normal1 = transform.applyVector( n1 );
				normal2 = targetChunk->transform().applyVector( n2 );
				normal1.normalise();
				normal2.normalise();
				bestNormal = normal;
				portalOffset = source->objectSpacePoint( 0 ) -
					targetChunk->transform().applyPoint( target->objectSpacePoint( startIndex2 ) );
			}
		}
	}

	if (!almostEqual( fabs( normal1.dotProduct( normal2 ) ), 1.f ))
	{
		binormal = normal1.crossProduct( normal2 );
		binormal.normalise();

		float angle = acosf( Math::clamp(-1.0f, normal1.dotProduct( normal2 ), +1.0f) );

		Quaternion q;
		q.fromAngleAxis( angle, binormal );
		q.normalise();

		rotation.setRotate( q );

		transform.postMultiply( rotation );
	}
	else
	{
		binormal = transform.applyVector( source->plane.normal() );

		if (normal1.dotProduct( normal2 ) < 0)
		{
			Quaternion q;
			q.fromAngleAxis( D3DX_PI, binormal );
			q.normalise();

			rotation.setRotate( q );

			transform.postMultiply( rotation );
		}
	}

	transform.postTranslateBy( p0Src );
	transform.postTranslateBy( -portalOffset );

	return transform;
}

Portal* getOtherSidePortal( Chunk* chunk, Portal* portal )
{
	BW_GUARD;

	if( !portal || !portal->hasChunk() || portal->isInvasive() )
		return NULL;
	for( ChunkBoundaries::iterator bit = portal->pChunk->joints().begin(); bit != portal->pChunk->joints().end(); ++bit )
	{
		for( uint i=0; i < (*bit)->boundPortals_.size(); ++i )
		{
			Portal* otherPortal = (*bit)->boundPortals_[i];

			if( otherPortal->hasChunk() && !otherPortal->isInvasive() &&
				otherPortal->pChunk == chunk )
			{
				Vector3 p1 = chunk->transform().applyPoint( portal->lcentre );
				Vector3 p2 = portal->pChunk->transform().applyPoint( otherPortal->lcentre );
				if( almostEqual( p1, p2 ) )
					return otherPortal;
			}
		}
	}
	return NULL;
}

std::pair<Portal*, Portal*> getSnappedPortals( Chunk* snapChunk, BW::vector<Chunk*> snapToChunks )
{
	BW_GUARD;

	for( BW::vector<Chunk*>::iterator iter = snapToChunks.begin(); iter != snapToChunks.end(); ++iter )
	{
		for( ChunkBoundaries::iterator bit = (*iter)->joints().begin(); bit != (*iter)->joints().end(); ++bit )
		{
			for( uint i=0; i < (*bit)->boundPortals_.size(); ++i )
			{
				Portal* portal = (*bit)->boundPortals_[i];

				if( portal->hasChunk() && !portal->isInvasive() &&
					portal->pChunk == snapChunk )
				{
					return std::make_pair( getOtherSidePortal( *iter, portal ), portal );
				}
			}
		}
	}
	return std::make_pair( (Portal*)NULL, (Portal*)NULL );
}

bool findAutoSnapTransform( Chunk* snapChunk, BW::vector<Chunk*> snapToChunks,
	Portals* snapPortals, Matrix* m )
{
	BW_GUARD;

	if( s_snapChunk != snapChunk )
		s_snapHistory.clear();
	s_snapChunk = snapChunk;

	Portal* snapPortal, *snapToPortal;
	Chunk* bestSnapToChunk = NULL;
	std::pair<Portal*, Portal*> snappedPortals = getSnappedPortals( snapChunk, snapToChunks );
	Portal* currentSnappingPortal = snappedPortals.second;
	Vector3 currentOffset( 0.f, 0.f, 0.f );
	SnapHistory::difference_type minIndex = (SnapHistory::difference_type)s_snapHistory.size();
	float minPitchAndRoll = 0.f;

	if( currentSnappingPortal && s_snapHistory.empty() )
		s_snapHistory.push_back( snappedPortals );

	enum
	{
		GET_ITEM_NOT_IN_HISTORY = 0,
		GET_ITEM_IN_HISTORY = 1,
		GET_ITEM_NUM = 2
	}
	getItemType = GET_ITEM_NOT_IN_HISTORY;
	while( getItemType != GET_ITEM_NUM )
	{
		// for each portal in snapPortals
		for (Portals::iterator si = snapPortals->begin(); si != snapPortals->end(); ++si)
		{
			// for each snapToChunk
			for (BW::vector<Chunk*>::iterator i = snapToChunks.begin(); i != snapToChunks.end(); ++i)
			{
				Chunk* snapToChunk = *i;

				// Get the free portals for snapToChunk
				Portals snapToPortals;
				gatherUnboundPortals(&snapToPortals, snapToChunk, snapChunk);

				// for each portal in snapToPortals
				for (Portals::iterator sti = snapToPortals.begin(); sti != snapToPortals.end(); ++sti)
				{
					// if they match
					unsigned int targetIndex;
					if ( portalsMatch(*si, *sti, snapChunk, snapToChunk, targetIndex) )
					{
						Vector3 portalOffset = snapChunk->transform().applyPoint( (*si)->objectSpacePoint( 0 ) ) -
							snapToChunk->transform().applyPoint( (*sti)->objectSpacePoint( targetIndex ) );

						SnapHistory::iterator iter = std::find( s_snapHistory.begin(), s_snapHistory.end(),
							std::make_pair( *si, *sti ) );

						Matrix m = getRotatedTransform( *si, *sti, snapToChunk );
						Matrix invChunkTransform = snapChunk->transform();
						invChunkTransform.invert();
						invChunkTransform.postMultiply( m );
						m = invChunkTransform;

						float pitchAndRoll = abs( m.pitch() ) + abs( m.roll() );

						if( almostZero( m.pitch() ) && almostZero( m.yaw() ) && almostZero( m.roll() ) &&
							almostZero( portalOffset.length() ) )
							continue;

						if( getItemType == GET_ITEM_NOT_IN_HISTORY )
						{
							if ( iter == s_snapHistory.end() )
							{
								if( bestSnapToChunk == NULL
									|| ( !almostEqual( pitchAndRoll, minPitchAndRoll ) && pitchAndRoll < minPitchAndRoll )
									|| ( almostEqual( pitchAndRoll, minPitchAndRoll ) &&
										( *sti == currentSnappingPortal ||
										portalOffset.lengthSquared() < currentOffset.lengthSquared() ) ) )
								{
									// set them to the best match
									currentOffset = portalOffset;
									bestSnapToChunk = snapToChunk;
									snapPortal = *si;
									snapToPortal = *sti;
									minPitchAndRoll = pitchAndRoll;
								}
							}
						}
						else if( getItemType == GET_ITEM_IN_HISTORY )
						{
							if ( iter != s_snapHistory.end() &&
								( bestSnapToChunk == NULL || std::distance( s_snapHistory.begin(), iter ) < minIndex ) )
							{
								// set them to the best match
								minIndex = std::distance( s_snapHistory.begin(), iter );
								currentOffset = portalOffset;
								bestSnapToChunk = snapToChunk;
								snapPortal = *si;
								snapToPortal = *sti;
								minPitchAndRoll = pitchAndRoll;
							}
						}
					}
				}
			}
		}
		if( bestSnapToChunk )
		{
			if( getItemType == GET_ITEM_IN_HISTORY )
				s_snapHistory.clear();
			break;
		}
		++(int&)getItemType;
	}

	if (bestSnapToChunk)
	{
		*m = getRotatedTransform( snapPortal, snapToPortal, bestSnapToChunk );

		s_snapHistory.push_back( std::make_pair( snapPortal, snapToPortal ) );
		return true;
	}
	else
	{
		return false;
	}
}

Matrix findAutoSnapTransform( Chunk* snapChunk, Chunk* snapToChunk )
{
	BW_GUARD;

	BW::vector<Chunk*> snapToChunks;
	snapToChunks.push_back( snapToChunk );

	return findAutoSnapTransform( snapChunk, snapToChunks );
}

Matrix findAutoSnapTransform( Chunk* snapChunk, BW::vector<Chunk*> snapToChunks )
{
	BW_GUARD;

	// Construct two lists of all the unbound portals on each chunk
	Portals snapPortals;
	Portals snapToPortals;

	// We're interested in all the portals on snapChunk, as we'll be
	// moving it and thus breaking any connections anyway
	gatherBoundPortals(&snapPortals, snapChunk);
	gatherUnboundPortals(&snapPortals, snapChunk);

	Matrix m;

	if (findAutoSnapTransform( snapChunk, snapToChunks, &snapPortals, &m ))
		return m;
	else
		return snapChunk->transform();
}

Matrix findRotateSnapTransform( const SnappedChunkSet& snapChunk, bool clockWise, Chunk* referenceChunk )
{
	BW_GUARD;

	BW::vector<Portal*> portals;
	BW::vector<Matrix> transforms;
	{for( unsigned int i = 0; i < snapChunk.portalSize(); ++i )
	{
		if( snapChunk.portal( i )->pChunk )
		{
			if( snapChunk.portal( i )->pChunk == referenceChunk )
			{
				portals.insert( portals.begin(), snapChunk.portal( i ) );
				transforms.insert( transforms.begin(), snapChunk.transform( i ) );
			}
			else
			{
				portals.push_back( snapChunk.portal( i ) );
				transforms.push_back( snapChunk.transform( i ) );
			}
		}
	}}
	{for( unsigned int i = 0; i < portals.size(); ++i )
	{
		Portal* portal = portals[ i ];

		unsigned int pointNum = static_cast<uint>(portal->points.size());
		for( unsigned int startIndex2 = clockWise ? 1 : -1;
			( startIndex2 + pointNum ) % pointNum != 0;
			clockWise ? ++startIndex2 : --startIndex2 )
		{
			Vector3 source;
			Vector3 target;
			if( !matchPortal( portal, 0, portal, startIndex2, false ) )
				continue;
			portal->objectSpacePoint( 0, source );
			portal->objectSpacePoint( ( startIndex2 + pointNum ) % pointNum, target );

			Matrix transform = transforms[ i ];
			Vector3 mean( 0.f, 0.f, 0.f );
			for( unsigned int index = 0; index < pointNum; ++index )
			{
				Vector3 op;
				portal->objectSpacePoint( index, op );
				mean += op;
			}
			if( pointNum )
				mean /= (float)pointNum;
			mean = transform.applyPoint( mean );

			Matrix translation1, rotation, translation2;
			translation1.setTranslate( -mean );
			translation2.setTranslate( mean );

			source = transform.applyPoint( source ) - mean;
			target = transform.applyPoint( target ) - mean;
			source.normalise();
			target.normalise();

			Vector3 axis = source.crossProduct( target );
			axis.normalise();

			float angle;
			if (almostEqual( source.dotProduct( target ), -1.f ))
			{
				angle = MATH_PI;
				axis = transform.applyVector( portal->plane.normal() );
				axis.normalise();
			}
			else
				angle = acosf( source.dotProduct( target ) );

			Quaternion q;
			q.fromAngleAxis( angle, axis );
			q.normalise();
			rotation.setRotate( q );

			transform = translation1;
			transform.postMultiply( rotation );
			transform.postMultiply( translation2 );
			return transform;
		}
	}}
	return Matrix::identity;
}
BW_END_NAMESPACE

