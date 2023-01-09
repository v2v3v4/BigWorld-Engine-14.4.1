#ifndef TERRAIN_LOD_CONTROLLER_HPP
#define TERRAIN_LOD_CONTROLLER_HPP

#include "math/vector3.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{

class TerrainBlock2;

#if 0 // unused?
/**
 *	This class implements the lod controller for the terrain, 
 *	it manages all the different lod styles for the terrain, such
 *	as the vertex lod, the texture lod, the normal map lod etc.
 */
class TerrainLodController : public ReferenceCount
{
public:
	TerrainLodController();
	~TerrainLodController();

	void	cameraPosition( const Vector3& value );

	void	addBlock( TerrainBlock2* pBlock, const Vector3& centre );
	void	delBlock( TerrainBlock2* pBlock );

private:
	Vector3		cameraPosition_;
	int32		blockFocusX_;
	int32		blockFocusZ_;
	int32		blockWindowX_;
	int32		blockWindowZ_;
	int32		windowDims_;
	bool		initialised_;

	typedef		BW::vector<TerrainBlock2*> Blocks;
	Blocks		activeBlocks_;

	typedef		std::pair<int32, int32> LocationToken;
	typedef		BW::map<LocationToken, TerrainBlock2*> BlockMap;

	BlockMap	homeless_;
};
#endif

class BasicTerrainLodController
{
public:	
	BasicTerrainLodController();

	~BasicTerrainLodController();

	void	setCameraPosition( const Vector3& position, float zoomFactor );
	void	addBlock( TerrainBlock2* pBlock, const Matrix& worldTransform );
	bool	delBlock( TerrainBlock2* pBlock );

	float closestUnstreamedBlock() const { return closestUnstreamedBlock_; }

	static BasicTerrainLodController& instance();

private:
	uint32		getNumBlocks() const
	{
		MF_ASSERT( blocks_.size() <= UINT_MAX );
		return ( uint32 ) blocks_.size();
	}

	// This is the distance to the first block that still needs to stream 
	// at least one LOD. FLT_MAX means there are none.
	float closestUnstreamedBlock_;

	// This is the list of block entries, so controller may update them at will.
	typedef		std::pair< Matrix, TerrainBlock2* > BlockEntry; 
	typedef		BW::vector< BlockEntry > BlockContainer;
	BlockContainer	blocks_;

	// This serialises access to the list, so we don't try to update a partially
	// inserted block entry.
	SimpleMutex		accessMutex_;
};

} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_LOD_CONTROLLER_HPP
