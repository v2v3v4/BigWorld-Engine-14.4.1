#include "pch.hpp"
#include "worldeditor/terrain/terrain_texture_utils.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/undo_redo/terrain_texture_layer_undo.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "terrain/terrain_texture_layer.hpp"
#include "romp/flora.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	/**
	 *	This function checks to see whether a terrain texture is almost zero,
	 *	and hence ready to be removed.
	 *
	 *  @param image		The terrain texture layer to check.
	 *  @param threshold	The threshold pixel value.
	 *  @returns			True if all pixels are below the threshold.
	 */
	bool 
    isAlmostZero
    (
        Terrain::TerrainTextureLayer::ImageType             const &image,
        Terrain::TerrainTextureLayer::ImageType::PixelType  threshold
    )
    {
		BW_GUARD;

        typedef Terrain::TerrainTextureLayer::ImageType ImageType;
        typedef ImageType::PixelType					PixelType;

        if (image.isEmpty())
            return true;

        for (uint32 h = 0; h < image.height(); ++h)
        {
            PixelType const *p = image.getRow(h);
            PixelType const *q = p + image.width();
            for (; p != q; ++p)
            {
                if (*p > threshold)
                    return false;
            }
        }

        return true;
    }
}


/**
 *	This is the LayerInfo default constructor.
 */
TerrainTextureUtils::LayerInfo::LayerInfo():
	terrain_(NULL),
	layerIdx_((size_t)-1),
	strength_(0)
{
}


/**
 *	This is used to sort by strength and/or LayerSet insertion.
 *
 *  @param layerInfo1		The first LayerInfo.
 *	@param layerInfo2		The second LayerInfo.
 *	@returns				True if the first LayerInfo is less than the 
 *							second.
 */
bool TerrainTextureUtils::operator<
(
	LayerInfo				const layerInfo1, 
	LayerInfo				const layerInfo2
)
{
	BW_GUARD;

	if (layerInfo1.strength_ != layerInfo2.strength_)
		return layerInfo1.strength_ > layerInfo2.strength_;
	else if (layerInfo1.terrain_ != layerInfo2.terrain_)
		return layerInfo1.terrain_ > layerInfo2.terrain_;
	else
		return layerInfo1.layerIdx_ > layerInfo2.layerIdx_;
}


/**
 *	This does a flood fill starting with the given terrain block and layer,
 *	and finds all connecting terrains with the same terrain layer.
 *
 *  @param layer		The layer to search for.
 *  @param layers		This is filled with connecting terrains with the
 *						same texture.
 *	@returns			An error code signifying if the find was completely
 *						successful or failed because it went into a locked/
 *						unloaded area.
 */
TerrainTextureUtils::FindError 
TerrainTextureUtils::findNeighboursWithTexture
(
	LayerInfo			const &layer,
	LayerSet			&layers
)
{
	BW_GUARD;

	Terrain::EditorBaseTerrainBlock &block = layer.terrain_->block();
	if (layer.layerIdx_ >= block.numberTextureLayers())
		return TerrainTextureUtils::FIND_NO_SUCH_LAYER;
	Terrain::TerrainTextureLayer &ttl = block.textureLayer(layer.layerIdx_);
	return 
		findNeighboursWithTexture
		(
			layer.terrain_,
			ttl.textureName(),
			ttl.bumpTextureName(),
			ttl.hasUVProjections() ? ttl.uProjection() : Vector4::zero(),
			ttl.hasUVProjections() ? ttl.vProjection() : Vector4::zero(),
			layers
		);
}


/**
 *	This does a flood fill starting with the given terrain and finds all
 *	connecting terrains with the same terrain alpha texture as given by the
 *	texture in the index.
 *
 *  @param seed			The initial terrain to start with.
 *	@param texture		The texture to search for.
 *	@param uProj		The u-projection of the alpha blend.
 *	@param vProj		The v-projection of the alpha blend.
 *  @param layers		This is filled with connecting terrains with the
 *						same texture.
 *	@returns			An error code signifying if the find was completely
 *						successful or failed because it went into a locked/
 *						unloaded area.
 */
