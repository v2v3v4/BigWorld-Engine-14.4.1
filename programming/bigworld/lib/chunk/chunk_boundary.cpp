#include "pch.hpp"

#include "chunk_boundary.hpp"


#ifndef MF_SERVER
#include "moo/render_context.hpp"
#include "moo/geometrics.hpp"
#include "chunk_manager.hpp"
#endif

#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"
#include "math/portal2d.hpp"

#include "chunk.hpp"
#include "chunk_space.hpp"
#include "geometry_mapping.hpp"
#include "portal_chunk_item.hpp"

#include "physics2/worldpoly.hpp"	// a bit unfortunate...

#include "cstdmf/config.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 );

BW_BEGIN_NAMESPACE

// Static initialiser.
bool ChunkBoundary::Portal::drawPortals_ = false;

namespace {

inline bool intersectLineSegmentCircle( const Vector2& p1, const Vector2& p2,
										const Vector2& pc, float r )
{
	Vector2 d = p2 - p1;	
	float t = -(p1 - pc).dotProduct( d ) / d.lengthSquared();
	t = std::min( 1.0f, std::max( 0.0f, t ) );
	return (p1 + t*d - pc).lengthSquared() < (r*r);
}

}


// ----------------------------------------------------------------------------
// Section: ChunkBoundary
// ----------------------------------------------------------------------------


/**
 *	Constructor and loader
 *	@param	pSection	the DataSection contains the data defining the boundary
 *	@param	pMapping	the chunk mapping of the chunk holding this boundary
 *	@param	ownerChunkName	the identifier of the chunk holding this boundary
 */
ChunkBoundary::ChunkBoundary( DataSectionPtr pSection,
							 GeometryMapping * pMapping,
							 const BW::string & ownerChunkName )
{
	BW_GUARD;
	// make sure something's there
	if (!pSection)
	{
		plane_ = PlaneEq( Vector3(0,0,0), 0 );
		return;
	}

	// read the plane
	Vector3 normal = pSection->readVector3( "normal" );
	float d = pSection->readFloat( "d" );
	float normalLen = normal.length();
	normal /= normalLen;
	d /= normalLen;
	plane_ = PlaneEq( normal, d );

	// prepare for some error checking
	bool someInternal = false;
	bool someExternal = false;

	// read any portals
	DataSectionIterator end = pSection->end();
	for (DataSectionIterator it = pSection->begin(); it != end; ++it)
	{
		if ((*it)->sectionName() != "portal") continue;

		Portal * newPortal = new Portal( *it, plane_,pMapping,ownerChunkName );

		if (newPortal->isEarth())
			boundPortals_.push_back( newPortal );
		else
			unboundPortals_.push_back( newPortal );

		if (newPortal->internal)
			someInternal = true;
		else
			someExternal = true;
	}

	// make sure no-one stuffed up
	MF_ASSERT_DEV(!(someInternal && someExternal))
}



/**
 * Quick constructor for ChunkBoundary
 *
 * @param  plane	plane used to define portal
 * @param  portal	prebuilt portal
 *
 */
ChunkBoundary::ChunkBoundary( const PlaneEq &plane, 
							  ChunkBoundary::Portal * portal )
{
	BW_GUARD;
	MF_ASSERT( portal );

	// read the plane
	plane_ = plane;

	// plane should be normalised.
	MF_ASSERT( almostEqual( plane_.normal().length(), 1.0f ) );

	// prepare for some error checking
	bool someInternal = false;
	bool someExternal = false;


	if (portal->isEarth())
		boundPortals_.push_back( portal );
	else
		unboundPortals_.push_back( portal );

	if (portal->internal)
		someInternal = true;
	else
		someExternal = true;

	// make sure no-one stuffed up
	MF_ASSERT_DEV( !( someInternal && someExternal ) )
}

/**
 *	Destructor
 */
ChunkBoundary::~ChunkBoundary()
{
	BW_GUARD;
	for (Portals::iterator pit = boundPortals_.begin();
		pit != boundPortals_.end();
		pit++)
	{
		bw_safe_delete(*pit);
	}
	boundPortals_.clear();

	for (Portals::iterator pit = unboundPortals_.begin();
		pit != unboundPortals_.end();
		pit++)
	{
		bw_safe_delete(*pit);
	}
	unboundPortals_.clear();
}

/**
 *	Move the given portal to the bound list
 *	@param	unboundIndex	the index of the unbound portal
 */
void ChunkBoundary::bindPortal( uint32 unboundIndex )
{
	BW_GUARD;
	Portal * pPortal = unboundPortals_[unboundIndex];
	boundPortals_.push_back( pPortal );
	unboundPortals_.erase( unboundPortals_.begin() + unboundIndex );
}

/**
 *	Move the given portal to the unbound list
 *	@param	boundIndex	the index of the bound portal
 */
void ChunkBoundary::unbindPortal( uint32 boundIndex )
{
	BW_GUARD;
	Portal * pPortal = boundPortals_[boundIndex];
	unboundPortals_.push_back( pPortal );
	boundPortals_.erase( boundPortals_.begin() + boundIndex );
}

/**
 *	This method adds a new invasive portal to the list of unbound portals
 *	in this boundary. Used only by editors.
 *	@param	pPortal	the portal to be added
 */
void ChunkBoundary::addInvasivePortal( Portal * pPortal )
{
	BW_GUARD;
	unboundPortals_.push_back( pPortal );
}

/**
 *	This method splits the identified invasive portal if necessary,
 *	i.e. if it overlaps chunks other than the one it points to.
 *	Also used only by editors. Note that this function is called on
 *	the inside chunk that already has an invasive portal, wheres the
 *	@see addInvasivePortal function is called on the invaded outside chunk.
 *	@param	pChunk	the reference chunk to split the portal
 *	@param	i	the index of the unbound portal
 */
