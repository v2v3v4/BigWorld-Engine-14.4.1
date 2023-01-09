#include "pch.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/terrain/terrain_set_height_functor.hpp"
#include "worldeditor/undo_redo/terrain_height_map_undo.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "gizmo/tool.hpp"
#include "gizmo/undoredo.hpp"
#include "romp/flora.hpp"

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( TerrainSetHeightFunctor )

PY_BEGIN_METHODS( TerrainSetHeightFunctor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TerrainSetHeightFunctor )
	PY_ATTRIBUTE( height )
	PY_ATTRIBUTE( relative )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( TerrainSetHeightFunctor, "TerrainSetHeightFunctor", Functor )

FUNCTOR_FACTORY( TerrainSetHeightFunctor )


/*explicit*/ TerrainSetHeightFunctor::TerrainSetHeightFunctor( PyTypeObject * pType ):
	TerrainFunctor( pType ),
	height_( 0.f )
{
	undoName_ = LocaliseUTF8( L"GIZMO/UNDO/TERRAIN_SET_HEIGHT" );
}


/** 
 *	This method updates the height pole height functor.
 *	if the left mouse button is down, the filter will be applied
 *
 *	@param dTime	The change in time since the last frame.
 *	@param tool		The tool we are using.
 */
void TerrainSetHeightFunctor::update( float dTime, Tool& tool )
{
	BW_GUARD;

	if (dragHandler().isDragging(MouseDragHandler::KEY_LEFTMOUSE) &&
		!WorldManager::instance().isMemoryLow( true /* test immediately */ ))
	{
		this->doApply( tool );
	}
	else
	{
		this->stopApplying( tool, true );
	}
}

void TerrainSetHeightFunctor::doApply( Tool& tool, bool allVerts )
{
	BW_GUARD;

	if (!applying_)
	{
		this->beginApply();
	}

	TerrainFunctor::doApply( tool );
}

void TerrainSetHeightFunctor::stopApplyCommitChanges( Tool& tool,
	bool addUndoBarrier )
{
	BW_GUARD;

	if (applying_)
	{
		// turn off functor
		TerrainFunctor::stopApplyCommitChanges( tool, addUndoBarrier );

		Flora::floraReset();
	}
}


void TerrainSetHeightFunctor::onBeginUsing( Tool & tool )
{
	BW_GUARD;
}


void TerrainSetHeightFunctor::onEndUsing( Tool & tool )
{
	BW_GUARD;
	this->stopApplying( tool, true );
}


/**
 *	This method is called to get the format used by the functor.
 *
 *	@param chunkTerrain	The EditorChunkTerrain whose format is requested.
 *	@param format	This is filled out with the format of the height-map.
 */
void TerrainSetHeightFunctor::getBlockFormat(
    const EditorChunkTerrain &		chunkTerrain,
    TerrainUtils::TerrainFormat &	format ) const
{	
	BW_GUARD;

	// lock for reading and writing
	Terrain::TerrainHeightMap const &heightMap = 
		chunkTerrain.block().heightMap();

	format.polesWidth	= heightMap.polesWidth();
	format.polesHeight	= heightMap.polesHeight();
	format.blockWidth	= heightMap.blocksWidth();
	format.blockHeight	= heightMap.blocksHeight();
	format.poleSpacingX = heightMap.spacingX();
	format.poleSpacingY = heightMap.spacingZ();
}


/**
 *	This is called whenever a new terrain is touched by the tool, it can be 
 *	used to save the undo/redo buffer for example.
 *
 *  @param chunkTerrain	The new terrain touched.
 */
void TerrainSetHeightFunctor::onFirstApply( EditorChunkTerrain & chunkTerrain )
{
	BW_GUARD;

	WorldManager::instance().lockChunkForEditing( chunkTerrain.chunk(), true );
	UndoRedo::instance().add( 
		new TerrainHeightMapUndo( &chunkTerrain.block(), chunkTerrain.chunk() ) );
	TerrainUtils::TerrainFormat format;
	this->getBlockFormat( chunkTerrain, format );
	VisitedImagePtr visited = 
		new VisitedImage(
			format.polesWidth, 
			format.polesHeight );
	visited->fill( 0 );
	poles_[ &chunkTerrain.block() ] = visited;
}


