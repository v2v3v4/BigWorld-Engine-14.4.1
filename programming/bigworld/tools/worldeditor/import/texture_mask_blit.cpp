#include "pch.hpp"

#include "texture_mask_blit.hpp"

#include "worldeditor/import/import_image.hpp"
#include "worldeditor/import/terrain_utils.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/terrain/terrain_texture_utils.hpp"
#include "worldeditor/terrain/texture_mask_cache.hpp"
#include "worldeditor/undo_redo/terrain_texture_layer_undo.hpp"
#include "worldeditor/project/space_information.hpp"
#include "worldeditor/project/chunk_walker.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/misc/sync_mode.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_clean_flags.hpp"

#include "terrain/editor_base_terrain_block.hpp"
#include "terrain/terrain_texture_layer.hpp"
#include "terrain/terrain_settings.hpp"

#include "math/rectt.hpp"

BW_BEGIN_NAMESPACE

/*
 *	This set of routines does the following:
 *
 *	1.  Calculates the number of chunks affected by a texture blit.  This is 
 *		used to determine whether the blit is too large for undo/redo.
 *	2.	Saves the area underneath the blit for undo/redo.
 *	3.	Does the actual blit.
 *
 *	Each of these operations is done internally by the visitor pattern.  We
 *	iterate over the chunks and check whether they intersect the blit area.  If 
 *	it does then we either increment a counter (counting the affected chunks), 
 *	save the existing blends via an undo/redo transaction or blit the terrain.
 *
 *	The parameters for the blit, including the texture to place down, masking
 *	options, the area to blit etc. are passed via a MaskOptions object.  The 
 *	area to blit is measured using chunk-grid coordinates with (0, 0) being the
 *	outer corner of the lower-left chunk.  If the spaces is w chunks wide and h
 *	chunks high then the coordinate (w, h) is the outer corner of the 
 *	upper-right chunk.  Internally we convert these units to a 'giant' bitmap
 *	coordinate system by multiplying the coordinates by the number of blends 
 *	wide (for x values) or high (for z values).  For example, if a space is 
 *	15x10 in size with a blend resolution of 64x64 then the coordinate (0, 0)
 *	is the lower-left corner of the lower-left chunk, (63, 63) is the 
 *	upper-right corner of the lower-left chunk, (14*64, 9*64) is the lower-left
 *	of the upper-right chunk etc.
 *
 *	To iterate over the chunks we use a TerrainBlockInfo object.  This can be
 *	used to force unloaded chunks into memory.  Once the blit is done it is
 *	used to either keep it in memory (for smaller spaces) or write it to disk 
 *	(larger spaces).
 *
 *	When doing the actual blitting (see ImportVisitor::visit).  We do the 
 *	following at each chunk:
 *
 *	1.	We search for an existing layer that matches the layer we are currently
 *		blitting.
 *	2.	If the layer does not yet exist then we try to add it.  For the newer
 *		terrain this should not fail.  For older terrain this can fail if there
 *		are already 4 layers.
 *	3.	If adding the new layer fails on the older terrain then we find the 
 *		weakest layer to replace it.
 *	4.	We calculate the mask for the chunk.  At each point this determines a
 *		mask value (0 - 255) where 0 means don't paint here at all and 255 
 *		means paint at full strength.  At each point the mask also determines a
 *		maximum value (0 - 255).  The maximum value helps prevent accumulation
 *		in fuzzy regions (e.g. the height fuzzy region) quickly losing their
 *		fuzziness as well as the case where we are replacing one texture with
 *		another (in which case we cannot paint with more strength than what
 *		we are replacing).  See the file texture_mask_cache.cpp for more
 *		details.
 *	5.	We then iterate over all of the blends in the chunk that intersect the
 *		blitting area.
 *
 *	At each point in the blends the sum of the blend weights is 255.  The
 *	texture blending re-weights the blends, but the sum still has to be 255.
 *	To prevent accumulation errors we quit often convert the 8 bit values to
 *	32 bit fixed point arithmetic, do the sums and then convert back to 8 bit
 *	values.  This can also help prevent overflow where the sum, due to rounding
 *	goes to 256 (which rounds to zero if 8 bit arithmetic was used).  The fixed
 *	point arithmetic has to be careful with rounding when converting to 8 bit.
 *	This is done by adding 0.5 in the fixed point representation (see the 
 *	constant HALF_STRENGTH below) before the right-shift operation.
 *
 *	At each point we do the following:
 *
 *	1.	We increase the value of the layer we are painting taking into account
 *		the mask value and the mask maximum value.
 *	2.	If the value was set to 255, which can happen a lot, then the other
 *		layers must be zero.  If the value was not 255 then we re-weight the
 *		other layers as in the following steps.
 *	3.	If we are replacing one or more texture layers with the new layer then
 *		we need to remove some or all of their contributions.  To do this we
 *		find the sum of their weights, subtract the amount that the painted 
 *		layer increased and re-weight their contributions so that they sum to
 *		this new sum and so that their relative contributions remains the same.
 *	4.	Finally we renormalise the remaining layers so that the sum of all 
 *		layers is 255 and so that their relative contributions remains the 
 *		same.
 *
 *	Once all blend values have been calculated we remove empty layers (which
 *	can happen if we completely replace one or more texture layers), rebuild
 *	the DirectX textures, rebuild the LOD textures.
 *
 *	The last thing that happens to the chunk is to either write the terrain 
 *	to disk for larger spaces, or mark the chunk as dirty for smaller spaces.
 */