void ChunkBoundary::splitInvasivePortal( Chunk * pChunk, uint i )
{
	BW_GUARD;
	return;
	Portal & p = *unboundPortals_[i];
	Chunk * pDest = p.pChunk;

	// get our plane in world coordinates
	const PlaneEq & srcLocalPlane = plane_;
	Vector3 ndtr = pChunk->transform().applyPoint(
		srcLocalPlane.normal() * srcLocalPlane.d() );
	Vector3 ntr = pChunk->transform().applyVector( srcLocalPlane.normal() );
	PlaneEq srcWorldPlane( ntr, ntr.dotProduct( ndtr ) );

	// make matrices to convert from portal space to world space and back again
	Matrix portalToWorld = Matrix::identity;
	portalToWorld[0] = p.uAxis;
	portalToWorld[1] = p.vAxis;
	portalToWorld[2] = p.plane.normal();
	portalToWorld[3] = p.origin;
	portalToWorld.postMultiply( pChunk->transform() );
	Matrix worldToPortal;
	worldToPortal.invert( portalToWorld );

	WorldPolygon srcPortalPoly;
	bool srcPortalPolyValid = false;

	// slice off a new portal for every boundary that intersects us
	for (ChunkBoundaries::iterator bit = pDest->bounds().begin();
		bit != pDest->bounds().end();
		++bit)
	{
		// get other plane in world coordinates
		const PlaneEq & dstLocalPlane = (*bit)->plane_;
		ndtr = pDest->transform().applyPoint(
			dstLocalPlane.normal() * dstLocalPlane.d() );
		ntr = pDest->transform().applyVector( dstLocalPlane.normal() );
		PlaneEq dstWorldPlane( ntr, ntr.dotProduct( ndtr ) );

		// if parallel or almost so then ignore it
		if (fabsf( dstWorldPlane.normal().dotProduct(
			srcWorldPlane.normal() ) ) > 0.99f)
				continue;

		Portal * pNewPortal = NULL;

		// see which points lie outside this boundary, and add them
		// to newPortal (under pChunk's transform) if we found any...
		// ... remove them from 'p' while we're at it.
		// binary/linear splits are fine - we can split either side again
		// if we have to (either later in this loop, or when this function is
		// called for pNewPortal)

		// we're going to use WorldPolygon to do our dirty work for us,
		// so first turn our current portal into a world coords if necessary
		if (!srcPortalPolyValid)
		{
			srcPortalPoly.clear();
			Vector3 worldPoint;
			for (uint j = 0; j < p.points.size(); ++j)
			{
				//pChunk->transform().applyPoint(
				//	worldPoint, p.origin +
				//		p.uAxis * p.points[j][0] + p.vAxis * (p.points[j][1] );
				Vector3 ptAug( p.points[j][0], p.points[j][1], 0.f );
				portalToWorld.applyPoint( worldPoint, ptAug );
				srcPortalPoly.push_back( worldPoint );
			}
			srcPortalPolyValid = true;
		}

		// ask the WorldPolygon to cleave itself in twain
		WorldPolygon insidePoly, outsidePoly;
		srcPortalPoly.split( dstWorldPlane, insidePoly, outsidePoly );

		// create the new portal and update the old
		if (!outsidePoly.empty())
		{
			MF_ASSERT_DEV( !insidePoly.empty() );

			Vector3 ptAug;
			Vector2 ptAvg;

			// new portal
			pNewPortal = new Portal( p );	// use copy constructor
			pNewPortal->points.resize( outsidePoly.size() );
			ptAvg.set( 0.f, 0.f );
			for (uint j = 0; j < outsidePoly.size(); ++j)
			{
				worldToPortal.applyPoint( ptAug, outsidePoly[j] );
				pNewPortal->points[j] = Vector2( ptAug[0], ptAug[1] );
				ptAvg += pNewPortal->points[j];
			}
			ptAvg /= float(pNewPortal->points.size());
			pNewPortal->lcentre = pNewPortal->uAxis * ptAvg.x +
				pNewPortal->vAxis * ptAvg.y + pNewPortal->origin;
			pChunk->transform().applyPoint(
				pNewPortal->centre, pNewPortal->lcentre );

			// old portal
			p.points.resize( insidePoly.size() );
			ptAvg.set( 0.f, 0.f );
			for (uint j = 0; j < insidePoly.size(); ++j)
			{
				worldToPortal.applyPoint( ptAug, insidePoly[j] );
				p.points[j] = Vector2( ptAug[0], ptAug[1] );
				ptAvg += p.points[j];
			}
			ptAvg /= float(p.points.size());
			p.lcentre = p.uAxis * ptAvg.x + p.vAxis * ptAvg.y + p.origin;
			pChunk->transform().applyPoint( p.centre, p.lcentre );
			srcPortalPolyValid = false;

			// don't really like these double-transformations, hmm
			// maybe better to bring plane into portal space and
			// pretend it's world space... still would need to
			// augment p.points to Vector3s .. 'tho it'd be trivial now...
		}

		// if we made a portal then add it
		if (pNewPortal != NULL)
		{
			DEBUG_MSG( "ChunkBoundary::splitInvasivePortal: "
				"Split portal in %s since it extends outside first hit %s\n",
				pChunk->identifier().c_str(), pDest->identifier().c_str() );

			pNewPortal->pChunk = (Chunk*)Portal::INVASIVE;
			unboundPortals_.push_back( pNewPortal );
		}
	}
}



// ----------------------------------------------------------------------------
// Section: ChunkBoundary::Portal
// ----------------------------------------------------------------------------

/**
 *	Constructor and loader
 *	@param	pSection	the DataSection contains the data defining the portal
 *	@param	iplane	the plane equation contains the data defining the portal
 *	@param	pMapping	the chunk mapping of the chunk holding this portal
 *	@param	ownerChunkName	the identifier of the chunk holding this portal
 */
