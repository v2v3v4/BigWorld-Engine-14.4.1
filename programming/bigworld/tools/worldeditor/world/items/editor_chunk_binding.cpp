#include "pch.hpp"
#include "worldeditor/world/items/editor_chunk_binding.hpp"
#include "worldeditor/world/items/editor_chunk_substance.ipp"
#include "worldeditor/world/items/editor_chunk_item_tree_node.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/editor/item_editor.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "appmgr/options.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/geometry_mapping.hpp"
#include "resmgr/bwresource.hpp"
#include "moo/geometrics.hpp"
#include "model/super_model.hpp"
#include "resmgr/resource_cache.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EditorChunkBinding
// -----------------------------------------------------------------------------


/**
 *	Constructor.
 */
EditorChunkBinding::EditorChunkBinding()
: transform_(Matrix::identity)
{
	BW_GUARD;

	this->wantFlags_ = WantFlags( this->wantFlags_ | WANTS_DRAW );

	model_ = Model::get( "resources/models/entity.model" );
	ResourceCache::instance().addResource( model_ );
}


/**
 *	Destructor.
 */
EditorChunkBinding::~EditorChunkBinding()
{
}

void EditorChunkBinding::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if (!edShouldDraw())
		return;

	if (WorldManager::instance().drawSelection())
	{
		WorldManager::instance().registerDrawSelectionItem( this );
	}
	else
	{
		// draw a line back to the cluster (if applicable)
		Moo::rc().push();
		Moo::rc().world( Matrix::identity );

		const Moo::Colour lineColour = 0xffff0000;

		Vector3 from = this->from()->chunk()->transform().applyPoint( this->from()->edTransform().applyToOrigin() )
			+ Vector3(0.f, 0.1f, 0.f);
		Vector3 to = this->to()->chunk()->transform().applyPoint( this->to()->edTransform().applyToOrigin() )
			+ Vector3(0.f, 0.1f, 0.f);

		Geometrics::drawLine( from, to, lineColour, false );	// false = z-buffer the lines

		Moo::rc().pop();
	}

	// draw binding item
	ModelPtr model = reprModel();
	if (model)
	{
		Moo::rc().push();
		Moo::rc().preMultiply( this->edTransform() );

		model->dress( Moo::rc().world() );
		model->draw( drawContext, true );

		Moo::rc().pop();
	}
}


bool EditorChunkBinding::load( DataSectionPtr pSection )
{
	BW_GUARD;

	bool ok = this->EditorChunkSubstance<ChunkBinding>::load( pSection );

	if (this->from())
	{
		calculateTransform(true);
	}

	return ok;
}


/**
 *	Save any property changes to this data section
 */
bool EditorChunkBinding::edSave( DataSectionPtr pSection )
{
	BW_GUARD;

	if (!ChunkBinding::save( pSection ))
		return false;

	if (!edCommonSave( pSection ))
		return false;

	return true;
}

/**
 *	Get the current transform
 */
const Matrix & EditorChunkBinding::edTransform()
{
	return transform_;
}


void EditorChunkBinding::calculateTransform(bool transient)
{
	BW_GUARD;

	if (!from() || !to())
		return;  // can't do anything, not fully loaded

	// place binding object a few metres away from the owner,
	// work in the binding's chunk space
	Matrix fromTrans = this->from()->edTransform();
	const Matrix& fromChunkTransform = this->from()->chunk()->transform();
	Matrix toTrans = this->to()->edTransform();
	const Matrix& toChunkTransform = this->to()->chunk()->transform();

	Chunk * myChunk = this->from()->chunk();
	if (chunk())
		myChunk = chunk();

	// convert the toTrans into the space of the fromTrans trans
	toTrans.postMultiply( toChunkTransform );
	toTrans.postMultiply( myChunk->transformInverse() );
	fromTrans.postMultiply( fromChunkTransform );
	fromTrans.postMultiply( myChunk->transformInverse() );

	Vector3 pos = 0.5f * (toTrans.applyToOrigin() - fromTrans.applyToOrigin()) 
					+ fromTrans.applyToOrigin() + Vector3(0.f, 0.1f, 0.f);

	// displacement is half way between
	Matrix final;
	final.setTranslate(pos);
	edTransform(final, transient);
}


/**
 *	Change our transform, temporarily or permanently
 */
bool EditorChunkBinding::edTransform( const Matrix & m, bool transient )
{
	BW_GUARD;

	// this is always a temporary change, as position of a binding is unimportant
	// if this is only a temporary change, keep it in the same chunk
	if (transient)
	{
		transform_ = m;
		return true;
	}


	// it's permanent, so find out where we belong now
	Chunk * pOldChunk = pChunk_;
	Chunk * pNewChunk = this->edDropChunk( m.applyToOrigin() );
	if (pNewChunk == NULL) return false;

	// make sure the chunks aren't readonly
	if (!EditorChunkCache::instance( *pOldChunk ).edIsWriteable() 
		|| !EditorChunkCache::instance( *pNewChunk ).edIsWriteable())
		return false;

	// ok, accept the transform change then
	transform_.multiply( m, pOldChunk->transform() );
	transform_.postMultiply( pNewChunk->transformInverse() );

	// note that both affected chunks have seen changes
	WorldManager::instance().changedChunk( pOldChunk );
	if (pNewChunk != pOldChunk )
	{
		WorldManager::instance().changedChunk( pNewChunk );
	}

	edMove( pOldChunk, pNewChunk );

	return true;
}


/**
 * Return false if any of the markers are not yet loaded
 */
bool EditorChunkBinding::edCanDelete()
{
	BW_GUARD;

	if (from() && to())
		return EditorChunkSubstance<ChunkBinding>::edCanDelete();	// make sure the linked entities are loaded

	return false;
}

/**
 * Tell the entities/markers they are no longer part of this binding
 */
void EditorChunkBinding::edPreDelete()
{
    // TODO: write EditorChunkBinding::edPreDelete
}

/**
 *	Copy the category, nothing else
 */
void EditorChunkBinding::edPostClone( EditorChunkItem* srcItem )
{
	BW_GUARD;

	EditorChunkItem::edPostClone( srcItem );
}


/**
 *	Return a modelptr that is the representation of this chunk item
 */
ModelPtr EditorChunkBinding::reprModel() const
{
	return model_;
}

/// Write the factory statics stuff
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection)
IMPLEMENT_CHUNK_ITEM( EditorChunkBinding, binding, 1 )

BW_END_NAMESPACE

