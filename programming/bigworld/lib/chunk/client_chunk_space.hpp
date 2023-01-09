#ifndef CLIENT_CHUNK_SPACE_HPP
#define CLIENT_CHUNK_SPACE_HPP

#include "base_chunk_space.hpp"

#include "moo/moo_math.hpp"
#include "moo/light_container.hpp"

#include "romp/enviro_minder.hpp"

#include "umbra_config.hpp"

#include "cstdmf/bw_unordered_map.hpp"

namespace Umbra {
	namespace OB {
		class Cell;
	}
}

BW_BEGIN_NAMESPACE

class Chunk;
class ChunkItem;
typedef SmartPointer<ChunkItem> ChunkItemPtr;
class BoundingBox;
class HullTree;
class HullBorder;
class HullContents;
class BSP;
class WorldTriangle;
class OutsideLighting;

class ChunkObstacle;

typedef BW::unordered_map<uint32,class ChunkQuadTree*>	ChunkQuadTreeMap;


/**
 *	This template class is a grid that shadows a well-defined part of a
 *	two-dimensional space. The contents of the grid cells are defined by the
 *	user, through the type of the template argument. The span of the grid may
 *	also be defined, but it must be a power of two minus one.
 */
template < class T, int ISPAN = 511 >
class FocusGrid
{
public:
	FocusGrid();
	~FocusGrid();

	enum { SPANH = ISPAN/2 };
	enum { SPAN = ISPAN };
	enum { SPANX = ISPAN+1 };

	void origin( int cx, int cz );

	int originX() const	{ return cx_; }
	int originZ() const	{ return cz_; }

	T * & operator()( int x, int z )
		{ return grid_[ z & SPAN ][ x & SPAN ]; }

	const T * operator()( int x, int z ) const
		{ return grid_[ z & SPAN ][ x & SPAN ]; }

	bool inSpan( int x, int z ) const
	{
		return	x >= cx_ - SPANH && x <= cx_ + SPANH &&
				z >= cz_ - SPANH && z <= cz_ + SPANH;
	}

	const int xBegin() const	{	return cx_ - SPANH; }
	const int xEnd() const		{	return cx_ + SPANH + 1; }
	const int zBegin() const	{	return cz_ - SPANH; }
	const int zEnd() const		{	return cz_ + SPANH + 1; }

private:
	void	eraseEntry( int x, int z );

	int		cx_;
	int		cz_;
	T *		grid_[SPANX][SPANX];
};


/**
 *	This class defines a space and maintains the chunks that live in it.
 *
 *	A space is a continuous three dimensional Cartesian medium. Each space
 *	is divided piecewise into chunks, which occupy the entire space but
 *	do not overlap. i.e. every point in the space is in exactly one chunk.
 *	Examples include: planets, parallel spaces, spacestations, 'detached'
 *	apartments / dungeon levels, etc.
 */
class ClientChunkSpace : public BaseChunkSpace
{
public:
	ClientChunkSpace( ChunkSpaceID id );
	~ClientChunkSpace();

	void mappingSettings( DataSectionPtr pSS );

	void blurSpace();
	void clear();

	void addTickItem( ChunkItemPtr pItem );
	void delTickItem( ChunkItemPtr pItem );

	void addUpdateItem( ChunkItem* pItem );
	void delUpdateItem( ChunkItem* pItem );

	void addHomelessItem( ChunkItemPtr pItem );
	void delHomelessItem( ChunkItemPtr pItem );

	void focus( const Vector3 & point );

	void tick( float dTime );
	void updateAnimations();

	ChunkMap & chunks()					{ return currentChunks_; }
	GridChunkMap & gridChunks()			{ return gridChunks_; }

	bool ticking() const				{ return ticking_; }

	const Moo::DirectionalLightPtr & sunLight() const	{ return sunLight_; }
	const Moo::Colour & ambientLight() const			{ return ambientLight_; }
    void ambientLight( Moo::Colour col );

	const Moo::LightContainerPtr& lights() const	{ return lights_; }

	void heavenlyLightSource( const OutsideLighting * pOL )
											{ pOutLight_ = pOL; }
	void updateHeavenlyLighting();

	EnviroMinder & enviro()					{ return enviro_; }

	BW::SceneProvider* getChunkSceneProvider();
	void setChunkSceneProvider( BW::SceneProvider* sceneProvider );

	/**
	 *	This class is used internally by the chunk space, to store information
	 *	local to a particular column of space.
	 */
	class Column : public BaseChunkSpace::Column
	{
	public:
		Column( ClientChunkSpace * pSpace, int x, int z );
		~Column();

		uint nHoldings() const;

		void		soft( bool v )		{ soft_ = v; }
		bool		soft() const		{ return soft_; }

	private:

		bool		soft_;

		void addObstacleInternal( const ChunkObstacle & obstacle );
	};

	typedef FocusGrid<Column> ColumnGrid;

#if UMBRA_ENABLE
	Umbra::OB::Cell* umbraCell();
#endif

	void getVisibleOutsideChunks( const Matrix& viewProjection, BW::vector<Chunk*>& chunks );
	void addOutsideChunkToQuadTree( Chunk* pChunk );
	void removeOutsideChunkFromQuadTree( Chunk* pChunk );
	void updateOutsideChunkInQuadTree( Chunk* pChunk );
protected:
	bool			ticking_;

	const OutsideLighting	* pOutLight_;
	Moo::DirectionalLightPtr		sunLight_;
	Moo::Colour						ambientLight_;
	Moo::LightContainerPtr			lights_;
	EnviroMinder	enviro_;

	ColumnGrid					currentFocus_;

	// Must use a list, to a void incRef/decRef on other items (unsafe) that
	// could occur when resizing a vector for example.
	BW::list<ChunkItemPtr>	tickItems_;
	BW::list<ChunkItemPtr>	pendingTickItems_;

	BW::list<ChunkItem*> updateItems_;
	BW::list<ChunkItem*> pendingUpdateItems_;

	BW::vector<ChunkItemPtr>	homeless_;

	SimpleMutex	tickMutex_;
	SimpleMutex	updateMutex_;

#if UMBRA_ENABLE
	Umbra::OB::Cell*				umbraCell_;
#endif

	ChunkQuadTreeMap			chunkQuadTrees_;
	BW::vector<Chunk*>			visibilityUpdates_;
	SimpleMutex					visibilityMutex_;

	BW::SceneProvider*			sceneProvider_;			
};

BW_END_NAMESPACE

#endif // CLIENT_CHUNK_SPACE_HPP
