#include "pch.hpp"
#include "worldeditor/world/editor_entity_proxy.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/entity_property_parser.hpp"
#include "worldeditor/world/editor_chunk_item_linker_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/world/items/editor_chunk_user_data_object.hpp"
#include "worldeditor/world/editor_chunk_item_linker.hpp"
#include "worldeditor/undo_redo/entity_array_undo.hpp"
#include "worldeditor/undo_redo/linker_operations.hpp"
#include "worldeditor/editor/user_data_object_link_locator.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/editor/link_gizmo.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/user_data_object_link_data_type.hpp"
#include "chunk/geometry_mapping.hpp"
#include "gizmo/general_editor.hpp"
#include "resmgr/xml_section.hpp"
#include "common/base_properties_helper.hpp"
#include "common/array_properties_helper.hpp"
#include "common/editor_views.hpp"
#include "entitydef/data_types.hpp"
#include "entitydef/data_types/sequence_data_type.hpp"
#include "worldeditor/editor/chunk_item_placer.hpp"

BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
//	EntityIntProxy: A helper class to access entity INT properties
///////////////////////////////////////////////////////////////////////////////
EntityIntProxy::EntityIntProxy( BasePropertiesHelper* props, int index ) :
	props_( props ),
	index_( index ),
	useTransient_( false ),
	min_( std::numeric_limits< int >::min() ),
	max_( std::numeric_limits< int >::max() )
{
	MF_ASSERT_DEBUG( min_ < max_ );
}


EntityIntProxy::EntityIntProxy( BasePropertiesHelper* props, int index,
								int min, int max ) :
	props_( props ),
	index_( index ),
	useTransient_( false ),
	min_( min ),
	max_( max )
{
	MF_ASSERT_DEBUG( min_ < max_ );
}



bool EntityIntProxy::getRange( int& min, int& max ) const
{
	min = min_;
	max = max_;
	return true;
}


int EntityIntProxy::get() const
{
	BW_GUARD;

	if (useTransient_)
		return transientValue_;
	else
		return props_->propGetInt( index_ );
}


void EntityIntProxy::setTransient( int i )
{
	transientValue_ = i;
	useTransient_ = true;
}


bool EntityIntProxy::setPermanent( int i )
{
	BW_GUARD;

	useTransient_ = false;

	// set it
	bool ok = props_->propSetInt( index_, i );
	if (!ok) return false;

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );
	props_->pItem()->edPostModify();

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	return true;
}


BW::string EntityIntProxy::opName()
{
	BW_GUARD;

	return LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/SET_OP",
		props_->pItem()->edDescription().c_str(),
		props_->propName( index_ ) );
}


///////////////////////////////////////////////////////////////////////////////
//	EntityUIntProxy: A helper class to access entity UINT properties
///////////////////////////////////////////////////////////////////////////////
EntityUIntProxy::EntityUIntProxy( BasePropertiesHelper* props, int index ) :
	props_( props ),
	index_( index ),
	useTransient_( false ),
	min_( std::numeric_limits< uint32 >::min() ),
	max_( std::numeric_limits< uint32 >::max() )
{
	MF_ASSERT_DEBUG( min_ < max_ );
}


EntityUIntProxy::EntityUIntProxy( BasePropertiesHelper* props, int index,
								uint32 min, uint32 max ) :
	props_( props ),
	index_( index ),
	useTransient_( false ),
	min_( min ),
	max_( max )
{
	MF_ASSERT_DEBUG( min_ < max_ );
}



bool EntityUIntProxy::getRange( uint32& min, uint32& max ) const
{
	min = min_;
	max = max_;
	return true;
}


uint32 EntityUIntProxy::get() const
{
	BW_GUARD;

	if (useTransient_)
		return transientValue_;
	else
		return props_->propGetUInt( index_ );
}


void EntityUIntProxy::setTransient( uint32 i )
{
	transientValue_ = i;
	useTransient_ = true;
}


bool EntityUIntProxy::setPermanent( uint32 i )
{
	BW_GUARD;

	useTransient_ = false;

	// set it
	bool ok = props_->propSetUInt( index_, i );
	if (!ok) return false;

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );
	props_->pItem()->edPostModify();

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	return true;
}


BW::string EntityUIntProxy::opName()
{
	BW_GUARD;

	return LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/SET_OP",
		props_->pItem()->edDescription().c_str(),
		props_->propName( index_ ) );
}


///////////////////////////////////////////////////////////////////////////////
//	EntityFloatProxy: A helper class to access entity FLOAT properties
///////////////////////////////////////////////////////////////////////////////
EntityFloatProxy::EntityFloatProxy( BasePropertiesHelper* props, int index ) :
	props_( props ),
	index_( index ),
	useTransient_( false )
{
}

float EntityFloatProxy::get() const
{
	BW_GUARD;

	if (useTransient_)
		return transientValue_;
	else
		return props_->propGetFloat( index_ );
}

void EntityFloatProxy::setTransient( float f )
{
	transientValue_ = f;
	useTransient_ = true;
}

