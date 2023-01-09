#ifndef EDITOR_GROUP_HPP
#define EDITOR_GROUP_HPP

#include "cstdmf/bw_string.hpp"

#include "chunk/chunk_item.hpp"

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class EditorGroup
{
public:
	EditorGroup( const BW::string& name );
	~EditorGroup();

	const BW::string& name() const { return name_; }
	void name( const BW::string& n );
	BW::string fullName() const;
	EditorGroup* parent() const { return parent_; }

	const BW::vector<EditorGroup*>& children() const { return children_; }
	const BW::vector<ChunkItem*>& items() const { return items_; }

	void enterGroup( ChunkItem* item );
	void leaveGroup( ChunkItem* item );

	void treeItemHandle( uint32 hItem ) { treeItemHandle_ = hItem; }
	uint32 treeItemHandle() const { return treeItemHandle_; }

	/**
	 *	Return the child group with the given name, slashes aren't allowed
	 */
	EditorGroup* findOrCreateChild( const BW::string& name );

	/** Remove the given group from us, and delete it for good measure */
	void removeChildGroup( EditorGroup* group );

	/**
	 *	Return the group with the given name, in the form of a/b/c
	 */
	static EditorGroup* findOrCreateGroup( const BW::string& fullName );

	static EditorGroup* rootGroup() { return &s_rootGroup_; }
private:
	BW::string name_;
	EditorGroup* parent_;
	BW::vector<ChunkItem*> items_;
	BW::vector<EditorGroup*> children_;

	uint32 treeItemHandle_;

	/** Call when the parent or name of this group has changed */
	void pathChanged();

	/** Recursively move all our items to the given group */
	void moveChunkItemsTo( EditorGroup* group );

	static EditorGroup s_rootGroup_;
};

BW_END_NAMESPACE
#endif // EDITOR_GROUP_HPP
