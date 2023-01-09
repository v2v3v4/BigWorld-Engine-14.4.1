#ifndef CHUNK_UMBRA_HPP
#define CHUNK_UMBRA_HPP

#include "umbra_config.hpp"
#include "cstdmf/bw_functor.hpp"


#if UMBRA_ENABLE

namespace Umbra
{
	namespace OB 
	{
		class Commander;
	}
};


BW_BEGIN_NAMESPACE

class ChunkCommander;
class ChunkUmbraSystemServices;
class ChunkUmbraGraphicsServices;
class Chunk;
class UmbraDrawItem;
class DataSection;

namespace Moo
{
	class DrawContext;
}
/**
 *	This class creates and destroys umbra objects it also contains
 *	the umbra commander for the chunks
 */
class ChunkUmbra
{
public:
	enum OBJECT_TYPE_MASK
	{
		SCENE_OBJECT  = 1,
		SHADOW_OBJECT = 1 << 1,
	};

	ChunkUmbra( SmartPointer<DataSection> configSection = NULL );
	~ChunkUmbra();

	Umbra::OB::Commander* pCommander(); 

	void terrainOverride( SmartPointer< BWBaseFunctor0 > pTerrainOverride );
	void repeat( Moo::DrawContext& drawContext );
	void tick();

	void drawContext( Moo::DrawContext* drawContext );
	
	void addUpdateItem( UmbraDrawItem* ptr );
	void delUpdateItem( UmbraDrawItem* ptr );
	void clearUpdateItems();

	// controls
	bool drawObjectBounds() const;
	void drawObjectBounds( bool b );

	bool drawVoxels() const;
	void drawVoxels( bool b );

	bool drawQueries() const;
	void drawQueries( bool b );

	bool drawShadowBoxes() const { return drawShadowBoxes_; } 
	void drawShadowBoxes( bool b ) { drawShadowBoxes_ = b; }

	bool occlusionCulling() const { return occlusionCulling_; } 
	void occlusionCulling(bool b) { occlusionCulling_ = b; }

	bool latentQueries() const { return latentQueries_; }
	void latentQueries(bool b) { latentQueries_ = b; }

#if ENABLE_WATCHERS
	bool umbraEnabled() const { return umbraEnabled_ && umbraWatcherEnabled_; }
#else
	bool umbraEnabled() const { return umbraEnabled_; }
#endif

	void umbraEnabled(bool b) { umbraEnabled_ = b; }

	bool castShadowCullingEnabled() const { return castShadowCullingEnabled_ && umbraEnabled_; }
	void castShadowCullingEnabled( bool b ) 
	{
		cullMask_ = ( b ) ? cullMask_ | SHADOW_OBJECT: cullMask_ & ~SHADOW_OBJECT;
		castShadowCullingEnabled_ = b; 
	}

	bool flushTrees() const { return flushTrees_; }
	void flushTrees(bool b) { flushTrees_ = b; }

	bool flushBatchedGeometry() const { return flushBatchedGeometry_; }
	void flushBatchedGeometry(bool b) { flushBatchedGeometry_ = b; }

	bool wireFrameTerrain() const { return wireFrameTerrain_; }
	void wireFrameTerrain( bool wireFrameTerrain ) { wireFrameTerrain_ = wireFrameTerrain; }

	unsigned int cullMask() const { return cullMask_; }
	void cullMask( unsigned int mask ) { cullMask_ = mask; }

	bool distanceEnabled() const { return distanceEnabled_; }
	void distanceEnabled( bool b ) { distanceEnabled_ = b; }

	float shadowDistanceVisible() const { return shadowDistanceVisible_ + shadowDistanceDelta(); }
	void  shadowDistanceVisible( float dist ) { shadowDistanceVisible_ = dist; }

	float shadowDistanceDelta() const { return shadowDistanceDelta_; }
	void  shadowDistanceDelta( float dist ) { shadowDistanceDelta_ = dist; }

#if ENABLE_WATCHERS
	bool umbraWatcherEnabled() const { return umbraWatcherEnabled_; }
	void umbraWatcherEnabled(bool b) { umbraWatcherEnabled_ = b; }
#endif

private:
	void initControls();

	class Statistic;

	ChunkCommander*				pCommander_;
	ChunkUmbraSystemServices*	pSystemServices_;
	ChunkUmbraGraphicsServices*	pGraphicsServices_;
	BW::set<UmbraDrawItem*>		updateItems_;
	// controls
	bool occlusionCulling_;
	bool umbraEnabled_;
	bool flushTrees_;
	bool flushBatchedGeometry_;
	bool wireFrameTerrain_;
	bool castShadowCullingEnabled_;
	bool drawShadowBoxes_;
	bool latentQueries_;
	bool distanceEnabled_;
	int  wireFrameMode_;
	unsigned int cullMask_;
	float shadowDistanceVisible_;
	float shadowDistanceDelta_;

#if ENABLE_WATCHERS
	bool umbraWatcherEnabled_;
#endif

};

BW_END_NAMESPACE

#endif // UMBRA_ENABLE

#endif // CHUNK_UMBRA_HPP