bool EntityFloatProxy::setPermanent( float f )
{
	BW_GUARD;

	useTransient_ = false;

	// set it
	bool ok = props_->propSetFloat( index_, f );
	if (!ok) return false;

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );
	props_->pItem()->edPostModify();

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	return true;
}

BW::string EntityFloatProxy::opName()
{
	BW_GUARD;

	return LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/SET_OP",
		props_->pItem()->edDescription().c_str(),
		props_->propName( index_ ) );
}


///////////////////////////////////////////////////////////////////////////////
//	EntityFloatEnumProxy: A helper class to access entity ENUM FLOAT properties
///////////////////////////////////////////////////////////////////////////////
EntityFloatEnumProxy::EntityFloatEnumProxy( BasePropertiesHelper* props, int index, BW::map<float,int> enumMap ) :
	props_( props ),
	index_( index ),
	useTransient_( false ),
	enumMapString_( enumMap )
{
	BW_GUARD;

	for( BW::map<float,int>::iterator iter = enumMapString_.begin(); iter != enumMapString_.end(); ++iter )
		enumMapInt_[ iter->second ] = iter->first;
}

int EntityFloatEnumProxy::get() const
{
	BW_GUARD;

	if (useTransient_)
		return transientValue_;
	else
		return enumMapString_.find( props_->propGetFloat( index_ ) )->second;
}

void EntityFloatEnumProxy::setTransient( int i )
{
	transientValue_ = i;
	useTransient_ = true;
}

bool EntityFloatEnumProxy::setPermanent( int i )
{
	BW_GUARD;

	useTransient_ = false;

	// set it
	bool ok = props_->propSetFloat( index_, enumMapInt_[ i ] );
	if (!ok) return false;

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );
	props_->pItem()->edPostModify();

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	return true;
}

BW::string EntityFloatEnumProxy::opName()
{
	BW_GUARD;

	return LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/SET_OP", props_->pItem()->edDescription().c_str(),
		props_->propName( index_ ) );
}


///////////////////////////////////////////////////////////////////////////////
//	EntityVector2Proxy: A helper class to access entity FLOAT properties
///////////////////////////////////////////////////////////////////////////////
EntityVector2Proxy::EntityVector2Proxy( BasePropertiesHelper* props, int index ) :
	props_( props ),
	index_( index ),
	useTransient_( false )
{
}

Vector2 EntityVector2Proxy::get() const
{
	BW_GUARD;

	if (useTransient_)
		return transientValue_;
	else
		return props_->propGetVector2( index_ );
}

void EntityVector2Proxy::setTransient( Vector2 v )
{
	transientValue_ = v;
	useTransient_ = true;
}

bool EntityVector2Proxy::setPermanent( Vector2 v )
{
	BW_GUARD;

	useTransient_ = false;

	// set it
	bool ok = props_->propSetVector2( index_, v );
	if (!ok) return false;

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	return true;
}

BW::string EntityVector2Proxy::opName()
{
	BW_GUARD;

	return LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/SET_OP",
		props_->pItem()->edDescription().c_str(),
		props_->propName( index_ ) );
}


///////////////////////////////////////////////////////////////////////////////
//	EntityVector2EnumProxy: A helper class to access entity ENUM FLOAT properties
///////////////////////////////////////////////////////////////////////////////
EntityVector2EnumProxy::EntityVector2EnumProxy( BasePropertiesHelper* props, int index, BW::map<Vector2,int> enumMap ) :
	props_( props ),
	index_( index ),
	useTransient_( false ),
	enumMapString_( enumMap )
{
	BW_GUARD;

	for( BW::map<Vector2,int>::iterator iter = enumMapString_.begin(); iter != enumMapString_.end(); ++iter )
		enumMapInt_[ iter->second ] = iter->first;
}

int EntityVector2EnumProxy::get() const
{
	BW_GUARD;

	if (useTransient_)
		return transientValue_;
	else
		return enumMapString_.find( props_->propGetVector2( index_ ) )->second;
}

void EntityVector2EnumProxy::setTransient( int i )
{
	transientValue_ = i;
	useTransient_ = true;
}

bool EntityVector2EnumProxy::setPermanent( int i )
{
	BW_GUARD;

	useTransient_ = false;

	// set it
	bool ok = props_->propSetVector2( index_, enumMapInt_[ i ] );
	if (!ok) return false;

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	return true;
}

BW::string EntityVector2EnumProxy::opName()
{
	BW_GUARD;

	return LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/SET_OP", props_->pItem()->edDescription().c_str(),
		props_->propName( index_ ) );
}


///////////////////////////////////////////////////////////////////////////////
//	EntityVector4Proxy: A helper class to access entity FLOAT properties
///////////////////////////////////////////////////////////////////////////////
EntityVector4Proxy::EntityVector4Proxy( BasePropertiesHelper* props, int index ) :
	props_( props ),
	index_( index ),
	useTransient_( false )
{
}

Vector4 EntityVector4Proxy::get() const
{
	BW_GUARD;

	if (useTransient_)
		return transientValue_;
	else
		return props_->propGetVector4( index_ );
}

