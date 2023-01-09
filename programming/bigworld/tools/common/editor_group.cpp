#include "pch.hpp"
#include "editor_group.hpp"
#include "chunk/chunk_item.hpp"



DECLARE_DEBUG_COMPONENT2( "WorldEditor", 2 );

BW_BEGIN_NAMESPACE

EditorGroup EditorGroup::s_rootGroup_ = EditorGroup("");


EditorGroup::EditorGroup( const BW::string& name )
	: name_( name )
	, parent_( NULL )
	, treeItemHandle_( 0 )
{
	MF_ASSERT( name.find_first_of( "/" ) == BW::string::npos );
}

EditorGroup::~EditorGroup()
{
	BW_GUARD;

	BW::vector<EditorGroup*>::iterator k = children_.begin();
	for (; k != children().end(); ++k)
		bw_safe_delete(*k);

	children_.clear();
}

void EditorGroup::name( const BW::string& n )
{
	BW_GUARD;

	name_ = n;

	pathChanged();
}

BW::string EditorGroup::fullName() const
{
	BW_GUARD;

	if (!parent_)
		return "";

	if (parent_ == rootGroup())
		return name_;

	return parent_->fullName() + "/" + name_;
}

void EditorGroup::enterGroup( ChunkItem* item )
{
	BW_GUARD;

	items_.push_back( item );

	//TODO : put back in when page scene is working correctly
	//PageScene::instance().addItemToGroup( item, this );
}

void EditorGroup::leaveGroup( ChunkItem* item )
{
	BW_GUARD;

	BW::vector<ChunkItem*>::iterator i =
		std::find( items_.begin(), items_.end(), item );

	MF_ASSERT( i != items_.end() );

	items_.erase( i );

	//TODO : put back in when page scene is working correctly
	//PageScene::instance().removeItemFromGroup( item, this );
}

EditorGroup* EditorGroup::findOrCreateGroup( const BW::string& fullName )
{
	BW_GUARD;

	EditorGroup* current = &s_rootGroup_;
	BW::string currentName = fullName;
	char* currentPart = strtok( (char*) currentName.c_str(), "/" );


	while (1)
	{
		if (currentPart == NULL)
			return current;

		current = current->findOrCreateChild( currentPart );
		currentPart = strtok( NULL, "/" );
	}
}

EditorGroup* EditorGroup::findOrCreateChild( const BW::string& name )
{
	BW_GUARD;

	MF_ASSERT( name.find_first_of( "/" ) == BW::string::npos );

	BW::vector<EditorGroup*>::iterator i;
	for (i = children_.begin(); i != children_.end(); ++i)
		if ((*i)->name() == name)
			return *i;

	EditorGroup* eg = new EditorGroup( name );
	eg->parent_ = this;
	children_.push_back( eg );
	return eg;
}

void EditorGroup::moveChunkItemsTo( EditorGroup* group )
{
	BW_GUARD;

	BW::vector<ChunkItem*> groupItems = items_;
	BW::vector<EditorGroup*> groups = children_;

	BW::vector<ChunkItem*>::const_iterator j = groupItems.begin();
	for (; j != groupItems.end(); ++j)
	{
		ChunkItem* chunkItem = *j;

		chunkItem->edGroup( group );
	}

	BW::vector<EditorGroup*>::const_iterator k = groups.begin();
	for (; k != groups.end(); ++k)
	{
		EditorGroup* g = *k;

		g->moveChunkItemsTo( group );
	}
}

void EditorGroup::removeChildGroup( EditorGroup* group )
{
	BW_GUARD;

	MF_ASSERT( group != this );

	group->moveChunkItemsTo( this );

	MF_ASSERT( group->items().empty() && group->children().empty() );

	BW::vector<EditorGroup*>::iterator i = std::find( children_.begin(), children_.end(), group );

	if (i == children_.end())
		return;

	children_.erase( i );

	bw_safe_delete(group);
}

void EditorGroup::pathChanged()
{
	BW_GUARD;

	BW::vector<ChunkItem*> groupItems = items_;
	BW::vector<EditorGroup*> groups = children_;

	// Notify out items their group path has changed
	BW::vector<ChunkItem*>::const_iterator j = groupItems.begin();
	for (; j != groupItems.end(); ++j)
	{
		ChunkItem* chunkItem = *j;

		chunkItem->edGroup( this );
	}

	// Ask our child groups to notify their items too
	BW::vector<EditorGroup*>::const_iterator k = groups.begin();
	for (; k != groups.end(); ++k)
	{
		EditorGroup* g = *k;

		g->pathChanged();
	}
}
BW_END_NAMESPACE

