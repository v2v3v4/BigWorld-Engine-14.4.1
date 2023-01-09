#ifndef TERRAIN_TERRAIN_HEIGHT_MAP2_HPP
#define TERRAIN_TERRAIN_HEIGHT_MAP2_HPP

#ifdef EDITOR_ENABLED
#define BW_MULTITHREADED_TERRAIN_COLLIDE 1
#else
#define BW_MULTITHREADED_TERRAIN_COLLIDE 0
#endif

#include "../terrain_height_map.hpp"
#include "terrain_quad_tree_cell.hpp"
#include "../terrain_data.hpp"
#include "resource.hpp"

// include header for specidic collision queries.
#include "physics2/worldtri.hpp"

#if BW_MULTITHREADED_TERRAIN_COLLIDE
#include <atomic>
#endif

BW_BEGIN_NAMESPACE

namespace Terrain
{
    class TerrainCollisionCallback;
}

namespace Terrain
{

#if BW_MULTITHREADED_TERRAIN_COLLIDE
	typedef std::atomic<TerrainQuadTreeCell*> HeightMapQuadtreePtr;
#else
	typedef TerrainQuadTreeCell* HeightMapQuadtreePtr;
#endif

    /**
     *  This class provides access to terrain height map data for the 
     *  second generation of terrain.
     */
    class TerrainHeightMap2 : public TerrainHeightMap
    {
    public:

		// These define the non-visible border around each side of the
		// height map.
		static const uint32 DEFAULT_VISIBLE_OFFSET = 2;

		class UnlockCallback
		{
		public:
			virtual ~UnlockCallback(){};
			virtual bool notify() = 0;
		};

   		explicit TerrainHeightMap2( float blockSize,
									uint32 size = 0,
									uint32 lodLevel = 0,
									UnlockCallback* unlockNotifier = NULL );

        ~TerrainHeightMap2();      

        uint32 width() const;
        uint32 height() const;

#ifdef EDITOR_ENABLED
		// Editor specific functionality
		bool create( uint32 size, BW::string *error = NULL );

		bool lock(bool readOnly);
        ImageType &image();        
		bool unlock();
		bool save( DataSectionPtr pSection ) const;
#endif	// EDITOR_ENABLED

		ImageType const &image() const;

		bool load(DataSectionPtr dataSection, BW::string *error = NULL);
		void unlockCallback( UnlockCallback* ulc ) { unlockCallback_ = ulc; };

		float spacingX() const;
        float spacingZ() const;

        uint32 blocksWidth() const;
        uint32 blocksHeight() const;

        uint32 verticesWidth() const;
        uint32 verticesHeight() const;

        uint32 polesWidth() const;
        uint32 polesHeight() const;

		virtual uint32 xVisibleOffset() const;
		virtual uint32 zVisibleOffset() const;

        uint32 internalVisibleOffset() const;

        float minHeight() const;
		float maxHeight() const;

		float heightAt(int x, int z) const;
        float heightAt(float x, float z) const;

		Vector3 normalAt(int x, int z) const;
        Vector3 normalAt(float x, float z) const;

	    bool collide
        ( 
            Vector3             const &start, 
            Vector3             const &end,
		    TerrainCollisionCallback *pCallback 
        ) const;

	    bool collide
        ( 
            WorldTriangle       const &start, 
            Vector3             const &end,
		    TerrainCollisionCallback *pCallback 
        ) const;

	    bool hmCollide
        (
            Vector3             const &originalSource,
            Vector3             const &start, 
            Vector3             const &end,
		    TerrainCollisionCallback *pCallback
        ) const;

		bool hmCollideAlongZAxis
        (
            Vector3             const &originalSource,
            Vector3             const &start, 
            Vector3             const &end,
		    TerrainCollisionCallback *pCallback
        ) const;

	    bool hmCollide
        (
            WorldTriangle		const &triStart, 
            Vector3             const &triEnd,
			float				xStart,
			float				zStart,
			float				xEnd,
			float				zEnd,
		    TerrainCollisionCallback *pCallback
        ) const;

		uint32					lodLevel() const;

		static BW::string getHeightSectionName(const BW::string&	base, 
												uint32				lod );

		// Optimized method for colliding short or near vertical segments with terrrain.
		bool collideShortSegment(
			float& outCollDist,
			Vector3& outTriNormal,
			const Vector3& start, 
			const Vector3& dir,
			float dist) const;

		// Visit triangles that are _may_ intersects with given oriented box.
		void visitTriangles(
			RealWTriangleSet& outTris,
			const Vector3& boxCenter, 
			const Vector3& boxE1,
			const Vector3& boxE2,
			const Vector3& boxE3,
			const Vector3& clippingPlanePt,
			const Vector3& clippingPlaneNormal) const;

		void visitTriangles(RealWTriangleSet& outTris, const BoundingBox& bb) const;

		static void optimiseCollisionAlongZAxis( bool enable );

    private:
		bool loadHeightMap( const void*			data, 
							size_t				length,
							BW::string *error = NULL );
		void recalcQuadTree() const;
		void invalidateQuadTree() const;
		void ensureQuadTreeValid() const;

		bool 
		checkGrid
		( 
			int  				gridX, 
			int					gridZ, 
			Vector3				const &start, 
			Vector3				const &end, 
			TerrainCollisionCallback* pCallback 
		) const;

		bool 
		checkGrid
		( 
			int		  			gridX, 
			int					gridZ, 
			WorldTriangle		const &start, 
			Vector3				const &end, 
			TerrainCollisionCallback* pCallback 
		) const;


