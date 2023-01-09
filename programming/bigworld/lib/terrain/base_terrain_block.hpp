#ifndef TERRAIN_BASE_TERRAIN_BLOCK_HPP
#define TERRAIN_BASE_TERRAIN_BLOCK_HPP

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/debug.hpp"
#include "base_render_terrain_block.hpp"
#include "terrain_finder.hpp"

BW_BEGIN_NAMESPACE

class WorldTriangle;

namespace Terrain
{
    class BaseTerrainBlock;
	class EffectMaterial;
    class HorizonShadowMap;
    class TerrainCollisionCallback;
    class TerrainHeightMap;
    class TerrainHoleMap;
    class DominantTextureMap;
	class TerrainSettings;
    typedef SmartPointer<DominantTextureMap> DominantTextureMapPtr;
    typedef SmartPointer<BaseTerrainBlock> BaseTerrainBlockPtr;
	typedef SmartPointer<TerrainSettings> TerrainSettingsPtr;

#ifdef MF_SERVER
	typedef SafeReferenceCount	BaseTerrainBlockBase;
#else
	typedef BaseRenderTerrainBlock BaseTerrainBlockBase;
#endif

    /**
     *  This is the interface for all terrain blocks.
     */
    class BaseTerrainBlock : public BaseTerrainBlockBase
    {
    public:
        /**
         *  This loads a BaseTerrainBlockPtr from disk.  It creates the
         *  appropriate implementation based on the terrain version.
         *
         *  @param filename     The name of the file to load.
		 *	@param worldTransform	The transform to use to place the
		 *							terrain block in world position.
		 *  @param cameraPosition	The position of the camera so that the
		 *						correct LOD can be loaded.
		 *  @param pSettings    TerrainSettings to use while loading the block.
		 *  @param error        If an error occurs in loading then this will be
         *						set to a description of the error.
		 *
         *  @return             A pointer to the loaded terrain block.
         */
        static BaseTerrainBlockPtr loadBlock( BW::string const &filename, 
			const Matrix& worldTransform, const Vector3& cameraPosition, 
			TerrainSettingsPtr pSettings, BW::string* error = NULL );

        /**
         *  This is the BaseTerrainBlock default constructor.
         */
        BaseTerrainBlock( float blockSize );

        /**
         *  This is the BaseTerrainBlock destructor.
         */
        virtual ~BaseTerrainBlock();

		/**
		 *  Gets the terrain version. This method should only be called when
		 *  about to create a new terrain block, to find out which version it
		 *  is.
		 *
		 *  @param resource         the path to the '/terrain' resource in the
		 *                          cdata file.
		 *  @return the terrain version or 0 otherwise.
		 */
		static uint32 terrainVersion( BW::string& resource );

        /**
         *  This function loads an BaseTerrainBlock from a file.
         *
         *  @param filename         The file to load the block from.
		 *	@param worldTransform	The transform to use to place the
		 *							terrain block in world position.
		 *	@param cameraPosition	The camera position so that the
		 *							correct LOD can be loaded.
         *  @param error            If not null and there was an error loading
         *                          the terrain then this will be set to a
         *                          description of the problem.
         *  @return                 True if the load was completely successful, 
         *                          false if a problem occurred.
         */
        virtual bool load(	BW::string const	&filename, 
							Matrix  const		&worldTransform,
							Vector3 const		&cameraPosition,
							BW::string			*error = NULL) = 0;

        /**
         *  This function gets the height map for the terrain.
         *
         *  @return                The height map for the terrain.
         */
        virtual TerrainHeightMap &heightMap() = 0;

        /**
         *  This function gets the height map for the terrain.
         *
         *  @return                The height map for the terrain.
         */
        virtual TerrainHeightMap const &heightMap() const = 0;

        /**
         *  This function gets the hole map for the terrain.
         *
         *  @return                The hole map for the terrain.
         */
        virtual TerrainHoleMap &holeMap() = 0;

        /**
         *  This function gets the hole map for the terrain.
         *
         *  @return                The hole map for the terrain.
         */
        virtual TerrainHoleMap const &holeMap() const = 0;

