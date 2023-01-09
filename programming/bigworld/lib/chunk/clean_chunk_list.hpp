#ifndef CLEAN_CHUNK_LIST_HPP
#define CLEAN_CHUNK_LIST_HPP


#include "cstdmf/bw_map.hpp"
#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

class GeometryMapping;
class Chunk;
class Progress;


/**
 *	This class manages the clean chunk list. In the list, it
 *	stores the time stamps for each chunk that is known to
 *	be clean. In case the chunk file gets modified outside WE,
 *	we will check the stored time stamp against its file time
 *	stamp to update the clean status.
 */
class CleanChunkList : public SafeReferenceCount
{
	CleanChunkList( CleanChunkList& );
	CleanChunkList& operator= ( CleanChunkList& );
public:
	CleanChunkList( GeometryMapping& dirMapping );

	void save() const;
	void sync( Progress* task );
	void update( Chunk* pChunk );
	void markAsClean( const BW::string& chunk );
	bool isClean( const BW::string& chunk ) const;

private:
	void load();
	DataSectionPtr getDataSection() const;

	GeometryMapping& dirMapping_;

	struct ChunkInfo
	{
		uint64 cdataModificationTime_;
		uint64 cdataCreationTime_;
		uint64 chunkModificationTime_;
		uint64 chunkCreationTime_;
	};

	BW::map<BW::string, ChunkInfo> checkedChunks_;
	BW::map<BW::string, ChunkInfo> uncheckedChunks_;
};

typedef SmartPointer<CleanChunkList> CleanChunkListPtr;

BW_END_NAMESPACE

#endif//CLEAN_CHUNK_LIST_HPP