/**
 *	This method is called to apply the functor to the given region.
 *
 *	@param	chunkTerrain The terrain that is being considered.
 *	@param	toolOffset	The offset of the tool.
 *	@param	chunkOffset	The offset of the chunk.
 *  @param  format		The format of the area being edited.
 *  @param  minx		The minimum x coord of the area of the block to change.
 *  @param  minz		The minimum z coord of the area of the block to change.
 *  @param  maxx		The maximum x coord of the area of the block to change.
 *  @param  maxz		The maximum z coord of the area of the block to change.
 */
void TerrainSetHeightFunctor::applyToSubBlock(
    EditorChunkTerrain &			chunkTerrain,
	const Vector3 &					toolOffset,
	const Vector3 &					/*chunkOffset*/,
	const TerrainUtils::TerrainFormat & /*format*/,
	int32							minx,
	int32							minz,
	int32							maxx,
	int32							maxz
)
{
	BW_GUARD;

	if (WorldManager::instance().isMemoryLow( true /* test immediately */ ))
	{
		ERROR_MSG( "TerrainSetHeightFunctor: Memory is Low, "
			"failed to set terrain heights on %s\n",
			chunkTerrain.block().resourceName().c_str() );
		return;
	}

	Terrain::TerrainHeightMap &		heightMap	= chunkTerrain.block().heightMap();
	Terrain::TerrainHeightMapHolder holder(&heightMap, false);
	VisitedImagePtr					visited		= poles_[ &chunkTerrain.block() ];

	for (int32 z = minz; z <= maxz; ++z)
	{
		for (int32 x = minx; x <= maxx; ++x)
		{
			if (relative_)
			{
				if (visited->get( x, z ) == 0)
				{
					visited->set( x, z, 1 );
					Terrain::TerrainHeightMap::Iterator pole = 
						heightMap.iterator( x, z );
					pole.set( pole.get() + height_ );
				}
			}
			else
			{
				Terrain::TerrainHeightMap::Iterator pole = 
					heightMap.iterator( x, z );
				pole.set( height_ );
			}
		}
	}
}


/**
 *	This method is called every frame after all the relevant chunks have
 *	been processed.
 *
 *	@param	t	The tool we are using.
 */
void TerrainSetHeightFunctor::onApplied( Tool & t )
{
	BW_GUARD;

	//Temporarily increase the tools area of influence so that
	//height poles at the edges of chunks are updated correctly.
	t.findRelevantChunks( 4.0f );
	
	ChunkPtrVector::iterator it  = t.relevantChunks().begin();
	ChunkPtrVector::iterator end = t.relevantChunks().end();

	while (it != end)
	{
		Chunk * pChunk = *it++;

		EditorChunkTerrain * pEditorChunkTerrain = 
			static_cast< EditorChunkTerrain * >(
				ChunkTerrainCache::instance( *pChunk ).pTerrain());

		if (pEditorChunkTerrain)
		{
			pEditorChunkTerrain->onEditHeights();
			if ( this->newTouchedChunk( pEditorChunkTerrain ) )
			{
				this->onFirstApply( *pEditorChunkTerrain );
			}
		}
	}

	t.findRelevantChunks();
}


/**
 *	This method is called on every chunk processed after the user has finished
 *	applying the tool (e.g. on left mouse button up).
 *
 *	@param chunkTerrain	An EditorChunkTerrain that processed during the apply. 
 */
void TerrainSetHeightFunctor::onLastApply( EditorChunkTerrain & chunkTerrain )
{
	BW_GUARD;

	chunkTerrain.block().rebuildNormalMap( Terrain::NMQ_NICE );
	chunkTerrain.toss( chunkTerrain.chunk() );
	WorldManager::instance().lockChunkForEditing( chunkTerrain.chunk(), false );
	poles_.clear();
}


/**
 *	This is the static Python factory method.
 *
 *	@return			A new TerrainSetHeightFunctor.
 */
PyObject * TerrainSetHeightFunctor::pyNew( PyObject * args )
{
	BW_GUARD;

	return new TerrainSetHeightFunctor();
}
BW_END_NAMESPACE

