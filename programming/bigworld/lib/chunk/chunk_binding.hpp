#ifndef CHUNK_BINDING_HPP
#define CHUNK_BINDING_HPP


#include "chunk_item.hpp"
#include "chunk_stationnode.hpp"
#include "cstdmf/bw_list.hpp"

BW_BEGIN_NAMESPACE

class ChunkItemTreeNode;
typedef SmartPointer<ChunkItemTreeNode> ChunkItemTreeNodePtr;
class ChunkItemTreeNodeCache;
class ChunkBinding;
typedef SmartPointer<ChunkBinding> ChunkBindingPtr;
class ChunkBindingCache;



class ChunkBinding : public ChunkItem
{
public:
	virtual bool load( DataSectionPtr pSection );
	virtual bool save( DataSectionPtr pSection );

	UniqueID fromID_;
	UniqueID toID_;

	ChunkItemTreeNodePtr from() const { return from_; }
	ChunkItemTreeNodePtr to() const { return to_; }

private:
	ChunkItemTreeNodePtr from_;
	ChunkItemTreeNodePtr to_;

	static ChunkItemFactory::Result create( Chunk * pChunk, DataSectionPtr pSection );
	static ChunkItemFactory	factory_;
};


class ChunkBindingCache
{
public:
	void add(ChunkBindingPtr binding);

	void addBindingFrom_OnLoad(ChunkBindingPtr binding);
	void addBindingTo_OnLoad(ChunkBindingPtr binding);

	void connect(ChunkItemTreeNodePtr node);

private:
	typedef BW::list<ChunkBindingPtr> BindingList;
	BindingList bindings_;

	// for when the bindings are loaded before the necessary chunk items
	BindingList waitingBindingFrom_;
	BindingList waitingBindingTo_;
};

BW_END_NAMESPACE

#endif // CHUNK_BINDING_HPP
