#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "cstdmf/bw_string.hpp"

#include "cstdmf/vectornodest.hpp"
#include "cstdmf/stringmap.hpp"

#include "math/boundbox.hpp"
#include "math/matrix.hpp"

#include "resmgr/datasection.hpp"
#include "resmgr/xml_section.hpp"

#include "chunk_boundary.hpp"
#include "chunk_item.hpp"

#include "umbra_config.hpp"

#if UMBRA_ENABLE

namespace Umbra
{
	namespace OB
	{
		class Cell;
	}
};


#endif


BW_BEGIN_NAMESPACE

class Portal2D;
class ChunkCache;
class ChunkSpace;
class GeometryMapping;
class ChunkProcessorManager;
#ifndef MF_SERVER
class ChunkTerrain;
#else
class ServerChunkTerrain;
typedef ServerChunkTerrain ChunkTerrain;
#endif

#ifdef EDITOR_ENABLED
class EditorChunkCache;
#endif


/**
 *	This class defines a chunk, the node of our scene graph.
 *
 *	A chunk is a convex three dimensional volume. It contains a description
 *	of the scene objects that reside inside it. Scene objects include
 *	lights, entities, sounds, and general drawable scene objects called
 *	items. It also defines the set of planes that form its boundary
 *	(with the exception of chunks reached through internal portals).
 *	Some planes have portals defined on them which indicate that a
 *	neighbouring chunk is visible through them.
 */
class Chunk
{
public:
	static void init();
	static void fini();

	Chunk( const BW::string & identifier, GeometryMapping * pMapping, 
			const Matrix& transform=Matrix::identity,
			const BoundingBox& localBounds=BoundingBox::s_insideOut_ );
	~Chunk();

	typedef BW::vector<ChunkItemPtr>	Items;

	// Set as authoritative version of this chunk.
	void appointAsAuthoritative();

	bool load( DataSectionPtr pSection );

	ChunkItemFactory::Result loadItem( DataSectionPtr pSection );
	static ChunkItemFactory::Result loadItem(
		DataSectionPtr pSection,
		Chunk * chunk );

	typedef StringHashMap<const ChunkItemFactory*> Factories;
	static Factories * getFactories() { return pFactories_; }


	static void setFactories( Factories & factories )
	{
		*pFactories_ = factories;
	}


	static void clearFactories();

	void unload();

	void bind( bool shouldFormPortalConnections );
	void bindPortals( bool shouldFormPortalConnections, bool shouldNotifyCaches );
	void unbind( bool cut );

	void focus();
	void smudge();

#ifdef EDITOR_ENABLED
	bool dirty() const;
#endif //EDITOR_ENABLED

	void resolveExterns( GeometryMapping * pDeadMapping = NULL );

	void updateBoundingBoxes( ChunkItemPtr pItem );

	// add or remove this static item from the chunk.
	// static items are rarely if ever removed
	bool addStaticItem( ChunkItemPtr pItem );
	void delStaticItem( ChunkItemPtr pItem );
	int staticItemIndex( ChunkItemPtr pItem ) const;
#ifdef EDITOR_ENABLED
	void moveStaticItem( ChunkItemPtr pItem );
#endif// EDITOR_ENABLED

	// add or remove this dynamic item from this chunk
	// usually only called by a ChunkItem on itself.
	void addDynamicItem( ChunkItemPtr pItem );
	bool modDynamicItem( ChunkItemPtr pItem,
		const Vector3 & oldPos, const Vector3 & newPos, const float diameter = 1.f,
		bool bUseDynamicLending = false);
	void delDynamicItem( ChunkItemPtr pItem, bool bUseDynamicLending=true );

	void jogForeignItems();

	// loan this (static) item to this chunk. Called by
	// a chunk item from its 'push' method implementation
	bool addLoanItem( ChunkItemPtr pItem );
	bool delLoanItem( ChunkItemPtr pItem, bool bCanFail=false );
    bool isLoanItem( ChunkItemPtr pItem ) const;

    size_t numItems() const;
    const ChunkItemPtr& item( size_t idx );

#ifndef MF_SERVER
	// draw methods
	//void draw( Portal2D * pClipPortal );
	void drawBeg( Moo::DrawContext& drawContext );
	void drawEnd();
	bool drawSelf( Moo::DrawContext& drawContext, bool lentOnly = false );

	// For umbra integration
	// This sets up per chunk objects for us
	void drawCaches( Moo::DrawContext& drawContext );


#endif // MF_SERVER

	Chunk *	findClosestUnloadedChunkTo( const Vector3 & point,
		float * pDist );


	bool						contains( const Vector3 & point, const float radius = 0.f ) const;
	bool						owns( const Vector3 & point );

	float						volume() const;
	const Vector3 &				centre() const		{ return centre_; }

	const BW::string &			identifier() const	{ return identifier_; }
	int16						x() const			{ return x_; }
	int16						z() const			{ return z_; }
	GeometryMapping *			mapping() const		{ return pMapping_; }
	ChunkSpace *				space() const		{ return pSpace_; }
	BW::string					binFileName() const;