void EntityVector4Proxy::setTransient( Vector4 v )
{
	transientValue_ = v;
	useTransient_ = true;
}

bool EntityVector4Proxy::setPermanent( Vector4 v )
{
	BW_GUARD;

	useTransient_ = false;

	// set it
	bool ok = props_->propSetVector4( index_, v );
	if (!ok) return false;

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	return true;
}

BW::string EntityVector4Proxy::opName()
{
	BW_GUARD;

	return LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/SET_OP",
		props_->pItem()->edDescription().c_str(),
		props_->propName( index_ ) );
}


///////////////////////////////////////////////////////////////////////////////
//	EntityVector4EnumProxy: A helper class to access entity ENUM FLOAT properties
///////////////////////////////////////////////////////////////////////////////
EntityVector4EnumProxy::EntityVector4EnumProxy( BasePropertiesHelper* props, int index, BW::map<Vector4,int> enumMap ) :
	props_( props ),
	index_( index ),
	useTransient_( false ),
	enumMapString_( enumMap )
{
	BW_GUARD;

	for( BW::map<Vector4,int>::iterator iter = enumMapString_.begin(); iter != enumMapString_.end(); ++iter )
		enumMapInt_[ iter->second ] = iter->first;
}

int EntityVector4EnumProxy::get() const
{
	BW_GUARD;

	if (useTransient_)
		return transientValue_;
	else
		return enumMapString_.find( props_->propGetVector4( index_ ) )->second;
}

void EntityVector4EnumProxy::setTransient( int i )
{
	transientValue_ = i;
	useTransient_ = true;
}

bool EntityVector4EnumProxy::setPermanent( int i )
{
	BW_GUARD;

	useTransient_ = false;

	// set it
	bool ok = props_->propSetVector4( index_, enumMapInt_[ i ] );
	if (!ok) return false;

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	return true;
}

BW::string EntityVector4EnumProxy::opName()
{
	BW_GUARD;

	return LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/SET_OP", props_->pItem()->edDescription().c_str(),
		props_->propName( index_ ) );
}

///////////////////////////////////////////////////////////////////////////////
//	EntityStringProxy: A helper class to access entity STRING properties
///////////////////////////////////////////////////////////////////////////////
EntityStringProxy::EntityStringProxy( BasePropertiesHelper* props, int index ) :
	props_( props ),
	index_( index )
{
}

BW::string EntityStringProxy::get() const
{
	BW_GUARD;

	return props_->propGetString( index_ );
}

void EntityStringProxy::setTransient( BW::string v )
{
	// we do absolutely nothing here
}

bool EntityStringProxy::setPermanent( BW::string v )
{
	BW_GUARD;

	// set it
	bool ok = props_->propSetString( index_, v );
	if (!ok) return false;

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );
	props_->pItem()->edPostModify();

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	return true;
}

BW::string EntityStringProxy::opName()
{
	BW_GUARD;

	return LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/SET_OP",
		props_->pItem()->edDescription().c_str(),
		props_->propName( index_ ) );
}


///////////////////////////////////////////////////////////////////////////////
//	EntityArrayProxy: A helper class to access entity ARRAY properties
///////////////////////////////////////////////////////////////////////////////

/*static*/ LinkGizmoPtr EntityArrayProxy::s_pGizmo_;
/*static*/ int EntityArrayProxy::s_gizmoCount_ = 0;

/**
 *	Constructor, initialises the ArrayPropertyHelper member used in setting and
 *	getting array item values from the original entity array property.
 *
 *	@param props	PropertyHelper that manages the entity's properties.
 *	@param dataType	Data type of the array property, must be derived from
 *					SequenceDataType.
 *	@param index	Index of the array property in the entity's properties list
 */
EntityArrayProxy::EntityArrayProxy( BasePropertiesHelper* props, DataTypePtr dataType, int index ) :
	props_( props ),
	dataType_( static_cast<SequenceDataType*>( dataType.getObject() ) ),
	index_( index ),
	alwaysShowGizmo_( false ),
	needsGizmo_( false )
{
	BW_GUARD;

	DataTypePtr itemDataType = &(dataType_->getElemType());
	PyObjectPtr ob( props->propGetPy( index_ ), PyObjectPtr::STEAL_REFERENCE );
	array_.init( props_->pItem(), itemDataType, ob.getObject() );

	PropertiesHelper* propsHelper = NULL;
	bool hasPatrolPathProperty = false;
	if ( props->pItem()->isEditorEntity() )
	{
		EditorChunkEntity* entity = static_cast<EditorChunkEntity*>( props->pItem() );
		propsHelper = entity->propHelper();
		if ( entity->patrolListPropIdx() != -1 )
			hasPatrolPathProperty = true;
	}
	else if ( props->pItem()->isEditorUserDataObject() )
	{
		EditorChunkUserDataObject* udo = static_cast<EditorChunkUserDataObject*>( props->pItem() );
		propsHelper = udo->propHelper();
	}

	if ( propsHelper != NULL )
	{
		if ( !hasPatrolPathProperty )
		{
			int linkPropCount = 0;
			int numProps = propsHelper->propCount();
			for (int i = 0; i < numProps && linkPropCount <= 1; ++i)
			{
				if ( (propsHelper->isUserDataObjectLink( i ) ||
					 propsHelper->isUserDataObjectLinkArray( i )) &&
					 propsHelper->pType()->property( i )->editable())
					linkPropCount++;
			}
			if ( linkPropCount == 1 )
				alwaysShowGizmo_ = true;
		}

		if ( propsHelper->isUserDataObjectLinkArray( index_ ) )
		{
			needsGizmo_ = true;
		}
	}
}


