#ifndef TERRAIN_SCENE_VIEW_HPP
#define TERRAIN_SCENE_VIEW_HPP

#include "scene/scene_provider.hpp"

#include "terrain_finder.hpp"

BW_BEGIN_NAMESPACE

class Vector3;

namespace Terrain
{

class ITerrainSceneViewProvider
{
public:
	virtual bool findTerrainBlock( Vector3 const & pos,
		Terrain::TerrainFinder::Details & result ) = 0;

	virtual float findTerrainHeight( const Vector3 & position ) const = 0;
};


class TerrainSceneView : public 
	SceneView<ITerrainSceneViewProvider>
{
public:
	virtual bool findTerrainBlock( Vector3 const & pos,
		Terrain::TerrainFinder::Details & result );

	virtual float findTerrainHeight( const Vector3 & position ) const;

};


} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_SCENE_VIEW_HPP