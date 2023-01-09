#ifndef REPLAY_DATA_HPP
#define REPLAY_DATA_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

namespace ReplayData
{
	typedef uint32 Timestamp;
	typedef uint16 HeaderSignatureLength;
	typedef uint32 SignedChunkLength;
	typedef uint32 TickDataLength;
	typedef uint8 BlockType;

	enum
	{
		BLOCK_TYPE_ENTITY_CREATE,
		BLOCK_TYPE_ENTITY_VOLATILE,
		BLOCK_TYPE_ENTITY_VOLATILE_ON_GROUND,
		BLOCK_TYPE_ENTITY_METHOD,
		BLOCK_TYPE_ENTITY_PROPERTY_CHANGE,
		BLOCK_TYPE_ENTITY_DELETE,
		BLOCK_TYPE_ENTITY_PLAYER_STATE_CHANGE,
		BLOCK_TYPE_PLAYER_AOI_CHANGE,
		BLOCK_TYPE_SPACE_DATA,
		BLOCK_TYPE_FINAL
	};
}

BW_END_NAMESPACE

#endif // REPLAY_DATA_HPP