	bool						isOutsideChunk() const { return isOutsideChunk_; }
	bool						hasInternalChunks()	const { return hasInternalChunks_; }
	void						hasInternalChunks( bool v ) { hasInternalChunks_ = v; }

	bool						isAppointed()	const	{ return isAppointed_; }
	bool						loading()	const	{ return loading_; }
	bool						loaded()	const	{ return loaded_; }
	bool						isBound()	const	{ return isBound_; }
	bool						completed()	const	{ return completed_; }
		/* See note about chunk states at the bottom of the cpp file */

	bool						focussed()	const	{ return focusCount_ > 0; }

	const Matrix &				unmappedTransform()	const	{ return unmappedTransform_; }
	const Matrix &				transform()	const	{ return transform_; }
	const Matrix &				transformInverse() const	{ return transformInverse_; }
	void						transform( const Matrix & transform );
	void						transformTransiently( const Matrix & transform );

	uint32						drawMark() const		{ return drawMark_; }
	void						drawMark( uint32 m )	{ drawMark_ = m; }

	uint32						reflectionMark() const		{ return reflectionMark_; }
	void						reflectionMark( uint32 m )	{ reflectionMark_ = m; }

	uint32						traverseMark() const		{ return traverseMark_; }
	void						traverseMark( uint32 m )	{ traverseMark_ = m; }

	static uint32				nextMark()			 { return s_nextMark_++; }
	static uint32				nextVisibilityMark() { return s_nextVisibilityMark_++; }

	float						pathSum() const		{ return pathSum_; }
	void						pathSum( float s )	{ pathSum_ = s; }

	const BoundingBox &			localBB() const				{ return localBB_; }
	const BoundingBox &			boundingBox() const			{ return boundingBox_; }
	bool						boundingBoxReady() const	{ return boundingBoxReady_; }

#ifndef MF_SERVER
	const BoundingBox &			visibilityBox() const { return visibilityBoxCache_; }
	bool						updateVisibilityBox();
	uint32						visibilityBoxMark() const { return visibilityBoxMark_; }
	void						visibilityBoxMark(uint32 mark) { visibilityBoxMark_ = mark; }
	static uint32				visibilityMark() { return s_nextVisibilityMark_; }
#endif // MF_SERVER

	void						localBB( const BoundingBox& bb ) { localBB_ = bb; }
	void						boundingBox( const BoundingBox& bb ) { boundingBox_ = bb; }

	ChunkBoundaries &			bounds()			{ return bounds_; }
	ChunkBoundaries &			joints()			{ return joints_; }
	bool						canSeeHeaven();

	const BW::string &			label() const		{ return label_; }

	BW::string					resourceID() const;

	static bool findBetterPortal( ChunkBoundary::Portal * curr, float withinRange,
			ChunkBoundary::Portal * test, const Vector3 & v );
	/**
	 *	Helper iterator class for iterating over all bound
	 *	portals in this chunk.
	 */
	class piterator
	{
	public:
		void operator++(int)
		{
			pit++;
			this->scan();
		}

		bool operator==( const piterator & other ) const
		{
			return bit == other.bit &&
				(bit == source_.end() || pit == other.pit);
		}

		bool operator!=( const piterator & other ) const
		{
			return !this->operator==( other );
		}

		ChunkBoundary::Portal & operator*()
		{
			return **pit;
		}

		ChunkBoundary::Portal * operator->()
		{
			return *pit;
		}


	private:
		piterator( ChunkBoundaries & source, bool end ) :
			source_( source )
		{
			if (!end)
			{
				bit = source_.begin();
				if (bit != source_.end())
				{
					pit = (*bit)->boundPortals_.begin();
					this->scan();
				}
			}
			else
			{
				bit = source_.end();
			}
		}

		void scan()
		{
			while (bit != source_.end() && pit == (*bit)->boundPortals_.end())
			{
				bit++;
				if (bit != source_.end())
					pit = (*bit)->boundPortals_.begin();
			}
		}

		ChunkBoundaries::iterator			bit;
		ChunkBoundary::Portals::iterator	pit;
		ChunkBoundaries	& source_;

		friend class Chunk;
	};

	piterator pbegin()
	{
		return piterator( joints_, false );
	}

	piterator pend()
	{
		return piterator( joints_, true );
	}

	ChunkBoundary::Portal * findClosestPortal( const Vector3 & point,
			float maxDistance = FLT_MAX );
	ChunkBoundary::Portal * findMatchingPortal(
			const Chunk * pDestChunk,
			const ChunkBoundary::Portal * pPortal ) const;

	//-- BigWorld has had only register method, because factories exist always. But now some
	//--	 object types may not exist even if they present in code. For example Decals, Lights and 
	//--	 so on.
	static void registerFactory( const BW::string & section,
		const ChunkItemFactory & factory );
	static void unregisterFactory(const BW::string& section);

	ChunkCache * & cache( int index ) const			{ return caches_[index]; }

	Chunk * fringeNext()							{ return fringeNext_; }
	Chunk * fringePrev()							{ return fringePrev_; }
	void fringeNext( Chunk * pChunk )				{ fringeNext_ = pChunk; }
	void fringePrev( Chunk * pChunk )				{ fringePrev_ = pChunk; }

