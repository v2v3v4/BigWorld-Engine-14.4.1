#include "pch.hpp"
#include "chunk_binding.hpp"
#include "chunk_item_tree_node.hpp"
#include "chunk.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkBinding
// -----------------------------------------------------------------------------

/**
 *	This static method creates a marker from the input section and adds
 *	it to the given chunk.
 */
ChunkItemFactory::Result ChunkBinding::create( Chunk* pChunk, DataSectionPtr pSection )
{
	ChunkBinding* b = new ChunkBinding();
	if (!b->load( pSection ))
	{
		bw_safe_delete(b);
		return ChunkItemFactory::Result( NULL, "Failed to create marker binding" );
	}
	else
	{
		pChunk->addStaticItem( b );
		return ChunkItemFactory::Result( b );
	}
}


/// Static factory initialiser
ChunkItemFactory ChunkBinding::factory_( "binding", 0, ChunkBinding::create );


bool ChunkBinding::load( DataSectionPtr pSection )
{
	BW::string idStr = pSection->readString( "fromID", "" );
	if (idStr.empty())
        return false;

	fromID_ = UniqueID(idStr);

	idStr = pSection->readString( "toID", "" );
	if (idStr.empty())
		return false;

	toID_ = UniqueID(idStr);

	from_ = ChunkItemTreeNode::nodeCache_.find(fromID_);
	if (from_)
	{
		from_->addBindingFromMe(this);
	}
	else
	{
		ChunkItemTreeNode::bindingCache_.addBindingFrom_OnLoad(this);
	}

	to_ = ChunkItemTreeNode::nodeCache_.find(toID_);
	if (to_)
	{
		to_->addBindingToMe(this);
	}
	else
	{
		ChunkItemTreeNode::bindingCache_.addBindingTo_OnLoad(this);
	}

	// add to cache
	ChunkItemTreeNode::bindingCache_.add(this);

	return true;
}

bool ChunkBinding::save( DataSectionPtr pSection )
{
	if (fromID_ == UniqueID::zero())
		return false;

	if (toID_ == UniqueID::zero())
		return false;

	pSection->writeString( "fromID", fromID_.toString() );
	pSection->writeString( "toID", toID_.toString() );

	return true;
}



// -----------------------------------------------------------------------------
// Section: ChunkBindingCache
// -----------------------------------------------------------------------------

void ChunkBindingCache::add(ChunkBindingPtr binding)
{
	bindings_.push_back(binding);
}

void ChunkBindingCache::addBindingFrom_OnLoad(ChunkBindingPtr binding)
{
	waitingBindingFrom_.push_back(binding);
}

void ChunkBindingCache::addBindingTo_OnLoad(ChunkBindingPtr binding)
{
	waitingBindingTo_.push_back(binding);
}

void ChunkBindingCache::connect(ChunkItemTreeNodePtr node)
{
	typedef BW::list<BindingList::iterator> BindingItList;
	BindingItList removeList;

	for (BindingList::iterator it = waitingBindingFrom_.begin();
		it != waitingBindingFrom_.end();
		it++)
	{
		if ((*it)->fromID_ == node->id())
		{
			node->addBindingFromMe(*it);
			removeList.push_back(it);
		}
	}

	for (BindingItList::iterator it = removeList.begin();
		it != removeList.end();
		it++)
	{
		waitingBindingFrom_.erase(*it);
	}
	removeList.clear();


	for (BindingList::iterator it = waitingBindingTo_.begin();
		it != waitingBindingTo_.end();
		it++)
	{
		if ((*it)->toID_ == node->id())
		{
			node->addBindingToMe(*it);
			removeList.push_back(it);
		}
	}

	for (BindingItList::iterator it = removeList.begin();
		it != removeList.end();
		it++)
	{
		waitingBindingTo_.erase(*it);
	}
	removeList.clear();
}

BW_END_NAMESPACE

// chunk_binding.cpp
