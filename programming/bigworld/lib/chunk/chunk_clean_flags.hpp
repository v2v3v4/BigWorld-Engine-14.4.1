#ifndef CHUNK_CLEAN_FLAGS_HPP
#define CHUNK_CLEAN_FLAGS_HPP

#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

/**
 *	A container used for storing which chunk processors are clean. This
 *	was kept instead of replacing it with a new system with the new
 *	chunk processing framework in order to keep binary compatability with
 *	older .cdata files. Ideally we can replace this in the future
 *
 *	NOTE: This stores which processors have been "updated". i.e. it is a
 *	clean flag, not a dirty flag.
 */
struct ChunkCleanFlagsBase
{
	uint32 thumbnail_;
	uint32 shadow_;
	uint32 terrainLOD_;
};

class ChunkCleanFlags : public ChunkCleanFlagsBase
{
public:
	explicit ChunkCleanFlags( DataSectionPtr cData );
	ChunkCleanFlags( uint32 thumbnail, uint32 shadow, uint32 terrainLOD );
	void save( DataSectionPtr cData = NULL );
	void merge( const ChunkCleanFlags& other );

private:
	DataSectionPtr cData_;
};

BW_END_NAMESPACE

#endif // CHUNK_CLEAN_FLAGS_HPP
