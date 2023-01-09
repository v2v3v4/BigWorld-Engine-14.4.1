#ifndef CHUNK_FORMAT_HPP
#define CHUNK_FORMAT_HPP

#include "cstdmf/stdmf.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

namespace ChunkFormat
{


/**
 *	Convert from grid positions to chunk identifier.
 *	For the opposite, see ChunkTerrain::outsideChunkIDToGrid().
 *	@param gridX x grid position of outside chunk.
 *	@param gridZ z grid position of outside chunk.
 *	@param singleDir if the space has chunks in subdirectories,
 *		eg. "xxxxzzzz/sep/xxxxzzzz.chunk"
 *	@return the outside chunk ID *with* an 'o' on the end. eg. "xxxxzzzzo".
 */
inline BW::string outsideChunkIdentifier( int gridX, int gridZ,
	bool singleDir = false )
{
	char chunkIdentifierCStr[32];
	BW::string gridChunkIdentifier;

    uint16 gridxs = uint16(gridX), gridzs = uint16(gridZ);
	if (!singleDir)
	{
		if (uint16(gridxs + 4096) >= 8192 || uint16(gridzs + 4096) >= 8192)
		{
				bw_snprintf( chunkIdentifierCStr, sizeof(chunkIdentifierCStr), "%01xxxx%01xxxx/sep/",
						int(gridxs >> 12), int(gridzs >> 12) );
				gridChunkIdentifier = chunkIdentifierCStr;
		}
		if (uint16(gridxs + 256) >= 512 || uint16(gridzs + 256) >= 512)
		{
				bw_snprintf( chunkIdentifierCStr, sizeof(chunkIdentifierCStr), "%02xxx%02xxx/sep/",
						int(gridxs >> 8), int(gridzs >> 8) );
				gridChunkIdentifier += chunkIdentifierCStr;
		}
		if (uint16(gridxs + 16) >= 32 || uint16(gridzs + 16) >= 32)
		{
				bw_snprintf( chunkIdentifierCStr, sizeof(chunkIdentifierCStr), "%03xx%03xx/sep/",
						int(gridxs >> 4), int(gridzs >> 4) );
				gridChunkIdentifier += chunkIdentifierCStr;
		}
	}
	bw_snprintf( chunkIdentifierCStr, sizeof(chunkIdentifierCStr), "%04x%04xo", int(gridxs), int(gridzs) );
	gridChunkIdentifier += chunkIdentifierCStr;

	return gridChunkIdentifier;
}

};

BW_END_NAMESPACE

#endif // CHUNK_FORMAT_HPP