/**
 *	Destructor, destroys the array item properties
 */
EntityArrayProxy::~EntityArrayProxy()
{
	clearProperties();
}


/**
 *	This method deletes an item from the array. If it's a special case, such
 *	an Entity or a UserDataObject, it'll make all the checks to ensure that 
 *	links are kept consistent with other Entities/UDOs.
 *
 *	@param index	index of the PropertyItem in the array.
 *	@return			true if the item could be deleted, false otherwise.
 */
bool EntityArrayProxy::deleteArrayItem( int index )
{
	BW_GUARD;

	if ( array_.isUserDataObjectLink( index ) )
	{
		DataSectionPtr linkInfo = array_.propGet( index );
		BW::string guid = linkInfo->readString( "guid" );
		BW::string chunkId = linkInfo->readString( "chunkId" );
		bool validLink = !guid.empty() && !chunkId.empty();

		if ( validLink )
		{
			GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
			int16 gridX, gridZ;
			if ( dirMap->gridFromChunkName( chunkId, gridX, gridZ ) &&
				!EditorChunk::outsideChunkWriteable( gridX, gridZ ) )
			{
				// can't touch the other linker, so return
				ERROR_MSG(
					"The link to %s can't be removed because the chunk %s is not locked for writing.\n",
					guid.c_str(), chunkId.c_str() );
				return false;
			}
		}
	}

	array_.delItem( index );

	return true;
}


/**
 *	This method is passed to the property list using a BWFunctor, and it's
 *	called when an array item's "Delete" button is clicked.
 *
 *	@param index	index of the PropertyItem in the array.
 */
void EntityArrayProxy::delItem( int index )
{
	BW_GUARD;

	// Need to determine what linker object this property currently points to
	// and create an undo/redo operation that can reverse the deleting of this
	// item
	EditorChunkItemLinkable* linker = NULL;
	BW::string guid;
	BW::string chunkId;
	bool validLink = false;
	if ( array_.isUserDataObjectLink( index ) )
	{
		if ( props_->pItem()->isEditorEntity() )
			linker = static_cast<EditorChunkEntity*>( props_->pItem() )->chunkItemLinker();
		else if ( props_->pItem()->isEditorUserDataObject() )
			linker = static_cast<EditorChunkUserDataObject*>( props_->pItem() )->chunkItemLinker();

		DataSectionPtr linkInfo = array_.propGet( index );
		guid = linkInfo->readString( "guid" );
		chunkId = linkInfo->readString( "chunkId" );
		validLink = !guid.empty() && !chunkId.empty();

		if (validLink)
		{
			EditorChunkItemLinkable* targetEcil =
				WorldManager::instance().linkerManager().forceLoad(guid, chunkId);
			if (targetEcil && targetEcil->chunkItem() &&
				!targetEcil->chunkItem()->edIsEditable())
			{
				// Link end is frozen or its chunk is not locked for editing.
				ERROR_MSG( "The linked object '%s' is not editable\n", guid.c_str() );
				return;
			}

			UndoRedo::instance().add
				(
					new LinkerUpdateLinkOperation(linker, guid, chunkId)
				);
		}
	}
	
	UndoRedo::instance().add(new EntityArrayUndo( props_, index_ ));
    
	if ( !deleteArrayItem( index ) )
	{
		props_->resetSelUpdate( true );
		return;
	}

	// Update the linker object previously linked to by this property
	if (linker && validLink)
	{
		EditorChunkItemLinkable* targetEcil =
			WorldManager::instance().linkerManager().forceLoad(guid, chunkId);
		if (targetEcil)
		{
			WorldManager::instance().linkerManager().updateLink(linker, targetEcil);

			// Save the changes
			targetEcil->chunkItem()->edSave( targetEcil->chunkItem()->pOwnSect() );
			if ( targetEcil->chunkItem()->chunk() != NULL )
				WorldManager::instance().changedChunk( targetEcil->chunkItem()->chunk() );
		}
		else
		{
			WARNING_MSG( "A bad linkable node is being removed." );
		}
	}

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );
	props_->pItem()->edPostModify();

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	// resetSelUpdate changes the selection, which can shuffle things around
	// and decref the proxy, so let's keep it alive inside the method.
	EntityArrayProxyPtr localCopy( this );

	// Refresh item
	props_->refreshItem();

	UndoRedo::instance().barrier( LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/UNDO_DEL_ARRAY_ITEM"), false );
}


