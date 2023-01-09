#ifndef DIRTY_CHUNK_LIST__
#define DIRTY_CHUNK_LIST__


#include "resmgr/datasection.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class Chunk;


class DirtyChunkList
{
	typedef BW::map<BW::string, Chunk*> ChunkMap;
public:
	typedef ChunkMap::iterator iterator;
	typedef ChunkMap::const_iterator const_iterator;

	DirtyChunkList( const BW::string& type );

	bool isDirty( const BW::string& chunkName ) const;
	void dirty( Chunk* pChunk );
	void clean( const BW::string& chunkName );

	bool any() const;
	int num() const;
	iterator begin();
	iterator end();
	iterator erase( iterator iter );

	const_iterator begin() const;
	const_iterator end() const;

	bool empty() const;
	void clear();

	bool requireProcessingInBackground() const { return requireProcessingInBackground_; }
	void requireProcessingInBackground( bool pib ) { requireProcessingInBackground_ = pib; }
	bool requireProcessingInMainThread() const { return requireProcessingInMainThread_; }
	void requireProcessingInMainThread( bool pim ) { requireProcessingInMainThread_ = pim; }

private:
	bool requireProcessingInBackground_;
	bool requireProcessingInMainThread_;
	BW::string type_;

	ChunkMap chunks_;
};


class DirtyChunkLists
{
public:
	DirtyChunkLists();

	DirtyChunkList& operator[]( const BW::string& type );
	const DirtyChunkList& operator[]( const BW::string& type ) const;
	size_t size() const;
	DirtyChunkList& operator[]( size_t index );
	const DirtyChunkList& operator[]( size_t index ) const;
	const BW::string& name( size_t index ) const;

	bool empty() const;
	bool any() const;
	int num() const;
	bool isDirty( const BW::string& chunkName ) const;
	void clean( const BW::string& chunkName );
	void clear();

private:
	bool needSync_;
	BW::string spaceName_;

	typedef BW::map<BW::string, DirtyChunkList> DirtyLists;
	DirtyLists dirtyLists_;
};

BW_END_NAMESPACE

#endif//DIRTY_CHUNK_LIST__
