#ifndef BASE_CHUNK_SPACE_HPP
#define BASE_CHUNK_SPACE_HPP

#include "cstdmf/bw_map.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stringmap.hpp"

#include "math/vector2.hpp"
#include "math/vector3.hpp"

#include "network/basictypes.hpp"

#include "physics2/quad_tree.hpp"


BW_BEGIN_NAMESPACE

class Chunk;
class ChunkItem;
class BoundingBox;
class HullTree;
class HullBorder;
class HullContents;
class BSP;
class WorldTriangle;
class OutsideLighting;
class DataSection;
class GeometryMapping;
class ChunkObstacle;

typedef SmartPointer<DataSection> DataSectionPtr;

typedef QuadTree< ChunkObstacle> ObstacleTree;
typedef QTTraversal< ChunkObstacle > ObstacleTreeTraversal;


/**
 *	Outside chunk size in metres. Chunks may not be bigger than this in the x or
 *	z dimension.
 */
const float MAX_CHUNK_HEIGHT = 10000.f;
const float MAX_SPACE_WIDTH = 50000.f;
const float MIN_CHUNK_HEIGHT = -10000.f;

typedef uint32 ChunkSpaceID;
/**
 *	The null value for an ChunkSpaceID meaning that no space is referenced. 
 */
const ChunkSpaceID NULL_CHUNK_SPACE = ((ChunkSpaceID)0);

/*
 *	Forward declarations relating to chunk obstacles
 */
class CollisionCallback;
extern CollisionCallback & CollisionCallback_s_default;


/**
 *	This map is used to map a chunk's name to its pointer.
 */
typedef StringHashMap< BW::vector<Chunk*> > ChunkMap;

/**
 *	This map is used to map an outside chunk's coordinate its pointer.
 */
typedef BW::map< std::pair<int, int>, BW::vector<Chunk*> > GridChunkMap;


extern const BW::string SPACE_SETTING_FILE_NAME;
extern const BW::wstring SPACE_SETTING_FILE_NAME_W;
extern const BW::string SPACE_LOCAL_SETTING_FILE_NAME;
extern const BW::wstring SPACE_LOCAL_SETTING_FILE_NAME_W;
extern const BW::string SPACE_FLORA_SETTING_FILE_NAME;
extern const BW::wstring SPACE_FLORA_SETTING_FILE_NAME_W;
/**
 *	This class is the base class that is used by the client and the server to
 *	implement their ChunkSpace.
 */
class BaseChunkSpace : public SafeReferenceCount
{
public:
	BaseChunkSpace( ChunkSpaceID id );
	virtual ~BaseChunkSpace();

	ChunkSpaceID id() const					{ return id_; }

	int boundChunkNum() const;
	void addChunk( Chunk * pChunk );
	Chunk * findOrAddChunk( Chunk * pChunk );
	Chunk * findChunk( const BW::string & identifier,
		const BW::string & mappingName );
	Chunk * findChunk( const BW::string & identifier,
		GeometryMapping * mapping  );
	void delChunk( Chunk * pChunk );
	void clear();

	const ChunkMap & chunks() const		{ return currentChunks_; }

	bool	inBounds( int gridX, int gridY ) const;

	float gridSize() const;
	bool gridSizeOK( float gridSize );

	int minGridX() const 			{ return minGridX_; }
	int maxGridX() const 			{ return maxGridX_; }
	int minGridY() const 			{ return minGridY_; }
	int maxGridY() const 			{ return maxGridY_; }

	int minSubGridX() const 			{ return minSubGridX_; }
	int maxSubGridX() const 			{ return maxSubGridX_; }
	int minSubGridY() const 			{ return minSubGridY_; }
	int maxSubGridY() const 			{ return maxSubGridY_; }

	float minCoordX() const 			{ return gridSize_ * minGridX_; }
	float maxCoordX() const 			{ return gridSize_ * (maxGridX_ + 1); }
	float minCoordZ() const 			{ return gridSize_ * minGridY_; }
	float maxCoordZ() const 			{ return gridSize_ * (maxGridY_ + 1); }

