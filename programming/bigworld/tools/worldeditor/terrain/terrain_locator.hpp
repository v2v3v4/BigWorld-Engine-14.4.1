#ifndef TERRAIN_LOCATOR_HPP
#define TERRAIN_LOCATOR_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/collisions/collision_callbacks.hpp"
#include "worldeditor/editor/chunk_obstacle_locator.hpp"
#include "gizmo/tool_locator.hpp"
#include "chunk/chunk_obstacle.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a tool locator that sits on a terrain
 */
class TerrainToolLocator : public ChunkObstacleToolLocator
{
	Py_Header( TerrainToolLocator, ChunkObstacleToolLocator )

public:
	explicit TerrainToolLocator( PyTypeObject * pType = &s_type_ );

	virtual void calculatePosition( const Vector3 & worldRay, Tool & tool );

	PY_FACTORY_DECLARE()

private:
	ClosestTerrainObstacle terrainCallback_;

	LOCATOR_FACTORY_DECLARE( TerrainToolLocator() )
};


/**
 *	This class implements a tool locator that sits on a terrain,
 *	finding quads of terrain, even if a hole is cut.
 */
class TerrainHoleToolLocator : public ChunkObstacleToolLocator
{
	Py_Header( TerrainHoleToolLocator, ChunkObstacleToolLocator )

public:
	explicit TerrainHoleToolLocator( PyTypeObject * pType = &s_type_ );

	virtual void calculatePosition( const Vector3 & worldRay, Tool & tool );

	PY_FACTORY_DECLARE()

private:
	ClosestTerrainObstacle terrainCallback_;

	LOCATOR_FACTORY_DECLARE( TerrainHoleToolLocator() )
};


/**
 *	This class implements a tool locator that finds the chunk a tool is in,
 *	based on the intersection of the tool with the terrain.
 */
class TerrainChunkLocator : public ChunkObstacleToolLocator
{
	Py_Header( TerrainChunkLocator, ChunkObstacleToolLocator )
	
public:
	explicit TerrainChunkLocator( PyTypeObject * pType = &s_type_ );

	virtual void calculatePosition( const Vector3 & worldRay, Tool & tool );
	PY_FACTORY_DECLARE()

private:
	ClosestTerrainObstacle terrainCallback_;

	LOCATOR_FACTORY_DECLARE( TerrainChunkLocator() )
};

BW_END_NAMESPACE

#endif // TERRAIN_LOCATOR_HPP
