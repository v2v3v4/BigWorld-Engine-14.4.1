#ifndef EDITOR_CHUNK_ITEM_TREE_NODE_HPP
#define EDITOR_CHUNK_ITEM_TREE_NODE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "chunk/chunk_item_tree_node.hpp"
#include "gizmo/undoredo.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkItemTreeNodeOperation
// -----------------------------------------------------------------------------


class ChunkItemTreeNodeOperation : public UndoRedo::Operation
{
public:
	ChunkItemTreeNodeOperation( ChunkItemTreeNodePtr n, bool removed )
		: UndoRedo::Operation( size_t(typeid(ChunkItemTreeNodeOperation).name()) )
		, node_( n )
		, removed_( removed )
	{
		BW_GUARD;

		// store the markers the cluster references
		 node_->getCopyOfChildren(children_);
		 parent_ = node_->getParent();
		 addChunk( node_->chunk() );
		 for (BW::list<ChunkItemTreeNodePtr>::iterator it = children_.begin();
			it != children_.end();
			it++)
		{
			addChunk( (*it)->chunk() );
		}
	}

	virtual void undo()
	{
		BW_GUARD;

		// First add a redo of this undo operation
		UndoRedo::instance().add( new ChunkItemTreeNodeOperation(node_, !removed_, children_, parent_) );

		if (removed_)
		{
			// add links to clusters
			for (BW::list<ChunkItemTreeNodePtr>::iterator it = children_.begin();
				it != children_.end();
				it++)
			{
				(*it)->setParent(node_);
			}

			// tell parent about us
			node_->setParent(parent_);
		}
		else
		{
			// removed links to clusters
			for (BW::list<ChunkItemTreeNodePtr>::iterator it = children_.begin();
				it != children_.end();
				it++)
			{
				MF_ASSERT((*it)->getParent());
				(*it)->setParent(NULL);
			}

			// tell parent about us
			node_->setParent(NULL);
		}
	}

	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		BW_GUARD;

		const ChunkItemTreeNodeOperation& o = static_cast<const ChunkItemTreeNodeOperation&>( oth );

		if (node_ != o.node_)
			return false;

		if (removed_ != o.removed_)
			return false;

		if (parent_ != o.parent_) 
			return false;

		for (BW::list<ChunkItemTreeNodePtr>::const_iterator it1 = children_.begin();
            it1 != children_.end();
			it1++)
		{
			bool found = false;
			for (BW::list<ChunkItemTreeNodePtr>::const_iterator it2 = o.children_.begin();
				it2 != o.children_.end();
				it2++)
			{
				if (*it1 == *it2)
					found = true;
			}	
			if (!found)
				return false;
		}

		return true;
	}

private:
	ChunkItemTreeNodeOperation( ChunkItemTreeNodePtr n, bool removed, 
		const BW::list<ChunkItemTreeNodePtr>& c, ChunkItemTreeNodePtr p )
		: UndoRedo::Operation( size_t(typeid(ChunkItemTreeNodeOperation).name()) )
		, removed_( removed )
		, node_( n )
		, parent_( p )
	{
		BW_GUARD;

		addChunk( node_->chunk() );
		children_.clear();
		for (BW::list<ChunkItemTreeNodePtr>::const_iterator it = c.begin();
            it != c.end();
			it++)
		{
			children_.push_back(*it);
			addChunk( (*it)->chunk() );
		}	
	}

	ChunkItemTreeNodePtr node_;
	bool removed_;
	BW::list<ChunkItemTreeNodePtr> children_;
	ChunkItemTreeNodePtr parent_;
};


class EditorChunkItemTreeNode : public ChunkItemTreeNode
{
};


BW_END_NAMESPACE

#endif // EDITOR_CHUNK_ITEM_TREE_NODE_HPP