ChunkBoundary::Portal::Portal( DataSectionPtr pSection,
							  const PlaneEq & iplane,
							  GeometryMapping * pMapping,
							  const BW::string & ownerChunkName ) :
	internal( false ),
	permissive( true ),
	hasChunkItem_( false ),
	collisionFlags_( 0 ),
	pChunk( NULL ),
	plane( iplane ),
	traverseMark_( -1 )
{
	BW_GUARD;
	// make sure there's something there
	if (!pSection) return;

	// set the label if it's got one
	label = pSection->asString();

	// read in our flags
	internal = pSection->readBool( "internal", internal );
	permissive = pSection->readBool( "permissive", permissive );

	// find out what to set pChunk to
	BW::string	chunkName = pSection->readString( "chunk" );
	if (chunkName == "")
	{
		pChunk = NULL;
	}
	else if (chunkName == "heaven")
	{
		pChunk = (Chunk*)HEAVEN;
	}
	else if (chunkName == "earth")
	{
		pChunk = (Chunk*)EARTH;
	}
	else if (chunkName == "invasive")
	{
		pChunk = (Chunk*)INVASIVE;
	}
	else if (chunkName == "extern")
	{
		pChunk = (Chunk*)EXTERN;
	}
	else
	{
		pChunk = new Chunk( chunkName, pMapping );
	}


	// read in the polygon points
	Vector2	avg(0,0);

	BW::vector<Vector3>	v3points;
	pSection->readVector3s( "point", v3points );
	for (uint i=0; i<v3points.size(); i++)
	{
		Vector2 next( v3points[i].x, v3points[i].y );

		avg += next;

		points.push_back( next );
		// TODO: These probably have to be in some sort of order...
	}

	// read in the axes
	uAxis = pSection->readVector3( "uAxis" );
	vAxis = plane.normal().crossProduct( uAxis );
	origin = plane.normal() * plane.d() / plane.normal().lengthSquared();

#ifdef EDITOR_ENABLED
	const float EPSILON = 0.5;
	if( !chunkName.empty() && *chunkName.rbegin() == 'o' &&
		!ownerChunkName.empty() && *ownerChunkName.rbegin() == 'o' &&
		points.size() == 4 )
	{
		if( uAxis[ 1 ] >= EPSILON || uAxis[ 1 ] <= -EPSILON )
		{
			for( int i = 0; i < 4; ++i )
			{
				if( points[ i ][ 0 ] > EPSILON )
					points[ i ][ 0 ] = MAX_CHUNK_HEIGHT;
				else if( points[ i ][ 0 ] < -EPSILON )
					points[ i ][ 0 ] = MIN_CHUNK_HEIGHT;
			}
		}
		else
		{
			for( int i = 0; i < 4; ++i )
			{
				if( points[ i ][ 1 ] > EPSILON )
					points[ i ][ 1 ] = MAX_CHUNK_HEIGHT;
				else if( points[ i ][ 1 ] < -EPSILON )
					points[ i ][ 1 ] = MIN_CHUNK_HEIGHT;
			}
		}
	}
#endif //EDITOR_ENABLED

	// figure out the centre of our portal (local coords)
	avg /= float(points.size());
	lcentre = uAxis * avg.x + vAxis * avg.y + origin;
	centre = lcentre;

	PlaneEq testPlane(
		points[0][0] * uAxis + points[0][1] * vAxis + origin,
		points[1][0] * uAxis + points[1][1] * vAxis + origin,
		points[2][0] * uAxis + points[2][1] * vAxis + origin );
	Vector3 n1 = plane.normal();
	Vector3 n2 = testPlane.normal();
	n1.normalise();	
	n2.normalise();//TODO: revise the code here to use dot product
	if ((n1 + n2).length() < 1.f)	// should be 2 if equal
	{
		std::reverse( points.begin(), points.end() );
	}
}


/**
 * This is a faster version of the Portal constructor which doesn't use 
 * DataSections as a container. This particular constructor is *only* 
 * called from createBoundaries(), the parent section is always outside
 *
 *  @param iplane		the plane equation contains the data defining the portal
 *  @param uAxisIn		uAxis
 *  @param pointsIn		The points that defile the portal bounds.
 *  @param pMapping		the chunk mapping of the chunk holding this portal
 *  @param chunkName	the name of this portal chunk 
 *
 */

ChunkBoundary::Portal::Portal( const PlaneEq &iplane,
							   const Vector3 &uAxisIn,
							   const BW::vector<Vector3> &pointsIn,
							   GeometryMapping * pMapping,
							   const BW::string & chunkName ) :
	internal( false ),
	permissive( true ),
	hasChunkItem_( false ),
	collisionFlags_( 0 ),
	pChunk( NULL ),
	plane( iplane ),
	traverseMark_( -1 )
{
	BW_GUARD;

	if (chunkName == "")
	{
		pChunk = NULL;
	}
	else if (chunkName == "heaven")
	{
		pChunk = (Chunk*)HEAVEN;
	}
	else if (chunkName == "earth")
	{
		pChunk = (Chunk*)EARTH;
	}
	else if (chunkName == "invasive")
	{
		pChunk = (Chunk*)INVASIVE;
	}
	else if (chunkName == "extern")
	{
		pChunk = (Chunk*)EXTERN;
	}
	else
	{
		pChunk = new Chunk( chunkName, pMapping );
	}


	// read in the polygon points
	Vector2	avg(0,0);
	points.reserve( pointsIn.size() );
	for (uint i=0; i<pointsIn.size(); i++)
	{
		Vector2 next( pointsIn[i].x, pointsIn[i].y );

		avg += next;

		points.push_back( next );
		// TODO: These probably have to be in some sort of order...
	}

	// read in the axes
	uAxis = uAxisIn; //pSection->readVector3( "uAxis" );
	vAxis = plane.normal().crossProduct( uAxis );
	origin = plane.normal() * plane.d() / plane.normal().lengthSquared();

#ifdef EDITOR_ENABLED
	const float EPSILON = 0.5;
// parent is always outside in this constructor, so no need to test for it
	if( !chunkName.empty() && *chunkName.rbegin() == 'o' &&
		points.size() == 4 )
	{
		if( uAxis[ 1 ] >= EPSILON || uAxis[ 1 ] <= -EPSILON )
		{
			for( int i = 0; i < 4; ++i )
			{
				if( points[ i ][ 0 ] > EPSILON )
					points[ i ][ 0 ] = MAX_CHUNK_HEIGHT;
				else if( points[ i ][ 0 ] < -EPSILON )
					points[ i ][ 0 ] = MIN_CHUNK_HEIGHT;
			}
		}
		else
		{
			for( int i = 0; i < 4; ++i )
			{
				if( points[ i ][ 1 ] > EPSILON )
					points[ i ][ 1 ] = MAX_CHUNK_HEIGHT;
				else if( points[ i ][ 1 ] < -EPSILON )
					points[ i ][ 1 ] = MIN_CHUNK_HEIGHT;
			}
		}
	}
#endif //EDITOR_ENABLED

	// figure out the centre of our portal (local coords)
	avg /= float(points.size());
	lcentre = uAxis * avg.x + vAxis * avg.y + origin;
	centre = lcentre;

	PlaneEq testPlane(
		points[0][0] * uAxis + points[0][1] * vAxis + origin,
		points[1][0] * uAxis + points[1][1] * vAxis + origin,
		points[2][0] * uAxis + points[2][1] * vAxis + origin );
	Vector3 n1 = plane.normal();
	Vector3 n2 = testPlane.normal();
	n1.normalise();	n2.normalise();//TODO: revise the code here to use dot product
	if ((n1 + n2).length() < 1.f)	// should be 2 if equal
	{
		std::reverse( points.begin(), points.end() );
	}
}


/**
 *	Destructor
 */
ChunkBoundary::Portal::~Portal()
{
}

/**
 *	This method saves a description of this portal into the given data section.
 *	@param	pSection	DataSection used to save the chunk boundary to
 *	@param	pOwnMapping	the mapping of the chunk containing the boundary
 */
