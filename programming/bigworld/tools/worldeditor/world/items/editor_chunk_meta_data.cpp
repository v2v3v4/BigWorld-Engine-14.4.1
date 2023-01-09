#include "pch.hpp"
#include "worldeditor/world/items/editor_chunk_meta_data.hpp"
#include "worldeditor/world/items/editor_chunk_substance.ipp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/editor/item_editor.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "appmgr/options.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/geometry_mapping.hpp"
#include "chunk/umbra_chunk_item.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/resource_cache.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EditorChunkMetaData
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EditorChunkMetaData::EditorChunkMetaData()
{
	BW_GUARD;

#if UMBRA_ENABLE
	this->wantFlags_ = WantFlags(
		this->wantFlags_ | WANTS_TICK | WANTS_UPDATE | WANTS_DRAW );
#else
	this->wantFlags_ = WantFlags(
		this->wantFlags_ | WANTS_UPDATE | WANTS_DRAW );
#endif
	model_ = Model::get( "resources/models/metaData.model" );
	ResourceCache::instance().addResource( model_ );
	this->updateReprModel();
}


/**
 *	Destructor.
 */
EditorChunkMetaData::~EditorChunkMetaData()
{
}


#if UMBRA_ENABLE
void EditorChunkMetaData::tick( float /*dTime*/ )
{
	BW_GUARD;

	ModelPtr model = reprModel();
	if (currentUmbraModel_ != model)
	{
		currentUmbraModel_ = model;
		this->syncInit();
	}
}
#endif


void EditorChunkMetaData::syncInit()
{
	// Grab the visibility bounding box
#if UMBRA_ENABLE
	BW_GUARD;

	bw_safe_delete(pUmbraDrawItem_);

	if (currentUmbraModel_ == NULL)
	{
		EditorChunkSubstance<ChunkItem>::syncInit();
		return;
	}	

	BoundingBox bb = BoundingBox::s_insideOut_;
	bb = currentUmbraModel_->boundingBox();
	
	// Set up object transforms
	Matrix m = pChunk_->transform();
	m.preMultiply( transform_ );

	// Create the umbra chunk item
	UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem();
	pUmbraChunkItem->init( this, bb, m, pChunk_->getUmbraCell());
	pUmbraDrawItem_ = pUmbraChunkItem;

	this->updateUmbraLenders();
#endif
}


/**
 *	This method saves the data section pointer before calling its
 *	base class's load method
 */
bool EditorChunkMetaData::load( DataSectionPtr pSection, Chunk * pChunk )
{
	BW_GUARD;

	pOwnSect_ = pSection;

	transform_ = pSection->readMatrix34( "transform", Matrix::identity );

	return edCommonLoad( pSection );
}



/**
 *	Save any property changes to this data section
 */
bool EditorChunkMetaData::edSave( DataSectionPtr pSection )
{
	BW_GUARD;

	pSection->writeMatrix34( "transform", transform_ );

	return edCommonSave( pSection );
}



/**
 *	Change our transform, temporarily or permanently
 */
bool EditorChunkMetaData::edTransform( const Matrix & m, bool transient )
{
	BW_GUARD;

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
 *	Add the properties of this flare to the given editor
 */
bool EditorChunkMetaData::edEdit( class GeneralEditor & editor )
{
	BW_GUARD;

	if (this->edFrozen())
		return false;

	STATIC_LOCALISE_NAME( s_position, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_META_DATA/POSITION" );

	MatrixProxy* pMP = new ChunkItemMatrix( this );
	editor.addProperty( new GenPositionProperty( s_position, pMP ) );

	return edCommonEdit( editor );
}


/**
 *	Return a modelptr that is the representation of this chunk item
 */
ModelPtr EditorChunkMetaData::reprModel() const
{
	return model_;
}

/// Write the factory statics stuff
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk)
IMPLEMENT_CHUNK_ITEM( EditorChunkMetaData, metaData, 1 )

BW_END_NAMESPACE
// editor_chunk_meta_data.cpp
