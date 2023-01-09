#include "pch.hpp"
#include "chunk_item_tree_node.hpp"
#include "chunk.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkItemTreeNode
// -----------------------------------------------------------------------------
ChunkItemTreeNodeCache ChunkItemTreeNode::nodeCache_;
ChunkBindingCache ChunkItemTreeNode::bindingCache_;


bool ChunkItemTreeNode::load( DataSectionPtr pSection ) 
{
	numberChildren_ = pSection->readInt( "number_children", 0 );

	BW::string idStr = pSection->readString( "id", "" );
	if (idStr.empty())
	{
		// if has no id, ccan't have any children, give it an id
		id_ = UniqueID::generate();
		numberChildren_ = 0;
	}
	else
	{
		id_ = UniqueID( idStr );
	}

	if ( nodeCache_.find( id() ) )
	{
		// it is already in the cache, must be cloned, let the cloning add it
		// properly
		return true;
	}


	idStr = pSection->readString( "parentID", "" );
	if (idStr.empty())
		parentID_ = UniqueID::zero();
	else
		parentID_ = UniqueID( idStr );

	// manage cache
	nodeCache_.add(this);

	// link to parent, if required
	if (parentID_ != UniqueID::zero())
	{
		ChunkItemTreeNodePtr parentNode = nodeCache_.find(parentID_);
		if (parentNode)
		{
			ChunkItemTreeNode::setParent(parentNode);
		}
		else
		{
			nodeCache_.addChildOnParentLoad(this);
		}
	}

	return true;
}

bool ChunkItemTreeNode::save( DataSectionPtr pSection )
{
	pSection->writeString( "id", id_.toString() );
	if ( children_.size() > 0 )
		pSection->writeInt( "number_children", (int)children_.size() );
	if ( parentID_ != UniqueID::zero() )
		pSection->writeString( "parentID", parentID_.toString() );
	else
		pSection->delChild( "parentID" );

	return true;
}


/**
 * Set the parent this node belongs
 */
void ChunkItemTreeNode::setParent(ChunkItemTreeNodePtr parent)
{
	// check parents for circular behaviour
	for (ChunkItemTreeNodePtr p = this;
		p;
		p = p->getParent())
	{
		if (p == parent)
			return;	// cannot make link
	}

	for (ChunkItemTreeNodePtr p = parent;
		p;
		p = p->getParent())
	{
		if (p == this)
			return;	// cannot make link
	}

	if (parent_)
		parent_->removeChild(this);

	if (parent)
	{
		parent_ = parent;
		parent_->addChild(this);

		parentID_ = parent_->id();
		MF_ASSERT(parentID_ != UniqueID::zero());
	}
	else
	{
		parent_ = NULL;

		parentID_ = UniqueID::zero();
	}
}


void ChunkItemTreeNode::addChild(ChunkItemTreeNodePtr child)
{
	// don't count children on load.. 
	// this is the test for whether all children have loaded

	if (allChildrenLoaded())	// allChildrenLoaded looks at children_.size()
	{
		children_.push_back(child);
        numberChildren_ = static_cast<int>(children_.size());
	}
	else
	{
		children_.push_back(child);
	}

	onAddChild();
}

void ChunkItemTreeNode::removeChild(ChunkItemTreeNodePtr child)
{
	MF_ASSERT(std::find(children_.begin(), children_.end(), child) != children_.end());
	children_.remove(child);
	numberChildren_ = static_cast<int>(children_.size());

	onRemoveChild();
}

void ChunkItemTreeNode::addBindingFromMeToThat(ChunkBindingPtr binding)
{
	addBindingFromMe(binding);
	binding->to()->addBindingToMe(binding);
}

void ChunkItemTreeNode::addBindingFromMe(ChunkBindingPtr binding)
{
	MF_ASSERT(binding);
	MF_ASSERT(binding->fromID_ == id());

	// check whether already exists
	BindingList::iterator it = std::find(bindings_.begin(), bindings_.end(), binding);
	if (it != bindings_.end())
		return; // can't have duplicates

	// insert into the multimap & tell the destination
    bindings_.push_back(binding);
}