/**
 *	This method is called when the array property item is added to the actual
 *	property list. It inserts a PropertyItem in the property list for each
 *	array item.
 *
 *	@param parent	Parent (array) property.
 */
void EntityArrayProxy::elect( GeneralProperty* parent )
{
	BW_GUARD;

	// create properties
	createProperties( parent );

	for( size_t i = 0; i < properties_.size(); ++i )
	{
		properties_[i]->elect();
	}

	if (needsGizmo_)
	{
		if (s_gizmoCount_++ == 0)
		{
			s_pGizmo_ = new LinkGizmo(
					new EntityArrayLinkProxy( this, props_->pItem(), props_->propName( index_ ) ),
					NULL );
		}
		if (alwaysShowGizmo_)
		{
			GizmoManager::instance().addGizmo( s_pGizmo_ );
		}
	}
}


/**
 *	This method is called when the array property item is removed
 *	from the actual property list. It removes the array items' properties.
 *
 *	@param parent	Parent (array) property.
 */
void EntityArrayProxy::expel( GeneralProperty* parent )
{
	BW_GUARD;

	if (needsGizmo_ && --s_gizmoCount_ == 0 && s_pGizmo_)
	{
		GizmoManager::instance().removeGizmo( s_pGizmo_ );
		s_pGizmo_ = NULL;
	}

	for( size_t i = 0; i < properties_.size(); ++i )
	{
		properties_[i]->expel();
	}
	clearProperties();
}


/**
 *	This method is called when the array property item is selected
 *	by the user. It shows the linking gizmo.
 *
 *	@param parent	Parent (array) property.
 */
void EntityArrayProxy::select( GeneralProperty* parent )
{
	BW_GUARD;

	if (needsGizmo_ && s_pGizmo_)
	{
		GizmoSetPtr set = new GizmoSet();
		s_pGizmo_->update( new EntityArrayLinkProxy(
					this, props_->pItem(), props_->propName( index_ ) ), NULL );
		set->add( s_pGizmo_ );
		GizmoManager::instance().forceGizmoSet( set );
	}
}


/**
 *	This method returns the properties helper used by this proxy.
 *
 *	@return		properties helper used by this proxy.
 */
BasePropertiesHelper* EntityArrayProxy::propsHelper()
{
	return props_;
}


/**
 *	This method returns the index of the array in the properties.
 *
 *	@return		index of the array in the properties.
 */
int EntityArrayProxy::index()
{
	return index_;
}


/**
 *	This method returns the array properties helper used by this proxy.
 *
 *	@return		array properties helper used by this proxy.
 */
ArrayPropertiesHelper* EntityArrayProxy::arrayPropsHelper()
{
	return &array_;
}


/**
 *	This method adds an item to the array using the ArrayPropertiesHelper.
 *
 *	@return		true if successful, false otherwise.
 */
bool EntityArrayProxy::addItem()
{
	BW_GUARD;

    UndoRedo::instance().add(new EntityArrayUndo( props_, index_ ));

	array_.addItem();

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );
	props_->pItem()->edPostModify();

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	UndoRedo::instance().barrier( LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/UNDO_ADD_ARRAY_ITEM"), false );

	return true;
}


/**
 *	Structure used by EntityArrayProxy::delItems
 */
namespace
{
	struct Link
	{
		BW::string guid;
		BW::string chunkId;
	};
}


/**
 *	This method deletes all items from the array iterating using the
 *	ArrayPropertiesHelper.
 *
 *	@return		true if successful, false otherwise.
 */