void ChunkBoundary::Portal::save( DataSectionPtr pSection,
	GeometryMapping * pOwnMapping ) const
{
	BW_GUARD;
	if (!pSection) return;

	DataSectionPtr pPS = pSection->newSection( "portal" );
	if (!label.empty()) pPS->setString( label );
	if (internal) pPS->writeBool( "internal", true );
	if (pChunk != NULL)
	{
		pPS->writeString( "chunk",
			isHeaven() ? "heaven" :
			isEarth() ? "earth" :
			isInvasive() ? "invasive" :
			isExtern() ? "extern" :
			pChunk->mapping() != pOwnMapping ? "extern" :
			pChunk->identifier() );
	}
	pPS->writeVector3( "uAxis", uAxis );
	for (uint i = 0; i < points.size(); i++)
	{
		pPS->newSection( "point" )->setVector3(
			Vector3( points[i][0], points[i][1], 0 ) );
	}

}

/**
 *	This method attempts to resolve an extern portal to find the chunk that
 *	it should be connected to - regardless of what mapping it is in.
 *	Note: if an appropriate chunk is found, it is returned holding a
 *	reference to its GeometryMapping.
 *	@param	pOwnChunk	the chunk containing the boundary
 *	@return	true is successfully resolved, otherwise false
 */
bool ChunkBoundary::Portal::resolveExtern( Chunk * pOwnChunk )
{
	BW_GUARD;
	Vector3 conPt = pOwnChunk->transform().applyPoint(
		lcentre + plane.normal() * -0.1f );
	Chunk * pExternChunk = pOwnChunk->space()->guessChunk( conPt, true );
	if (pExternChunk != NULL)
	{
		if (pExternChunk->mapping() != pOwnChunk->mapping())
		{
			pChunk = pExternChunk;
			return true;
		}
		else
		{
			// we don't want it because it's not extern
			// (although technically it should be allowed...)
			// Drop the GeometryMapping reference guessChunk took
			pExternChunk->mapping()->decRef();
			bw_safe_delete(pExternChunk);
		}
	}

	return false;
}

/**
 *	This method projects the given chunk local space 3D point onto the
 *	portal plane, returning a 2D point.
 */
Vector2 ChunkBoundary::Portal::project( const Vector3& point ) const
{
	Vector3 rel = point - origin;
	return Vector2( rel.dotProduct( uAxis ), rel.dotProduct( vAxis ) );
}

/**
 *	This method checks if the point is inside the portal
 *	@param point the point to check, assumed to already be on the portal plane
 *	@return true if the point is inside the portal
 */
bool ChunkBoundary::Portal::inside( const Vector3& point ) const
{
	BW_GUARD;
	return this->inside( this->project( point ) );
}


/**
 *	This method checks if the point is inside the portal
 *
 *	@param point	The point to check.
 *	@return true if the point is inside the portal, false otherwise.
 */
bool ChunkBoundary::Portal::inside( const Vector2& point ) const
{
	BW_GUARD;
	for (uint32 i = 0; i < points.size(); i++)
	{
		const Vector2& p1 = points[i];
		const Vector2& p2 = points[(i+1) % points.size()];

		Vector2 diff(p1.y - p2.y, p2.x - p1.x);

		if (diff.dotProduct( point - p1 ) < 0.f)
		{
			return false;
		}
	}
	return true;
}

/**
 *	This method calculates the object-space point of our portal points.
 *	Our portal points are defined along a plane and thus are not necessarily
 *	useful for those needing to project them into the world.
 *	@param	i		index of the point to transform
 *	@param	ret		return vector, is set with a true object space position.
 */
void ChunkBoundary::Portal::objectSpacePoint( int i, Vector3& ret )
{
	ret = uAxis * points[i][0] + vAxis * points[i][1] + origin;
}

/**
 *	This method calculates the object-space point of our portal points.
 *	Our portal points are defined along a plane and thus are not necessarily
 *	useful for those needing to project them into the world.
 *	@param	i		index of the point to transform
 *	@param	ret		return vector, is set with a true object space position.
 */
void ChunkBoundary::Portal::objectSpacePoint( int i, Vector4& ret )
{
	ret = Vector4(uAxis * points[i][0] + vAxis * points[i][1] + origin,1);
}

/**
 *	This method calculates the object-space point of our portal points.
 *	Our portal points are defined along a plane and thus are not necessarily
 *	useful for those needing to project them into the world.
 *	@param	i		index of the point to transform
 *	@return			return vector, is set with a true object space position.
 */
Vector3 ChunkBoundary::Portal::objectSpacePoint( int i ) const
{
	return uAxis * points[i][0] + vAxis * points[i][1] + origin;
}


/**
 *	This method calculates which edges on a portal a point is outside
 *	useful for calculating if a number of points are inside a portal.
 *	@param point the point to check
 *	@return the outcode mask for the point each bit represents one edge
 *	if the bit is set, the point is outside that edge
 */
uint32 ChunkBoundary::Portal::outcode( const Vector3& point ) const
{
	BW_GUARD;
	return this->outcode( this->project( point ) );	
}

/**
 *	This method calculates which edges on a portal a point is outside
 *	useful for calculating if a number of points are inside a portal.
 *
 *	@param point	The point to check.
 *
 *	@return	The outcode mask for the point each bit represents one edge
 *	if the bit is set, the point is outside that edge
 */
uint32 ChunkBoundary::Portal::outcode( const Vector2 & point ) const
{
	uint32 res = 0;
	for (uint32 i = 0; i < points.size(); i++)
	{
		const Vector2& p1 = points[i];
		const Vector2& p2 = points[(i+1) % points.size()];

		Vector2 diff(p1.y - p2.y, p2.x - p1.x);

		if (diff.dotProduct( point - p1 ) < 0.f)
		{
			res |= 1 << i;
		}
	}

	return res;
}


/**
 *	This method takes a chunk space sphere and checks if it overlaps with
 *	this portal.
 *
 *	@param centre	Location of the sphere in chunk space.
 *	@param radius	Radius of the sphere
 *
 *	@return true if the sphere overlaps, false otherwise.
 */
bool ChunkBoundary::Portal::intersectSphere( const Vector3 & centre,
	float radius ) const
{
	float dist = fabs( this->plane.distanceTo( centre ) );
	if (dist > radius )
	{
		return false;
	}

	// turn it into a 2d problem by slicing the sphere by the plane
	// (creating a circle).
	float circleRadius = sqrtf(radius*radius - dist*dist);

	Vector2 p = this->project( centre );

	// Go through each line segment in the portal polygon
	bool fullyEnclosed = true;
	for (uint32 i = 0; i < points.size(); i++)
	{
		const Vector2& p1 = points[i];
		const Vector2& p2 = points[(i+1) % points.size()];

		Vector2 diff(p1.y - p2.y, p2.x - p1.x);

		// See if we're fully enclosed within the portal.
		if (diff.dotProduct( p - p1 ) < 0.f)
		{
			fullyEnclosed = false;
		}

		// See if the circle intersects with this edge.
		if ( intersectLineSegmentCircle( p1, p2, p, circleRadius ) )
		{
			return true;
		}
	}

	return fullyEnclosed;
}