		/**
		 *	This function gets the dominant texture map for the terrain.
		 *
		 *	@return				The dominant texture map for the terrain.
		 */
		 virtual DominantTextureMapPtr dominantTextureMap() = 0;

		/**
		 *	This function gets the dominant texture map for the terrain.
		 *
		 *	@return				The dominant texture map for the terrain.
		 */
		 virtual DominantTextureMapPtr const dominantTextureMap() const = 0;

        /**
         *  This function gets the containing BoundingBox for the terrain.  
         *
         *  @return                The bounding box to get.
         */
        virtual BoundingBox const& boundingBox() const = 0;

        /**
         *  This function determines whether the given line segment intersects
         *  the terrain, and if so where.
         *
         *  @param start            The start of the line segment.
         *  @param end              The end of the line segment.
         *  @param callback         The collision callback.
         *  @return                 True if there was a collision, false otherwise.
         */
        virtual bool 
        collide
        (
            Vector3                 const &start, 
            Vector3                 const &end,
            TerrainCollisionCallback *callback
        ) const = 0;

        /**
         *  This function determines whether the given prism intersects
         *  the terrain, and if so where.
         *
         *  @param start            The start face of the prism.
         *  @param end              The end of the line segment.
         *  @param callback         The collision callback.
         *  @return                 True if there was a collision, false otherwise.
         */
        virtual bool 
        collide
        (
            WorldTriangle           const &start, 
            Vector3                 const &end,
            TerrainCollisionCallback *callback
        ) const = 0;

        /**
         *  This function finds the block going at the given position.
         *
         *  @param pos              The (world) position of the required block.
         *                          Only x and z are used.
         *  @return                 The block and it's transformations.  These
         *                          are set to NULL if no block is found.
         */
        static TerrainFinder::Details findOutsideBlock(Vector3 const &pos);

        static float NO_TERRAIN;

        /**
         *  This function gets the height at the given position.
         *
         *  @param x                The x coordinate.
         *  @param z                The z coordinate.
         *  @return                 The height at the given position, or
         *                          NO_TERRAIN if there is no terrain under the
         *                          given position.
         */
        static float getHeight(float x, float z);

        /**
         *  This is used to set the terrain finder.
         */
	    static void setTerrainFinder(TerrainFinder & terrainFinder);

        /**
         *  This function determines the height at the given point, taking into
         *  account holes.
         *
         *  @param x            The x coordinate to get the height at.
         *  @param z            The z coordinate to get the height at.
         *  @return             The height at x, z.
         */
        virtual float heightAt(float x, float z) const = 0;

        /**
         *  This function determines the normal at the given point, taking into
         *  account holes.
         *
         *  @param x            The x coordinate to get the normal at.
         *  @param z            The z coordinate to get the normal at.
         *  @return             The normal at x, z.
         */ 
        virtual Vector3 normalAt(float x, float z) const = 0;

        /**
         *  Returns the appropriate name of the datasection for the block's
		 *  terrain data, for instance, "terrain", "terrain2", etc.
         *
         *  @return             Terrain data section name.
         */ 
		virtual const BW::string dataSectionName() const = 0;

        /**
         *  Returns if the chunk is currently doing background tasks,
		 *  either loading or unloading in the background
         *
         *  @return             true if doing background tasks, otherwise false
         */ 
		virtual bool doingBackgroundTask() const {	return false;	}

		/**
         *  Returns the edge length of this block in metres.
         *
         *  @return             width and height of this block in meters.
         */ 
		float blockSize() const { return blockSize_; }

		virtual int getForcedLod() const { return -1; }
		virtual void setForcedLod( int forcedLod ) { }

		static bool s_disableStreaming_;

#ifdef MF_SERVER
		const BW::string & resourceName() const;

		void resourceName( const BW::string & name );
#endif

    private:
        // Not permitted:
        BaseTerrainBlock(BaseTerrainBlock const &);
        BaseTerrainBlock &operator=(BaseTerrainBlock const &);

    private:
        static TerrainFinder            *terrainFinder_;
#ifdef MF_SERVER
		BW::string						resourceName_;
#endif
		float blockSize_;
    };
} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_BASE_TERRAIN_BLOCK_HPP