bool EntityArrayProxy::delItems()
{
	BW_GUARD;

	// Need to determine what linker objects this property array points to
	// and create an undo/redo operation for each.  These linker objects
	// will also need to be updated after the array has been cleared below
	bool validLink = false;
	Link* linkArray = NULL;
	int linkArraySize = 0;
	EditorChunkItemLinkable* linker = NULL;
	if ( array_.isUserDataObjectLink(0) )
	{
		linkArraySize = array_.propCount();
		linkArray = new Link[linkArraySize];

		if ( props_->pItem()->isEditorEntity() )
			linker = static_cast<EditorChunkEntity*>( props_->pItem() )->chunkItemLinker();
		else if ( props_->pItem()->isEditorUserDataObject() )
			linker = static_cast<EditorChunkUserDataObject*>( props_->pItem() )->chunkItemLinker();

		// First make sure all items about to be deleted are editable.
		for (int i = 0; i < array_.propCount(); i++)
		{
			DataSectionPtr linkInfo = array_.propGet( i );
			BW::string guid = linkInfo->readString( "guid" );
			BW::string chunkId = linkInfo->readString( "chunkId" );
			if (!guid.empty() && !chunkId.empty())
			{
				EditorChunkItemLinkable* targetEcil =
					WorldManager::instance().linkerManager().forceLoad(guid, chunkId);
				if (targetEcil && targetEcil->chunkItem() &&
					!targetEcil->chunkItem()->edIsEditable())
				{
					// Link end is frozen or its chunk is not locked for editing.
					ERROR_MSG( "The linked object '%s' is not editable\n", guid.c_str() );
					bw_safe_delete_array( linkArray );
					return false;
				}
			}
		}

		for (int i = 0; i < array_.propCount(); i++)
		{
			DataSectionPtr linkInfo = array_.propGet( i );
			BW::string guid = linkInfo->readString( "guid" );
			BW::string chunkId = linkInfo->readString( "chunkId" );
			if (!guid.empty() && !chunkId.empty())
			{
				validLink = true;

				linkArray[i].guid = guid;
				linkArray[i].chunkId = chunkId;

				UndoRedo::instance().add
					(
						new LinkerUpdateLinkOperation(linker, guid, chunkId)
					);
			}
		}
	}
    UndoRedo::instance().add(new EntityArrayUndo( props_, index_ ));

	while( array_.propCount() > 0 )
		if ( !deleteArrayItem( array_.propCount() - 1 ) )
			break;

	// Update the linkers the properties were pointing to
	if (linker && validLink)
	{
		for (int i = 0; i < linkArraySize; i++)
		{
			if (!linkArray[i].guid.empty() && !linkArray[i].chunkId.empty())
			{
				EditorChunkItemLinkable* targetEcil =
					WorldManager::instance().linkerManager().forceLoad(linkArray[i].guid, linkArray[i].chunkId);		
				WorldManager::instance().linkerManager().updateLink(linker, targetEcil);

				// Save the changes
				targetEcil->chunkItem()->edSave( targetEcil->chunkItem()->pOwnSect() );
				if ( targetEcil->chunkItem()->chunk() != NULL )
					WorldManager::instance().changedChunk( targetEcil->chunkItem()->chunk() );
			}
		}
	}

	// Delete link array if it was created
	bw_safe_delete_array(linkArray);

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );
	props_->pItem()->edPostModify();

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	// resetSelUpdate changes the selection, which can shuffle things around
	// and decref the proxy, so let's keep it alive inside the method.
	EntityArrayProxyPtr localCopy( this );

	// Refresh item
	props_->refreshItem();

	UndoRedo::instance().barrier( LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/UNDO_CLEAR_ARRAY"), false );

	return true;
}


/**
 *	This method creates a property item in the property list for each array
 *	item by using the EntityPropertyParser.
 *
 *	@param parent	Parent (array) property.
 */
void EntityArrayProxy::createProperties( GeneralProperty* parent )
{
	BW_GUARD;

	clearProperties();

	int cnt = array_.propCount();
	if ( cnt < 0 )
		return;

	DataTypePtr elemDataType = &(dataType_->getElemType());
	for( int i = 0; i < cnt; ++i )
	{
		BW::stringstream name;
		name << parent->name().c_str() << "[" << i << "]";
		GeneralProperty* prop = elemDataType->createEditorProperty(
			name.str(), static_cast<ChunkItem*>( array_.pItem() ), i );

		if ( !prop )
		{
			EntityPropertyParserPtr parser = EntityPropertyParser::create(
				NULL, Name( name.str() ), elemDataType, NULL );
			if ( parser )
			{
				prop = parser->createProperty(
					&array_, i, Name( name.str() ), elemDataType, NULL, NULL );
			}
		}
		if ( prop )
		{
			prop->setGroup(
				parent->name()[0] ? parent->getGroup().str() + parent->name().c_str() :
				parent->getGroup() );
			properties_.push_back( prop );
		}
	}
}


/**
 *	This method deletes all the array items' properties.
 */
void EntityArrayProxy::clearProperties()
{
	BW_GUARD;

	for( size_t i = 0; i < properties_.size(); ++i )
	{
		properties_[i]->deleteSelf();
	}
	properties_.clear();
}


///////////////////////////////////////////////////////////////////////////////
// EntityArrayLinkProxy : handles linking when the array property is selected.
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor, initialises member variables.
 */
EntityArrayLinkProxy::EntityArrayLinkProxy( EntityArrayProxy* arrayProxy, EditorChunkItem* item, const BW::string& propName ) :
	arrayProxy_ ( arrayProxy ),
	item_( item ),
	propName_( propName )
{
}


/**
 *	This method returns the link type.
 *
 *	@return		The link type.
 */
EntityArrayLinkProxy::LinkType EntityArrayLinkProxy::linkType() const
{
	BW_GUARD;

	if ( item_->isEditorUserDataObject() &&
		static_cast<EditorChunkUserDataObject*>(item_)->showAddGizmo( propName_ ) )
		return (LinkType)( LT_ADD | LT_LINK );
	else
		return LT_LINK;
}


/**
 *	This method is only an empty implementation of a pure virtual method.
 *
 *	@return		NULL
 */
