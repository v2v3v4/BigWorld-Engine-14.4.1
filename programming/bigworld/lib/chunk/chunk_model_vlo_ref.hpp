#ifndef CHUNK_MODEL_VLO_REFERENCE_HPP
#define CHUNK_MODEL_VLO_REFERENCE_HPP

#include "chunk_vlo.hpp"

BW_BEGIN_NAMESPACE

/**
 *
 * This class is the chunk item reference holder for a Very Large Object ( VLO )
 * as a VLO spans many chunks so there will be one instance of this reference
 * per chunk that the VLO spans.
 *
 */
class ChunkModelVLORef
	: public ChunkVLO
{
public:
	DECLARE_CHUNK_ITEM( ChunkModelVLORef )
	explicit ChunkModelVLORef( WantFlags wantFlags = WANTS_DRAW );

	virtual void addCollisionScene();
	virtual void removeCollisionScene();

	static BW::string & getSectionName();

protected:
	bool collisionAdded_;
};

BW_END_NAMESPACE

#endif // CHUNK_MODEL_VLO_REFERENCE_HPP