TerrainTextureUtils::FindError 
TerrainTextureUtils::findNeighboursWithTexture
(
	EditorChunkTerrainPtr	seed,
	BW::string				const &texture,
	BW::string				const &bumpTexture,
	Vector4					const &uProj,
	Vector4					const &vProj,
	LayerSet				&layers
) 
{
	BW_GUARD;

	TerrainTextureUtils::FindError result = FIND_OK;

	if (seed == NULL)
		return result;

	size_t layerIdx = 
		seed->block().findLayer
		(
			texture, 
			bumpTexture,
			uProj, vProj, 
			PROJECTION_COMPARISON_EPSILON
		);
	if (layerIdx != (size_t)-1)
	{
		LayerInfo layerInfo;
		layerInfo.terrain_	= seed;
		layerInfo.layerIdx_	= layerIdx;

		if (layers.find(layerInfo) == layers.end())
		{
			layers.insert(layerInfo);

			// Recurse over all neighbours that have the same texture and which 
			// are not yet in the terrain set.
			for 
			(
				uint32 i = EditorChunkTerrain::FIRST_NEIGHBOUR; 
				i != EditorChunkTerrain::LAST_NEIGHBOUR;
				++i
			)
			{
				EditorChunkTerrain::NeighbourError error = 
					EditorChunkTerrain::NEIGHBOUR_OK;
				EditorChunkTerrain *neighbour = 
					seed->neighbour
					(
						(EditorChunkTerrain::Neighbour)i, 
						false,
						true,
						&error
					);
				if (neighbour == NULL && error != EditorChunkTerrain::NEIGHBOUR_OK)
				{
					if (error == EditorChunkTerrain::NEIGHBOUR_NOTLOADED) 
						result = (FindError)(result | FIND_EXTENDS_INTO_UNLOADED);
					if (error == EditorChunkTerrain::NEIGHBOUR_LOCKED)
						result = (FindError)(result | FIND_EXTENDS_INTO_LOCKED);
				}
				if (neighbour != NULL)
				{
					result = 
						(FindError)
						(
							result 
							|
							findNeighboursWithTexture
							(
								neighbour, 
								texture, 
								bumpTexture,
								uProj, vProj, 
								layers
							)
						);
				}
			}
		}
	}
	return result;
}


/**
 *	This function takes the result of the find error and prints an error
 *	message.
 *
 *  @param findError		The error returned by findNeighboursWithTexture.
 */
