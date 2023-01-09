#include "pch.hpp"

#include "terrain_hole_functor.hpp"

#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/undo_redo/terrain_hole_map_undo.hpp"
#include "worldeditor/world/world_manager.hpp"

#include "chunk/geometry_mapping.hpp"

#include "gizmo/tool.hpp"
#include "gizmo/undoredo.hpp"

#include "romp/flora.hpp"

#include "terrain/terrain_hole_map.hpp"

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( TerrainHoleFunctor )

PY_BEGIN_METHODS( TerrainHoleFunctor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TerrainHoleFunctor )
	PY_ATTRIBUTE( fillNotCut )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( TerrainHoleFunctor, "TerrainHoleFunctor", Functor )

FUNCTOR_FACTORY( TerrainHoleFunctor )


/**
 *	Constructor.
 *
 *	@param pType		The type.
 */
/*explicit*/ TerrainHoleFunctor::TerrainHoleFunctor( PyTypeObject * pType ):
	TerrainFunctor( pType ),
	falloff_( 0.f ),
	fillNotCut_( false )
{
	undoName_ = LocaliseUTF8( L"GIZMO/UNDO/TERRAIN_HOLE" );
}


/** 
 *	This method updates the height pole hole functor.  If the left mouse button 
 *	is down, the functor is applied.
 *
 *	@param dTime	The change in time since the last frame.
 *	@param tool		The tool that we are using.
 */
void TerrainHoleFunctor::update( float dTime, Tool & tool )
{
	BW_GUARD;

	if (dragHandler().isDragging(MouseDragHandler::KEY_LEFTMOUSE) &&
		!WorldManager::instance().isMemoryLow( true /* test immediately */ ))
	{
		if (!applying_)
		{
			this->beginApply();
		}

		this->doApply( dTime, tool );
	}
	else
	{
		this->stopApplyCommitChanges( tool, true );
	}
}

void TerrainHoleFunctor::doApply( float dTime, Tool & tool )
{
	BW_GUARD;

	//per update calculations
	falloff_ = 2.0f / tool.size();

	// Doing all this offset shenanigans is necessary because although the 
	// user "points" at quads, the underlying mesh is still vertex based : 
	// the quad actually touched hangs off the corner of the vertex.		
	Matrix savedTransform = tool.locator()->transform();
	Matrix newTransform( savedTransform );		

	float gridSize = WorldManager::instance().geometryMapping()->pSpace()->gridSize();
	float size = float( gridSize ) / 
		float( WorldManager::instance().pTerrainSettings()->holeMapSize() );
	float offset = -size / 2.0f;
	newTransform._41 += offset;
	newTransform._43 += offset;

	tool.locator()->transform( newTransform );
	TerrainFunctor::doApply( tool );
	tool.locator()->transform( savedTransform );
}


void TerrainHoleFunctor::stopApplyCommitChanges( Tool& tool,
	bool addUndoBarrier )
{
	BW_GUARD;

	if (applying_)
	{
		TerrainFunctor::stopApplyCommitChanges( tool, addUndoBarrier );

		Flora::floraReset();
	}
}


void TerrainHoleFunctor::onBeginUsing( Tool & tool )
{
	BW_GUARD;
}


void TerrainHoleFunctor::onEndUsing( Tool & tool )
{
	BW_GUARD;
	this->stopApplying( tool, true );
}


/**
 *	This method is called before any of perheightpole class are made.
 *	It saves the current hole map in the undo/redo list
 *
 *  @param chunkTerrain	The terrain to get the format of.
 *	@param format		This is set to the format of the terrain.
 */
void TerrainHoleFunctor::getBlockFormat(
    const EditorChunkTerrain &		chunkTerrain,
    TerrainUtils::TerrainFormat &	format ) const
{
	BW_GUARD;

	const Terrain::TerrainHoleMap & holeMap = 
		chunkTerrain.block().holeMap();
	format.polesWidth	= holeMap.width();
	format.polesHeight	= holeMap.height();
	format.blockWidth	= holeMap.width();
	format.blockHeight	= holeMap.height();
	format.poleSpacingX	= holeMap.blockSize() / format.blockWidth;
	format.poleSpacingY	= holeMap.blockSize() / format.blockHeight;
}


/**
 *	This is called whenever a new terrain is touched by the tool, it can be 
 *	used to save the undo/redo buffer for example.
 *
 *  @param ect		The new terrain touched.
 */
void TerrainHoleFunctor::onFirstApply(EditorChunkTerrain &ect)
{
	BW_GUARD;

	UndoRedo::instance().add( new TerrainHoleMapUndo( &ect.block(), ect.chunk() ) );
}


/**
 *	This method cuts out or fills in a hole
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
void TerrainHoleFunctor::applyToSubBlock(
	EditorChunkTerrain &		chunkTerrain,
	const Vector3 &				/*toolOffset*/,
	const Vector3 &				/*chunkOffset*/,
	const TerrainUtils::TerrainFormat &/*format*/,
	int32						minx,
	int32						minz,
	int32						maxx,
	int32						maxz )
{
	BW_GUARD;

	if (WorldManager::instance().isMemoryLow( true /* test immediately */ ))
	{
		ERROR_MSG( "TerrainHoleFunctor: Memory is Low, "
			"failed to edit terrain holes on %s\n",
			chunkTerrain.block().resourceName().c_str() );
		return;
	}

	Terrain::TerrainHoleMap &holeMap = chunkTerrain.block().holeMap();
	Terrain::TerrainHoleMapHolder holder( &holeMap, false );
	Terrain::TerrainHoleMap::ImageType &image = holeMap.image();

	// These x & z might be outside of the terrain coordinate for this
	// chunk. Make sure they are in range.
	minx = Math::clamp< int32 >( 0, minx, minx );
	minz = Math::clamp< int32 >( 0, minz, minz );
	maxx = Math::clamp< int32 >( maxx, maxx, (int32)image.width() - 1 );
	maxz = Math::clamp< int32 >( maxz, maxz, (int32)image.height() - 1 );

	for (int32 z = minz; z <= maxz; ++z)
	{
		for (int32 x = minx; x <= maxx; ++x)
		{
			image.set( x, z, !fillNotCut_ );
		}
	}
}


/**
 *	This is called when the tool has finished being applied for one frame.
 *
 *	@param tool			The tool being applied.
 */
void TerrainHoleFunctor::onApplied( Tool & /*tool*/ )
{
}


/**
 *	This is for every chunk that the tool was applied to in the current frame.
 *
 *	@param chunkTerrain	A chunk that the tool was applied to.
 */
void TerrainHoleFunctor::onLastApply( EditorChunkTerrain & chunkTerrain )
{
}


/**
 *	This is the static Python factory method.
 *
 *  @param pArgs	The creation arguments.
 *	@return			A new TerrainHoleFunctor.
 */
PyObject * TerrainHoleFunctor::pyNew( PyObject * /*pArgs*/ )
{
	BW_GUARD;

	return new TerrainHoleFunctor();
}
BW_END_NAMESPACE

