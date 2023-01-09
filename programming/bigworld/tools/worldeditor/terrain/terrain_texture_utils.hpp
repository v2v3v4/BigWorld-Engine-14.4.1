#ifndef TERRAIN_TEXTURE_UTILS_HPP
#define TERRAIN_TEXTURE_UTILS_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

namespace TerrainTextureUtils
{
	/**
	 *	Value used to compare two projection matrix entries to see if they
	 *	are the 'same' matrix.
	 */
	const float	PROJECTION_COMPARISON_EPSILON	= 0.01f;

	/**
	 *	Layers whose blends are all below this value should be removed.
	 */
	const uint8 REMOVE_LAYER_THRESHOLD			= 8;

	/**
	 *	Information about a layer at a chunk.
	 */
	struct LayerInfo
	{
		EditorChunkTerrainPtr			terrain_;
		size_t							layerIdx_;
		uint8							strength_;
		BW::string						textureName_;
		BW::string						bumpTextureName_;

		LayerInfo();
	};

	bool operator<(LayerInfo const layerInfo1, LayerInfo const layerInfo2);

	/**
	 *	Typedefs for a set of related layers.
	 */
	typedef BW::set<LayerInfo>			LayerSet;
	typedef BW::vector<LayerInfo>		LayerVec;
	typedef LayerSet::iterator			LayerSetIter;
	typedef LayerSet::const_iterator	LayerSetConstIter;

	/**
	 *	Possible errors when finding chunks that have matching textures.
	 *	Note that this is a bitfield.
	 */
	enum FindError
	{
		FIND_OK						= 0,
		FIND_EXTENDS_INTO_LOCKED	= 1,
		FIND_EXTENDS_INTO_UNLOADED	= 2,
		FIND_NO_SUCH_LAYER			= 4
	};

	FindError findNeighboursWithTexture
	(
		LayerInfo						const &layer,
		LayerSet						&layers
	);

	FindError findNeighboursWithTexture
	(
		EditorChunkTerrainPtr			seed,
		BW::string						const &texture,
		BW::string						const &bumpTexture,
		Vector4							const &uProj,
		Vector4							const &vProj,
		LayerSet						&layers
	);

	void printErrorMessage(FindError error);

	void setProjections
	(
		LayerSet						const &layers,
		Vector4							const &uProj,
		Vector4							const &vProj,
		bool							temporary
	);

	bool dominantTexture
	(
		Vector3							const &position,
		size_t							*idx,
		EditorChunkTerrain				**ect,
		Chunk							**chunk,
		uint8							*strength	= NULL
	);

	bool layerInfo
	(
		Vector3							const &position,
		LayerVec						&info
	);

	bool canReplaceTexture
	(
		size_t							idx,
		EditorChunkTerrainPtr			ect,
		BW::string						const &textureName,
		BW::string						const &bumpTextureName,
		Vector4							const &uProjection,
		Vector4							const &vProjection
	);

	bool replaceTexture
	(
		size_t							idx,
		EditorChunkTerrainPtr			ect,
		BW::string						const &textureName,
		BW::string						const &bumpTextureName,
		Vector4							const &uProjection,
		Vector4							const &vProjection,
		bool							undoable			= true,
		bool							undoBarrier			= true,
		bool							doFloodFill			= true,
		bool							regenLODs			= true
	);

	void deleteEmptyLayers
	(
		Terrain::EditorBaseTerrainBlock	&block,
		uint8							threshold
	);

	int mostPopulousLayer
	(
		Terrain::EditorBaseTerrainBlock	const &block,
		int								ignoreLayer
	);

	int mostPopulousLayer
	(
		Terrain::EditorBaseTerrainBlock	const &block,
		BW::vector<size_t>				const &ignoreLayers
	);

	int leastPopulousLayer
	(
		Terrain::EditorBaseTerrainBlock	const &block,
		int								ignoreLayer
	);

	int leastPopulousLayer
	(
		Terrain::EditorBaseTerrainBlock	const &block,
		BW::vector<size_t>				const &ignoreLayers
	);

	void layersDistributions
	(
		Terrain::EditorBaseTerrainBlock	const &block,
		BW::vector<uint64>				&distribution
	);

	void limitLayers
	(
		EditorChunkTerrainPtr			ect,
		size_t							maxLayers,
		size_t							ignoreLayer = (size_t)-1
	);

	void mergeLayers
	(
		EditorChunkTerrainPtr			ect,
		BW::vector<size_t>				const &layers
	);
}

BW_END_NAMESPACE

#endif // TERRAIN_TEXTURE_UTILS_HPP