namespace
{
// These values are used in fixed-point inside the painting calculations:
static const uint32 TEX_MASK_BLIT_STRENGTH_EXP	= 20;
static const uint32 TEX_MASK_BLIT_FULL_STRENGTH	= 1 << TEX_MASK_BLIT_STRENGTH_EXP;
static const uint32 TEX_MASK_BLIT_HALF_STRENGTH	= 1 << (TEX_MASK_BLIT_STRENGTH_EXP - 1);


/******************************************************************************
 *	Section - TerrainBlockInfo
 *****************************************************************************/

/**
 *	This is used to track information about a terrain block between calls
 *	of getting and setting the block.
 */
class TerrainBlockInfo
{
public:
	TerrainBlockInfo();

	Chunk *		chunk() const;
	void		chunk( Chunk *c );

	bool		forceSave() const;
	void		forceSave( bool force );

	Terrain::EditorBaseTerrainBlockPtr getTerrainBlock(
				const BW::string & chunkName,
				bool forceSave );	

	bool		setTerrainBlock(
				Terrain::EditorBaseTerrainBlock	* pTerrainBlock );

private:
	Chunk *		pChunk_;		// chunk for the terrain block
	bool		inMemory_;		// was the chunk already in memory?
	bool		forceSave_;		// force the chunk to be savedf
};


/**
 *	This is the TerrainBlockInfo default constructor.
 */
TerrainBlockInfo::TerrainBlockInfo():
	pChunk_( NULL ),
	inMemory_( false ),
	forceSave_( false )
{
}


/**
 *	This gets the chunk that the TerrainBlockInfo represents.
 *
 *	@return				The chunk that the TerrainBlockInfo represents.
 */
Chunk * TerrainBlockInfo::chunk() const
{ 
	return pChunk_; 
}


/**
 *	This sets the chunk that the TerrainBlockInfo represents.
 *
 *	@param c			The chunk that the TerrainBlockInfo represents.
 */
void TerrainBlockInfo::chunk( Chunk *c ) 
{ 
	pChunk_ = c; 
}


/**
 *	This gets whether the chunk should be saved.
 *
 *	@return				True if the chunk should be saved and unloaded, false 
 *						if it should remain in memory.
 */
bool TerrainBlockInfo::forceSave() const
{
	return forceSave_;
}


/**
 *	This sets whether the chunk should be saved.
 *
 *	@param force		If true then the chunk will be marked as needing saving
 *						when set, if false then the chunk won't be saved.
 */
void TerrainBlockInfo::forceSave( bool force )
{
	forceSave_ = force;
}


/** 
 *	This function gets an EditorBaseTerrainBlock for the given chunk.
 *
 *  @param chunkName	The name of the chunk to load.
 *  @param forceSave	Force the chunks to memory and save.
 *	@return				The terrain block for the given chunk.
 */
Terrain::EditorBaseTerrainBlockPtr TerrainBlockInfo::getTerrainBlock(	
	const BW::string &	chunkName,		
	bool				forceSave )
{
	BW_GUARD;

	Chunk *pChunk = 
		ChunkManager::instance().findChunkByName(
			chunkName,
			WorldManager::instance().geometryMapping() );

	MF_ASSERT( pChunk != NULL );
	if (pChunk == NULL)
	{
		return NULL;
	}

	pChunk_		= pChunk;
	inMemory_	= pChunk->loaded();
	forceSave_	= forceSave;

	if (inMemory_)
	{
		// Get the terrain:
		ChunkTerrain *pChunkTerrain = 
			ChunkTerrainCache::instance( *pChunk ).pTerrain();
		if (!pChunkTerrain)
		{
			return NULL;
		}
		return static_cast< Terrain::EditorBaseTerrainBlock * >(
												pChunkTerrain->block().get() );
	}
	else if (!forceSave)
	{
		ChunkManager::instance().loadChunkNow(
			chunkName,
			WorldManager::instance().geometryMapping() );	
		ChunkManager::instance().checkLoadingChunks();

		ChunkTerrain *pChunkTerrain = 
	        ChunkTerrainCache::instance( *pChunk ).pTerrain();

		if (pChunkTerrain == NULL)
		{
			if (pChunk->removable())
			{
				pChunk->unbind( false );
				pChunk->unload();
			}
			return NULL;
		}

		return static_cast< Terrain::EditorBaseTerrainBlock * >(
												pChunkTerrain->block().get() );
	}
	else
	{
		return static_cast< Terrain::EditorBaseTerrainBlock * >(
					ChunkTerrain::loadTerrainBlockFromChunk( pChunk ).get() );
	}
}

/**
 *	This function should be called after a call to getTerrainBlock when the
 *	EditorBaseTerrainBlock has been changed.
 *
 *  @param pTerrainBlock	The EditorBaseTerrainBlock.
 *  @return					True upon success, false on failure.
 */
bool TerrainBlockInfo::setTerrainBlock(
	Terrain::EditorBaseTerrainBlock	*	pTerrainBlock )
{
	BW_GUARD;

	bool ok = true;
	if (pChunk_ == NULL)
	{
		return false;
	}
	if (forceSave_)
	{
		BW::string filename = pTerrainBlock->resourceName();
		// Strip off the data section name:
		BW::string::size_type pos = filename.find_last_of( "/\\" );
		if (pos != BW::string::npos)
		{
			filename = filename.substr( 0, pos );
		}			
		DataSectionPtr pCDataSection = BWResource::openSection( filename );
		if (pCDataSection)
		{
			ChunkCleanFlags flags( pCDataSection );

			flags.thumbnail_ = 0;
			flags.terrainLOD_ = !pTerrainBlock->isLodTextureDirty();
			flags.save();

			pCDataSection->deleteSection( "thumbnail.dds" );
			ok = pTerrainBlock->saveToCData( pCDataSection );
		}
		else
		{
			ok = false;
		}
	}

	if (pChunk_->isBound())
	{
		// Let WorldEditor know about the changed chunk, but don't ask that 
		// the navmesh be regenerated.
		WorldManager::instance().changedTerrainBlock( pChunk_, false ); 
	}
	return ok;
}


/******************************************************************************
 *	Section - ChunkVisitor
 *****************************************************************************/

/**
 *	This is an abstract class that is called when we visit a new chunk.
 */
class ChunkVisitor
{
public:
	ChunkVisitor();
	virtual ~ChunkVisitor();

