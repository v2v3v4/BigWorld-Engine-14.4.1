#ifndef CHUNK_MARKER_BASE_HPP
#define CHUNK_MARKER_BASE_HPP


#include "chunk_item.hpp"
#include "chunk_stationnode.hpp"
#include "chunk_binding.hpp"
#include "cstdmf/bw_list.hpp"

BW_BEGIN_NAMESPACE

class ChunkItemTreeNode;
typedef SmartPointer<ChunkItemTreeNode> ChunkItemTreeNodePtr;
class ChunkItemTreeNodeCache;


/**
 *	This class is the basis for markers AND cluster of markers and/or clusters
 *	it provides the tree functionality where children reference the parent
 *	(parents also reference their children, but this is built on load)
 */
class ChunkItemTreeNode : public ChunkItem
{
public:
	ChunkItemTreeNode()
		: ChunkItem( WANTS_DRAW )
	{
		id_ = UniqueID::generate();
		parentID_ = UniqueID::zero();
		numberChildren_ = 0;

		children_.clear();
		parent_ = NULL;
	}

	virtual bool load( DataSectionPtr pSection );
	virtual bool save( DataSectionPtr pSection );

	UniqueID id() const { return id_; }
	UniqueID parentID() const { return parentID_; }

	virtual void setParent(ChunkItemTreeNodePtr parent);
	ChunkItemTreeNodePtr getParent() const { return parent_; }

	void getCopyOfChildren(BW::list<ChunkItemTreeNodePtr>& outList) const;
	bool fullyLoaded() { return allChildrenLoaded() && allBindingsLoaded(); }
	bool allChildrenLoaded() { return numberChildren_ == (int)children_.size(); }
	bool allBindingsLoaded();

	void removeThisNode();

	virtual void onRemoveChild() {}
	virtual void onAddChild() {}

	bool isNodeConnected() const;
	void setNewNode();

	void addBindingFromMeToThat(ChunkBindingPtr binding);
	void removeBindingFromMeToThat(ChunkBindingPtr binding);

	int numberChildren() { return numberChildren_; }

	static ChunkItemTreeNodeCache& nodeCache() { return nodeCache_; }	

protected:
	void addChild(ChunkItemTreeNodePtr child);
	void removeChild(ChunkItemTreeNodePtr child);

	UniqueID id_;
	UniqueID parentID_;

	typedef BW::list<ChunkBindingPtr> BindingList;
	BindingList bindings_;

	BindingList bindingsToMe_;	// this is build on load

	friend class ChunkBinding;
	friend class ChunkBindingCache;

	void addBindingFromMe(ChunkBindingPtr binding);
	void addBindingToMe(ChunkBindingPtr binding);
	void removeBindingFromMe(ChunkBindingPtr binding);
	void removeBindingToMe(ChunkBindingPtr binding);

private:
	friend class ChunkItemTreeNodeCache;
	static ChunkItemTreeNodeCache nodeCache_;

	BW::list<ChunkItemTreeNodePtr> children_;	// this is build on load
	ChunkItemTreeNodePtr parent_;

	int numberChildren_;

	static ChunkBindingCache bindingCache_;
};


class ChunkItemTreeNodeCache
{
public:
	ChunkItemTreeNodePtr find(UniqueID nodeID) const;	
	void fini();

private:
	friend class ChunkItemTreeNode;	

	void add(ChunkItemTreeNodePtr node);
	void addChildOnParentLoad(ChunkItemTreeNodePtr child);

	typedef BW::map<UniqueID, ChunkItemTreeNodePtr> ChunkItemTreeNodeMap;
	ChunkItemTreeNodeMap nodeMap_;

	// for when marker is loaded before the cluster
	typedef BW::list<ChunkItemTreeNodePtr> ChunkItemTreeNodeList;
	typedef BW::map<UniqueID, ChunkItemTreeNodeList> ChunkItemTreeNodeListMap;
	ChunkItemTreeNodeListMap waitingNodeListMap_;
};

BW_END_NAMESPACE

#endif // CHUNK_MARKER_BASE_HPP
