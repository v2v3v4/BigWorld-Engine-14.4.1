#include "pch.hpp"
#include "editor_chunk_model_vlo.hpp"
#include "worldeditor/world/items/editor_chunk_substance.ipp"
#include "worldeditor/world/items/editor_chunk_vlo.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/vlo_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/editor/item_editor.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "model/super_model.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/resource_cache.hpp"

BW_BEGIN_NAMESPACE

BW::vector< EditorChunkModelVLO * >	EditorChunkModelVLO::instances_;
SimpleMutex								EditorChunkModelVLO::instancesMutex_;

// -----------------------------------------------------------------------------
// Section: EditorChunkModelVLO
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EditorChunkModelVLO::EditorChunkModelVLO( const BW::string & uid )
	: ChunkModelVLO( uid )
	, changed_( false )
{
	BW_GUARD;

	proxiedModel_ = new EditorChunkModel();
	EditorChunkModel* editorProxiedModel_ = dynamic_cast<EditorChunkModel*>( proxiedModel_.get() );
	editorProxiedModel_->edSelectable( false );

	SimpleMutexHolder holder(instancesMutex_);

	// Add us to the list of instances
	instances_.push_back( this );
	setUID(uid);
}


/**
 *	Destructor.
 */
EditorChunkModelVLO::~EditorChunkModelVLO()
{
	BW_GUARD;

	SimpleMutexHolder holder(instancesMutex_);

	// Remove us from the list of instances
	BW::vector< EditorChunkModelVLO* >::iterator p = std::find(
		instances_.begin(), instances_.end(), this );

	MF_ASSERT( p != instances_.end() );

	instances_.erase( p );
}


bool EditorChunkModelVLO::load( DataSectionPtr pSection, Chunk * pChunk )
{
	BW_GUARD;
	EditorChunkModel::BigModelLoader bigModelLoader;
	if (ChunkModelVLO::load( pSection, pChunk ) == false)
	{
		return false;
	}

	objectCreated();
	return true;
}


void EditorChunkModelVLO::cleanup()
{
	BW_GUARD;

	if ( VLOManager::instance()->needsCleanup( uid_ ) )
	{
		// If the object was saved (not dirty) and was deleted, then we can 
		// remove the VLO file.

		BW::string fileName( '_' + uid_ + ".vlo" );
		DataSectionPtr ds =
			WorldManager::instance().geometryMapping()->pDirSection();
		ds->deleteSection( fileName );
			
		if (BWResource::fileExists( fileName ))
		{
			WorldManager::instance().eraseAndRemoveFile( fileName );
		}
		fileName = chunkPath_ + fileName;

		if (BWResource::fileExists( fileName ))
		{
			WorldManager::instance().eraseAndRemoveFile( fileName );
		}
		fileName = uid_ + ".vlo";
		ds = WorldManager::instance().geometryMapping()->pDirSection();
		ds->deleteSection( fileName );
			
		if (BWResource::fileExists( fileName ))
		{
			WorldManager::instance().eraseAndRemoveFile( fileName );
		}
		fileName = chunkPath_ + fileName;

		if (BWResource::fileExists( fileName ))
		{
			WorldManager::instance().eraseAndRemoveFile( fileName );
		}
		dataSection_ = NULL;
	}
}


/**
 *
 */
void EditorChunkModelVLO::saveFile( Chunk* pChunk )
{
	BW_GUARD;

	if (VLOManager::instance()->isDeleted( uid_ ))
	{
		return;
	}

	if (changed_ && dataSection_)
	{
		dataSection_->save();
		changed_ = false;
	}
}


/**
 *
 */
void EditorChunkModelVLO::save(
	DataSectionPtr pDataSection, const Matrix & worldTransform )
{
	BW_GUARD;
	if (pDataSection == NULL)
	{
		return;
	}
	VeryLargeObject::save( pDataSection );

	DataSectionPtr proxySection = pDataSection->openSection( getSectionName() );

	if (proxySection == NULL)
	{
		return;
	}

	metaData_.save( proxySection );

	DataSectionPtr modelSection =
		proxySection->openSection( ChunkModel::getSectionName(), true );

	if (modelSection == NULL)
	{
		return;
	}
	proxiedModel_->edSave( modelSection );

	modelSection->writeMatrix34( getWorldTransformAttrName(), worldTransform );
}