/**
 *	Returns the closest distance to this portal from the given local space
 *	position (local to the chunk we're in).
 */
float ChunkBoundary::Portal::distanceTo( const Vector3& localPos ) const
{
	float perpendicularDist = fabs( this->plane.distanceTo( localPos ) );

	// If we're fully contained in the polgon then just
	// return the perpendicular distance.
	uint32 outcode = this->outcode( localPos );
	if (outcode == 0)
	{
		return perpendicularDist;
	}

	// Turn this into a 2D problem now.
	Vector2 pos2d = this->project( localPos );

	// Go through each edge in the portal and find the closest point.
	// For each edge check dist to points and project distance.
	float minHorzDistSqr = FLT_MAX;
	for (uint32 i = 0; i < this->points.size(); i++)
	{
		const Vector2& p1 = this->points[i];
		const Vector2& p2 = this->points[(i+1) % this->points.size()];

		// Calculate the closest point on the line.
		LineEq lineEq( p1, p2 );
		float d = lineEq.project( pos2d );

		float d1 = lineEq.project( p1 );
		float d2 = lineEq.project( p2 );

		d = Math::clamp( std::min( d1, d2 ), d, std::max( d1, d2 ) );
		Vector2 closestPoint = lineEq.param( d );

		float dist2dSqr = (closestPoint - pos2d).lengthSquared();
		minHorzDistSqr = std::min( minHorzDistSqr, dist2dSqr );
	}

	return sqrtf( minHorzDistSqr + perpendicularDist * perpendicularDist );
}


/**
 *	This method sets whether this portal is open or not.
 */
void ChunkBoundary::Portal::setCollisionState( Chunk * pChunkPortalIsIn,
		bool isOpen, WorldTriangle::Flags collisionFlags )
{
	if (!hasChunkItem_ && !isOpen)
	{
		MF_ASSERT( pChunkPortalIsIn );

		PortalChunkItem * pChunkItem = new PortalChunkItem( this );
		pChunkPortalIsIn->addStaticItem( pChunkItem );
		hasChunkItem_ = true;
	}

	this->permissive = isOpen;
	collisionFlags_ = collisionFlags;
}


/**
 *	This method finds the portal in the adjacent chunk that matches this one.
 */
ChunkBoundary::Portal *
	ChunkBoundary::Portal::findPartner( const Chunk * pCurrChunk ) const
{
	return this->pChunk->findMatchingPortal( pCurrChunk, this );
}


// -----------------------------------------------------------------------------
// Section: Client only
// -----------------------------------------------------------------------------

#ifndef MF_SERVER

/**
 *	This class stores up ready-made portals
 */
class Portal2DStore
{
public:
	/**
	 *	This method returns an empty Portal2DRef object from the pool
	 *	@return an empty Portal2DRef object
	 */
	static Portal2DRef grab()
	{
		BW_GUARD;
		if (s_portalStore_.empty())
		{
			Portal2D * pPortal = new Portal2D();
			for (int i = 0; i < 8; i++) pPortal->addPoint( Vector2::zero() );
			s_portalStore_.push_back( pPortal );
		}

		Portal2DRef pref;
		pref.pVal_ = s_portalStore_.back();
		s_portalStore_.pop_back();
		grab( pref.pVal_ );
		pref->erasePoints();
		return pref;
	}

	static void fini()
	{
		BW_GUARD;
		for (uint i=0; i<s_portalStore_.size(); i++)
			bw_safe_delete(s_portalStore_[i]);

		s_portalStore_.clear();
	}

private:
	/**
	 *	This method increase reference count of a Portal2D object
	 *	@param	pPortal	pointer to Portal2D object whose reference count will be increased
	 */
	static void grab( Portal2D * pPortal )
	{
		BW_GUARD;
		pPortal->refs( pPortal->refs() + 1 );
	}

	/**
	 *	This method decrease reference count of a Portal2D object,
	 *	if the reference count is zero, it put the object back to pool
	 *	@param	pPortal	pointer to Portal2D object whose reference count will be decreased
	 */
	static void give( Portal2D * pPortal )
	{
		BW_GUARD;
		if (pPortal == NULL) return;
		pPortal->refs( pPortal->refs() - 1 );
		if (pPortal->refs() == 0)
		{
			s_portalStore_.push_back( pPortal );
		}
	}

	static VectorNoDestructor<Portal2D*>		s_portalStore_;

	friend class Portal2DRef;
};

VectorNoDestructor<Portal2D*> Portal2DStore::s_portalStore_;

/**
 *	Constructor
 *	@param	valid	if the object to be created is valid
 */
Portal2DRef::Portal2DRef( bool valid ) :
	pVal_( valid ? NULL : (Portal2D*)-1 )
{
}

/**
 *	Copy constructor
 *	@param	other	the object to be copied
 */
Portal2DRef::Portal2DRef( const Portal2DRef & other ) :
	pVal_( other.pVal_ )
{
	BW_GUARD;
	if (uintptr(pVal_)+1 > 1)
		Portal2DStore::grab( pVal_ );
}

/**
 *	Assignment Operator
 *	@param	other	the object to be copied
 */
Portal2DRef & Portal2DRef::operator =( const Portal2DRef & other )
{
	BW_GUARD;
	if (uintptr(pVal_)+1 > 1)
		Portal2DStore::give( pVal_ );
	pVal_ = other.pVal_;
	if (uintptr(pVal_)+1 > 1)
		Portal2DStore::grab( pVal_ );
	return *this;
}

/**
 *	Destructor
 */
Portal2DRef::~Portal2DRef()
{
	BW_GUARD;
	if (uintptr(pVal_)+1 > 1)
		Portal2DStore::give( pVal_ );
}

// E3'04 option to turn off clipping outside to portals
// ... needed until it works better! (portal aggregation)
/**
 * This class adds a watcher named 'Render/clipOutsideToPortal' so the user
 * can enable or disable the clipping of an outside chunk to a portal.
 */
static struct ClipOutsideToPortal
{
	/**
	 *	Constructor
	 */
	ClipOutsideToPortal() : b_( false )
	{
		MF_WATCH( "Render/clipOutsideToPortal", b_, Watcher::WT_READ_WRITE,
			"Clip outdoor chunks to indoor portals" );
	}
	/**
	 *	conversion operator to bool
	 *	@return	bool	true if clip outside chunk to portal, otherwise false
	 */
	operator bool() { return b_; }
	bool b_;
} s_clipOutsideToPortal;


/*static*/ void ChunkBoundary::fini()
{
	BW_GUARD;
	Portal2DStore::fini();

}


