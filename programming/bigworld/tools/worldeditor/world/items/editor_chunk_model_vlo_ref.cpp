#include "pch.hpp"
#include "editor_chunk_model_vlo_ref.hpp"
#include "worldeditor/world/items/editor_chunk_substance.ipp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/items/very_large_object_behaviour.hpp"
#include "worldeditor/world/items/editor_chunk_model_vlo.hpp"
#include "worldeditor/world/vlo_manager.hpp"
#include "worldeditor/world/item_info_db.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_model_vlo.hpp"
#include "chunk/chunk_vlo_obstacle.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "moo/debug_geometry.hpp"
#include "moo/line_helper.hpp"

#include "vlo_existence_operation.hpp"

BW_BEGIN_NAMESPACE


void EditorChunkModelVLOParent::addAsObstacle()
{
}


void EditorChunkModelVLOParent::toss( Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk_ != NULL)
	{
		if (pOwnSect_)
		{
			EditorChunkCache::instance( *pChunk_ ).
				pChunkSection()->delChild( pOwnSect_ );
			pOwnSect_ = NULL;
		}
	}

	ChunkModelVLORef::toss( pChunk );

	if (pChunk_ != NULL)
	{
		if (!pOwnSect_ && pChunk_->loaded())
		{
			pOwnSect_ = EditorChunkCache::instance( *pChunk_ ).
				pChunkSection()->newSection( this->sectName() );
			edSave( pOwnSect_ );
		}
	}

	VeryLargeObjectPtr pObject = object();
	if (pObject)
	{
		if (pObject->lastDbItem())
		{
			ItemInfoDB::instance().toss( pObject->lastDbItem(), false );
			pObject->lastDbItem( NULL );

			if (!pChunk)
			{
				// If we are removing the lastDbItem, try to find another
				// suitable VLO item in its place.
				VeryLargeObject::ChunkItemList items = pObject->chunkItems();

				for (VeryLargeObject::ChunkItemList::iterator
					it = items.begin(); it != items.end(); ++it)
				{
					if ((*it) != this && (*it)->chunk())
					{
						// Found another suitable item, toss it in.
						ItemInfoDB::instance().toss( (*it), true );
						pObject->lastDbItem( *it );
						break;
					}
				}
			}
		}

		if (pChunk)
		{
			ItemInfoDB::instance().toss( this, true );
			pObject->lastDbItem( this );
		}
	}
}


ChunkItemFactory EditorChunkModelVLORef::factory_(
	ChunkModelVLORef::getSectionName(), 1, EditorChunkModelVLORef::create );

bool EditorChunkModelVLORef::s_creating = false;


EditorChunkModelVLORef::EditorChunkModelVLORef()
{
}


EditorChunkModelVLORef::~EditorChunkModelVLORef()
{
}


ChunkItemFactory::Result EditorChunkModelVLORef::create(
	Chunk * pChunk, DataSectionPtr pSection )
{
	BW_GUARD;
	ChunkModelVLORef * pVLO = new EditorChunkModelVLORef();
	BW::string type =
		pSection->readString( ChunkVLO::getTypeAttrName(), "" );
	BW::string uid =
		pSection->readString( ChunkVLO::getUIDAttrName(), "" );

	VeryLargeObjectPtr object = VeryLargeObject::getObject( uid );
	if(object == NULL)
	{
		if (s_creating)
		{
			DataSectionPtr chunkSection =
				pSection->openSection( ChunkModelVLO::getSectionName() );
			pVLO->buildVLOSection( chunkSection, pChunk, type, uid );
		}
	}
	return ChunkVLO::create( pVLO, pChunk, pSection );
}


/*static */DataSectionPtr EditorChunkModelVLORef::createDataSection(
	DataSectionPtr pModelSection )
{
	DataSectionPtr xmlSectionPtr =
		new XMLSection( EditorChunkModelVLORef::getSectionName() );
	xmlSectionPtr->writeString(
		ChunkVLO::getTypeAttrName(), ChunkModelVLO::getLargeObjectType() );
	xmlSectionPtr->writeString(
		ChunkVLO::getUIDAttrName(), VeryLargeObject::generateUID() );
	DataSectionPtr chunkProxySection =
		xmlSectionPtr->newSection( ChunkModelVLO::getSectionName() );
	DataSectionPtr modelSection =
		chunkProxySection->newSection( ChunkModel::getSectionName() );
	modelSection->copy( pModelSection );

	return xmlSectionPtr;
}


/*virtual */const char * EditorChunkModelVLORef::sectName() const
{
	return getSectionName().c_str();
}


/*virtual*/ bool EditorChunkModelVLORef::isDrawFlagVisible() const
{
	return true;
}


/*virtual */const char * EditorChunkModelVLORef::drawFlag() const
{
	return "render/scenery/drawVLO";
}