void ChunkItemTreeNode::addBindingToMe(ChunkBindingPtr binding)
{
	MF_ASSERT(binding);
	MF_ASSERT(binding->toID_ == id());

	// check whether already exists
	BindingList::iterator it = std::find(bindingsToMe_.begin(), bindingsToMe_.end(), binding);
	if (it != bindingsToMe_.end())
		return; // can't have duplicates

	// insert into the multimap & tell the destination
    bindingsToMe_.push_back(binding);
}


void ChunkItemTreeNode::removeBindingFromMeToThat(ChunkBindingPtr binding)
{
	removeBindingFromMe(binding);
	binding->to()->removeBindingToMe(binding);
}

void ChunkItemTreeNode::removeBindingFromMe(ChunkBindingPtr binding)
{
	MF_ASSERT(binding);
	MF_ASSERT(binding->fromID_ == id());

	// check whether already exists
	BindingList::iterator it = std::find(bindings_.begin(), 
										bindings_.end(),
										binding);
	if (it != bindings_.end())
		return; // should be there

	bindings_.remove(*it);
}

void ChunkItemTreeNode::removeBindingToMe(ChunkBindingPtr binding)
{
	MF_ASSERT(binding);
	MF_ASSERT(binding->toID_ == id());

	// check whether already exists
	BindingList::iterator it = std::find(bindingsToMe_.begin(), 
										bindingsToMe_.end(),
										binding);
	if (it != bindingsToMe_.end())
		return; // should be there

	bindingsToMe_.remove(*it);
}


void ChunkItemTreeNode::removeThisNode()
{
	// tell children no longer have a parent
	for (BW::list<ChunkItemTreeNodePtr>::iterator it = children_.begin();
		it != children_.end();
		it = children_.begin())
	{
		// markers are being deleted from the list
		(*it)->setParent(NULL);
	}

	// tell the parent they not longer have a child
	MF_ASSERT(getParent() || (!getParent() && (parentID_ == UniqueID::zero())));
	setParent(NULL);

	// tell bindings we no longer exist
//#pragma message("TODO: binding support in removeThisNode")
}

bool ChunkItemTreeNode::isNodeConnected() const
{
//#pragma message("TODO: binding support in isNodeConnected")
	return (numberChildren_ > 0) || (parentID_ != UniqueID::zero());
}

void ChunkItemTreeNode::setNewNode()
{
//#pragma message("TODO: binding support in setNewNode")

	id_ = UniqueID::generate();
	parentID_ = UniqueID::zero();
	numberChildren_ = 0;

	children_.clear();
	parent_ = NULL;

	// manage cache
	nodeCache_.add(this);
}


void ChunkItemTreeNode::getCopyOfChildren(BW::list<ChunkItemTreeNodePtr>& outList) const
{
	for (BW::list<ChunkItemTreeNodePtr>::const_iterator it = children_.begin();
		it != children_.end();
		it++)
	{
		outList.push_back(*it);
	}	
}



// -----------------------------------------------------------------------------
// Section: ChunkMarkerClusterCache
// -----------------------------------------------------------------------------

void ChunkItemTreeNodeCache::add(ChunkItemTreeNodePtr node)
{
	nodeMap_[node->id()] = node;

	// check whether there are children to add
	ChunkItemTreeNodeListMap::iterator it = waitingNodeListMap_.find(node->id());
	if (it != waitingNodeListMap_.end())
	{
		for (ChunkItemTreeNodeList::const_iterator itc = it->second.begin();
			itc != it->second.end();
			itc++)
		{
			(static_cast<ChunkItemTreeNodePtr>(*itc))->ChunkItemTreeNode::setParent(node);
		}

		waitingNodeListMap_.erase(it);
	}

	// check to see whether there are bindings to connect up
	ChunkItemTreeNode::bindingCache_.connect(node);
}

ChunkItemTreeNodePtr ChunkItemTreeNodeCache::find(UniqueID clusterID) const
{
	ChunkItemTreeNodeMap::const_iterator it = nodeMap_.find(clusterID);
	if (it == nodeMap_.end())
		return NULL;
	return it->second;
}

void ChunkItemTreeNodeCache::addChildOnParentLoad(ChunkItemTreeNodePtr node)
{
	// for when children are loaded before their parent
	waitingNodeListMap_[node->parentID()].push_back(node);
}

void ChunkItemTreeNodeCache::fini()
{
	waitingNodeListMap_.clear();
	nodeMap_.clear();
}

BW_END_NAMESPACE

// chunk_item_tree_node.cpp
