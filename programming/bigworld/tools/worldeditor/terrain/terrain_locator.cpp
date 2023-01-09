#include "pch.hpp"
#include "worldeditor/terrain/terrain_locator.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "gizmo/tool.hpp"
#include "appmgr/options.hpp"
#include "terrain/terrain_settings.hpp"

BW_BEGIN_NAMESPACE

//----------------------------------------------------
//	Section : TerrainToolLocator
//----------------------------------------------------

PY_TYPEOBJECT( TerrainToolLocator )

PY_BEGIN_METHODS( TerrainToolLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TerrainToolLocator )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( TerrainToolLocator, "TerrainToolLocator", Locator )

LOCATOR_FACTORY( TerrainToolLocator )

/**
 *	Constructor.
 *
 *	@param pType		The type.
 */
/*explicit*/ TerrainToolLocator::TerrainToolLocator( PyTypeObject * pType ):
	ChunkObstacleToolLocator( terrainCallback_, pType )
{
}


/**
 *	This method calculates the position of the terrain tool locator,
 *	being defined as the intersection from the world ray to the terrain.
 *
 *	@param	worldRay	The camera's world ray.
 *	@param	tool		The tool we are locating.
 */
void TerrainToolLocator::calculatePosition( 
	const Vector3 &		worldRay, 
	Tool &				tool )
{
	BW_GUARD;

	Vector3 groundPos = 
		Snap::toGround(Moo::rc().invView().applyToOrigin(), worldRay, 20000.f);

	if (groundPos != Moo::rc().invView().applyToOrigin())
	{
		transform_.setTranslate( groundPos );
		positionValid_ = true;

		// Snap in the XZ direction.
		if (WorldManager::instance().snapsEnabled())
		{
			Vector3 snaps = WorldManager::instance().movementSnaps();
			snaps.y = 0.0f;
			Snap::vector3( *(Vector3*)&transform_._41, snaps );
			Snap::toGround( *(Vector3*)&transform_._41 );
		}
	}
	else
	{
		positionValid_ = false;
	}
}


/**
 *	This is the static python factory method.
 *
 *	@param pArgs		The arguments for the factory.
 */
PyObject * TerrainToolLocator::pyNew( PyObject * pArgs )
{
	BW_GUARD;

	return new TerrainToolLocator();
}


//----------------------------------------------------
//	Section : TerrainHoleToolLocator
//----------------------------------------------------

PY_TYPEOBJECT( TerrainHoleToolLocator )

PY_BEGIN_METHODS( TerrainHoleToolLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TerrainHoleToolLocator )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( TerrainHoleToolLocator, "TerrainHoleToolLocator", Locator )

LOCATOR_FACTORY( TerrainHoleToolLocator )


/**
 *	Constructor.
 *
 *	@param pType		The type.
 */
/*explicit*/ TerrainHoleToolLocator::TerrainHoleToolLocator( PyTypeObject * pType ):
	ChunkObstacleToolLocator( terrainCallback_, pType )
{
}


/**
 *	This method calculates the position of the terrain tool locator,
 *	being defined as the intersection from the world ray to the terrain,
 *	and any holes underneath that area.
 *
 *	@param	worldRay	The camera's world ray.
 *	@param	tool		The tool we are locating.
 */
void TerrainHoleToolLocator::calculatePosition(
	const Vector3 &		worldRay, 
	Tool &				tool )
{
	BW_GUARD;

	Vector3 groundPos = 
			Snap::toGround( 
				Moo::rc().invView().applyToOrigin(), 
				worldRay,
				WorldManager::instance().getMaxFarPlane(), 
				true );		// ignore holes

	if (groundPos != Moo::rc().invView().applyToOrigin())
	{
		transform_.setTranslate( groundPos );
		positionValid_ = true;

		// Always snap in the XZ direction.	
		float size = float( WorldManager::instance().pTerrainSettings()->blockSize() ) / 
			float( WorldManager::instance().pTerrainSettings()->holeMapSize() );
		Vector3 snaps( size, 0.0f, size );
		float offset = -size / 2.0f;
		Snap::vector3( *(Vector3*)&transform_._41, snaps, Vector3( offset, 0.f, offset ) );	
		Snap::toGround( *(Vector3*)&transform_._41, Vector3(0.f, -1.f, 0.f), 500.f, true );
	}
	else
	{
		positionValid_ = false;
	}
}


/**
 *	This is the static python factory method.
 *
 *	@param pArgs	The arguments for the factory.
 */
PyObject * TerrainHoleToolLocator::pyNew( PyObject * pArgs )
{
	BW_GUARD;

	return new TerrainHoleToolLocator;
}



//----------------------------------------------------
//	Section : TerrainChunkLocator
//----------------------------------------------------

PY_TYPEOBJECT( TerrainChunkLocator )

PY_BEGIN_METHODS( TerrainChunkLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TerrainChunkLocator )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( TerrainChunkLocator, "TerrainChunkLocator", Locator )

LOCATOR_FACTORY( TerrainChunkLocator )


/**
 *	Constructor.
 *
 *	@param pType		The type.
 */
/*explicit*/ TerrainChunkLocator::TerrainChunkLocator( PyTypeObject * pType ):
	ChunkObstacleToolLocator( terrainCallback_, pType )
{
}


/**
 *	This method calculates the position of the terrain chunk locator,
 *	being defined as the intersection from the world ray to the terrain,
 *	and from there, the chunks surrounding the tool given the tool's size.
 *
 *	@param	worldRay	The camera's world ray.
 *	@param	tool		The tool we are locating.
 */
void TerrainChunkLocator::calculatePosition( 
	const Vector3 &		worldRay, 
	Tool &				tool )
{
	BW_GUARD;

	terrainCallback_.reset();
	Vector3 extent = Moo::rc().invView().applyToOrigin() +
		( worldRay * WorldManager::instance().getMaxFarPlane() );

	ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
	if (space)
	{
		space->collide( 
			Moo::rc().invView().applyToOrigin(),
			extent,
			*callback_ );
	}

	if (terrainCallback_.collided())
	{
		Vector3	pos = Moo::rc().invView().applyToOrigin() +
				( worldRay * terrainCallback_.dist() );

		if (ChunkManager::instance().cameraSpace())
		{
			ChunkSpace::Column * pColumn =
				ChunkManager::instance().cameraSpace()->column( pos, false );

			if (pColumn != NULL)
			{
				if (pColumn->pOutsideChunk())
				{
					transform_ = pColumn->pOutsideChunk()->transform();
					transform_[3] += Vector3( 50.f, 0.f, 50.f );
				}
			}
		}
	}
}


/**
 *	This is the static python factory method.
 *
 *	@param pArgs	The arguments for the factory.
 */
PyObject * TerrainChunkLocator::pyNew( PyObject * pArgs )
{
	BW_GUARD;

	return new TerrainChunkLocator;
}

BW_END_NAMESPACE

