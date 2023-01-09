#include "pch.hpp"
#include "item_view.hpp"
#include "ashes/simple_gui.hpp"
#include "ashes/text_gui_component.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_item.hpp"
#include "chunk/chunk_vlo.hpp"
#include "moo/geometrics.hpp"
#include "gizmo/general_properties.hpp"
#include "moo/draw_context.hpp"


// -----------------------------------------------------------------------------
// Section: ChunkItemRevealer
// -----------------------------------------------------------------------------

BW_BEGIN_NAMESPACE

static PySequenceMethods ChunkItemRevealer_seqfns =
{
	ChunkItemRevealer::_pySeq_length,	/* sq_length */
	ChunkItemRevealer::_pySeq_concat,	/* sq_concat */
	0,									/* sq_repeat */
	ChunkItemRevealer::_pySeq_item,		/* sq_item */
	ChunkItemRevealer::_pySeq_slice,	/* sq_slice */
	0,									/* sq_ass_item */
	0,									/* sq_ass_slice */
	ChunkItemRevealer::_pySeq_contains,	/* sq_contains */
	0,									/* sq_inplace_concat */
	0,									/* sq_inplace_repeat */
};

PY_TYPEOBJECT_WITH_SEQUENCE( ChunkItemRevealer, &ChunkItemRevealer_seqfns )

PY_BEGIN_METHODS( ChunkItemRevealer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ChunkItemRevealer )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( ChunkItemRevealer )


/**
 *	Python sequence length method
 */
Py_ssize_t ChunkItemRevealer::pySeq_length()
{
	BW_GUARD;

	ChunkItems items;
	this->reveal( items );
	return items.size();
}

/**
 *	Python sequence concat method
 */
PyObject * ChunkItemRevealer::pySeq_concat( PyObject * pOther )
{
	BW_GUARD;

	if (!ChunkItemRevealer::Check( pOther ))
	{
		PyErr_SetString( PyExc_TypeError, "ChunkItemRevealer: "
			"Argument to + must be a ChunkItemRevealer" );
		return NULL;
	}

	ChunkItems ourItems;
	this->reveal( ourItems );
	ChunkItems newItems;
	((ChunkItemRevealer*)pOther)->reveal( newItems );

	ChunkItems items = ourItems;
	items.insert( items.end(), newItems.begin(), newItems.end() );
	return new ChunkItemGroup( items );
}


/**
 *	Python sequence index method
 */
PyObject * ChunkItemRevealer::pySeq_item( Py_ssize_t index )
{
	BW_GUARD;

	ChunkItems items;
	this->reveal( items );
	if (index < 0 || items.size() <= uint(index) )
	{
		PyErr_SetString( PyExc_IndexError,
			"ChunkItemRevealer index out of range" );
		return NULL;
	}

	ChunkItems solo( 1, items[index] );
	return new ChunkItemGroup( solo );
}

/**
 *	Python sequence slice method
 */
PyObject * ChunkItemRevealer::pySeq_slice( Py_ssize_t startIndex, Py_ssize_t endIndex )
{
	BW_GUARD;

	ChunkItems items;
	this->reveal( items );	

	if (startIndex < 0)
		startIndex = 0;

	if (uint(endIndex) > items.size())
		endIndex = items.size();

	if (endIndex < startIndex)
		endIndex = startIndex;

	Py_ssize_t length = endIndex - startIndex;
	if (length == items.size())
	{
		Py_INCREF( this );
		return this;
	}

	items.erase( items.begin() + endIndex, items.end() );
	items.erase( items.begin(), items.begin() + startIndex );
	return new ChunkItemGroup( items );
}



/**
 *	See if the given object is in the sequence
 */
int ChunkItemRevealer::pySeq_contains( PyObject * pObject )
{
	BW_GUARD;

	if (!ChunkItemRevealer::Check( pObject )) return 0;

	ChunkItems ourItems;
	this->reveal( ourItems );
	ChunkItems othItems;
	((ChunkItemRevealer*)pObject)->reveal( othItems );

	if (othItems.empty()) return 0;	// weird I know

	// check each one then
	for (uint i = 0; i < othItems.size(); i++)
	{
		if (std::find( ourItems.begin(), ourItems.end(), othItems[i] ) ==
			ourItems.end()) return 0;
	}
	return 1;	// all there!
}