BoundingBox ChunkBoundary::Portal::bbFrustum_;

void ChunkBoundary::Portal::updateFrustumBB()
{
	BW_GUARD;
	static Vector4 clipSpaceFrustum[8] =
	{
		Vector4( -1.f, -1.f, 0.f, 1.f ),
		Vector4( +1.f, -1.f, 0.f, 1.f ),
		Vector4( -1.f, +1.f, 0.f, 1.f ),
		Vector4( +1.f, +1.f, 0.f, 1.f ),
		Vector4( -1.f, -1.f, 1.f, 1.f ),
		Vector4( +1.f, -1.f, 1.f, 1.f ),
		Vector4( -1.f, +1.f, 1.f, 1.f ),
		Vector4( +1.f, +1.f, 1.f, 1.f )
	};

	Vector4 worldSpaceFrustum[8];
	Matrix m;
	m.invert( Moo::rc().viewProjection() );
	for( size_t i = 0; i < 8; ++i )
	{
		m.applyPoint( worldSpaceFrustum[i], clipSpaceFrustum[i] );
		if( worldSpaceFrustum[i].w < 0.0000001f && worldSpaceFrustum[i].w > -0.0000001f )
			worldSpaceFrustum[i].x = 0.0000001f;
		worldSpaceFrustum[i] = worldSpaceFrustum[i] / worldSpaceFrustum[i].w;
	}

	bbFrustum_ = BoundingBox( Vector3( worldSpaceFrustum[0].x, worldSpaceFrustum[0].y, worldSpaceFrustum[0].z ),
		 Vector3( worldSpaceFrustum[0].x, worldSpaceFrustum[0].y, worldSpaceFrustum[0].z ) );
	for( size_t i = 1; i < 8; ++i )
	{
		bbFrustum_.addBounds( Vector3( worldSpaceFrustum[i].x, worldSpaceFrustum[i].y, worldSpaceFrustum[i].z ) );
	}
}


/**
 *	This method traverses this portal and draws the chunk it's connected to, if
 *	the portal is visible. Assumes that 'hasChunk' would return true.
 *	It returns the portal to be used to traverse any portals inside that chunk.
 *	An invlaid Portal2DRef is returned if the chunk was not drawn.
 *
 *	@param	transform	world transform of the portal
 *	@param	transformInverse	inverse world transform of the portal
 *	@param	pClipPortal	portal used to clip the traverse, we traverse through
 *			this portal to the current chunk. Could be NULL, which means no
 *			clip.
 *	@param	traversalData	Common traversal data to use during this traversal.
 *	@param	nearDepth		Optional arg to return an altered near plane depth.
 *
 *	@return	the portal to be used to traverse any portals inside that chunk
 */
Portal2DRef ChunkBoundary::Portal::traverse(
	const Matrix & transform,
	const Matrix & transformInverse,
	Portal2DRef pClipPortal,
	const TraversalData& traversalData,
	float* nearDepth) const
{
	BW_GUARD;

	// Check if the camera is in front of the portal we are trying to traverse through
	// if it is, stop traversal
	Vector3 localCamera = transformInverse.applyPoint( traversalData.cameraPosition_ );

	if (!plane.isInFrontOf( localCamera ))
	{
		this->traverseMark_ = traversalData.nextMark_;
		return Portal2DRef( false );
	}

	// declare these static so that they don't keep allocating memory
	static BW::vector<Vector4> portalClipPoints;
	static BW::vector<Outcode> portalOutcodes;

	portalClipPoints.clear();
	portalOutcodes.clear();

	// Construct the matrix to transform from local space to clip space
	Matrix		objectToClip;
	objectToClip.multiply( transform, Moo::rc().viewProjection() );

	// set up two outcode variables. new outcodes are accumulated into
	// outProduct by 'and' operations, and into outSum by 'or' operations.
	Outcode outProduct = OUTCODE_MASK;
	Outcode outSum = 0;	// Remember circuit analysis: SoP and PoS?

	Vector4 clipPoint;

	// Create all the points in our portal
	for( uint32 i = 0; i < points.size(); i++ )
	{
		Vector4 worldPoint;
		// project the point into clip space

		Vector4 objectPoint( uAxis * points[i][0] + vAxis * points[i][1] + origin, 1 );
		objectToClip.applyPoint( clipPoint, objectPoint );
		transform.applyPoint( worldPoint, objectPoint );

		Vector3 worldPoint3( worldPoint.x, worldPoint.y, worldPoint.z );

		// see where it lies
		Outcode ocPoint = clipPoint.calculateOutcode();

		outSum |= ocPoint;

		outProduct &= ocPoint;

		portalClipPoints.push_back( clipPoint );
		portalOutcodes.push_back( ocPoint );
	}

	// if all the points have at least one outcode bit in common,
	// then the whole portal must be out of the volume, so ignore it.
	// We ignore the NEAR outcode here, as the portal might exist
	// in the space between the near plane and the camera
	if ((outProduct & ~OUTCODE_NEAR) != 0)
	{
		// Portal is outside the view frustum,
		// don't bother traversing it again
		this->traverseMark_ = traversalData.nextMark_;
		return Portal2DRef( false );
	}

	Portal2DRef ourClipPortal = Portal2DStore::grab();

	// Check if the portal intersects the near plane
	if (outSum & OUTCODE_NEAR)
	{
		// If the portal intersects the part of the view
		// frustum that is behind the near plane, this portal
		// is treated as covering the entire screen.
		for (uint32 i = 0; i < 4 ;i++)
		{
			Vector3 localNPPoint = transformInverse.applyPoint( traversalData.nearPlanePositions_[i] );
			if (!plane.isInFrontOf( localNPPoint ))
			{
				if (&*pClipPortal == NULL)
				{
					this->traverseMark_ = traversalData.nextMark_;
				}

				// If the camera is at the near plane the near depth
				// is the near plane
				if (nearDepth)
				{
					*nearDepth = Moo::rc().camera().nearPlane();
				}

				return pClipPortal;
			}
		}

		// Now check the near outcode to see if the portal
		// is actually behind the camera.
		if ((outProduct & OUTCODE_NEAR) != 0)
		{
			// Portal is behind the view frustum,
			// don't bother traversing it again
			this->traverseMark_ = traversalData.nextMark_;
			return Portal2DRef( false );
		}


		// Clip the portal to the near plane
		Vector4 firstPoint = portalClipPoints.front();
		Outcode firstOc = portalOutcodes.front();

		portalClipPoints.push_back( firstPoint );
		portalOutcodes.push_back( firstOc );

		for (uint32 i = 0; i < (portalClipPoints.size() - 1) && portalClipPoints.size() != 0; i++)
		{
			if ((portalOutcodes[i] ^ portalOutcodes[i+1]) & OUTCODE_NEAR)
			{
				const Vector4& pa = portalClipPoints[i];
				const Vector4& pb = portalClipPoints[i + 1];
				Vector4 delta = pb - pa;
				float t = (0.f - pa.z) / (pb.z - pa.z);

				Vector4 res = pa + delta * t;

				portalClipPoints.insert( portalClipPoints.begin() + i + 1, res );
				portalOutcodes.insert( portalOutcodes.begin() + i + 1, res.calculateOutcode() & ~OUTCODE_NEAR);

				++i;
			}
		}
		portalClipPoints.pop_back();
		portalOutcodes.pop_back();
	}

	outProduct = OUTCODE_MASK;
	outSum = 0;

	// Initialise the near depth to be at the far plane
	if (nearDepth)
	{
		*nearDepth = Moo::rc().camera().farPlane();
	}

	// Create the 2d screen space portal from the clipped portal
	// making sure we ignore all points that are behind the camera
	for (uint32 i = 0; i < portalClipPoints.size(); i++)
	{
		if (!(portalOutcodes[i] & OUTCODE_NEAR))
		{
			Vector4& clipPoint = portalClipPoints[i];
			float oow = 1.f/clipPoint.w;
			ourClipPortal->addPoint(
				Vector2( clipPoint.x*oow, clipPoint.y*oow ) );
			outProduct &= portalOutcodes[i];
			outSum |= portalOutcodes[i];

			// the near depth is the closest depth to the camera
			if (nearDepth)
			{
				*nearDepth = std::min( *nearDepth, clipPoint.w );
			}
		}
	}

	if (ourClipPortal->points().size() < 3 || outProduct)
	{
		this->traverseMark_ = traversalData.nextMark_;
		return Portal2DRef( false );
	}


	// ok, let's combine them Portal2D's
	Portal2DRef combinedP2D = ourClipPortal;
	if (&*pClipPortal != NULL)
	{
		combinedP2D = Portal2DStore::grab();
		if (!combinedP2D->combine( &*pClipPortal, &*ourClipPortal ))
		{
			return Portal2DRef( false );
		}
	}
	else
	{
		this->traverseMark_ = traversalData.nextMark_;
	}

	ChunkManager::drawTreeBranch( pChunk, " + combined" );

	#if ENABLE_DRAW_PORTALS
	if (drawPortals_)
	{
		// Set up RS not set by Geometrics::drawLinesInClip
		Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESS );
		Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_FOGENABLE, FALSE );

		Geometrics::drawLinesInClip(
			(Vector2*)&*combinedP2D->points().begin(),
			static_cast<uint>(combinedP2D->points().size()) );
	}
	#endif

	return combinedP2D;
}

