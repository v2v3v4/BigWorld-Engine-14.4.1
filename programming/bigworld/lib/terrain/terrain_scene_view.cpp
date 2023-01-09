#include "pch.hpp"
#include "terrain_scene_view.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{
	bool TerrainSceneView::findTerrainBlock( Vector3 const & pos,
			Terrain::TerrainFinder::Details & result )
	{
		for ( ProviderCollection::const_iterator iter = providers_.begin();
			iter != providers_.end(); ++iter )
		{
			ITerrainSceneViewProvider* pProvider = *iter;
			if (pProvider->findTerrainBlock( pos, result ))
			{
				return true;
			}
		}

		return false;
	}

	float TerrainSceneView::findTerrainHeight( const Vector3 & position ) const
	{
		for ( ProviderCollection::const_iterator iter = providers_.begin();
			iter != providers_.end(); ++iter )
		{
			ITerrainSceneViewProvider* pProvider = *iter;
			float height = pProvider->findTerrainHeight( position );
			if (height != FLT_MAX)
			{
				return height;
			}
		}

		return 0.f;
	}

} // namespace Terrain

BW_END_NAMESPACE