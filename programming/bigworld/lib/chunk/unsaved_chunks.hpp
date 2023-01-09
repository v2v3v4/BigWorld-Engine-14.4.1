#ifndef UNSAVED_CHUNKS_HPP__
#define UNSAVED_CHUNKS_HPP__


#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

class Chunk;
class Progress;

/**
 *	This class is responsible for storing chunks to save and saving the chunks.
 */
class UnsavedChunks
{
public:
	class IChunkSaver
	{
	public:
		virtual bool save( Chunk* chunk ) = 0;
		virtual bool isDeleted( Chunk& chunk ) const = 0;
		virtual ~IChunkSaver() {}
	};

	class IChunkLoader
	{
	public:
		virtual void load( const BW::string& chunk ) = 0;
		virtual Chunk* find( const BW::string& chunk ) = 0;
		virtual ~IChunkLoader() {}
	};

	typedef BW::set<BW::string> ChunkIds;
	typedef BW::set<Chunk*> Chunks;

	void add( Chunk* chunk );
	void clear();
	bool save( IChunkSaver& saver, Progress* progress = NULL, 
		IChunkLoader* loader = NULL, ChunkIds extraChunks = ChunkIds() );

	bool empty() const;
	void mark();

	void filter( const UnsavedChunks& uc );
	void filterWithoutSaving( const Chunks& toFilter );

private:
	bool saveChunks( IChunkSaver& saver, Progress* progress );

	Chunks chunks_;
	Chunks savedChunks_;
};

BW_END_NAMESPACE

#endif//UNSAVED_CHUNKS_HPP__