/*virtual */ModelPtr EditorChunkModelVLORef::reprModel() const
{
	return NULL;
}

/*virtual */void EditorChunkModelVLORef::edBounds(
	BoundingBox & bbRet ) const
{
	const EditorChunkModelVLO * vlo =
		dynamic_cast< const EditorChunkModelVLO * >( object().get() );
	vlo->edBounds( bbRet, chunk() );
}


/*virtual */void EditorChunkModelVLORef::edWorldBounds( BoundingBox & bbRet )
{
	const EditorChunkModelVLO * vlo =
		dynamic_cast< const EditorChunkModelVLO * >( object().get() );

	bbRet = BoundingBox::s_insideOut_;
	vlo->edBounds( bbRet, chunk() );

	// in case the item doesn't have size at all
	if ( bbRet.insideOut() )
	{
		bbRet = BoundingBox( Vector3(0,0,0), Vector3(0,0,0) );
	}

	bbRet.transformBy( vlo->worldTransform() );
}

/*virtual */void EditorChunkModelVLORef::edPostCreate()
{
	BW_GUARD;

	if (object())
	{
		VeryLargeObject::ChunkItemList & chunkList = object()->chunkItems();
		VeryLargeObject::ChunkItemList::iterator it = chunkList.begin();
		for ( ; it != chunkList.end(); it++ )
		{
			dynamic_cast< EditorChunkModelVLORef * >(*it)->objectCreated();
		}

		MetaData::updateCreationInfo( object()->metaData() );
	}

	EditorChunkItem::edPostCreate();
}


/**
 *	Get the current transform
 */
/*virtual */const Matrix & EditorChunkModelVLORef::edTransform()
{
	BW_GUARD;

	static Matrix transform;
	if (object() && pChunk_)
	{
		transform = object()->localTransform( pChunk_ );
	}

	return transform;
}


/*virtual */bool EditorChunkModelVLORef::edTransform(
	const Matrix & m, bool transient /*= false */)
{
	if (object() == NULL)
	{
		return false;
	}

	BoundingBox vloBB;
	const EditorChunkModelVLO * vlo =
		dynamic_cast< const EditorChunkModelVLO * >( object().get() );
	edBounds( vloBB );

	Matrix transform = edTransform();
	return VeryLargeObjectBehaviour::edTransform(
		*pObject_, pChunk_, vloBB, this, transform, drawTransient_,
		m, transient );
}


bool EditorChunkModelVLORef::load( const BW::string & uid, Chunk * pChunk )
{
	BW_GUARD;

	if ( VLOManager::instance()->contains( pChunk, uid )  )
	{
		return false;
	}

	pObject_ = VeryLargeObject::getObject( uid );
	if (pObject_)
	{	
		uid_ = uid;
		updateTransform( pChunk );
		return true;
	}	

	return false;
}


bool EditorChunkModelVLORef::load(
	DataSectionPtr pSection, Chunk * pChunk, BW::string * errorString )
{
	BW_GUARD;

	BW::string uid = pSection->readString( "uid", "" );

	if ( VLOManager::instance()->isDeleted( uid ) )
	{
		return false;
	}

	if ( VLOManager::instance()->contains( pChunk, uid )  )
	{
		return false;
	}

	bool loaded = this->EditorChunkModelVLOParent::load( pSection, pChunk );
	bool created = loaded ? false : createVLO( pSection, pChunk );
	if (loaded || created)
	{
		if (loaded)
			uid_ = uid;
		else
			uid_ = pObject_->getUID();

		pObject_->addItem( this );
		return true;
	}

	if ( errorString )
	{
		*errorString = "Failed to load " +
			pSection->readString( "type", "<unknown type>" ) +
			" VLO " + pSection->readString( "uid", "<unknown id>" );
	}
	return false;
}


/*virtual */bool EditorChunkModelVLORef::edShouldDraw()
{
	return EditorChunkModelVLOParent::edShouldDraw()
		&& !this->edHidden();
}


/*virtual */void EditorChunkModelVLORef::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if(edShouldDraw() == false)
	{
		return;
	}

	VeryLargeObjectPtr pObject = object();
	if (pObject)
	{	
		bool drawRed = readonly_ && OptionsMisc::readOnlyVisible();

		bool projectModule =
			ProjectModule::currentInstance() ==
			ModuleManager::instance().currentModule();

		pObject->drawRed( drawRed && !projectModule );

		if (highlight_)
		{
			// Set the VLO object to highlight, and reset the flag.
			pObject->highlight(true);
			highlight_ = false;
		}
	}

	if (WorldManager::instance().drawSelection())
	{
		WorldManager::instance().registerDrawSelectionItem( this );
	}

	EditorChunkModelVLOParent::draw( drawContext );
}