// -----------------------------------------------------------------------------
// Section: ChunkItemGroup
// -----------------------------------------------------------------------------

static PySequenceMethods ChunkItemGroup_seqfns =
{
	ChunkItemGroup::_pySeq_length,		/* sq_length */
	ChunkItemGroup::_pySeq_concat,		/* sq_concat */
	0,									/* sq_repeat */
	ChunkItemGroup::_pySeq_item,		/* sq_item */
	ChunkItemGroup::_pySeq_slice,		/* sq_slice */
	ChunkItemGroup::_pySeq_ass_item,	/* sq_ass_item */
	ChunkItemGroup::_pySeq_ass_slice,	/* sq_ass_slice */
	ChunkItemGroup::_pySeq_contains,	/* sq_contains */
	0,									/* sq_inplace_concat */
	0,									/* sq_inplace_repeat */
};

PY_TYPEOBJECT_WITH_SEQUENCE( ChunkItemGroup, &ChunkItemGroup_seqfns )

PY_BEGIN_METHODS( ChunkItemGroup )
	PY_METHOD( add )
	PY_METHOD( rem )
	PY_METHOD( filterTypes )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ChunkItemGroup )
	PY_ATTRIBUTE( size )
PY_END_ATTRIBUTES()

PY_FACTORY( ChunkItemGroup, WorldEditor )

PY_SCRIPT_CONVERTERS( ChunkItemGroup )


/**
 *	Constructor
 */
ChunkItemGroup::ChunkItemGroup( ChunkItems items, PyTypeObject * pType ) :
	ChunkItemRevealer( pType ),
	items_( items )
{
}


/**
 *	Destructor.
 */
ChunkItemGroup::~ChunkItemGroup()
{
}


/**
 *	Adds an item to the group
 *
 *	@return true if the item wasn't already there
 */
bool ChunkItemGroup::add( ChunkItemPtr pNewbie )
{
	BW_GUARD;

	if (!pNewbie)
		return false;
    if (pNewbie && !pNewbie->edCanAddSelection())
        return false;
	ChunkItems::iterator found =
		std::find( items_.begin(), items_.end(), pNewbie );
	if (found == items_.end())
	{
		if (pNewbie->edIsVLO())
		{
			EditorChunkItem::updateSelectionMark();
			for( ChunkItems::iterator iter = items_.begin(); iter != items_.end(); ++iter )
				(*iter)->edCheckMark( EditorChunkItem::selectionMark() );
			if( pNewbie->edCheckMark( EditorChunkItem::selectionMark() ) )
				items_.push_back( pNewbie );
		}
		else
			items_.push_back( pNewbie );
		return true;
	}

	return false;
}


/**
 *	Python sequence length method (overidden for speed)
 */
Py_ssize_t ChunkItemGroup::pySeq_length()
{
	return items_.size();
}



/**
 *	Python sequence item assignment method
 */
int ChunkItemGroup::pySeq_ass_item( Py_ssize_t index, PyObject * pItem )
{
	BW_GUARD;

	// check index in range
	if (index < 0 || uint(index) >= items_.size())
	{
		PyErr_SetString( PyExc_IndexError,
			"ChunkItemGroup assignment index out of range" );
		return -1;
	}

	return this->pySeq_ass_slice( index, index+1, pItem );
}


/**
 *	Python sequence slice assignment method
 */