	/**
	 *	This is called whenever a chunk is visited.
	 *
	 *	@param chunkName	The name of the chunk.
	 *	@param irect		The area to import in this chunk inside the giant
	 *						bitmap coordinate system.
	 *	@param trect		The coordinates of the chunk inside the giant 
	 *						bitmap coordinate system.
	 *	@param polesWide	The width of the terrain blend maps.
	 *	@param polesHigh	The height of the terrain blend maps.
	 */
	virtual bool visit(
		const BW::string &	chunkName,
		const BW::RectInt &	irect,
		const BW::RectInt &	trect,	
		uint32				polesWide,
		uint32				polesHigh ) = 0;

private:
	ChunkVisitor( const ChunkVisitor & );
	ChunkVisitor &operator=( const ChunkVisitor & );
};


/**
 *	This is the ChunkVisitor constructor.
 */
ChunkVisitor::ChunkVisitor()
{
}


/**
 *	This is the ChunkVisitor destructor.
 */
/*virtual*/ ChunkVisitor::~ChunkVisitor()
{
}


/******************************************************************************
 *	Section - CountVisitor
 *****************************************************************************/


/**
 *	This visitor counts the affected chunks of an import for undo size.
 */
class CountVisitor : public ChunkVisitor
{
public:
	CountVisitor();