	int				sizeStaticItems() const;
	BW::vector<ChunkItemPtr>	staticItems() const	{ return selfItems_; }

	void			loading( bool b );

	bool			removable() const				{ return removable_; }
	void			removable( bool b )				{ removable_ = b; }

#ifdef EDITOR_ENABLED
	friend class EditorChunkCache;
	static bool hideIndoorChunks_;
#endif

	static uint32		s_nextMark_;
	static BW::vector<Chunk*>		s_bindingChunks_;

#if UMBRA_ENABLE
	Umbra::OB::Cell* getUmbraCell() const;
	void addUmbraShadowCasterItem( ChunkItem* pItem );
	void clearShadowCasters();
	Items& shadowItems() { return shadowItems_; }
	static void setUmbraChunks( BW::vector<Chunk*>* pUmbraChunks ) { s_umbraChunks_ = pUmbraChunks; }
#endif

	bool getTerrainHeight( float x, float z, float &fHeight );
	ChunkTerrain* getTerrain();
	void collectOverlappedOutsideChunksForShell( Chunk* chunks[4] ) const;

	static bool isOutsideChunk( const BW::StringRef & identifier );
	static bool loadInclude( DataSectionPtr pSection, const Matrix & flatten,
			BW::string* errorStr, Chunk * chunk, bool isOutsideChunk );

private:
	void syncInit();
	bool loadInclude( DataSectionPtr pSection, const Matrix & flatten, BW::string* errorStr );
	bool formBoundaries( DataSectionPtr pSection );

	void bind( Chunk * pChunk );
	bool formPortal( Chunk * pChunk, ChunkBoundary::Portal & portal );
	void unbind( Chunk * pChunk, bool cut );
	void updateCompleted();

	void notifyCachesOfBind( bool isUnbind );

	BW::string		identifier_;
	int16			x_;
	int16			z_;
	GeometryMapping * pMapping_;
	ChunkSpace		* pSpace_;

	bool			isOutsideChunk_;
	bool			hasInternalChunks_;

	// Accepted by the main thread as the authoritative version of this chunk.
	bool			isAppointed_;

	bool			loading_;
	bool			loaded_;
	bool			isBound_;
	// set to true when all contained shells are focussed
	bool			completed_;
	int				focusCount_;

	Matrix			unmappedTransform_;
	Matrix			transform_;
	Matrix			transformInverse_;

	BoundingBox		localBB_;

	BoundingBox		boundingBox_;
	bool			boundingBoxReady_;
	bool			gotShellModel_;

#ifndef MF_SERVER
	BoundingBox		visibilityBox_;	
	BoundingBox		visibilityBoxCache_;	
	uint32			visibilityBoxMark_;
#endif // MF_SERVER

	static uint32   s_nextVisibilityMark_;

	Vector3			centre_;

	ChunkBoundaries		bounds_;	// physical edges (convex)
	ChunkBoundaries		joints_;	// logical joints (scattered)

	/**
	 *	@note Loading a chunk is NOT permitted to touch the 'mark_' or
	 *	'pathSum_' fields, as these fields and the methods that access
	 *	them may be used by the main thread at the same time that the
	 *	loading thread is loading a chunk.
	 */
	//@{
	uint32				drawMark_;
	uint32				traverseMark_;
	uint32				reflectionMark_;

	float				pathSum_;
	//@}

	ChunkCache * *		caches_;

#ifndef MF_SERVER
public:
	const Items & selfItems() const { return selfItems_; }
	const Items & dynoItems() const { return dynoItems_; }
private:
#endif // !MF_SERVER

	mutable RecursiveMutex chunkMutex_;
	Items				selfItems_;
	Items				dynoItems_;
	Items				swayItems_;

	class Lender : public ReferenceCount
	{
	public:
		~Lender();
		void releaseItems( Chunk * pOwner );

		Chunk *				pLender_;
		Items				items_;
	};
	typedef SmartPointer<Lender> LenderPtr;

#ifndef MF_SERVER
public:
	typedef BW::vector<LenderPtr>	Lenders;
	const Lenders & lenders() const { return lenders_; }
private:
#else
	typedef BW::vector<LenderPtr>	Lenders;
#endif // !MF_SERVER

	Lenders				lenders_;

	typedef BW::vector<Chunk *>	Borrowers;
	Borrowers			borrowers_;

	VectorNoDestructor<Items>		lentItemLists_;

	BW::string			label_;

	static Factories	* pFactories_;

	friend class HullChunk;

	Chunk *				fringeNext_;
	Chunk *				fringePrev_;

	bool				inTick_;
	bool				removable_;

#if UMBRA_ENABLE
	Items shadowItems_;
	static BW::vector<Chunk*>*	s_umbraChunks_;
#endif
	ChunkTerrain*		pChunkTerrain_;

	//debug

public: static	uint32	s_instanceCount_;
public: static	uint32	s_instanceCountPeak_;

};

typedef Chunk*	ChunkPtr;


// Draws the culling HUD
void Chunks_drawCullingHUD();

BW_END_NAMESPACE

#endif // CHUNK_HPP
