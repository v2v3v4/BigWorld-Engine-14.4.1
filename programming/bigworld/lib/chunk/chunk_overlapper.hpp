#ifndef CHUNK_OVERLAPPER_HPP
#define CHUNK_OVERLAPPER_HPP

#include "chunk_item.hpp"
#include "chunk_cache.hpp"
#include "chunk.hpp"


BW_BEGIN_NAMESPACE

class ChunkOverlappers;

/**
 *	This class is a record in an outside chunk that another chunk overlaps it.
 */
class ChunkOverlapper : public ChunkItem
{
	DECLARE_CHUNK_ITEM( ChunkOverlapper )
public:
	ChunkOverlapper();
	~ChunkOverlapper();

	bool load( DataSectionPtr pSection, Chunk * pChunk,
				BW::string * errorString = NULL );
	virtual void toss( Chunk * pChunk );
	void bind( bool isUnbind );

	bool bbReady() const
		{ return pOverlappingChunk_->boundingBoxReady(); }
	const BoundingBox & bb() const
		{ return pOverlappingChunk_->boundingBox(); }

	bool focussed() const			{ return pOverlappingChunk_->focussed(); }
	void findAppointedChunk();

	// pOverlapper() is now deprecated in favour of pOverlappingChunk()
	Chunk * pOverlapper() const				{ return pOverlappingChunk_; }
	Chunk * pOverlappingChunk() const		{ return pOverlappingChunk_; }
	Chunk * pOverlappedChunk() const		{ return this->chunk(); }

	void alsoInAdd( ChunkOverlappers * ai );
	void alsoInDel( ChunkOverlappers * ai );

	const BW::string& overlapperID() const { return overlapperID_; }

protected:
	ChunkOverlapper( const ChunkOverlapper& );
	ChunkOverlapper& operator=( const ChunkOverlapper& );

	BW::string		overlapperID_;
	Chunk *			pOverlappingChunk_;
	BW::vector<ChunkOverlappers*>	alsoIn_;
};

typedef SmartPointer<ChunkOverlapper> ChunkOverlapperPtr;


/**
 *	This class is a cache of all the overlappers for a given chunk.
 *	It may include overlappers from neighbouring chunks which determine
 *	that they also overlap this chunk.
 */
class ChunkOverlappers : public ChunkCache
{
public:
	ChunkOverlappers( Chunk & chunk );
	virtual ~ChunkOverlappers();

	virtual void bind( bool isUnbind );
	static void touch( Chunk & chunk );

	bool empty() const						{ return overlappers_.empty(); }
	bool complete() const					{ return complete_; }

	void add( ChunkOverlapperPtr pOverlapper, bool foreign = false );
	void del( ChunkOverlapperPtr pOverlapper, bool foreign = false );

	typedef BW::vector<ChunkOverlapperPtr> Overlappers;
	const Overlappers & overlappers() const	{ return overlappers_; }
	Overlappers& overlappers() { return overlappers_; }

	void findAppointedChunks();

	static Instance<ChunkOverlappers>	instance;

private:
	void share();
	void copyFrom( const ChunkOverlappers & other );
	void checkIfComplete( bool checkNeighbours );

	Chunk & chunk_;
	Overlappers	overlappers_;
	Overlappers foreign_;
	bool		bound_;
	bool		halfBound_;
	bool		complete_;
	bool		binding_;
};

BW_END_NAMESPACE

#endif // CHUNK_OVERLAPPER_HPP
