#include "pch.hpp"
#include "worldeditor/world/items/editor_chunk_user_data_object_link.hpp"
#include "worldeditor/world/items/editor_chunk_user_data_object.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_item_linker_manager.hpp"

BW_BEGIN_NAMESPACE

BW::vector<BW::string> EditorChunkUserDataObjectLink::edCommand(
	BW::string const &path ) const
{
	BW_GUARD;

	BW::vector<BW::string> commands;

	ChunkItem * startCI = startItem().getObject();
	ChunkItem * endCI = endItem().getObject();

	if (startCI->edIsEditable() && endCI->edIsEditable())
	{
		// TODO:UNICODE: Verify this is what is requiered...
		commands.push_back(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/DELETE"));

		if (!startCI->isEditorUserDataObject() || !endCI->isEditorUserDataObject())
			return commands;

		EditorChunkUserDataObject* start =
			static_cast<EditorChunkUserDataObject*>( startItem().getObject() );
		EditorChunkUserDataObject* end =
			static_cast<EditorChunkUserDataObject*>( endItem().getObject() );
		// At the moment, it doesn't matter if "getLinkCommands" is called from
		// start or end, and it only works if both UDOs are the same type.
		start->getLinkCommands( commands, end );
	}

	return commands;
}


bool EditorChunkUserDataObjectLink::edExecuteCommand(
	BW::string const &path, BW::vector<BW::string>::size_type index )
{
	BW_GUARD;

	ChunkItem * startCI = startItem().getObject();
	ChunkItem * endCI = endItem().getObject();

	if (startCI->isEditorEntity() && endCI->isEditorUserDataObject())
	{
		// Links to entities can only be deleted
		deleteCommand();

		BW::vector<ChunkItemPtr> emptyList;
		WorldManager::instance().setSelection(emptyList, true);
		return true;
	}
	else if (startCI->isEditorUserDataObject() && endCI->isEditorUserDataObject())
	{
		if (index == 0)
		{
			// Clicked "Delete", so delete the link
			deleteCommand();
		}
		else if ( index >= 1 )
		{
			// Clicked a python command, so execute it.
			EditorChunkUserDataObject* start =
				static_cast<EditorChunkUserDataObject*>( startItem().getObject() );
			EditorChunkUserDataObject* end =
				static_cast<EditorChunkUserDataObject*>( endItem().getObject() );
			// At the moment, it doesn't matter if "executeLinkCommand" is called from
			// start or end, and it only works if both UDOs are the same type.
			MF_ASSERT( index - 1 <= INT_MAX );
			start->executeLinkCommand( ( int ) ( index - 1 ), end );
		}
		else
		{
			return false;
		}

		BW::vector<ChunkItemPtr> emptyList;
		WorldManager::instance().setSelection(emptyList, true);
		return true;
	}

	return false;
}


void EditorChunkUserDataObjectLink::deleteCommand()
{
	BW_GUARD;

	ChunkItem * startCI = startItem().getObject();
	ChunkItem * endCI = endItem().getObject();

	EditorChunkItemLinkable* startLinker = 0;
	EditorChunkItemLinkable* endLinker = 0;

	if (startCI->isEditorEntity())
		startLinker = static_cast<EditorChunkEntity*>(startCI)->chunkItemLinker();
	else if (startCI->isEditorUserDataObject())
		startLinker = static_cast<EditorChunkUserDataObject*>(startCI)->chunkItemLinker();

	if (endCI->isEditorEntity())
		endLinker = static_cast<EditorChunkEntity*>(endCI)->chunkItemLinker();
	else if (endCI->isEditorUserDataObject())
		endLinker = static_cast<EditorChunkUserDataObject*>(endCI)->chunkItemLinker();

	// Inform the linker manager that all links are to be deleted
	WorldManager::instance().linkerManager().deleteAllLinks(startLinker, endLinker);

	UndoRedo::instance().barrier( 
		LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/PROPERTIES/STATION_NODE_LINK_PROXY/LINK_NODES" ), false);
}


/**
 *  This method prevents collisions against this kind of links
 *
 *	@param source	Starting point of the collision ray
 *	@param dir		Direction of the collision ray
 *	@param wt		Triangle to test, in world coordinates
 *	@return			distance from 'source' to the collision point.
 */
/*virtual*/ float EditorChunkUserDataObjectLink::collide(
    const Vector3& source, const Vector3& dir, WorldTriangle& wt ) const
{
	BW_GUARD;

	// check to see that both sides of the link are writeable
	ChunkItem * start = startItem().getObject();
	ChunkItem * end = endItem().getObject();
	if ( start == NULL || end == NULL ||
		!start->edIsEditable() || !end->edIsEditable())
	{
		return std::numeric_limits<float>::max();
	}

	return EditorChunkLink::collide( source, dir, wt );
}

BW_END_NAMESPACE