void EditorChunkModelVLORef::edPreDelete()
{
	BW_GUARD;

	// Don't delete a VLO object when deleting a shell if the VLO is not
	// entirely contained inside the shell (edBelongToChunk).
	if ( chunk() && EditorChunkCache::instance( *chunk() ).edIsDeleting() &&
		!chunk()->isOutsideChunk() && !edBelongToChunk() )
		return;

	if ( pObject_ )
	{
		UndoRedo::instance().add(
			new VLOExistenceOperation( this, chunk() ) );
		VLOManager::instance()->deleteFromAllChunks( this );
		pObject_->edDelete( this );		
#if UMBRA_ENABLE				
		bw_safe_delete(pUmbraDrawItem_);
		pObject_->syncInit( NULL );	
		this->updateUmbraLenders();
#endif	
	}
}


/**
 *	This method does extra stuff when this item is tossed between chunks.
 *
 *	Ensures the world variables used to create the proxied vlo get updated.
 */
void EditorChunkModelVLORef::toss( Chunk * pChunk )
{
	BW_GUARD;

	// get the template class to do the real work
	EditorChunkModelVLOParent::toss( pChunk );

	// and update our world vars if we're in a chunk
	if (pChunk_ != NULL)
	{
		this->updateWorldVars( pChunk_->transform() );

		if (object()->isObjectCreated())
		{
			objectCreated();
		}
	}
}


/*virtual */DataSectionPtr EditorChunkModelVLORef::pOwnSect()
{
	if (!pOwnSect_ && pChunk_->loaded())
	{
		pOwnSect_ = EditorChunkCache::instance( *pChunk_ ).
			pChunkSection()->newSection( this->sectName() );
		edSave( pOwnSect_ );
	}
	return EditorChunkModelVLOParent::pOwnSect();
}



/**
 *	Add the properties in the given editor
 */
bool EditorChunkModelVLORef::edEdit( class GeneralEditor & editor )
{
	BW_GUARD;

	if (object())
	{
		if (this->edFrozen())
			return false;
		
		return object()->edEdit( editor, this );
	}
	return true;
}

/**
 *	This method updates our caches of world space variables
 */
void EditorChunkModelVLORef::updateWorldVars( const Matrix & m )
{
	BW_GUARD;

	if (object())
	{
		object()->updateWorldVars( m );
	}
}


/**
 *	Save any property changes to this data section
 */
/*virtual */bool EditorChunkModelVLORef::edSave( DataSectionPtr pSection )
{
	BW_GUARD;

	if (!object())
	{
		return false;
	}

	pSection->writeString( ChunkVLO::getUIDAttrName(), object()->getUID() );
	pSection->writeString(
		ChunkVLO::getTypeAttrName(), ChunkModelVLO::getLargeObjectType() );

	object()->save();	
	return true;
}


ChunkItemPtr EditorChunkModelVLORef::convertToChunkModel( const Matrix & world )
{
	ChunkModelPtr chunkModel =
		dynamic_cast< ChunkModelVLO * >( object().get() )->getProxiedModel();
	MF_ASSERT(chunkModel != NULL)
	DataSectionPtr chunkModelDataPtr = chunkModel->pOwnSect();

	Vector3 worldTranslation = world[ 3 ];
	Chunk * pChunk = ChunkManager::instance().findOutdoorChunkByPosition(
		worldTranslation.x, worldTranslation.z,
		WorldManager::instance().geometryMapping() );

	if( pChunk == NULL )
	{
		return NULL;
	}

	ChunkItemFactory::Result result = pChunk->loadItem( chunkModelDataPtr );
	if( result )
	{
		if( result.item() == NULL )
		{
			return NULL;
		}
		Matrix destPose( world );
		destPose.postMultiply( pChunk->transformInverse() );
		result.item()->edTransform( destPose );
		return result.item();
	}
	return NULL;
}


/*static */ChunkItemPtr EditorChunkModelVLORef::convertToChunkModelVLO(
	ChunkItemPtr chunkModel, const Matrix & world )
{
	DataSectionPtr xmlSectionPtr =
		EditorChunkModelVLORef::createDataSection(
			chunkModel->pOwnSect() );

	ProxyChunkCreationHelper proxyChunkCreationHelper;
	ChunkPtr pChunk = chunkModel->chunk();
	ChunkItemFactory::Result result = pChunk->loadItem( xmlSectionPtr );
	if( result )
	{
		if( result.item() == NULL )
		{
			return NULL;
		}
		Matrix destPose( world );
		ChunkPtr createdChunk = result.item()->chunk();
		destPose.postMultiply( createdChunk->transformInverse() );
		result.item()->edTransform( destPose );
		return result.item();
	}
	return NULL;
}

BW_END_NAMESPACE

// editor_chunk_model_vlo_ref.cpp
