#include "pch.hpp"

#include "chunk_clean_flags.hpp"

#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

// Yes, this is an odd name for storing clean flags.
// It is kept like this for compatability reasons.
static const char* s_cleanFlagsName = "dirtyFlags";

/**
 *	This method initialises the update flags with a cdata section
 */
ChunkCleanFlags::ChunkCleanFlags( DataSectionPtr cData )
	: cData_( cData )
{
	BW_GUARD;

	thumbnail_ = shadow_ = terrainLOD_ = 0;

	if (cData_)
	{
		DataSectionPtr flagSec = cData_->openSection( s_cleanFlagsName );

		if (flagSec)
		{
			BinaryPtr bp = flagSec->asBinary();

			if (bp->len() == sizeof( ChunkCleanFlagsBase ))
			{
				*(ChunkCleanFlagsBase*)this = *(ChunkCleanFlagsBase*)bp->data();
			}
		}
	}
}


/**
 *	This method initialises the update flags invividually
 */
ChunkCleanFlags::ChunkCleanFlags( uint32 thumbnail,
								uint32 shadow,
								uint32 terrainLOD )
{
	BW_GUARD;

	thumbnail_ = thumbnail;
	shadow_ = shadow;
	terrainLOD_ = terrainLOD;
}


/**
 *	This method saves the flags into a cdata section
 */
void ChunkCleanFlags::save( DataSectionPtr cData /*= NULL*/ )
{
	BW_GUARD;

	if (cData)
	{
		cData_ = cData;
	}

	MF_ASSERT( cData_ );

	DataSectionPtr flagSec = cData_->openSection( s_cleanFlagsName, true );

	flagSec->setBinary( new BinaryBlock(
		(ChunkCleanFlagsBase*)this, sizeof( ChunkCleanFlagsBase ), "BinaryBlock/WorldEditor" ) );
}


/**
 *	This method merges the dirty flags from another ChunkCleanFlags object
 */
void ChunkCleanFlags::merge( const ChunkCleanFlags& other )
{
	BW_GUARD;

	thumbnail_ = thumbnail_ && other.thumbnail_;
	shadow_ = shadow_ && other.shadow_;
	terrainLOD_ = terrainLOD_ && other.terrainLOD_;
}

BW_END_NAMESPACE

// chunk_clean_flags.cpp
