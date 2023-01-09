#ifndef TERRAIN_TERRAIN_VERTEX_LOD_HPP
#define TERRAIN_TERRAIN_VERTEX_LOD_HPP

#include "terrain_block2.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{
    /**
     *	This class is a helper class for calculating lod and lod values for the 
     *  terrain.
     */
	class TerrainVertexLod : public BW::vector<float>
    {
    public:

		uint32	calculateLodLevel( float dist, const Terrain::TerrainSettings* settings ) const;

		void	calculateMasks(	const TerrainBlock2::DistanceInfo& distanceInfo,
								TerrainBlock2::LodRenderInfo& lodRenderInfo ) 
								const;

		float	distance( uint32 lod ) const;

		void	distance( uint32 lod, float value );

		float	startBias() const { return startBias_; }
		void	startBias( float value ) { startBias_ = value; }
		float	endBias() const { return endBias_; }
		void	endBias( float value ) { endBias_ = value; }

		void	setBlockSize( float value );

		static float xzDistance(const Vector3&	chunkCorner,
								float			blockSize,
								const Vector3&	cameraPosition );
		static void minMaxXZDistance( const Vector3& relativeCameraPos,
			const float blockSize, float& minDistance, float& maxDistance );

		void getDistanceForLod( uint32 lodLevel, float & o_MinDistance, float & o_MaxDistance ) const;

		static void	changeZoomFactor(float zoomFactor);

	private:
		// Test sub-blocks against themselves for degenerates.
		void internalSubBlockTests(	NeighbourMasks&  neighbourMasks,
									uint8			subBlockMask ) const;

		void externalSubBlockTests( uint32			mainBlockLod,
									uint8			subBlockMask,
									const Vector3&  cameraPosition,
									NeighbourMasks& neighbourMasks ) const;


		Vector2 calcMorphRanges( uint32 lodLevel ) const;

		// Start end bias for vertex lod, these are the proportions
		// of the lod-deltas the geomorphing happens between.
		float	startBias_;
		float	endBias_;
		float	blockSize_;

		//-- scale factors for lod distances in case if we change FOV angle of the camera.
		//--	 This may be useful for instance for sniper mode where we need zooming main camera.
		static float s_zoomFactor_;
		static float s_invZoomFactor_;
	};

} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_TERRAIN_VERTEX_LOD_HPP
