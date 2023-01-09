#ifndef WORLD_APP_HPP
#define WORLD_APP_HPP


#include "cstdmf/main_loop_task.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{
	class Manager;
}

class AssetClient;

/**
 *	World task
 */
class WorldApp : public MainLoopTask
{
public:
	WorldApp();
	~WorldApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void updateAnimations( float dGameTime ) const;
	virtual void draw();
	virtual void inactiveTick( float dGameTime, float dRenderTime );

	static WorldApp instance;

	bool canSeeTerrain() const					{ return canSeeTerrain_; }

	void assetClient( AssetClient* pAssetClient );

private:
	bool canSeeTerrain_;
	float dGameTime_;

	typedef std::auto_ptr< Terrain::Manager > TerrainManagerPtr;
	TerrainManagerPtr terrainManager_;

public:
	uint32				wireFrameStatus_;
	uint32				debugSortedTriangles_;
	AssetClient*		pAssetClient_;
};

BW_END_NAMESPACE


#endif // WORLD_APP_HPP


// world_app.hpp
