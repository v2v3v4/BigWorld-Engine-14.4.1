#include "pch.hpp"
#include "worldeditor/editor/item_editor.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/editor/chunk_editor.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/geometry_mapping.hpp"
#include "gizmo/item_view.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkItemEditor
// -----------------------------------------------------------------------------

// declare attributes deferred until chunk item was included
#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE ChunkItemEditor::

PY_RO_ATTRIBUTE_DECLARE( pItem_->edClassName().c_str(), className )
PY_RO_ATTRIBUTE_DECLARE( pItem_->edDescription().c_str(), description )
PyObject * ChunkItemEditor::pyGet_transform()
{
	BW_GUARD;

	Matrix m = pItem_->chunk()->transform();
	m.preMultiply( pItem_->edTransform() );
	return Script::getData( m );
}
PY_RO_ATTRIBUTE_SET( transform )

// and make the standard python stuff
PY_TYPEOBJECT( ChunkItemEditor )

PY_BEGIN_METHODS( ChunkItemEditor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ChunkItemEditor )
	PY_ATTRIBUTE( className )
	PY_ATTRIBUTE( description )
	PY_ATTRIBUTE( transform )
PY_END_ATTRIBUTES()

PY_FACTORY( ChunkItemEditor, WorldEditor )


/**
 *	Constructor.
 */
ChunkItemEditor::ChunkItemEditor( ChunkItemPtr pItem, PyTypeObject * pType ) :
	GeneralEditor( pType ),
	pItem_( pItem )
{
	BW_GUARD;

	// notify the item's name
	lastItemName_ = pItem_->edDescription();

	STATIC_LOCALISE_NAME( s_chunk, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/ITEM_EDITOR/CHUNK" );

	this->addProperty( new StaticTextProperty( s_chunk, 
		new ConstantChunkNameProxy<ChunkItem>( pItem_ ) ) );

	pItem->edEdit( *this );

	constructorOver_ = true;
}

/**
 *	Destructor.
 */
ChunkItemEditor::~ChunkItemEditor()
{
}

/**
 *	Static python factory method
 */
PyObject * ChunkItemEditor::pyNew( PyObject * args )
{
	BW_GUARD;

	// parse arguments
	PyObject * pPyRev = NULL;
	int alwaysChunkItem = 0;
	if (!PyArg_ParseTuple( args, "O|i", &pPyRev, &alwaysChunkItem ) ||
		!ChunkItemRevealer::Check( pPyRev ))
	{
		PyErr_SetString( PyExc_TypeError, "ChunkItemEditor() "
			"expects a Revealer argument and optionally alwaysChunkItem flag" );
		return NULL;
	}

	ChunkItemRevealer * pRevealer =
		static_cast<ChunkItemRevealer*>( pPyRev );

	/*
	// make sure there's only one item
	ChunkItemRevealer::ChunkItems items;
	pRevealer->reveal( items );
	if (items.size() != 1)
	{
		PyErr_Format( PyExc_ValueError, "ChunkItemEditor() "
			"Revealer must reveal exactly one item, not %d", items.size() );
		return NULL;
	}

	// make sure it hasn't already been 'deleted'
	ChunkItemPtr pItem = items[0];
	if (pItem->chunk() == NULL)
	{
		PyErr_Format( PyExc_ValueError, "ChunkItemEditor() "
			"Item must be in the scene" );
		return NULL;
	}

	// ok, that's it then
	return new ChunkItemEditor( pItem );
	*/


	ChunkItemRevealer::ChunkItems items;
	pRevealer->reveal( items );

	// Check items
	for (uint i = 0; i < items.size(); i++)
	{
		if (items[i]->chunk() == NULL)
		{
			PyErr_Format( PyExc_ValueError, "ChunkItemEditor() "
				"Item must be in the scene" );
			return NULL;
		}
	}

	if (items.size() == 1)
	{
		if (items[0]->isShellModel() && alwaysChunkItem == 0)
			return new ChunkEditor( items[0]->chunk() );
		else
			return new ChunkItemEditor( items[0] );
	}
	else
	{
		PyObject *t;
		t = PyTuple_New( items.size() );
		for (uint i = 0; i < items.size(); i++)
		{
			GeneralEditor* ge;

			if (items[i]->isShellModel() && alwaysChunkItem == 0)
				ge = new ChunkEditor( items[i]->chunk() );
			else
				ge = new ChunkItemEditor( items[i] );

			PyTuple_SetItem( t, i, ge );
		}
		return t;
	}
}

/*~ function WorldEditor.chunkItems
 *	@components{ worldeditor }
 *
 *	This function returns a Reavealer containing all items within
 *	the given chunk.
 *
 *	@param ChunkItemRevealer or Chunk ID (as a string) representing
 *			the desired chunk.
 *
 *	@return Returns a ChunkItemGroup object of the all the items in 
 *			the given chunk.
 */
static ChunkItemGroup* chunkItems( PyObjectPtr arg )
{
	if (!ChunkItemRevealer::Check( arg.get() ) && 
		!PyString_Check( arg.get() ))
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.chunkItems "
			"expects a Revealer or Chunk ID string argument" );
		return NULL;
	}

	Chunk* pChunk = NULL;

	if (ChunkItemRevealer::Check( arg.get() ))
	{
		ChunkItemRevealer * pRevealer =
			static_cast<ChunkItemRevealer*>( arg.get() );

		ChunkItemRevealer::ChunkItems items;
		pRevealer->reveal( items );
		if (items.size() != 1)
		{
			PyErr_Format( PyExc_ValueError, "WorldEditor.chunkItems "
				"Revealer must reveal exactly one item, not %d.", 
				items.size() );
			return NULL;
		}

		pChunk = items.front()->chunk();
		if (!pChunk || !pChunk->isBound())
		{
			PyErr_Format( PyExc_ValueError, "WorldEditor.chunkItems "
				"Revealed chunk is not valid." );
			return NULL;
		}
	}
	else if (PyString_Check( arg.get() ))
	{
		ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
		MF_ASSERT( pSpace != NULL );

		char *chunkID = PyString_AsString( arg.get() );

		pChunk = pSpace->findChunk( chunkID, 
			WorldManager::instance().geometryMapping()->name() );
		if (!pChunk)
		{
			PyErr_Format( PyExc_ValueError, "WorldEditor.chunkItems "
				"Chunk ID '%s' is not valid.", chunkID );
			return NULL;
		}

		if (!pChunk->loaded() || !pChunk->isBound())
		{
			PyErr_Format( PyExc_ValueError, "WorldEditor.chunkItems "
				"Chunk '%s' is not loaded and bound.", chunkID );
			return NULL;
		}
	}

	MF_ASSERT( pChunk != NULL );

	BW::vector<ChunkItemPtr> items;
	EditorChunkCache::instance( *pChunk ).allItems( items );
	return new ChunkItemGroup( items );
}

PY_AUTO_MODULE_FUNCTION( RETOWN, chunkItems, ARG( PyObjectPtr, END ), WorldEditor )

BW_END_NAMESPACE
// item_editor.cpp