ChunkBoundary::TraversalData::TraversalData(uint32 nextMark) :
	cameraPosition_(Moo::rc().invView().applyToOrigin()),
	nextMark_(nextMark)
{
	// Calculate the corners of the near plane in world space
	Matrix invProj = Moo::rc().viewProjection();
	invProj.invert();

	float np = Moo::rc().camera().nearPlane();

	Vector4 tPos;
	invProj.applyPoint( tPos, Vector4( -np,  np, 0, np ) );
	nearPlanePositions_[0].set( tPos.x, tPos.y, tPos.z );
	invProj.applyPoint( tPos, Vector4(  np,  np, 0, np ) );
	nearPlanePositions_[1].set( tPos.x, tPos.y, tPos.z );
	invProj.applyPoint( tPos, Vector4(  np, -np, 0, np ) );
	nearPlanePositions_[2].set( tPos.x, tPos.y, tPos.z );
	invProj.applyPoint( tPos, Vector4( -np, -np, 0, np ) );
	nearPlanePositions_[3].set( tPos.x, tPos.y, tPos.z );
}

/**
 *	Debugging method for displaying a portal. It is drawn in purple if the
 *	camera is on the inside of the portal plane, and green if it's outside.
 *	
 */
void ChunkBoundary::Portal::display( const Matrix & transform,
	const Matrix & transformInverse, float inset ) const
{
	BW_GUARD;
	Vector3 worldPoint[16];

	// find the centre
	Vector2 avgPt(0.f,0.f);
	for( uint32 i = 0; i < points.size(); i++ ) avgPt += points[i];
	avgPt /= float(points.size());

	// transform all the points
	for( uint32 i = 0; i < points.size(); i++ )
	{
		// project the point into clip space
		transform.applyPoint( worldPoint[i], Vector3(
			uAxis * (points[i][0] + (points[i][0] < avgPt[0] ? inset : -inset)) +
			vAxis * (points[i][1] + (points[i][1] < avgPt[1] ? inset : -inset)) +
			origin ) );
	}

	// set the colour based on which side of the portal the camera is on
	uint32 colour = plane.distanceTo( transformInverse.applyPoint(
		Moo::rc().invView().applyToOrigin() ) ) < 0.f ? 0x0000ff00 : 0x00ff00ff;

	// draw the lines
	for( uint32 i = 0; i < points.size(); i++ )
	{
		Geometrics::drawLine( worldPoint[i], worldPoint[(i+1)%points.size()], colour );
	}
}
#endif // !MF_SERVER

/**
 *	add points into the portal
 *	@param	pd	the points to be added
 */
void PrivPortal::addPoints( const BW::vector<Vector3> & pd )
{
	BW_GUARD;
	const float IN_PORTAL_PLANE			= 0.2f;

	points_.assign( pd.begin(), pd.end() );

	BW::vector<Vector3>::size_type i = 0;

	// find a plane
	for (i = 0; i < points_.size() - 2; ++i)
	{
		planeEquation_.init( points_[ i ], points_[ i + 1 ], points_[ i + 2 ] );

		if (! almostEqual( planeEquation_.normal(), Vector3( 0.f, 0.f, 0.f ) ) )
		{
			break;
		}
	}

	// remove points not on the plane
	bool foundOne = true;

	while (foundOne)
	{
		for (i = 0; i < points_.size() && foundOne; ++i)
		{
			foundOne = false;

			if (fabs( planeEquation_.distanceTo( points_[ i ] ) ) > IN_PORTAL_PLANE)
			{
				ERROR_MSG( "PrivPortal::addPoints: found a point that is not on the portal plane.\n" );
				points_.erase( points_.begin() + i );
				foundOne = true;
				break;
			}
		}
	}

	// remove points on the same line
	foundOne = true;

	while (foundOne)
	{
		foundOne = false;

		for (i = 0; i < points_.size(); ++i)
		{
			Vector3 v0 = points_[ ( i + points_.size() ) % points_.size() ];
			Vector3 v1 = points_[ ( i + 1 + points_.size() ) % points_.size() ];
			Vector3 v2 = points_[ ( i + 2 + points_.size() ) % points_.size() ];
			Vector3 n1 = v0 - v1;
			Vector3 n2 = v1 - v2;
			n1.normalise();
			n2.normalise();

			if (almostEqual( v0, v1 ) || // v0 == v1
				almostEqual( v1, v2 ) || // v1 == v2
				almostEqual( n1, n2 )) // v0, v1, v2 colinear
			{
				points_.erase( points_.begin() + i + 1 );
				foundOne = true;
				break;
			}
		}
	}
}