MatrixProxyPtr EntityArrayLinkProxy::createCopyForLink()
{
	BW_GUARD;

	//TODO: add undo support

    // Create a copy of node_:
    SmartPointer<EditorChunkUserDataObject> newNode = new EditorChunkUserDataObject();
    DataSectionPtr newSection = new XMLSection("copy");
    newSection->copy( item_->pOwnSect() );
	newSection->delChild( "guid" );
	newSection->delChild( "backLinks" );

	// delete the link property, or the whole array if it's an array of links
	DataSectionPtr propsSection = newSection->openSection( "properties" );
	if ( propsSection != NULL )
		propsSection->delChild( propName_ );

	// and load it.
    newNode->load( newSection, item_->chunk() );
    item_->chunk()->addStaticItem( newNode );
    newNode->edTransform( item_->edTransform(), false );

	ChunkItemPtr pItem = (ChunkItem *)newNode.get();
	
	UndoRedo::instance().add(
		new ChunkItemExistenceOperation( pItem, NULL ) );

	// set the link in the current node to point to the newNode
	ArrayPropertiesHelper* propHelper = arrayProxy_->arrayPropsHelper();
	propHelper->addItem();
	int newItemIndex = propHelper->propCount() - 1;

	PropertyIndex propIdx( arrayProxy_->index() );
	propIdx.append( newItemIndex );

	WorldManager::instance().linkerManager().addLink(
		static_cast<EditorChunkUserDataObject*>( item_ )->chunkItemLinker(), newNode->chunkItemLinker(), propIdx );
	
	PyObjectPtr ob( arrayProxy_->arrayPropsHelper()->propGetPy( newItemIndex ), PyObjectPtr::STEAL_REFERENCE );
	bw_utf8tow( UserDataObjectLinkDataType::asString( ob.getObject() ), linkValue_ );

	// Set the new node as the selection:
	int curSel = PropTable::table()->propertyList()->GetCurSel();
    BW::vector<ChunkItemPtr> items;
    items.push_back( newNode );
    WorldManager::instance().setSelection( items );
	PropTable::table()->propertyList()->selectItem( curSel, false );

	newNode->propHelper()->resetSelUpdate( true );

    // Return a ChunkItemMatrix for the new node so that its position can be
    // edited:
    ChunkItemMatrix *result = new ChunkItemMatrix( newNode );
    result->recordState();
    return result;
}


/**
 *	This method determines if 'this->item' can be linked to the destination
 *	item in the toolLocator.
 *
 *	@param toolLocator	Tool locator with the link destination.
 *	@return				TS_CAN_LINK if it can be linked, TS_CANT_LINK if it
 *						cannot be linked, TS_NO_TARGET if there is no valid
 *						object to link to under the locator.
 */
LinkProxy::TargetState EntityArrayLinkProxy::canLinkAtPos(ToolLocatorPtr toolLocator) const
{
	BW_GUARD;

	UserDataObjectLinkLocator *locator =
		( UserDataObjectLinkLocator *)toolLocator.getObject();

    if (locator->chunkItem() == NULL || !item_->edIsEditable())
		return TS_NO_TARGET;

	if ( locator->chunkItem().getObject() == item_ )
		return TS_NO_TARGET;	// avoid linking to ourselves

	if (!locator->chunkItem()->edIsEditable())
		return TS_CANT_LINK;

	EditorChunkItem *other = static_cast<EditorChunkItem*>( locator->chunkItem().getObject() );
	if ( other == NULL || !other->isEditorUserDataObject() )
		return TS_NO_TARGET;

	EditorChunkUserDataObject *ecudo = static_cast<EditorChunkUserDataObject*>( other );

	// check with them if they are linkable
	if ( item_->isEditorEntity() )
	{
		EditorChunkEntity *entity =  static_cast<EditorChunkEntity*>( item_ );
		PyObjectPtr ecudoInfo( ecudo->infoDict(), PyObjectPtr::STEAL_REFERENCE );
		if ( !entity->canLinkTo(
				arrayProxy_->propsHelper()->propName( arrayProxy_->index() ),
				ecudoInfo.getObject() ) )
			return TS_CANT_LINK;
	}
	else if ( item_->isEditorUserDataObject() )
	{
		EditorChunkUserDataObject *udo =  static_cast<EditorChunkUserDataObject*>( item_ );
		if ( !udo->canLinkTo(
				arrayProxy_->propsHelper()->propName( arrayProxy_->index() ),
				ecudo ) )
			return TS_CANT_LINK;
	}

	return TS_CAN_LINK;
}


/**
 *	This method creates a link between 'this->item' and the destination item
 *	specified in the tool locator, by adding a new item into the array.
 *
 *	@param toolLocator	Tool locator with the link destination.
 *	@return				True if successful.
 */