int ChunkItemGroup::pySeq_ass_slice(
	Py_ssize_t indexA, Py_ssize_t indexB, PyObject * pOther )
{
	BW_GUARD;

	// make sure we're setting it to a sequence
	if (!ChunkItemRevealer::Check( pOther ))
	{
		PyErr_Format( PyExc_TypeError, "ChunkItemRevealer slices "
			"can only be assigned to a ChunkItemRevealer" );
		return -1;
	}

	ChunkItems newItems;
	(static_cast<ChunkItemRevealer *>(pOther))->reveal( newItems );

	MF_ASSERT( items_.size() <= INT_MAX );
	const int sz = ( int )items_.size();

	// put indices in range (slices don't generate index errors)
	if (indexA > sz )
	{
		indexA = sz;
	}
	if (indexA < 0)
	{
		indexA = 0;
	}
	if (indexB > sz )
	{
		indexB = sz;
	}
	if (indexB < 0)
	{
		indexB = 0;
	}

	// only erase if there's something to erase
	if (indexA < indexB)
	{
		items_.erase( items_.begin() + indexA, items_.begin() + indexB );
	}

	EditorChunkItem::updateSelectionMark();
	for(ChunkItems::iterator iter = items_.begin();
		iter != items_.end();
		++iter)
	{
		(*iter)->edCheckMark( EditorChunkItem::selectionMark() );
	}
	for (ChunkItems::iterator iter = newItems.begin();
		iter != newItems.end();
		)
	{
		if ((*iter)->edIsVLO()
			&&
			!(*iter)->edCheckMark( EditorChunkItem::selectionMark() ))
		{
			iter = newItems.erase( iter );
		}
		else
		{
			++iter;
		}
	}
	// and add the new ones
	items_.insert( items_.begin() + indexA, newItems.begin(), newItems.end() );

	return 0;
}


/**
 *	Adds the items from the given revealer to this group
 */
PyObject * ChunkItemGroup::py_add( PyObject * args )
{
	BW_GUARD;

	PyObject * pToAdd;
	if (!PyArg_ParseTuple( args, "O", &pToAdd) ||
		!ChunkItemRevealer::Check( pToAdd ))
	{
		PyErr_SetString( PyExc_TypeError, "ChunkItemGroup.add "
			"expects a ChunkItemRevealer" );
		return NULL;
	}

	ChunkItemRevealer * pRevealer = static_cast<ChunkItemRevealer*>( pToAdd );

	BW::vector< ChunkItemPtr > items;
	pRevealer->reveal( items );

	EditorChunkItem::updateSelectionMark();

	for (uint i = 0; i < items_.size(); ++i)
	{
		items_[i]->edCheckMark( EditorChunkItem::selectionMark() );
	}

	for (uint i = 0; i < items.size(); ++i)
	{
		ChunkItemPtr pNewbie = items[i];

		if (!pNewbie || !pNewbie->edCanAddSelection())
			continue;

		if (pNewbie->edCheckMark( EditorChunkItem::selectionMark() ))
			items_.push_back( pNewbie );
	}

	Py_RETURN_NONE;
}

/**
 *	Removes the items from the given revealer from this group
 */
PyObject * ChunkItemGroup::py_rem( PyObject * args )
{
	BW_GUARD;

	PyObject * pToDel;
	if (!PyArg_ParseTuple( args, "O", &pToDel) ||
		!ChunkItemRevealer::Check( pToDel ))
	{
		PyErr_SetString( PyExc_TypeError, "ChunkItemGroup.rem "
			"expects a ChunkItemRevealer" );
		return NULL;
	}

	ChunkItemRevealer * pRevealer = static_cast<ChunkItemRevealer*>( pToDel );

	BW::vector< ChunkItemPtr > items;
	pRevealer->reveal( items );

	for (uint i = 0; i < items.size(); i++)
	{
		ChunkItems::iterator found =
			std::find( items_.begin(), items_.end(), items[i] );
		if (found != items_.end())
		{
			items_.erase( found );
		}
	}

	Py_RETURN_NONE;
}



/**
 *	Factory method
 */
PyObject * ChunkItemGroup::pyNew( PyObject * args )
{
	BW_GUARD;

	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.ChunkItemGroup() "
			"expects no arguments" );
		return NULL;
	}

	return new ChunkItemGroup();
}


/**
 *	This method reveals the items in this group
 */
void ChunkItemGroup::reveal( BW::vector< ChunkItemPtr > & items )
{
	items = items_;
}

/**
 *	This method filters out chunk items if the class name isn't specified
 *	in the given list of strings.
 */