void TerrainTextureUtils::printErrorMessage(FindError findError)
{
	BW_GUARD;

	if ((findError & TerrainTextureUtils::FIND_EXTENDS_INTO_LOCKED) != 0)
		ERROR_MSG(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/LAYER_NOT_WRITABLE").c_str());
	if ((findError & TerrainTextureUtils::FIND_EXTENDS_INTO_UNLOADED) != 0)
		ERROR_MSG(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/LAYER_NOT_LOADED").c_str());
}


/**
 *	This function sets the projection of a LayerSet.
 *
 *  @param layers			The layers to modify.
 *  @param uProj			The new u-projection.
 *  @param vProj			The new v-projection.
 *	@param temporary		If true then the change is only temporary, don't
 *							recalcuate 
 */
void TerrainTextureUtils::setProjections
(
	LayerSet				const &layers,
	Vector4					const &uProj,
	Vector4					const &vProj,
	bool					temporary
)
{
	BW_GUARD;

	for (LayerSetConstIter it = layers.begin(); it != layers.end(); ++it)
	{
		EditorChunkTerrainPtr		 ect		= it->terrain_;
		size_t						 layerIdx	= it->layerIdx_;
		Terrain::TerrainTextureLayer &layer		= ect->block().textureLayer(layerIdx);

		if (layer.hasUVProjections())
		{
			layer.uProjection(uProj);
			layer.vProjection(vProj);
			ect->block().dirtyLodTexture();
			if (!temporary)
				WorldManager::instance().chunkTexturesPainted(ect->chunk(), false); // let WorldEditor do the LOD textures			
			WorldManager::instance().changedTerrainBlock(ect->chunk(), false);
		}
	}
}


/**
 *	This finds the dominant terrain texture at the given location.
 *
 *  @param position			The position to look at.
 *  @param idx				This is set to the index of the layer.
 *	@param ect				This is set to the EditorChunkTerrain under 
 *							position.
 *	@param chunk			This is set to the chunk under the position.
 *  @param strength			If this is not NULL then it will be set to the
 *							strength of the dominant texture.
 */
bool TerrainTextureUtils::dominantTexture
(
	Vector3					const &position,
	size_t					*idx,
	EditorChunkTerrain		**ect,
	Chunk					**chunk,
	uint8					*strength
)
{
	BW_GUARD;

	*chunk = 
		ChunkManager::instance().cameraSpace()->findChunkFromPoint(position);		 
	if (chunk != NULL)
	{		
		*ect = 
			static_cast<EditorChunkTerrain*>
			(
				ChunkTerrainCache::instance(**chunk).pTerrain()
			);
		if (*ect != NULL)
		{
			Vector3 offs = position - (*chunk)->transform().applyToOrigin();
			*idx = (*ect)->dominantBlend(offs.x, offs.z, strength);
			if (*idx != EditorChunkTerrain::NO_DOMINANT_BLEND)
				return true;
		}
	}

	*idx	= EditorChunkTerrain::NO_DOMINANT_BLEND;
	*chunk	= NULL;
	*ect	= NULL;
	return false;
}


/**
 *	This function fills out information about texture layers at the 
 *	given position.
 */
bool TerrainTextureUtils::layerInfo
(
	Vector3							const &position,
	LayerVec						&infoVec
)
{
	BW_GUARD;

	Chunk *chunk = 
		ChunkManager::instance().cameraSpace()->findChunkFromPoint(position);		 
	if (chunk != NULL)
	{		
		EditorChunkTerrain *ect = 
			static_cast<EditorChunkTerrain*>
			(
				ChunkTerrainCache::instance(*chunk).pTerrain()
			);
		if (ect != NULL)
		{
			const float gridSize = ChunkManager::instance().cameraSpace()->gridSize();
			Vector3 offs = position - chunk->transform().applyToOrigin();
			Terrain::EditorBaseTerrainBlock const &b = ect->block();
			for (size_t i = 0; i < b.numberTextureLayers(); ++i)
			{
				Terrain::TerrainTextureLayer const &ttl = b.textureLayer(i);
				Terrain::TerrainTextureLayerHolder 
					holder(const_cast<Terrain::TerrainTextureLayer *>(&ttl), true);
				Terrain::TerrainTextureLayer::ImageType const &image = ttl.image();
				int sx = (int)(offs.x*image.width ()/gridSize + 0.5f);
				int sz = (int)(offs.z*image.height()/gridSize + 0.5f);
				uint8 v = image.get(sx, sz);
				LayerInfo info;
				info.terrain_		= ect;
				info.layerIdx_		= i;
				info.strength_		= v;
				info.textureName_	= ttl.textureName();
				info.bumpTextureName_	= ttl.bumpTextureName();
				infoVec.push_back(info);
			}
		}
	}
	return false;
}


/**
 *	This function returns true if the given texture can be replaced by
 *	another.
 *
 *	@param idx				The index of the layer to replace.
 *	@param ect				The chunk terrain to replace.
 *	@param textureName		The texture to replace with.
 *  @param uProjection		The uProjection of the new layer.
 *  @param vProjection		The vProjection of the new layer.
 */
bool TerrainTextureUtils::canReplaceTexture
(
	size_t							idx,
	EditorChunkTerrainPtr			ect,
	BW::string						const &textureName,
	BW::string						const &bumpTexturName,
	Vector4							const &uProjection,
	Vector4							const &vProjection
)
{
	BW_GUARD;

    Terrain::EditorBaseTerrainBlock &block = ect->block();
    Terrain::TerrainTextureLayer &ttl = block.textureLayer(idx);
	bool sameTexture = 
		ttl.sameTexture
		(
			textureName, 
			bumpTexturName,
			uProjection, vProjection,  
			TerrainTextureUtils::PROJECTION_COMPARISON_EPSILON
		);
	return !sameTexture;
}


/**
 *	This replaces one texture layer with another.  The algorithm does a 
 *	floodfill for neighbouring chunks that have the same texture layer and
 *	changes them too.
 *
 *	@param idx				The index of the layer to replace.
 *	@param ect				The chunk terrain to replace.
 *	@param textureName		The texture to replace with.
 *  @param uProjection		The uProjection of the new layer.
 *  @param vProjection		The vProjection of the new layer.
 *	@param undoable			If true then undo operations are added.
 *  @param undoBarrier		If true then an undo barrier is created.
 *	@param doFloodFill		If true then do a flood fill to replace 
 *							neighbouring chunks that have the same texture.  If
 *							false 
 *	@param regenLODs		If true then texture LODs are regenerated.  If 
 *							false then they are marked as needing regeneration.
 *	@returns				True if something was changed.
 */
bool TerrainTextureUtils::replaceTexture
(
	size_t					idx,
	EditorChunkTerrainPtr	ect,
	BW::string				const &textureName,
	BW::string				const &bumpTextureName,
	Vector4					const &uProjection,
	Vector4					const &vProjection,
	bool					undoable			/*= true*/,
	bool					undoBarrier			/*= true*/,
	bool					doFloodFill			/*= true*/,
	bool					regenLODs			/*= true*/
)
{
	BW_GUARD;

	bool changedOne = false;

    Terrain::EditorBaseTerrainBlock &block = ect->block();
    Terrain::TerrainTextureLayer &ttl = block.textureLayer(idx);
    if (canReplaceTexture(idx, ect, textureName, bumpTextureName, uProjection, vProjection))
    {
		BW::string oldTextureName = ttl.textureName();
		BW::string oldBumpTextureName = ttl.bumpTextureName();
		Vector4     uProj;
		Vector4		vProj;
		if ( ttl.hasUVProjections() )
		{
			uProj = ttl.uProjection();
			vProj = ttl.vProjection();
		}
		BW::set<TerrainTextureUtils::LayerInfo> neighbours;
		TerrainTextureUtils::FindError findError = FIND_OK;
		if (doFloodFill)
		{
			findError =
				TerrainTextureUtils::findNeighboursWithTexture
				(
					ect, 
					oldTextureName, 
					oldBumpTextureName,
					uProj, vProj,
					neighbours
				);	
		}
		else
		{
			TerrainTextureUtils::LayerInfo layerInfo;
			layerInfo.terrain_	= ect;
			layerInfo.layerIdx_	= idx;
			layerInfo.strength_	= 0;
			neighbours.insert(layerInfo);
		}
		TerrainTextureUtils::printErrorMessage(findError);
		for 
		(
			BW::set<TerrainTextureUtils::LayerInfo>::iterator it = neighbours.begin();
			it != neighbours.end();
			++it
		)
		{
			EditorChunkTerrainPtr			thisECT		= it->terrain_;
			size_t							layerIdx	= it->layerIdx_;
			Terrain::EditorBaseTerrainBlock &thisBlock	= thisECT->block();					

			if (undoable)
			{
				UndoRedo::instance().add
				( 
					new TerrainTextureLayerUndo
					(
						&thisBlock, 
						thisECT->chunk()
					) 
				);
			}
			Terrain::TerrainTextureLayer &thisLayer = 
				thisBlock.textureLayer(layerIdx);
			// Copy the blends for this layer, reset the texture and copy
			// the blends back:						
			{
				Terrain::TerrainTextureLayerHolder holder(&thisLayer, false); 
				Terrain::TerrainTextureLayer::ImageType oldBlends = thisLayer.image();
				thisLayer.textureName(textureName); // this resets the blends to 0
				thisLayer.bumpTextureName(bumpTextureName);
				thisLayer.image().blit(oldBlends); // restore the old blends
				if (thisLayer.hasUVProjections())
				{
					thisLayer.uProjection(uProjection);
					thisLayer.vProjection(vProjection);
				}
			}
			thisBlock.optimiseLayers
			(
				TerrainTextureUtils::PROJECTION_COMPARISON_EPSILON
			);
			thisBlock.rebuildCombinedLayers(true, true);
			bool rebuiltTextureLOD = false;
			if (regenLODs)
			{
				rebuiltTextureLOD =
					thisBlock.rebuildLodTexture( thisECT->chunk()->transform() );
			}
			WorldManager::instance().changedTerrainBlock(thisECT->chunk(), false);
			WorldManager::instance().chunkTexturesPainted(thisECT->chunk(), rebuiltTextureLOD);
			changedOne = true;
		}
		if (changedOne)
		{
			if (undoable && undoBarrier)
				UndoRedo::instance().barrier("Terrain Texture Replace", true);
			Flora::floraReset();
		}
	}

	return changedOne;
}


/**
 *	This function removes all layers whose blends lie below the given 
 *	threshold.
 *
 *	@param ect			The block whose layers should be cleaned up.
 *	@param threshold	Any layers whose blend values are below the threshold
 *						get removed.
 */
void TerrainTextureUtils::deleteEmptyLayers
(
	Terrain::EditorBaseTerrainBlock	&block,
	uint8							threshold
)
{
	BW_GUARD;

	// Lock all of the textures:
    for (size_t i = 0; i < block.numberTextureLayers(); ++i)
    {
        block.textureLayer(i).lock(true);
    }   

    // Get a list of empty blends:
    BW::vector<size_t> emptyIdxs;
	int firstNonEmptyLayer = -1;
    for (size_t i = 0; i < block.numberTextureLayers(); ++i)
    {
        if (isAlmostZero(block.textureLayer(i).image(), threshold))
		{
            emptyIdxs.push_back(i);    
		}
		else if (firstNonEmptyLayer == -1)
		{
			firstNonEmptyLayer = static_cast<int>(i);
		}
	}

	size_t numEmptyLayers = emptyIdxs.size();
	if (firstNonEmptyLayer >= 0 && numEmptyLayers > 0)
	{
		// If we are going to remove layers, normalise the sum for each column of
		// pixels by adding the removed values to the first non-zero layer.
		// NOTE: This code assumes the layers were normalised beforehand.
		BW::vector< uint8 * > pEmptyLayers( numEmptyLayers );
		int width = block.textureLayer( 0 ).width();
		int height = block.textureLayer( 0 ).height();
		uint8 * pNonEmptyLayer = NULL;

		for (int y = 0; y < height; ++y)
		{
			for (size_t i = 0; i < numEmptyLayers; ++i)
			{
				pEmptyLayers[ i ] = block.textureLayer( emptyIdxs[ i ] ).image().getRow( y );
			}
			pNonEmptyLayer = block.textureLayer( firstNonEmptyLayer ).image().getRow( y );

			for (int x = 0; x < width; ++x)
			{
				uint32 sumOther = 0;
				for (size_t i = 0; i < numEmptyLayers; ++i)
				{
					sumOther += *pEmptyLayers[ i ];
				}
				if (sumOther != 0)
				{
					*pNonEmptyLayer += sumOther;
				}

				for (size_t i = 0; i < numEmptyLayers; ++i)
				{
					++pEmptyLayers[ i ];
				}
				++pNonEmptyLayer;
			}
		}
	}

    // Unlock all of the textures:
    for (size_t i = 0; i < block.numberTextureLayers(); ++i)
    {
        block.textureLayer(i).unlock();
    }    

    // Remove the empty layers:
    // (using reverse order of emptyIdxs so that indices remain valid)
    for
    (
        BW::vector<size_t>::reverse_iterator it = emptyIdxs.rbegin();
        it != emptyIdxs.rend();
        ++it
    )
    {
        block.removeTextureLayer(*it);
    }
}


/**
 *	This function finds the layer with the most contribution to the chunk,
 *	possibly ignoring a particular layer (e.g. the layer about to be deleted).
 *
 *	@param block			The block to search.
 *	@param ignoreLayer		This layer will not be included in the result.  If
 *							this is negative or greater than the number of 
 *							layers then this parameter is ignored.
 *	@returns				The layer with the most contribution and -1 if no
 *							such layer exists.
 */
int TerrainTextureUtils::mostPopulousLayer
(
	Terrain::EditorBaseTerrainBlock	const &block,
	int								ignoreLayer
)
{
	BW_GUARD;

	BW::vector<size_t> layers;
	layers.push_back(ignoreLayer);
	return mostPopulousLayer(block, layers);
}


/**
 *	This function finds the layer with the most contribution to the chunk,
 *	ignoring the indicated layers.
 *
 *	@param block			The block to search.
 *	@param ignoreLayers		These layer will not be included in the result.
 *	@returns				The layer with the most contribution and -1 if no
 *							such layer exists.
 */
int TerrainTextureUtils::mostPopulousLayer
(
	Terrain::EditorBaseTerrainBlock	const &block,
	BW::vector<size_t>				const &ignoreLayers
)
{
	BW_GUARD;

	int result = -1;

	uint64 largestSum = 0;

	// Find the layer contributions:
    for (size_t i = 0; i < block.numberTextureLayers(); ++i)
    {        
		bool ignore = false;
		for (size_t j = 0; j < ignoreLayers.size() && !ignore; ++j)
		{
			if (ignoreLayers[j] == i)
				ignore = true;
		}
		if (!ignore)
		{
			Terrain::TerrainTextureLayer &layer = 
				const_cast<Terrain::TerrainTextureLayer &>(block.textureLayer(i));

			layer.lock(true);

			uint64 layerSum = 0;
			
			Terrain::TerrainTextureLayer::ImageType &image = layer.image();
			for (uint32 y = 0; y < image.height(); ++y)
			{
				uint8 const *p = image.getRow(y);
				uint8 const *q = p + image.width();
				for (; p != q; ++p)
					layerSum += *p;
			}
			
			layer.unlock();

			if (layerSum > largestSum)
			{
				largestSum = layerSum;
				result = static_cast<int>(i);
			}
		}        
    }   

	return result;
}


/**
 *	This function finds the layer with the least contribution to the chunk,
 *	possibly ignoring a particular layer (e.g. the layer about to be deleted).
 *
 *	@param block			The block to search.
 *	@param ignoreLayer		This layer will not be included in the result.  If
 *							this is negative or greater than the number of 
 *							layers then this parameter is ignored.
 *	@returns				The layer with the most contribution and -1 if no
 *							such layer exists.
 */
int TerrainTextureUtils::leastPopulousLayer
(
	Terrain::EditorBaseTerrainBlock	const &block,
	int								ignoreLayer
)
{
	BW_GUARD;

	BW::vector<size_t> layers;
	layers.push_back(ignoreLayer);
	return leastPopulousLayer(block, layers);
}


/**
 *	This function finds the layer with the least contribution to the chunk,
 *	ignoring the indicated layers.
 *
 *	@param block			The block to search.
 *	@param ignoreLayers		These layer will not be included in the result.
 *	@returns				The layer with the most contribution and -1 if no
 *							such layer exists.
 */
int TerrainTextureUtils::leastPopulousLayer
(
	Terrain::EditorBaseTerrainBlock	const &block,
	BW::vector<size_t>				const &ignoreLayers
)
{
	BW_GUARD;

	int result = -1;

	uint64 leastSum = std::numeric_limits<uint64>::max();

	// Find the layer contributions:
    for (size_t i = 0; i < block.numberTextureLayers(); ++i)
    {        
		bool ignore = false;
		for (size_t j = 0; j < ignoreLayers.size() && !ignore; ++j)
		{
			if (ignoreLayers[j] == i)
				ignore = true;
		}
		if (!ignore)
		{
			Terrain::TerrainTextureLayer &layer = 
				const_cast<Terrain::TerrainTextureLayer &>(block.textureLayer(i));

			layer.lock(true);

			uint64 layerSum = 0;
			
			Terrain::TerrainTextureLayer::ImageType &image = layer.image();
			for (uint32 y = 0; y < image.height(); ++y)
			{
				uint8 const *p = image.getRow(y);
				uint8 const *q = p + image.width();
				for (; p != q; ++p)
					layerSum += *p;
			}
			
			layer.unlock();

			if (layerSum < leastSum)
			{
				leastSum = layerSum;
				result = static_cast<int>(i);
			}
		}        
    }   

	return result;
}


/**
 *	This function finds the strengths of the layers by summing up the 
 *	contribution made to each layer.
 *
 *	@param block			The block to search.
 *	@param distribution		This is filled with the distribution of each
 *							layer in a one-to-one fashion.
 */
void TerrainTextureUtils::layersDistributions
(
	Terrain::EditorBaseTerrainBlock	const &block,
	BW::vector<uint64>				&distribution
)
{
	BW_GUARD;

	distribution.clear();
	distribution.reserve(block.numberTextureLayers());

	// Find the layer contributions:
    for (size_t i = 0; i < block.numberTextureLayers(); ++i)
    {        
		Terrain::TerrainTextureLayer &layer = 
			const_cast<Terrain::TerrainTextureLayer &>(block.textureLayer(i));

		layer.lock(true);

		uint64 layerSum = 0;
		
		Terrain::TerrainTextureLayer::ImageType &image = layer.image();
		for (uint32 y = 0; y < image.height(); ++y)
		{
			uint8 const *p = image.getRow(y);
			uint8 const *q = p + image.width();
			for (; p != q; ++p)
				layerSum += *p;
		}
		
		layer.unlock();     

		distribution.push_back(layerSum);
    }   
}


namespace
{
	// This is used to sort layers by their contribution size
	struct SizeAndIdx
	{
		uint64	contribution_;
		size_t	index_;
	};

	bool operator<(SizeAndIdx const &si1, SizeAndIdx const &si2)
	{
		return si1.contribution_ < si2.contribution_;
	}
}


/**
 *	This function limits the number of layers in a block by merging the
 *	least populous layers to the most populous layer if necessary.
 *
 *	@param ect				The terrain to limit the layers of.
 *	@param maxLayers		The maximum number of layers allowed.  This has to
 *							be strictly a positive number.
 *	@param ignoreLayer		If this is not -1 then this can be used to 
 *							preserve a particular layer.
 */
void TerrainTextureUtils::limitLayers
(
	EditorChunkTerrainPtr		ect,
	size_t						maxLayers,
	size_t						ignoreLayer /*= (size_t)-1*/
)
{
	BW_GUARD;

	Terrain::EditorBaseTerrainBlock	&block = ect->block();
	size_t numLayers = block.numberTextureLayers();

	// Handle degenerate input:
	if (maxLayers == 0 || maxLayers >= numLayers)
		return;

	// Find the contributions of each layer:
	BW::vector<uint64> distributions;
	layersDistributions(block, distributions);

	// Sort the contributions from least to largest:
	BW::vector<SizeAndIdx> sizesAndIdxs(numLayers);
	for (size_t i = 0; i < numLayers; ++i)
	{
		sizesAndIdxs[i].contribution_	= distributions[i];
		sizesAndIdxs[i].index_			= i;
	}
	std::sort(sizesAndIdxs.begin(), sizesAndIdxs.end());

	// Merge the least layers with the largest layer:
	BW::vector<size_t> mergeIdxs;
	// Add least populous layers (skip over ignoreLayer if it exists):
	for (size_t i = 0; mergeIdxs.size() < numLayers - maxLayers; ++i)
	{
		if (sizesAndIdxs[i].index_ != ignoreLayer)
			mergeIdxs.push_back(sizesAndIdxs[i].index_);
	}
	// Add the most populous layer:
	mergeIdxs.push_back(sizesAndIdxs[sizesAndIdxs.size() - 1].index_);

	// Merge them:
	mergeLayers(ect, mergeIdxs);
}



/**
 *	This merges all the given layers into one.  The layer to merge to is the
 *	last layer in the layers array.
 *
 *	@param ect				The chunk terrain whose layers are to be merged.
 *	@param layers			The layers to merge.  The layer to merge to is the
 *							last layer in the array.
 */
void TerrainTextureUtils::mergeLayers
(
	EditorChunkTerrainPtr			ect,
	BW::vector<size_t>				const &layers
)
{
	BW_GUARD;

	// Handle the case where there is nothing to do.
	if (ect == NULL || layers.size() <= 1)
		return;

	Terrain::EditorBaseTerrainBlock &block = ect->block();

	size_t mergedLayer = layers[layers.size() - 1];
	size_t numLayers   = block.numberTextureLayers();

	// Lock the layers:
    for (size_t i = 0; i < numLayers; ++i)
		block.textureLayer(i).lock(false);

	BW::vector<bool> mergedLayers(numLayers, false);
	for (size_t i = 0; i < layers.size() - 1; ++i)
		mergedLayers[layers[i]] = true;

	BW::vector<uint8 *>layerPtrs(numLayers);

	// This assumes layers are the same size:
	uint32 width  = block.textureLayer(0).width();
	uint32 height = block.textureLayer(0).height();
	for (uint32 y = 0; y < height; ++y)
	{
		for (size_t i = 0; i < numLayers; ++i)
			layerPtrs[i] = block.textureLayer(i).image().getRow(y);
		for (uint32 x = 0; x < width; ++x)
		{
			uint32 mergedValue = *layerPtrs[mergedLayer];
			for (size_t i = 0; i < numLayers; ++i)
			{
				if (mergedLayers[i])
				{
					uint8 val = *layerPtrs[i];
					*layerPtrs[i] = 0;
					mergedValue += val;
				}
			}
			if (mergedValue > 255) mergedValue = 255;
			*layerPtrs[mergedLayer] = (uint8)mergedValue;
			for (size_t i = 0; i < numLayers; ++i)
				++layerPtrs[i];
		}
	}

	// Unlock the layers:
    for (size_t i = 0; i < numLayers; ++i)
		block.textureLayer(i).unlock();

	// Delete the empty layers and optimise them:
	deleteEmptyLayers(block, 0);

	// Rebuild the layers, the LODs and let WorldEditor know about the changes:
	block.rebuildCombinedLayers(true, true);
	block.rebuildLodTexture(ect->chunk()->transform());
	WorldManager::instance().changedTerrainBlock(ect->chunk(), false);
	WorldManager::instance().chunkTexturesPainted(ect->chunk(), true);
}
BW_END_NAMESPACE