	/*virtual*/ bool visit(
		const BW::string &	chunkName,
		const BW::RectInt &	irect,
		const BW::RectInt &	trect,
		uint32				polesWide,
		uint32				polesHigh );

	size_t count() const;

private:
	size_t					count_;
};


/**
 *	This is the CountVisitor constructor.
 */
CountVisitor::CountVisitor():
	ChunkVisitor(),
	count_( 0 )
{
}


/* 
 *	This is called whenever we visit a new chunk with the CountVisitor.  It 
 *	just increments the count by one.
 *
 *  Overrides ChunkVisitor::visit.
 */
bool CountVisitor::visit(
	const BW::string &	/*chunkName*/,
	const BW::RectInt &	/*irect*/,
	const BW::RectInt &	/*trect*/,
	uint32				/*polesWide*/,
	uint32				/*polesHigh*/ )
{
	++count_;
	return true;
}


/**
 *	This gets the number of chunks the visitor has been called on.
 *
 *  @return				The number of chunks the visitor has been called
 *						on.
 */
size_t CountVisitor::count() const
{
	return count_;
}


/******************************************************************************
 *	Section - UndoVisitor
 *****************************************************************************/


/**
 *	This visitor saves the terrain blend state of all affected chunks.
 */
class UndoVisitor : public ChunkVisitor
{
public:
	UndoVisitor();

	/*virtual*/ bool visit(
		const BW::string &	chunkName,
		const BW::RectInt &	irect,
		const BW::RectInt &	trect,
		uint32				polesWide,
		uint32				polesHigh );

	bool foundOne() const;

private:
	bool hasFoundOne_;
};


/**
 *	This is the UndoVisitor constructor.
 */
UndoVisitor::UndoVisitor():
	ChunkVisitor(),
	hasFoundOne_( false )
{
}


/* 
 *	This is called on every affected chunk to save it's texture layer 
 *	state.
 *
 *  Overrides ChunkVisitor::visit.
 */
/*virtual*/ bool UndoVisitor::visit(
	const BW::string &	chunkName,
	const BW::RectInt &	/*irect*/,
	const BW::RectInt &	/*trect*/,
	uint32				/*polesWide*/,
	uint32				/*polesHigh*/ )
{
	BW_GUARD;

	TerrainBlockInfo info;
	Terrain::EditorBaseTerrainBlockPtr pTerrainBlock = 
		info.getTerrainBlock( chunkName, false );
	bool thisHasOne = (pTerrainBlock != NULL) && (info.chunk() != NULL);
	if (thisHasOne)
	{
		hasFoundOne_ = true;
		TerrainTextureLayerUndo *pUndoOp = 
			new TerrainTextureLayerUndo( pTerrainBlock, info.chunk() );
		UndoRedo::instance().add( pUndoOp );
	}

	return thisHasOne;
}


/**
 *	This returns whether at least one undo/redo operation was added.
 *
 *	@return		True if at least one undo/redo operation was added, false if
 *				none were added.
 */
bool UndoVisitor::foundOne() const
{
	return hasFoundOne_;
}


/******************************************************************************
 *	Section - ImportVisitor
 *****************************************************************************/


/**
 *	This visitor does the texture mask import on a single chunk.
 */
class ImportVisitor : public ChunkVisitor
{
public:
	ImportVisitor(
		TerrainPaintBrushPtr	pBrush,
		const BW::string &		texture,
		const BW::string &		bumpTexture,
		const Vector4 &			uProj,
		const Vector4 &			vProj,
		bool					forceSave );

