#ifndef CHUNK_BOUNDARY_HPP
#define CHUNK_BOUNDARY_HPP

#include "math/boundbox.hpp"
#include "math/planeeq.hpp"
#include "math/vector2.hpp"
#include "moo/moo_math.hpp"
#include "physics2/worldtri.hpp" // For WorldTriangle::Flags

#include "cstdmf/bw_vector.hpp"

#include "cstdmf/smartpointer.hpp"
#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE


class Chunk;
class ChunkSpace;
class Portal2D;
class GeometryMapping;

/**
 *	This is a helper class used to generate portal data from points
 */
class PrivPortal
{
public:
	/**
	 *	Constructor
	 */
	PrivPortal() : hasPlaneEquation_( false ), flags_( 0 ) { }
	/**
	 *	Desstructor
	 */
	~PrivPortal() { }

	/**
	 *	return the plane equation of the portal
	 *	@return	the plane equation of the portal
	 */
	const PlaneEq&			getPlaneEquation( void ) const { return planeEquation_; }

	void					addPoints( const BW::vector<Vector3> & pd );

	void					flag( const BW::string& flag );

	/**
	 *	get a point from the portal
	 *	@param	i	the index of the point
	 *	@return	the ith point of the portal
	 */
	const Vector3&			getPoint( uint32 i ) const	{ return points_[i]; }
	/**
	 *	get the number of points of the portal
	 *	@return	the number of points of the portal
	 */
	uint32					getNPoints() const
	{ 
		MF_ASSERT( points_.size() <= UINT_MAX ); 
		return ( uint32 )points_.size();
	}

	void					transform( const class Matrix& transform );

	/**
	 *	check if it is a heaven portal
	 *	@return	true if it is a heaven portal, false otherwise
	 */
	bool					isHeaven() const	{ return !!(flags_ & (1<<1)); }
	/**
	 *	check if it is an earth portal
	 *	@return	true if it is an earth portal, false otherwise
	 */
	bool					isEarth() const		{ return !!(flags_ & (1<<2)); }
	/**
	 *	check if it is an invasive portal
	 *	@return	true if it is an invasive portal, false otherwise
	 */
	bool					isInvasive() const	{ return !!(flags_ & (1<<3)); }

private:
	PlaneEq					planeEquation_;
	bool					hasPlaneEquation_;

	int						flags_;

	BW::vector< Vector3 >	points_;
};

DataSectionPtr createBoundarySections( DataSectionPtr pVis, const Matrix & wantWorld );

/**
 *	This class wraps a Portal2D so it can be reused when no longer needed.
 */
class Portal2DRef
{
public:
	Portal2DRef( bool valid = true );
	Portal2DRef( const Portal2DRef & other );
	Portal2DRef & operator =( const Portal2DRef & other );
	~Portal2DRef();

	/**
	 *	check if the contained Portal2D pointer is valid
	 *	@return	true if the portal is valid, false otherwise
	 */
	bool valid()			{ return (uintptr)pVal_ != (uintptr)-1; }
	/**
	 *	return the contained Portal2D pointer
	 *	@return	the contained Portal2D pointer
	 */
	const Portal2D * pVal() const { return pVal_; }
	/**
	 *	overload version of operator-> to privide smart pointer like access
	 *	@return	the contained Portal2D pointer
	 */
	Portal2D * operator->()	{ return pVal_; }
	/**
	 *	overload version of operator* to privide reference like access
	 *	@return	the Portal2D object pointed by the contained pointer
	 */
	Portal2D & operator*()	{ return *pVal_; }

private:
	Portal2D * pVal_;

	friend class Portal2DStore;
};


typedef SmartPointer<struct ChunkBoundary>	ChunkBoundaryPtr;

/**
 *	This class defines a boundary plane of a chunk.
 *	It includes any portals to other chunks in that plane.
 */
struct ChunkBoundary : public ReferenceCount
{
	struct Portal;

	ChunkBoundary(	DataSectionPtr pSection,
					GeometryMapping * pMapping,
					const BW::string & ownerChunkName = "" );

	ChunkBoundary( const PlaneEq &plane, 
				   Portal * portal);

	~ChunkBoundary();

	static void fini();

	/**
	 *	return the plane equation of the portal
	 *	@return	the plane equation of the portal
	 */
	const PlaneEq & plane() const		{ return plane_; }

	typedef BW::vector<Vector2>	V2Vector;

	/*
	 * Common data used when traversing portals
	 */
	struct TraversalData
	{
		TraversalData(uint32 nextMark);
		Vector3 cameraPosition_;
		Vector3 nearPlanePositions_[4];
		uint32	nextMark_;
	};

	/**
	 *	This class defines a portal.
	 *
	 *	A portal is a polygon plus a reference to the chunk that is
	 *	visible through that polygon. It includes a flag to indicate
	 *	whether it permits objects to pass through it, and another flag
	 *	to indicate whether its plane is internal to another chunk.
	 */
	struct Portal
	{
		Portal(	DataSectionPtr pSection,
				const PlaneEq & plane,
				GeometryMapping * pMapping,
				const BW::string & ownerChunkName = "" );
		
		// fast constructor that doesn't use DataSections.
		Portal(	const PlaneEq & iplane, 
				const Vector3 &uAxisIn, 
				const BW::vector<Vector3> &pointsIn, 
				GeometryMapping * pMapping, 
				const BW::string & chunkName );

		~Portal();

