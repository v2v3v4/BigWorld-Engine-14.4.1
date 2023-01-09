#include "pch.hpp"
#include "cstdmf/guard.hpp"

#include "chunk/chunk.hpp"
#include "chunk/geometry_mapping.hpp"
#include "chunk/chunk_manager.hpp"

#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

int PyChunk_token = 0;

namespace {

/**
 *	Static helper method to get chunk from info describing it
 */
Chunk * lookupChunk( const BW::string & chunkNMapping, SpaceID spaceID,
	const char * methodName )
{
	BW_GUARD;
	// look up the space
	ChunkSpacePtr pSpace;

	ChunkManager & cm = ChunkManager::instance();
	pSpace = cm.space( spaceID, false );

	if (!pSpace)
	{
		PyErr_Format( PyExc_ValueError,
			"%s: space ID %d not found", methodName, int(spaceID) );
		return NULL;
	}

	// look up the chunk
	BW::string chunkOnly = chunkNMapping;
	BW::string mappingOnly;
	const size_t firstAt = chunkNMapping.find_first_of( '@' );
	if (firstAt < chunkNMapping.size())
	{
		chunkOnly = chunkNMapping.substr( 0, firstAt );
		mappingOnly = chunkNMapping.substr( firstAt+1 );
	}
	Chunk * pChunk = pSpace->findChunk( chunkOnly, mappingOnly );
	if (pChunk == NULL)
	{
		PyErr_Format( PyExc_ValueError,
			"%s: chunk '%s' not found", methodName, chunkNMapping.c_str() );
		return NULL;
	}

	return pChunk;
}

} // namespace

/*~ function BigWorld findChunkFromPoint
 *  @components{ client, cell }
 *  findChunkFromPoint is used to identify the chunk which surrounds a given
 *  location. It throws a ValueError if no chunk is found at the given
 *  location.  It also throws a ValueError if the chunk that is found has
 *	not yet been fully loaded, since there may be an indoor chunk yet to
 *	load that would better contain the point.
 *  @param point point is a Vector3 describing the location to search for a
 *  chunk, in world space.
 *  @param spaceID spaceID is the id of the space to search in.
 *  @return The name of the chunk found, as a string.
 */
/**
 *	Function to find a chunk from a point.
 */
static PyObject* findChunkFromPoint( const Vector3 & point,
	SpaceID spaceID )
{
	BW_GUARD;
	// look up the space
	ChunkSpacePtr pSpace = NULL;
	ChunkManager & cm = ChunkManager::instance();
	pSpace = cm.space( spaceID, false );

	if (!pSpace)
	{
		PyErr_Format( PyExc_ValueError,
			"BigWorld.findChunkFromPoint(): space ID %d not found", int(spaceID) );
		return NULL;
	}

	// ask it to find the chunk
	Chunk * pChunk = pSpace->findChunkFromPointExact( point );
	if (!pChunk)
	{
		// Note: we use bw_snprintf because PyErr_Format does not support %f.
		char buf[256];
		bw_snprintf( buf, sizeof(buf), "BigWorld.findChunkFromPoint(): "
			"chunk at (%f,%f,%f) not found", point.x, point.y, point.z );
		PyErr_SetString( PyExc_ValueError, buf );
		return NULL;
	}

	// return the chunk identifier
	return Script::getData(
		pChunk->identifier() + "@" + pChunk->mapping()->name() );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, findChunkFromPoint,
	ARG( Vector3, ARG( SpaceID, END ) ), BigWorld )


/*~ function BigWorld chunkTransform
 *  @components{ client, cell }
 *  This gives access to a chunk's transform.
 *  It throws a ValueError if no chunks are found in the searched space that
 *  match argument chunkNMapping, or if the space argument is specified and a
 *  corresponding space is not found.
 *  @param chunkNMapping chunkNMapping is a string containing the name of the
 *  chunk whose transform is to be returned.
 *  @param spaceID spaceID is the id of the space in which the chunk resides.
 *  @return A Matrix which describes the chunk's transform.
 */
/**
 *	Function to let Python get a chunk's transform
 */
static PyObject * chunkTransform( const BW::string & chunkNMapping,
	SpaceID spaceID )
{
	BW_GUARD;
	Chunk * pChunk = lookupChunk( chunkNMapping, spaceID,
		"BigWorld.chunkTransform()" );
	if (!pChunk) return NULL;

	return Script::getData( pChunk->transform() );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, chunkTransform,
	ARG( BW::string, ARG( SpaceID, END ) ), BigWorld )

BW_END_NAMESPACE

// py_chunk.cpp