/**
 *	set the flag of the portal
 *	@param	flag	the flag to be set
 */
void PrivPortal::flag( const BW::string& flag )
{
	flags_ |= int( flag == "heaven" ) << 1;
	flags_ |= int( flag == "earth" ) << 2;
	flags_ |= int( flag == "invasive" ) << 3;
}

/**
 *	This method applies the given transform to this portal
 *	@param	transform	the transform to be applied
 */
void PrivPortal::transform( const class Matrix& transform )
{
	BW_GUARD;
	Vector3 pos = transform.applyPoint( planeEquation_.normal() * planeEquation_.d() );
	Vector3 norm = transform.applyVector( planeEquation_.normal() );
	norm.normalise();
	planeEquation_ = PlaneEq( norm, pos.dotProduct( norm ) );

	for( uint32 i = 0; i < points_.size(); i++ )
	{
		points_[ i ] = transform.applyPoint( points_[ i ] );
	}
}

/**
 *	This method creates the boundary sections from a visual data section
 *	@param	pVis	the visual data section
 *	@param	wantWorld	world transform
 *	@return	Smart pointer to DataSection object contains the boundary sections
 */
DataSectionPtr createBoundarySections( DataSectionPtr pVis, const Matrix & wantWorld )
{
	BW_GUARD;
	// set up some matrices
	Matrix parentWorld = Matrix::identity;
	Matrix wantWorldInv; wantWorldInv.invert( wantWorld );

	// get the (world) bounding box
	BoundingBox bb;
	bb.setBounds( pVis->readVector3( "boundingBox/min" ),
		pVis->readVector3( "boundingBox/max" ) );

	bb.transformBy( wantWorld );
	Vector3 bbMin = bb.minBounds();
	Vector3 bbMax = bb.maxBounds();

	BW::vector<PrivPortal *>		portals[6];

	BW::vector<DataSectionPtr> visualPortals;
	pVis->openSections( "portal", visualPortals );

	// now look at all our portals, and assign them to a boundary
	for (uint i = 0; i < visualPortals.size(); i++)
	{
		BW::vector<Vector3> portalPoints;
		visualPortals[i]->readVector3s( "point", portalPoints );
		if( !portalPoints.empty() )
		{
			PrivPortal* bpNew = new PrivPortal();
			bpNew->addPoints( portalPoints );
			bpNew->flag( visualPortals[i]->asString() );
			PrivPortal & bp = *bpNew;
			bp.transform( parentWorld );

			const PlaneEq & peq = bp.getPlaneEquation();
			Vector3 normal = peq.normal();
			Vector3 point = normal * peq.d();

			// figure out which side it's on
			int side = 0;
			Vector3 anormal( fabsf(normal[0]), fabsf(normal[1]), fabsf(normal[2]) );
			if (anormal[0] > anormal[1] && anormal[0] > anormal[2])
			{	// on yz plane (l or r)
				side = 0 + int(fabs(point[0] - bbMin[0]) > fabsf(point[0] - bbMax[0]));
			}
			else if (anormal[1] > anormal[0] && anormal[1] > anormal[2])
			{	// on xz plane (d or u)
				side = 2 + int(fabs(point[1] - bbMin[1]) > fabsf(point[1] - bbMax[1]));
			}
			else
			{	// on xy plane (f or b)
				side = 4 + int(fabs(point[2] - bbMin[2]) > fabsf(point[2] - bbMax[2]));
			}

			// add it to that side's list
			portals[side].push_back( &bp );
		}
	}

	DataSectionPtr result = new XMLSection( "root" );
	// now write out the boundary
	for (int b = 0; b < 6; b++)
	{
		DataSectionPtr pBoundary = result->newSection( "boundary" );

		// figure out the boundary plane in world coordinates
		int sign = 1 - (b&1)*2;
		int axis = b>>1;
		Vector3 normal(
			sign * ((axis==0)?1.f:0.f),
			sign * ((axis==1)?1.f:0.f),
			sign * ((axis==2)?1.f:0.f) );
		float d = sign > 0 ? bbMin[axis] : -bbMax[axis];

		Vector3 localCentre = wantWorldInv.applyPoint( normal * d );
		Vector3 localNormal = wantWorldInv.applyVector( normal );
		localNormal.normalise();
		PlaneEq localPlane( localNormal, localNormal.dotProduct( localCentre ) );

		pBoundary->writeVector3( "normal", localPlane.normal() );
		pBoundary->writeFloat( "d", localPlane.d() );

		for (uint i = 0; i < portals[b].size(); i++)
		{
			PrivPortal  & bp = *portals[b][i];

			// write out the link
			DataSectionPtr pPortal = pBoundary->newSection( "portal" );

			if (bp.isHeaven())
				pPortal->writeString( "chunk", "heaven" );
			else if (bp.isEarth())
				pPortal->writeString( "chunk", "earth" );
			else if (bp.isInvasive())
				pPortal->writeString( "chunk", "invasive" );

			// figure out a uAxis and a vAxis ... for calculation purposes,
			// make (uAxis, vAxis, normal) a basis in world space.
			Vector3	uAxis(
				(axis==1)?1.f:0.f,
				(axis==2)?1.f:0.f,
				(axis==0)?1.f:0.f );
			Vector3 vAxis = normal.crossProduct( uAxis );

			// but write out a uAxis that turns the 2D points into local space
			pPortal->writeVector3( "uAxis", wantWorldInv.applyVector( uAxis ) );

			// now transform and write out the points
			Matrix basis;	// (actually using matrix for ordinary maths!)
			basis[0] = uAxis;
			basis[1] = vAxis;
			basis[2] = normal;		// so error from plane is in the z.
			basis.translation( normal * d + wantWorld.applyToOrigin() );
			Matrix invBasis; invBasis.invert( basis );

			for (uint j = 0; j < bp.getNPoints(); j++)
			{
				pPortal->newSection( "point" )->setVector3(
					invBasis.applyPoint( bp.getPoint(j) ) );
			}

			bw_safe_delete(portals[b][i]);
		}
	}

	return result;
}

BW_END_NAMESPACE

// chunk_boundary.cpp
