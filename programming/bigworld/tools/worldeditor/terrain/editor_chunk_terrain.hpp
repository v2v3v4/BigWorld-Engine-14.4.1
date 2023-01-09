#ifndef EDITOR_CHUNK_TERRAIN_HPP
#define EDITOR_CHUNK_TERRAIN_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "terrain/editor_base_terrain_block.hpp"
#include "terrain/terrain_height_map.hpp"
#include "chunk/chunk_terrain.hpp"
#include "common/editor_chunk_terrain_base.hpp"
#include "max_layers_overlay.hpp"
#include "forced_lod_overlay.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the editor version of a chunk item for a terrain block.
 */
class EditorChunkTerrain : public EditorChunkTerrainBase
{
	DECLARE_EDITOR_CHUNK_ITEM( EditorChunkTerrain )
public:
	enum Neighbour
	{
		FIRST_NEIGHBOUR	= 0,	// for iteration
		NORTH_WEST		= 0,
		NORTH,
		NORTH_EAST,
		EAST,
		SOUTH_EAST,
		SOUTH,
		SOUTH_WEST,
		WEST,
		LAST_NEIGHBOUR			// for iteration (always make sure this is last)
	};

	enum NeighbourError
	{
		NEIGHBOUR_OK,
		NEIGHBOUR_NONEXISTANT,
		NEIGHBOUR_NOTLOADED,
		NEIGHBOUR_LOCKED
	};

	EditorChunkTerrain();
	virtual ~EditorChunkTerrain();

	void onEditHeights();

    void relativeElevations(Terrain::TerrainHeightMap::ImageType &relHeights) const;

	float slope(int xIdx, int zIdx) const;

    static const size_t NO_DOMINANT_BLEND = (size_t)-1;

    size_t dominantBlend(float x, float z, uint8 *strength = NULL) const;

	virtual const Matrix & edTransform();
	virtual bool edTransform(const Matrix & m, bool transient);
	virtual void edBounds(BoundingBox & bbRet) const;
	virtual bool edSave( DataSectionPtr pSection );

	virtual DataSectionPtr pOwnSect();
	virtual const DataSectionPtr pOwnSect()	const;

	virtual void edPreDelete();
	virtual void edPostCreate();
	virtual void edPostClone( EditorChunkItem* srcItem );
	virtual BinaryPtr edExportBinaryData();
	virtual bool edShouldDraw();
	virtual Vector3 edMovementDeltaSnaps();
	virtual float edAngleSnaps();

	virtual bool edIsSnappable() { return edShouldDraw(); }
	virtual bool edEdit( class GeneralEditor & editor );

	virtual void draw( Moo::DrawContext& drawContext );

	virtual void toss(Chunk * pChunk);

	virtual Moo::LightContainerPtr edVisualiseLightContainer();

	EditorChunkTerrain *
	neighbour
	(
		Neighbour			n, 
		bool				forceLoad		= false,
		bool				writable		= true,
		NeighbourError		*isValid		= NULL
	) const;

    static Terrain::EditorBaseTerrainBlockPtr 
    loadBlock
    (
        BW::string         const&	resource,
        EditorChunkTerrain*			ect,
		const Vector3&				worldPosition,
		BW::string * errorString = NULL
    );

	static EditorChunkTerrainPtr findChunkFromPoint( const Vector3& pos );

	static size_t findChunksWithinBox( BoundingBox const &bb, EditorChunkTerrainPtrs &outVector );

	bool brushTextureExist_;

	virtual void syncInit();
	
	/**
	 * This method should be called if the terrain has been changed
	 * in such a way that the bounding box and Umbra model
	 * needs to be regenerated 
	 */
	void onTerrainChanged();
	
	virtual void edHidden( bool value );
	virtual bool edHidden() const;
	virtual void edFrozen( bool value );
	virtual bool edFrozen() const;

	int getForcedLod() const;
	bool setForcedLod( const int & forcedLod );

private:	
	bool validatePrefabVersion( BW::string& resource );

    bool load( DataSectionPtr pSection, Chunk * pChunk, BW::string* errorString = NULL );

    void copyHeights
    (
        int32           chunkDx, 
        int32           chunkDz, 
        uint32          srcX,
        uint32          srcZ,
        uint32          srcW,
        uint32          srcH,
        uint32          dstX,
        uint32          dstZ
    );

	static uint32 prefabVersion( BW::string& resource );

private:
	Matrix              transform_;
	DataSectionPtr      pOwnSect_;

	MaxLayersOverlayPtr   maxLayersWarning_;
	ForcedLodOverlayPtr   forcedLodNotice_;
};


typedef SmartPointer<EditorChunkTerrain>	EditorChunkTerrainPtr;
typedef BW::vector<EditorChunkTerrainPtr>	EditorChunkTerrainPtrs;

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_TERRAIN_HPP