		void save( DataSectionPtr pSection,
			GeometryMapping * pOwnMapping ) const;
		bool resolveExtern( Chunk * pOwnChunk );
		void objectSpacePoint( int idx, Vector3& ret );
		Vector3 objectSpacePoint( int idx ) const;
		void objectSpacePoint( int idx, Vector4& ret );

		bool			internal;
		bool			permissive;

private:
		bool			hasChunkItem_;
		WorldTriangle::Flags collisionFlags_;

public:
		Chunk			* pChunk;
		V2Vector		points;
		Vector3	uAxis;		// in local space
		Vector3	vAxis;		// in local space
		Vector3 origin;		// in local space
		Vector3	lcentre;	// in local space
		Vector3	centre;		// in world space
		PlaneEq			plane;		// same as plane in boundary

		BW::string		label;	// TODO: Remove this

		bool hasChunkItem() const	{ return hasChunkItem_; }
		WorldTriangle::Flags collisionFlags() const	{ return collisionFlags_; }

		void setCollisionState( Chunk * pChunkPortalIsIn,
				bool isOpen, WorldTriangle::Flags collisionFlags );
		void resetCollisionState()
		{
			this->setCollisionState( NULL, true, 0 );
		}

		Portal * findPartner( const Chunk * pCurrChunk ) const;

#ifndef MF_SERVER
		static BoundingBox bbFrustum_;
		static void updateFrustumBB();

		Portal2DRef traverse(
			const Matrix & world,
			const Matrix & worldInverse,
			Portal2DRef pClipPortal,
			const TraversalData& traversalData,
			float* nearDepth = NULL) const;
#endif // MF_SERVER

		enum
		{
			NOTHING = 0,
			HEAVEN = 1,
			EARTH = 2,
			INVASIVE = 3,
			EXTERN = 4,
			LAST_SPECIAL = 15
		};

		/**
		 *	Check if the portal is connected to a non-NULL chunk.
		 *	@return	true if it is connected with a chunk, false otherwise
		 */
		bool hasChunk() const
			{ return uintptr( pChunk ) > uintptr( LAST_SPECIAL ); }

		/**
		 *	Check if this portal did or will connect to a chunk.
		 *	For example, the chunk is NULL at the moment because
		 *	the portal is still being bound or it has been unbound,
		 *	but this is not a "special" portal.
		 *	@return true if this portal will connect to a chunk.
		 */
		bool hasChunkOrIsNothing() const
		{
			return !this->isConnectingToSpecial();
		}

		/**
		 *	Check if this portal is marked as a "special" portal.
		 *	Can be connecting to one of the types in the enum above.
		 *	@return true if this portal will not connect to a chunk.
		 */
		bool isConnectingToSpecial() const
		{
			return (pChunk != NULL) &&
				(uintptr( pChunk ) <= uintptr( LAST_SPECIAL ));
		}

		/**
		 *	check if it is a heaven portal
		 *	@return	true if it is a heaven portal, false otherwise
		 */
		bool isHeaven() const
			{ return uintptr(pChunk) == uintptr(HEAVEN); }

		/**
		 *	check if it is an earth portal
		 *	@return	true if it is an earth portal, false otherwise
		 */
		bool isEarth() const
			{ return uintptr(pChunk) == uintptr(EARTH); }

		/**
		 *	check if it is an invasive portal
		 *	@return	true if it is an invasive portal, false otherwise
		 */
		bool isInvasive() const
			{ return uintptr(pChunk) == uintptr(INVASIVE); }

		/**
		 *	check if it is an extern portal
		 *	@return	true if it is an extern portal, false otherwise
		 */
		bool isExtern() const
			{ return uintptr(pChunk) == uintptr(EXTERN); }

#ifndef MF_SERVER
		void display( const Matrix & transform,
			const Matrix & transformInverse, float inset = 0.f ) const;
#endif // MF_SERVER

		Vector2 project( const Vector3& point ) const;
		bool inside( const Vector3& point ) const;
		bool inside( const Vector2& point ) const;
		uint32 outcode( const Vector3& point ) const;
		uint32 outcode( const Vector2& point ) const;
		bool intersectSphere( const Vector3& centre, float radius ) const;
		float distanceTo( const Vector3& localPos ) const;

		mutable uint32	traverseMark_;

		static bool drawPortals_;
	};

	typedef BW::vector<Portal*>	Portals;

	void validatePortals( DataSectionPtr boundarySection, DataSectionPtr chunkSection );
	void bindPortal( uint32 unboundIndex );
	void unbindPortal( uint32 boundIndex );

	void addInvasivePortal( Portal * pPortal );
	void splitInvasivePortal( Chunk * pChunk, uint i );


// variables
	PlaneEq		plane_;

	Portals		boundPortals_;
	Portals		unboundPortals_;
};

typedef BW::vector<ChunkBoundaryPtr>	ChunkBoundaries;


// Note: chunks in unbound portals are created by the loading
// thread when it loads. They are only added to the space's list
// when the chunk is bound, and if they're not already there.
// Originally I had two different types of portal - BoundPortal
// and UnboundPortal, but this was easier in a lot of ways.
// This also means that chunks need a way of going back to being
// 'unloaded' after they've been loaded. So they probably want
// a reference count too. We could maybe use a smart pointer for
// this actually, because the ChunkBoundaries will all be cleared
// when a chunk is unloaded, and the references will go away.

BW_END_NAMESPACE


#endif // CHUNK_BOUNDARY_HPP