	static int obstacleTreeDepth()		{ return s_obstacleTreeDepth; }
	static void obstacleTreeDepth( int v )	{ s_obstacleTreeDepth = v; }

	/**
	 *	This class is used to represent a grid square of the space. It stores
	 *	all of the chunks and obstacles that overlap this grid.
	 */
	class Column
	{
	public:
		Column( BaseChunkSpace * pSpace, int x, int z );
		~Column();

		// Chunk related methods
		void addChunk( HullBorder & border, Chunk * pChunk );
		bool hasInsideChunks() const			{ return pChunkTree_ != NULL; }

		Chunk * findChunk( const Vector3 & point );
		Chunk * findChunkExcluding( const Vector3 & point, Chunk * pNot );

		// Obstacle related methods
		void addObstacle( const ChunkObstacle & obstacle );
		const ObstacleTree &	obstacles() const	{ return *pObstacleTree_; }
		ObstacleTree &			obstacles()			{ return *pObstacleTree_; }

		void addDynamicObstacle( const ChunkObstacle & obstacle );
		void delDynamicObstacle( const ChunkObstacle & obstacle );

		Chunk *		pOutsideChunk() const	{ return pOutsideChunk_; }

		// The stale flag gets set to indicate that the column needs to be
		// recreated. For example, when obstacles want to get removed from the
		// space, the will mark the column as stale, and the column will get
		// recreated in the next frame.
		void		stale()				{ stale_ = true; }
		bool		isStale() const		{ return stale_; }

		long size() const;

		void		openAndSee( Chunk * pChunk );
		void		shutIfSeen( Chunk * pChunk );

	protected:

		typedef BW::vector< const ChunkObstacle * > HeldObstacles;
		typedef BW::vector< HullContents * >	Holdings;

		HullTree *		pChunkTree_;
		ObstacleTree *	pObstacleTree_;
		HeldObstacles	heldObstacles_;
		Holdings		holdings_;

		Chunk *			pOutsideChunk_;

		bool 			stale_;
		Chunk *			shutTo_;

		BW::vector< Chunk * >			seen_;

	};

	inline float gridToPoint( int grid ) const
	{
		return grid * gridSize_;
	}

	inline int pointToGrid( float point ) const
	{
		return static_cast<int>( floorf( point / gridSize_ ) );
	}

	inline float alignToGrid( float point ) const
	{
		return gridToPoint( pointToGrid( point ) );
	}

	void mappingSettings( DataSectionPtr pSS );
	virtual bool isMapped() const = 0;

	void blurredChunk( Chunk * pChunk );
	bool removeFromBlurred( Chunk * pChunk );

	enum NavmeshGenerator
	{
		NAVMESH_GENERATOR_NONE,
		NAVMESH_GENERATOR_NAVGEN,
		NAVMESH_GENERATOR_RECAST,
	};

	NavmeshGenerator navmeshGenerator() const { return navmeshGenerator_; }
	void navmeshGenerator( const BW::string& navmeshGenerator );

	bool scaleNavigation() { return scaleNavigation_; }
	void scaleNavigation( bool scale ) { scaleNavigation_ = scale; }

private:
	ChunkSpaceID	id_;

protected:
	void recalcGridBounds()		{ }	// implemented in derived classes

	void fini();

	float			gridSize_;
	int				minGridX_;
	int				maxGridX_;
	int				minGridY_;
	int				maxGridY_;
	int				minSubGridX_;
	int				maxSubGridX_;
	int				minSubGridY_;
	int				maxSubGridY_;

	ChunkMap		currentChunks_;
	GridChunkMap	gridChunks_;

	BW::vector<Chunk*>		blurred_;	// bound but not focussed

	static int				s_obstacleTreeDepth;

	NavmeshGenerator		navmeshGenerator_;
	bool					scaleNavigation_;
};

BW_END_NAMESPACE

#endif // BASE_CHUNK_SPACE_HPP
