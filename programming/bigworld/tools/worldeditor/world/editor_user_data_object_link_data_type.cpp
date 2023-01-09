#include "pch.hpp"
#include "worldeditor/world/user_data_object_link_proxy.hpp"
#include "worldeditor/world/editor_chunk_item_linker.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/world/items/editor_chunk_user_data_object.hpp"
#include "chunk/user_data_object_link_data_type.hpp"
#include "worldeditor/editor/link_property.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This method returns the appropriate editor property for a UDO_REF link.
 *	This method is declared in "user_data_object_link_data_type" as editor only
 *  and is not implemented in the user_data_object_link_data_type.cpp file, so
 *  EDITOR_ENABLED apps must include this file to use this functionality.
 *
 *  @param name					Name of the property.
 *  @param item					Chunk item (must be a linkable object).
 *  @param editorPropertyId		Ignored.
 *	@return						Editor property for the link.
 */
GeneralProperty * UserDataObjectLinkDataType::createEditorProperty(
	const BW::string& name, ChunkItem* item, int editorPropertyId )
{
	BW_GUARD;

	EditorChunkItemLinkable* linker = NULL;
	bool alwaysShow = false;
	if ( item->isEditorEntity() )
	{
		EditorChunkEntity* entity =
			static_cast<EditorChunkEntity*>( item );

		// show the gizmo always for the first link property.
		alwaysShow = !entity->firstLinkFound();
		entity->firstLinkFound( true );
		linker = entity->chunkItemLinker();
	}
	else if ( item->isEditorUserDataObject() )
	{
		EditorChunkUserDataObject* udo =
			static_cast<EditorChunkUserDataObject*>( item );

		// show the gizmo always for the first link property.
		alwaysShow = !udo->firstLinkFound();
		udo->firstLinkFound( true );
		linker = udo->chunkItemLinker();
	}
	else
	{
		// Should never get here.
		MF_ASSERT( 0 && "Creating a UserDataObjectLinkDataType from an item that "
			"it's neither a EditorChunkEntity nor a EditorChunkUserDataObject." );
		return NULL;
	}

	return new LinkProperty
		(
			Name(name),
			new UserDataObjectLinkProxy(name.c_str(), linker),
			NULL,	// use the selection's matrix
			alwaysShow
		);

	// Should never reach here
	MF_ASSERT( 0 && "Creating a UserDataObjectLinkDataType from an item that "
		"it's neither a EditorChunkEntity nor a EditorChunkUserDataObject." );
	return NULL;
}

BW_END_NAMESPACE