ChunkItemGroup* ChunkItemGroup::filterTypes( const BW::vector<BW::string>& typeNames )
{
	BW_GUARD;

	ChunkItems filteredItems;
	for (ChunkItems::iterator it = items_.begin(); it != items_.end(); ++it)
	{
		if (std::find( typeNames.begin(), typeNames.end(), (*it)->edClassName() ) !=
			typeNames.end())
		{
			filteredItems.push_back( *it );
		}		
	}

	return new ChunkItemGroup( filteredItems );
}


// -----------------------------------------------------------------------------
// Section: ChunkItemView
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( ChunkItemView )

PY_BEGIN_METHODS( ChunkItemView )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ChunkItemView )
	PY_ATTRIBUTE( revealer )
PY_END_ATTRIBUTES()

PY_FACTORY( ChunkItemView, View )

/**
 *	Constructor.
 */
ChunkItemView::ChunkItemView( PyTypeObject * pType ) :
	ToolView( pType )
{
}


/**
 *	Destructor.
 */
ChunkItemView::~ChunkItemView()
{
}


/**
 *	This method updates LODs and animations of the selected item.
 * 	@param tool the tool.
 */
void ChunkItemView::updateAnimations( const Tool& tool )
{
	BW_GUARD;

	if (!revealer_)
	{
		return;
	}

	revealer_->reveal( items_ );

	Moo::rc().push();

	for (BW::vector< ChunkItemPtr >::iterator itr = items_.begin();
		itr != items_.end();
		++itr)
	{
		ChunkItemPtr cip = (*itr);

		if (cip->chunk() == NULL)
		{
			continue;
		}

		Matrix scaleUp = cip->chunk()->transform();
		scaleUp.translation( scaleUp.applyToOrigin() + Vector3( 0, 1, 0 ) );
		Moo::rc().world( scaleUp );
		cip->updateAnimations();
	}

	Moo::rc().pop();
}


/**
 *	This method draws the appropriate item selected
 */
void ChunkItemView::render( Moo::DrawContext& drawContext, const Tool& tool )
{
	BW_GUARD;

	if (!revealer_) return;

	Moo::rc().push();

	for (BW::vector< ChunkItemPtr >::iterator itr = items_.begin();
		itr != items_.end();
		++itr)
	{
		ChunkItemPtr cip = (*itr);

		if (cip->chunk() == NULL) continue;

		//cip->drawSelected();

		for (int j = 0; j < 4; j++)
		{
			Matrix scaleUp = cip->chunk()->transform();
			scaleUp.translation( scaleUp.applyToOrigin() +
				Vector3(0,1,0 ) );
			Moo::rc().world( scaleUp );
			cip->draw( drawContext );
		}		
	}

	Moo::rc().pop();
}


/**
 *	Python factory method
 */
PyObject * ChunkItemView::pyNew( PyObject * args )
{
	BW_GUARD;

	PyObject * pRevealer = NULL;
	if (!PyArg_ParseTuple( args, "|O", &pRevealer ) ||
		pRevealer && !ChunkItemRevealer::Check( pRevealer ))
	{
		PyErr_SetString( PyExc_TypeError, "ChunkItemView() "
			"expects an optional Revealer argument" );
		return NULL;
	}

	SmartPointer<ChunkItemRevealer> spRevealer =
		static_cast<ChunkItemRevealer*>( pRevealer );

	ChunkItemView * civ = new ChunkItemView();
	civ->revealer_ = spRevealer;
	return civ;
}



// -----------------------------------------------------------------------------
// Section: ChunkItemBounds
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( ChunkItemBounds )

PY_BEGIN_METHODS( ChunkItemBounds )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ChunkItemBounds )
PY_END_ATTRIBUTES()

PY_FACTORY( ChunkItemBounds, View )


/**
 *	Constructor.
 */
ChunkItemBounds::ChunkItemBounds( PyTypeObject * pType ) :
	ChunkItemView( pType ),
	growFactor_( 0.0f ),
	colour_( 0xffffffff ),
	offset_( 0.0f ),
	tile_( 1.0f )
{
}


/**
 *	This method updates LODs and animations.
 * 	@param tool the tool.
 */
