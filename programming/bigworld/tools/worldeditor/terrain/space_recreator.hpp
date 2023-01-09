#ifndef SPACE_RECREATOR_HPP
#define SPACE_RECREATOR_HPP

#include "worldeditor/misc/sync_mode.hpp"

BW_BEGIN_NAMESPACE

class ChunkVLO;

/**
 *
 * Convenience class that handles the recreation of spaces.
 * 
 */
class SpaceRecreator
{
public:

	// Holder structure for user configurable terrain 2 information
	struct TerrainInfo
	{
		uint32	version_;
		uint32	heightMapEditorSize_;
		uint32	heightMapSize_;
		uint32	normalMapSize_;
		uint32	holeMapSize_;
		uint32	shadowMapSize_;
		uint32	blendMapSize_;
	};

	SpaceRecreator(
		const BW::string& spaceName, float chunkSize,
		int width, int height,
		const TerrainInfo& terrainInfo );

	~SpaceRecreator();

	bool create();

	bool createInternal();

	// Populated if create() returns false;
	BW::string errorMessage() { return errorMessage_; }

private:
	SpaceRecreator( const SpaceRecreator& rhs );
	SpaceRecreator& operator=( const SpaceRecreator& rhs );

	bool shouldAbort();

	void calcBounds(
		int srcMinX, int srcMinZ, int srcMaxX, int srcMaxZ,
		int width, int height );

	const BW::string createSpaceDirectory();

	// Copy SPACE_SETTING_FILE_NAME from the old space, changing the values
	// we've been given and keeping the rest.
	const BW::string createSpaceSettings();

	BW::string chunkName( int x, int z ) const;

	// Loosely based on ImportVisitor::visit
	bool normaliseTerrainBlockBlends( Terrain::EditorBaseTerrainBlock &block );

	bool createChunk( int x, int z );

	const BW::string createChunks( bool withPurge );

	const BW::string copyAssets();

	void fixChunkLinkArray(
		DataSectionPtr arrayDS, bool knownToBeChunkLinks = false );

	bool fixChunkLinksInChunk( DataSectionPtr chunkDS );

	Terrain::EditorTerrainBlock2Ptr readTerrainBlock(
		DataSectionPtr spaceDS, int x, int z );

	bool writeTerrainBlock(
		DataSectionPtr spaceDS, int x, int z,
		Terrain::EditorTerrainBlock2Ptr pTerrainBlock );

	void blitHeights( Terrain::TerrainHeightMap::ImageType &heightMapImage,
		Terrain::EditorTerrainBlock2Ptr pNeighbourBlock,
		uint32 srcX, uint32 srcY, uint32 srcW, uint32 srcH,
		uint32 dstX, uint32 dstY );


	bool patchHeightMapEdges( DataSectionPtr spaceDS, int x, int z );

	const BW::string fixChunks( bool withPurge );

	SyncMode syncMode_;
	const BW::string srcSpaceName_;
	const BW::string spaceName_;
	const float chunkSize_;
	BoundingBox srcSpaceBB_;
	BW::string errorMessage_;
	int minX_, maxX_, minZ_, maxZ_;
	int xOffset_, zOffset_;
	const TerrainInfo terrainInfo_;
	Terrain::TerrainSettingsPtr pTerrainSettings_;
	DataSectionPtr terrainSettingsDS_;
	BW::set<BW::string> subDir_;
	BW::set<BW::string> overlappers_;
	struct VLOMemento
	{
		Matrix transform;
		BW::string type;
	};
	StringHashMap<VLOMemento> vlos_;
	BW::set<BW::string> stationGraphs_;
	// This is GUID => ( OldChunk, NewChunk )
	
	StringHashMap< std::pair<BW::string, BW::string> > guidChunks_;
	bool changingChunkSize_;

	BW::set<ChunkItemPtr> itemsWithPossibleBrokenLinks;

	class Impl;
	Impl* pImpl;
	friend Impl;
};

BW_END_NAMESPACE

#endif // SPACE_RECREATOR_HPP
