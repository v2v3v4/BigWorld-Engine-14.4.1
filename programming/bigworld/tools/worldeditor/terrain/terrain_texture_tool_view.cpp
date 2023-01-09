#include "pch.hpp"
#include "worldeditor/terrain/terrain_texture_tool_view.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/terrain/editor_chunk_terrain_projector.hpp"


BW_BEGIN_NAMESPACE

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE TerrainTextureToolView::


PY_TYPEOBJECT( TerrainTextureToolView )

PY_BEGIN_METHODS( TerrainTextureToolView )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TerrainTextureToolView )
	PY_ATTRIBUTE( rotation )
	PY_ATTRIBUTE( showHoles )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( TerrainTextureToolView, "TerrainTextureToolView", View )

VIEW_FACTORY( TerrainTextureToolView )


/**
 *	Constructor.
 */
TerrainTextureToolView::TerrainTextureToolView(
		const BW::string & resourceID,
		PyTypeObject * 		pType ): 
	TextureToolView( resourceID, pType ),
	rotation_( 0.0f ),
 	showHoles_( false )
{
}


/**
 *	This method renders the terrain texture tool, by projecting the texture 
 *	onto the relevant terrain chunks as indicated in tool.relevantChunks().
 *
 *	@param	tool	The tool that we are viewing.
 */
void TerrainTextureToolView::render( Moo::DrawContext& drawContext,
									const Tool & tool )
{
	BW_GUARD;

	EditorChunkTerrainPtrs spChunks;

	ChunkPtrVector::const_iterator it  = tool.relevantChunks().begin();
	ChunkPtrVector::const_iterator end = tool.relevantChunks().end();

	while (it != end)
	{
		Chunk * pChunk = *it++;

		if (!pChunk->loaded())
		{
			continue;
		}
		
		EditorChunkTerrain * pChunkTerrain = 
			static_cast<EditorChunkTerrain*>(
				ChunkTerrainCache::instance( *pChunk ).pTerrain());

		if (pChunkTerrain != NULL)
		{
			spChunks.push_back( pChunkTerrain );
		}
	}

	Moo::rc().push();

	EditorChunkTerrainProjector::instance().projectTexture(
			pTexture_,
			tool.size(),
			rotation_,			
			tool.locator()->transform().applyToOrigin(),
			D3DTADDRESS_BORDER,
			spChunks,
			showHoles_ );

	Moo::rc().pop();
}


/**
 *	Static python factory method.
 *
 *	@param args		The creation arguments.
 *	@return			A new TerrainTextureToolView.
 */
PyObject * TerrainTextureToolView::pyNew( PyObject * args )
{
	BW_GUARD;

	char * pTextureName = NULL;
	if (!PyArg_ParseTuple( args, "|s", &pTextureName ))
	{
		PyErr_SetString( 
			PyExc_TypeError, 
			"View.TerrainTextureToolView: "
			"Argument parsing error: Expected an optional texture name" );
		return NULL;
	}

	if (pTextureName != NULL)
	{
		return new TerrainTextureToolView( pTextureName );
	}
	else
	{
		return new TerrainTextureToolView( "resources/maps/gizmo/disc.dds" );
	}
}
BW_END_NAMESPACE