	/*virtual*/ bool visit(
		const BW::string &		chunkName,
		const BW::RectInt &		irect,
		const BW::RectInt &		trect,
		uint32					polesWide,
		uint32					polesHigh );

private:
	BW::string					texture_;
	BW::string					bumpTexture_;
	Vector4						uProj_;
	Vector4						vProj_;
	bool						forceSave_;
};


/**
 *	This is the ImportVisitor constructor.
 *
 *	@param pBrush		The terrain paint brush.
 *  @param texture		The texture being applied.
 *  @param uProj		The u-projection coordinates.
 *  @param vProj		The v-projection coordinates.
 *  @param forceSave	Force chunks to be saved
 */
ImportVisitor::ImportVisitor(
	TerrainPaintBrushPtr  	pBrush,
	const BW::string &		texture,
	const BW::string &		bumpTexture,
	const Vector4 &			uProj,
	const Vector4 &			vProj,
	bool					forceSave ):
	ChunkVisitor(),
	texture_( texture ),
	bumpTexture_( bumpTexture ),
	uProj_( uProj ),
	vProj_( vProj ),
	forceSave_( forceSave )
{
	BW_GUARD;

	// See the texture mask cache:
	TextureMaskCache::instance().paintBrush( pBrush );
}


/* 
 *	This is called on every affected chunk to do the texture mask import.
 *
 *   Overrides ChunkVisitor::visit.
 */
/*virtual*/ bool ImportVisitor::visit(
	const BW::string &	chunkName,
	const BW::RectInt &	irect,
	const BW::RectInt &	trect,
	uint32				polesWide,
	uint32				polesHigh )
{
	BW_GUARD;

	TerrainBlockInfo					info;

	Terrain::EditorBaseTerrainBlockPtr pTerrainBlock =
								info.getTerrainBlock( chunkName, forceSave_ );

	if (!pTerrainBlock)
	{
		return false;
	}

	// See if there is already a layer for the texture:
	bool newLayer = false;
	size_t layer = 
		pTerrainBlock->findLayer(
			texture_, bumpTexture_,
			uProj_, vProj_, 
			TerrainTextureUtils::PROJECTION_COMPARISON_EPSILON);

	// Add a new layer if necessary:
	if (layer == Terrain::EditorBaseTerrainBlock::INVALID_LAYER)
	{
		layer = pTerrainBlock->insertTextureLayer();

		// May fail with simple terrain, so choose the weakest layer:
		if (layer == Terrain::EditorBaseTerrainBlock::INVALID_LAYER)
		{
			layer = pTerrainBlock->findWeakestLayer();
			if (layer == Terrain::EditorBaseTerrainBlock::INVALID_LAYER)
			{
				MF_ASSERT(false);
				return false;
			}
		}
		newLayer = true;
	}

	size_t numLayers = pTerrainBlock->numberTextureLayers();

	// Pointers to each layer:
	BW::vector<uint8 *> pLayers( numLayers );

	// Lock all of the layers:
	for (size_t i = 0; i < numLayers; ++i)
	{
		pTerrainBlock->textureLayer( i ).lock( false );
	}

	Terrain::TerrainTextureLayer &ttl = pTerrainBlock->textureLayer( layer );

	// Set the texture and projections if the layer is new:
	if (newLayer)
	{					
		ttl.textureName( texture_ );
		ttl.bumpTextureName( bumpTexture_ );
		if (ttl.hasUVProjections())
		{
			ttl.uProjection( uProj_ );
			ttl.vProjection( vProj_ );
		}
		Terrain::TerrainTextureLayer::ImageType &img = ttl.image();
		img.fill( 0 );
	}

	// Destination coordinates for this layer:
	int dleft   = Math::lerp( irect.xMin_, trect.xMin_, trect.xMax_, 0, (int)polesWide - 1 );
	int dtop    = Math::lerp( irect.yMin_, trect.yMax_, trect.yMin_, 0, (int)polesHigh - 1 );
	int dright  = Math::lerp( irect.xMax_, trect.xMin_, trect.xMax_, 0, (int)polesWide - 1 );
	int dbottom = Math::lerp( irect.yMax_, trect.yMax_, trect.yMin_, 0, (int)polesHigh - 1 );                 

	MaskResult const *pMaskResult = 
							TextureMaskCache::instance().mask( pTerrainBlock );
	MaskImage const *pMask = &pMaskResult->image_;

	// Blit the mask into this layer:
	for (int dy = dbottom; dy <= dtop; ++dy)
	{
		for (size_t i = 0; i < numLayers; ++i)
		{
			pLayers[ i ] = 
				pTerrainBlock->textureLayer( i ).image().getRow( dy ) + dleft;
		}
		for (int dx = dleft; dx <= dright; ++dx)
		{
			// Calculate the value for the painted layer
			uint8 startValue = *pLayers[ layer ];
			MaskPixel mv = pMask->get( dx, dy );
			uint8 value = 
				(uint8)std::min( (uint32)(startValue + mv.fuzziness_), 
					(uint32)mv.maxValue_ );
			value = std::max( value, startValue );
			*pLayers[ layer ] = value;				

			// Normalise all of the other layers:
			uint8 remainder = 255 - value;
			if (remainder == 0)
			{
				// If the remainder is zero, which can happen a lot, then the 
				// other layers at this pixel must be zero:
				for (size_t i = 0; i < numLayers; ++i)
				{
					if ( i != layer )
					{
						*pLayers[ i ] = 0;
					}
				}
			}
			else
			{
				// Find the other contributions by other texture
				// mask layers.
				BW::vector<bool> const &layerMask = 
					pMaskResult->maskTexLayer_;
				uint32 sumMask = 0;
				for (size_t i = 0; i < numLayers; ++i)
				{
					if (layerMask[ i ] && i != layer)
					{
						sumMask += *pLayers[ i ];
					}
				}

				// Remove the new contribution to the destination pixel
				// from the other layers in the mask:
				int sumDestAndMask = value;
				if (sumMask != 0)
				{
					uint8 incr       = value - startValue;
                    uint32 maskRatio = (incr << TEX_MASK_BLIT_STRENGTH_EXP)/sumMask;
                    maskRatio = std::min( maskRatio, TEX_MASK_BLIT_FULL_STRENGTH );

					for (size_t i = 0; i < numLayers; ++i)
					{									
						if (layerMask[ i ] && i != layer)
						{
							uint8 *pPixel = pLayers[ i ];
							uint8 subValue = 
                                (uint8)((*pPixel*maskRatio + TEX_MASK_BLIT_HALF_STRENGTH)
                                    >> TEX_MASK_BLIT_STRENGTH_EXP);
							*pPixel -= subValue;
							sumDestAndMask += *pPixel;
						}
					}
				}

				// Normalise the remaining layers not yet considered.
				uint32 sumOther = 0;
				for (size_t i = 0; i < numLayers; ++i)
				{
					if (!layerMask[ i ] && i != layer)
					{
						sumOther += *pLayers[ i ];
					}
				}
				if (sumOther != 0)
				{
					sumDestAndMask = std::min( sumDestAndMask, 255 );
                    int ratio = ((255 - sumDestAndMask) << TEX_MASK_BLIT_STRENGTH_EXP)
						/sumOther;
					for (size_t i = 0; i < numLayers; ++i)
					{
						if (!layerMask[ i ] && i != layer)
						{
							uint8 *pPixel = pLayers[ i ];
                            *pPixel = (uint8)((*pPixel*ratio + TEX_MASK_BLIT_HALF_STRENGTH)
                                >> TEX_MASK_BLIT_STRENGTH_EXP);
						}
					}
				}
			}

			// Increment the layer pointers:
			for (size_t i = 0; i < numLayers; ++i)
			{
				++pLayers[ i ];
			}
		}
	}

	// Unlock all of the layers:
	for (size_t i = 0; i < numLayers; ++i)
	{
		pTerrainBlock->textureLayer( i ).unlock();
	}

	// Cleanup any 'empty' layers:
	TerrainTextureUtils::deleteEmptyLayers(
		*pTerrainBlock, 
		TerrainTextureUtils::REMOVE_LAYER_THRESHOLD	);

	// Clear the texture mask cache.  We don't re-use the mask values and
	// we don't want the cache to become too large:
	TextureMaskCache::instance().clear();

	// Update layers, dominant texture map, etc:
	pTerrainBlock->rebuildCombinedLayers();

	// Get the chunk and rebuilt the texture LOD:
	Chunk *pChunk =
		ChunkManager::instance().findChunkByName(
			chunkName,
			WorldManager::instance().geometryMapping() );
	MF_ASSERT( pChunk );
	pTerrainBlock->rebuildLodTexture( pChunk->transform() );

	// Set the terrain (possibly saves)
	return info.setTerrainBlock( pTerrainBlock.get() );
};


/******************************************************************************
 *	Section - visitChunks
 *****************************************************************************/


/**
 *	This visits affected chunks of a texture mask import and applies the 
 *	provided visitor.
 *
 *  @param leftf		The left world coordinate.
 *  @param topf			The top world coordinate.
 *  @param rightf		The right world coordinate.
 *  @param bottomf		The bottom world coordinate.
 *  @param description	The progress description.
 *  @param showProgress	Show a progress indicator?
 *  @param visitor		The visitor to apply to each affected chunk.
 */
bool visitChunks(
	float				leftf,
	float				topf,
	float				rightf,
	float				bottomf,
	const BW::string &	description,
	bool				showProgress,
	ChunkVisitor		&visitor )
{
	BW_GUARD;

	// Stop loading chunks and halt WE's background processing:
	SyncMode modifyTerrain;
	if (!modifyTerrain)
	{
		return false;
	}

	// Get the space and its size:
	BW::string space = WorldManager::instance().getCurrentSpace();
	uint16 gridWidth, gridHeight;
	GridCoord localToWorld( 0, 0 );
	float gridSize;
	if (!TerrainUtils::spaceSettings( space, gridSize, gridWidth, gridHeight, localToWorld) )
	{
		return false;
	}

	// An iterator over the space:
	LinearChunkWalker lcw;
	lcw.spaceInformation(
		SpaceInformation( space, localToWorld, gridWidth, gridHeight ) );

	Terrain::TerrainSettingsPtr pTerrainSettings = 
		WorldManager::instance().pTerrainSettings();

	const uint32 polesWide = pTerrainSettings->blendMapSize();
	const uint32 polesHigh = pTerrainSettings->blendMapSize();

	// Pretend the texture layer is one giant bitmap of size 
	// widthPixels x heightPixels:
	unsigned int widthPixels  = gridWidth *polesWide;
	unsigned int heightPixels = gridHeight*polesHigh;

	// Convert the selection from metres into the giant bitmap coordinates:
	int left   = (int)((leftf  /gridSize + gridWidth /2)/gridWidth *(widthPixels  - 1));
	int top    = (int)((topf   /gridSize + gridHeight/2)/gridHeight*(heightPixels - 1));
	int right  = (int)((rightf /gridSize + gridWidth /2)/gridWidth * widthPixels);
	int bottom = (int)((bottomf/gridSize + gridHeight/2)/gridHeight* heightPixels);

	// Handle some degenerate cases:
	if (left == right || top == bottom)
	{
		return false;
	}

	// Normalise the coordinates:
	if (left > right ) 
	{
		std::swap(left, right);
	}
	if (top  < bottom) 
	{
		std::swap(top , bottom);
	}

	// Setup the progress indicator:
	ProgressBarTask        *pImportTask = NULL;
	if (showProgress)
	{
		pImportTask = 
			new ProgressBarTask( 
				description, 
				float(gridWidth*gridHeight), false );
	}

	BW::string chunkName;
	int16 gridX, gridZ;
	while (lcw.nextTile( chunkName, gridX, gridZ ))
	{
		if (pImportTask != NULL)
		{
			pImportTask->step();
		}

		if (TerrainUtils::isEditable( gridX, gridZ ))
		{
			// The coordinates of this chunk in the giant bitmap system:
			int tileLeft   = (gridX - localToWorld.x)*polesWide;
			int tileRight  = tileLeft + polesWide;
			int tileBottom = (gridZ - localToWorld.y)*polesHigh;
			int tileTop    = tileBottom + polesHigh;

			// Only process chunks that intersect the selection:
			int ileft, itop, iright, ibottom;
			if 
			(
				TerrainUtils::rectanglesIntersect(
					tileLeft, tileTop, tileRight, tileBottom,
					left    , top    , right    , bottom    ,
					ileft   , itop   , iright   , ibottom )
			)
			{
				BW::RectInt irect( ileft   , itop   , iright   , ibottom    );
				BW::RectInt trect( tileLeft, tileTop, tileRight, tileBottom );
				visitor.visit(
					chunkName, 
					irect, trect, 
					polesWide, polesHigh );
			}
		}
	}

	// Cleanup the progress indicator:
	bw_safe_delete(pImportTask);

	return true;
}


} // anonymous namespace


