#include "script/first_include.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#ifdef MF_SERVER
#include "chunk/server_chunk_terrain.hpp"
#else
#include "chunk/chunk_terrain.hpp"
#endif

#include "math/vector3.hpp"

#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE

#ifdef MF_SERVER
typedef ServerChunkTerrain ChunkTerrain;
typedef ServerChunkTerrainCache ChunkTerrainCache;
#endif


/*~ function BigWorld.getHeightAtPos
 *	@components{ bots, cell }
 *
 *	This method returns the terrain height at a given point and space.
 *
 * 	@param spaceID	The ID of the space.
 * 	@param point	The point in space.
 *
 *	@return		The terrain height at the given position and space,
 *				None if terrain chunk at the position is loaded.
 */
PyObject * getHeightAtPos( ChunkSpaceID spaceID, Vector3 point )
{
	ChunkSpacePtr pSpace =
		ChunkManager::instance().space( spaceID, false/*createIfMissing*/ );
	if (!pSpace)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.getHeightAtPos: "
			"No space ID %d", int(spaceID) );
		return NULL;
	}

	ChunkSpace::Column * pColumn = pSpace->column( point, false/*canCreate*/ );
	if (!pColumn)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.getHeightAtPos: "
			"Point %s is not within boundry of space %d\n",
			point.desc().c_str(), int(spaceID) );
		return NULL;
	}

	Chunk * pChunk = pColumn->pOutsideChunk();
	if (!pChunk)
	{
		Py_RETURN_NONE;
	}

	ChunkTerrain * pTerrain = ChunkTerrainCache::instance( *pChunk ).pTerrain();
	if (pTerrain)
	{
		const Matrix & inv = pChunk->transformInverse();
		Vector3 terrainPos = inv.applyPoint( point );
		float height =
			pTerrain->block()->heightAt( terrainPos[0], terrainPos[2] );
		height -= inv.applyToOrigin()[1];

		return PyFloat_FromDouble( height );
	}

	Py_RETURN_NONE;
}
PY_AUTO_MODULE_FUNCTION( RETOWN, getHeightAtPos,
	ARG( ChunkSpaceID, ARG( Vector3, END ) ), BigWorld )

BW_END_NAMESPACE