/**
 *
 */
void EditorChunkModelVLO::save()
{
	BW_GUARD;
	save( dataSection_, worldTransform() );
}


/**
 *	Save any property changes to this data section
 */
bool EditorChunkModelVLO::edSave( DataSectionPtr pSection )
{
	return true;
}


/**
 *	Add the properties to the given editor
 */
bool EditorChunkModelVLO::edEdit(
	class GeneralEditor & editor, const ChunkItemPtr pItem )
{
	BW_GUARD;

	metaData_.edit( editor, Name(), false );
	getProxiedModel()->edEdit( editor, pItem );

	return true;
}


/**
 *	Called when an property has been modified
 */
void EditorChunkModelVLO::edCommonChanged()
{
	BW_GUARD;

	VeryLargeObject::ChunkItemList& items = chunkItems();

	if (!items.empty())
	{
		ChunkItemPtr item = *items.begin();

		item->edCommonChanged();
	}
}


/**
 *	Returns true if the proxied model should be visible inside shells.
 */
bool EditorChunkModelVLO::visibleInside() const
{
	return true;
}


/**
 *	Returns true if the proxied model should be visible outside shells.
 */
bool EditorChunkModelVLO::visibleOutside() const
{
	return true;
}


/**
 *
 */
void EditorChunkModelVLO::dirty()
{
	BW_GUARD;

	ChunkModelVLO::dirty();
	changed_ = true;
	updateWorldVars(Matrix::identity);

	// Make sure at least one of its chunks gets marked as dirty.
	VeryLargeObject::ChunkItemList items = this->chunkItems();
	for (VeryLargeObject::ChunkItemList::iterator it = items.begin();
		it != items.end(); ++it)
	{
		if ((*it)->chunk())
		{
			WorldManager::instance().changedChunk( (*it)->chunk() );
		}
	}
}


/**
 *	This returns the number of triangles of this water body.
 */
int EditorChunkModelVLO::numTriangles() const
{
	BW_GUARD;
	return proxiedModel_->edNumTriangles();
}


/**
 *	This returns the number of primitives of this body.
 */
int EditorChunkModelVLO::numPrimitives() const
{
	BW_GUARD;

	return proxiedModel_->edNumPrimitives();
}


VLOFactory EditorChunkModelVLO::factory_(
	ChunkModelVLO::getSectionName(), 1, EditorChunkModelVLO::create );


/**
 *
 */
bool EditorChunkModelVLO::create(
	Chunk * pChunk, DataSectionPtr pSection, const BW::string & uid )
{
	BW_GUARD;

	EditorChunkModelVLO * pItem = new EditorChunkModelVLO( uid );	

	if (pItem->load( pSection, pChunk ))
	{
		return true;
	}

	//the VLO destructor should clear out the static pointer map
	bw_safe_delete(pItem);
	return false;
}


void EditorChunkModelVLO::edBounds( BoundingBox & bbRet, Chunk * pChunk ) const
{
	proxiedModel_->edBounds( bbRet );
}


EditorChunkModelPtr EditorChunkModelVLO::getProxiedModel()
{
	return dynamic_cast< EditorChunkModel * >( proxiedModel_.get() );
}


Chunk * EditorChunkModelVLO::vloChunk() const
{
	Vector3 worldTranslation = worldTransform()[ 3 ];
	Chunk * pChunk = ChunkManager::instance().findOutdoorChunkByPosition(
		worldTranslation.x, worldTranslation.z,
		WorldManager::instance().geometryMapping() );
	return pChunk;
}


const Matrix & EditorChunkModelVLO::localTransform()
{
	Vector3 worldTranslation = worldTransform()[ 3 ];
	Chunk * pChunk = vloChunk();
	if (pChunk == NULL)
	{
		return Matrix::identity;
	}
	return ChunkModelVLO::localTransform( pChunk );
}


BW_END_NAMESPACE
// editor_chunk_model_vlo.cpp