void ChunkItemBounds::updateAnimations( const Tool& tool )
{
	BW_GUARD;
}


/**
 *	This method draws the appropriate item selected
 */
void ChunkItemBounds::render( Moo::DrawContext& drawContext, const Tool& tool )
{
	BW_GUARD;

	if (!revealer_) return;

	static DogWatch dw( "ChunkItemBounds" );
	ScopedDogWatch sdw( dw );

	BW::vector< ChunkItemPtr > items;
	revealer_->reveal( items );

	Moo::rc().push();

	bool renderStatesSet = false;

	for (uint i = 0; i < items.size(); i++)
	{
		ChunkItemPtr cip = items[i];

		if (cip->chunk() == NULL) continue;

		BoundingBox bb;

		Moo::rc().world( cip->chunk()->transform() );
		Moo::rc().preMultiply( cip->edTransform() );
		Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
		cip->edSelectedBox( bb );

		// grow the bb using the grow factor
		float addScaled = (bb.maxBounds() - bb.minBounds()).length() * growFactor_;
		Vector3 boundsAdd( addScaled, addScaled, addScaled );
		bb.addBounds( bb.minBounds() - boundsAdd );
		bb.addBounds( bb.maxBounds() + boundsAdd );

		if (!renderStatesSet)
		{
			// setup rendering and texturing states
			Moo::rc().setVertexShader( NULL );
			Moo::rc().setPixelShader( NULL );
			if ( texture_ != NULL && texture_->pTexture() != NULL )
			{
				Moo::rc().setTexture( 0, texture_->pTexture() );
				Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
				Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
				Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
				Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
				Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
				Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
				Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
				Moo::rc().setSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
				Moo::rc().setSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
				Moo::rc().setSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
				Moo::rc().setSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
				Moo::rc().setSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
			}
			else
			{
				Moo::rc().setTexture( 0, NULL );
			}
			Moo::rc().setTexture( 1, NULL );

			Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
			Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
			Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
			Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
			Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		}

		// draw using the colour, offset and tiling values.
		Geometrics::texturedWireBox( bb, colour_, offset_, tile_, false,
			!renderStatesSet /* set states only on first draw */ );

		renderStatesSet = true;
	}

	Moo::rc().pop();
}


/**
 *	Python factory method for ChunkItemBounds
 *
 *  @param spRevealer	item revealer
 *  @param col			colour to use when drawing the bounds
 *  @param growFactor	grows the bounds by the size of the object multiplied
 *                      by this factor. A value of 0.01 means 1% percent the
 *                      size of the object (approx).
 *  @param texture      name of the texture, or "" to disable texturing.
 *  @param offset       texture coordinates offset
 *  @param tile         texture coordinates tiling. 1 means that each line tiles
 *                      the texture one time.
 *  @returns            the new ChunkItemBounds object.
 */
ChunkItemBounds * ChunkItemBounds::New(
	SmartPointer<ChunkItemRevealer> spRevealer, uint32 col,
	float growFactor, BW::string texture, float offset, float tile )
{
	BW_GUARD;

	ChunkItemBounds * cib = new ChunkItemBounds();
	cib->revealer_ = spRevealer;
	cib->growFactor_ = growFactor;
	cib->colour_ = col;
	if ( !texture.empty() )
	{
		cib->texture_ =
			Moo::TextureManager::instance()->get( texture );
		cib->offset_ = offset;
		cib->tile_ = tile;
	}
	return cib;
}

BW::vector<Chunk*> extractChunks( BW::vector<ChunkItemPtr> chunkItems )
{
	BW_GUARD;

	BW::vector<Chunk*> v;

	for (BW::vector<ChunkItemPtr>::iterator i = chunkItems.begin(); i != chunkItems.end(); ++i)
	{
		if ((*i)->isShellModel())
		{
			v.push_back( (*i)->chunk() );
		}
	}

	return v;
}

BW::vector<Chunk*> extractChunks( ChunkItemRevealer* revealer )
{
	BW_GUARD;

	BW::vector<ChunkItemPtr> chunkItems;
	revealer->reveal( chunkItems );
	return extractChunks( chunkItems );
}

BW_END_NAMESPACE
// item_view.cpp