		void	allocateQuadTreeData() const;
		void	freeQuadTreeData() const;
		/**
		 * Internal accessor functions. Use these within class instead of the 
		 * public versions to avoid virtual function calls.
		 */
		void			refreshInternalDimensions();
		inline uint32	internalBlocksWidth()			const;
		inline uint32	internalBlocksHeight()			const;
		inline float	internalSpacingX()				const;
		inline float	internalSpacingZ()				const;
		inline float	internalHeightAt(int x, int z)	const;

		/**
		 *	This method returns the threshold for using the quadtree on this block
		 */
		inline float	quadtreeThreshold()				const;


#ifdef EDITOR_ENABLED
		// editor specific functionality
		void recalcMinMax();

		size_t				lockCount_;
		bool				lockReadOnly_;
#endif

		Moo::Image<float>	heights_;
		float				minHeight_;
		float				maxHeight_;

		// This member is used to cache an expensive calculation in order to
		// optimise "normalAt" (avoiding expensive sqrtf call).
		float				diagonalDistanceX4_;

		// This can notify user when "unlock" is called.
		UnlockCallback*				unlockCallback_;

		// These are mutable as the quad tree can be generated on demand
		// when the collide methods are being called
        mutable HeightMapQuadtreePtr quadTreeRoot_;
		// pre allocated block of memory to store all quadtree cells
		mutable TerrainQuadTreeCell* quadTreeData_;
		mutable int					 numQuadTreeCells_;

		uint32						visibleOffset_;
		uint32						lodLevel_;

		uint32						internalBlocksWidth_;
		uint32						internalBlocksHeight_;
		float						internalSpacingX_;
		float						internalSpacingZ_;

		size_t						sizeInBytes_;
		mutable bw_atomic32_t isQuadTreeInUse_;
	};

	inline uint32 TerrainHeightMap2::internalVisibleOffset() const
	{
		return visibleOffset_;
	}

	inline void TerrainHeightMap2::refreshInternalDimensions()
	{
		internalBlocksWidth_ = heights_.width() - ( internalVisibleOffset() * 2 ) - 1;
		internalBlocksHeight_ = heights_.height() - ( internalVisibleOffset() * 2 ) - 1;
		internalSpacingX_ = blockSize() / internalBlocksWidth_;
		internalSpacingZ_ = blockSize() / internalBlocksHeight_;

		// Cache the diagonal distance X 4, to improve normalAt performance.
		diagonalDistanceX4_ =
			sqrtf( internalSpacingX_ * internalSpacingZ_ * 2 ) * 4;
	}

	inline uint32 TerrainHeightMap2::internalBlocksWidth() const
	{
		return internalBlocksWidth_;
	}

	inline uint32 TerrainHeightMap2::internalBlocksHeight() const
	{
		return internalBlocksHeight_;
	}

	inline float TerrainHeightMap2::internalSpacingX() const
	{
		return internalSpacingX_;
	}

	inline float TerrainHeightMap2::internalSpacingZ() const
	{
		return internalSpacingZ_;
	}

	inline float TerrainHeightMap2::internalHeightAt(int x, int z) const
	{
		return heights_.get( x + internalVisibleOffset(), z + internalVisibleOffset() );
	}

	inline uint32 TerrainHeightMap2::lodLevel() const
	{
		return lodLevel_;
	}

	/**
	 *	Get the quadtree treshold, this value is used as the minimum size
	 *	for the quadtree operations.
	 *	@return the quadtree threshold
	 */
	inline float TerrainHeightMap2::quadtreeThreshold() const
	{
		//-- quad-tree cell below 4 centimeter doesn't have much sense.
		//return this->internalSpacingX() * 4.f;
		return std::max<float>(this->internalSpacingX() * 4.f, 4.0f);
	}

	typedef TerrainMapIter<TerrainHeightMap2>   TerrainHeightMap2Iter;
	typedef TerrainMapHolder<TerrainHeightMap2> TerrainHeightMap2Holder;

#ifndef MF_SERVER
	class HeightMapResource : public Resource<TerrainHeightMap2>
	{
	public:
#ifdef _NAVGEN
		HeightMapResource( ){};
#endif // _NAVGEN		
		HeightMapResource( float blockSize, const BW::string& heightsSectionName, uint32 lodLevel );

		inline  ResourceRequired	evaluate(	uint32	requiredVertexGridSize,
												uint32	standardHeightMapSize,
												float	detailHeightMapDistance,
												float	blockDistance,
												uint32 topLodLevel);
	protected:
		virtual bool load();

		BW::string terrainSectionName_;

		uint32 lodLevel_;

		float blockSize_;
	};

	inline ResourceRequired HeightMapResource::evaluate(	
		uint32	requiredVertexGridSize,
		uint32	standardHeightMapSize,
		float	detailHeightMapDistance,
		float	blockDistance,
		uint32 topLodLevel)
	{
		required_ = RR_No;

		// Is the distance within the area we need full height maps, OR
		// Is the standard height map too small to create required vertex lod?
		if ( blockDistance < detailHeightMapDistance || 
			( standardHeightMapSize <  requiredVertexGridSize ) )
		{
			required_ = RR_Yes;
			if (object_.exists() && topLodLevel != lodLevel_)
			{
				object_ = NULL;
				lodLevel_ = topLodLevel;
			}
		}

		return required_ ;
	}

#endif

} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_TERRAIN_HEIGHT_MAP2_HPP