/******************************************************************************
 *	Section - functions exposed in the TextureMaskBlit namespace
 *****************************************************************************/


/**
 *	This function counts the number of chunks affected if a terrain texture
 *	import was done.  This can be used to determine whether an undo is 
 *	possible.
 *
 *	@param pBrush		The terrain paint brush.
 *	@return				The number of chunks affected.
 */
size_t TextureMaskBlit::numChunksAffected( TerrainPaintBrushPtr pBrush )
{
	BW_GUARD;

	CountVisitor countVisitor;
	visitChunks(
		pBrush->importMaskTL_.x, pBrush->importMaskTL_.y, 
		pBrush->importMaskBR_.x, pBrush->importMaskBR_.y, 
		"",			// No progress text
		false,		// No progress bar
		countVisitor );
	return countVisitor.count();
}


/**
 *	This saves the texture layers of all chunks affected by a terrain texture
 *	layer mask import.
 *
 *	@param options		The mask options.
 *	@param barrierDesc	The description of the undo/redo operation.
 *  @param progressDesc The description of the progress.
 *  @return				True if an undo/redo was possible.
 */
bool TextureMaskBlit::saveUndoForImport(
    TerrainPaintBrushPtr	pBrush,
    const BW::string &		barrierDesc,
	const BW::string &		progressDesc,
    bool					showProgress )
{
	BW_GUARD;

	UndoVisitor undoVisitor;
	visitChunks(
		pBrush->importMaskTL_.x, pBrush->importMaskTL_.y, 
		pBrush->importMaskBR_.x, pBrush->importMaskBR_.y, 
		progressDesc, 
		showProgress, 
		undoVisitor );
	if (undoVisitor.foundOne()) 
	{
		UndoRedo::instance().barrier( barrierDesc, false );
		return true;
	}
	else
	{
		return false;
	}
}


/**
 *	This function does a terrain texture mask import.
 *
 *	@param pBrush		The brush used to paint the terrain
 *	@param texture		The name of the texture file.
 *  @param uProj		The u-projection of the texture layer.
 *  @Param vProj		The v-projection of the texture layer.
 *  @param showProgress	Show progress while importing?
 *  @param progDesc		Progress description.
 *  @param forceSave	Force saving of affected chunks?
 *	@return				True if the import was successfull, false otherwise.
 */
bool
TextureMaskBlit::import(
	TerrainPaintBrushPtr	pBrush,
	const BW::string &		texture,
	const BW::string &		bumpTexture,
	const Vector4 &			uProj,
	const Vector4 &			vProj,
	bool					showProgress,
	const BW::string &		progDesc,
	bool					forceSave )
{
	BW_GUARD;

	ImportVisitor importVisitor( pBrush, texture, bumpTexture, uProj, vProj, forceSave );
	visitChunks(
		pBrush->importMaskTL_.x, pBrush->importMaskTL_.y, 
		pBrush->importMaskBR_.x, pBrush->importMaskBR_.y,  
		progDesc, 
		showProgress, 
		importVisitor );
	return true;
}

BW_END_NAMESPACE