void EntityArrayLinkProxy::createLinkAtPos(ToolLocatorPtr toolLocator)
{
	BW_GUARD;

    UserDataObjectLinkLocator *locator =
        (UserDataObjectLinkLocator *)toolLocator.getObject();
    ChunkItemPtr chunkItem = locator->chunkItem();
    if (chunkItem == NULL)
        return;

    // The linked udo
    EditorChunkItem *other = static_cast<EditorChunkItem*>( chunkItem.getObject() );
    if (!other->isEditorUserDataObject())
        return;
    EditorChunkUserDataObject *ecudo = static_cast<EditorChunkUserDataObject*>( other );

    if (!ecudo->edIsEditable())
		return;

	EditorChunkItemLinkable* linker = NULL;
	if ( item_->isEditorEntity() )
		linker = static_cast<EditorChunkEntity*>( item_ )->chunkItemLinker();
	else if ( item_->isEditorUserDataObject() )
		linker = static_cast<EditorChunkUserDataObject*>( item_ )->chunkItemLinker();
	else
		return;

	// TODO: Create undo point
    // UndoRedo::instance().add(new EntityArrayUndo( arrayProxy_->propsHelper(), arrayProxy_->index() ));
	// UndoRedo::instance().barrier( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/UNDO_ADD_ARRAY_ITEM"), false );

	// add the new element to the array and link it
	ArrayPropertiesHelper* propHelper = arrayProxy_->arrayPropsHelper();
	propHelper->addItem();
	int newItemIndex = propHelper->propCount() - 1;

	PropertyIndex propIdx( arrayProxy_->index() );
	propIdx.append( newItemIndex );

	WorldManager::instance().linkerManager().addLink(
		linker, ecudo->chunkItemLinker(), propIdx );

	PyObjectPtr ob( arrayProxy_->arrayPropsHelper()->propGetPy( newItemIndex ), PyObjectPtr::STEAL_REFERENCE );
	bw_utf8tow( UserDataObjectLinkDataType::asString( ob.getObject() ), linkValue_ );

	UndoRedo::instance().barrier(
		LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/PROPERTIES/STATION_NODE_LINK_PROXY/LINK_NODES" ), false);

	propHelper->resetSelUpdate( true );
}


/**
 *	This method creates the appropriate locator for linking the array.
 *
 *	@return		Tool locator that receives input from the user.
 */
ToolLocatorPtr EntityArrayLinkProxy::createLocator() const
{
	BW_GUARD;

	BW::string nlinkValue;
	bw_wtoutf8( linkValue_, nlinkValue );
    return 
        ToolLocatorPtr
        (
            new  UserDataObjectLinkLocator(nlinkValue, UserDataObjectLinkLocator::LOCATE_USER_DATA_OBJECTS), 
            true
        );
}


///////////////////////////////////////////////////////////////////////////////
//	EntityStringEnumProxy: A helper class to access entity ENUM STRING properties
///////////////////////////////////////////////////////////////////////////////
EntityStringEnumProxy::EntityStringEnumProxy( BasePropertiesHelper* props, int index, BW::map<BW::string,int> enumMap ) :
	props_( props ),
	index_( index ),
	useTransient_( false ),
	enumMapString_( enumMap )
{
	BW_GUARD;

	for( BW::map<BW::string,int>::iterator iter = enumMapString_.begin(); iter != enumMapString_.end(); ++iter )
		enumMapInt_[ iter->second ] = iter->first;
}

int EntityStringEnumProxy::get() const
{
	BW_GUARD;

	if (useTransient_)
		return transientValue_;
	else
		return enumMapString_.find(
			props_->propGetString( index_ ) )->second;
}

void EntityStringEnumProxy::setTransient( int i )
{
	transientValue_ = i;
	useTransient_ = true;
}

bool EntityStringEnumProxy::setPermanent( int i )
{
	BW_GUARD;

	useTransient_ = false;

	// set it
	bool ok = props_->propSetString( index_, enumMapInt_[ i ] );
	if (!ok) return false;

	// flag the chunk as having changed
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );
	props_->pItem()->edPostModify();

	// update its data section
	props_->pItem()->edSave( props_->pItem()->pOwnSect() );

	return true;
}

BW::string EntityStringEnumProxy::opName()
{
	BW_GUARD;

	return LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/SET_OP",
		props_->pItem()->edDescription().c_str(),
		props_->propName( index_ ) );
}


///////////////////////////////////////////////////////////////////////////////
//	EntityPythonProxy: A helper class to access the entity specific properties
///////////////////////////////////////////////////////////////////////////////
EntityPythonProxy::EntityPythonProxy( BasePropertiesHelper* props, int index ) :
	props_( props ),
	index_( index )
{
}

PyObjectPtr EntityPythonProxy::get() const
{
	BW_GUARD;

	return PyObjectPtr( props_->propGetPy( index_ ), PyObjectPtr::STEAL_REFERENCE );
}

void EntityPythonProxy::setTransient( PyObjectPtr v )
{
	// we do absolutely nothing here
}

bool EntityPythonProxy::setPermanent( PyObjectPtr v )
{
	BW_GUARD;

	if( props_->propSetPy(
			index_, v.getObject() ) )
	{
		WorldManager::instance().changedChunk( props_->pItem()->chunk() );
		props_->pItem()->edPostModify();

		// update its data section
		props_->pItem()->edSave( props_->pItem()->pOwnSect() );
	}
	return true;
}

BW::string EntityPythonProxy::opName()
{
	BW_GUARD;

	return LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_ENTITY_PROXY/SET_OP",
		props_->pItem()->edDescription().c_str(),
		props_->propName( index_ ) );
}

BW_END_NAMESPACE

