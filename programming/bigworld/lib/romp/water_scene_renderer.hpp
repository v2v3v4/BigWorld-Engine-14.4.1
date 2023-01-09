#ifndef _WATER_SCENE_RENDERER_HPP
#define _WATER_SCENE_RENDERER_HPP

#include "moo/texture_renderer.hpp"
#include "math/planeeq.hpp"


BW_BEGIN_NAMESPACE

class PyModel;
class Water;
class ChunkSpace;
typedef SmartPointer<ChunkSpace> ChunkSpacePtr;

namespace Moo
{
	class LightContainer;
	typedef SmartPointer<LightContainer>	LightContainerPtr;
	class DrawContext;
};

class WaterSceneRenderer;

/**
 * TODO: to be documented.
 */
class WaterScene : public TextureSceneInterface, public ReferenceCount
{
public:
	WaterScene( float height, float waveHeight = 0.0f );
	virtual ~WaterScene();

	bool checkOwners() const;
	void addOwner( const Water* owner ) { owners_.push_back(owner); }
	void removeOwner( const Water* owner ) { owners_.remove(owner); }
	bool getVisibleChunks( BW::vector<class Chunk*>& visibleChunks, bool& outsideVisible, BW::vector<Vector3>& verts );
#ifdef EDITOR_ENABLED
	bool drawRed();
	bool highlight();
#endif

	bool recreate( );
	void deleteUnmanagedObjects();
	bool closeToWater( const Vector3& pos );
	float waterHeight() const { return waterHeight_; }
	float waterWaveHeight() const { return waterWaveHeight_; }
	const Vector4& cameraPos() const { return camPos_; }
	bool shouldReflect( const Vector3& pos, PyModel* model );
	void setCameraPos( const Vector4& val ) { camPos_ = val; }

	void dynamic( bool isDynamic );	
	void staggeredDynamic( bool state );
	bool dynamic() const { return dynamic_; }

	Moo::RenderTargetPtr reflectionTexture();

	virtual bool cachable() const { return true; }
	virtual bool shouldDraw() const;
	virtual void render( float dTime );

	//TODO: create some general purpose clip plane functionality
	PlaneEq currentClipPlane() 
	{ 
		return PlaneEq(Vector3(0,waterHeight(),0), Vector3(0,1,0));
	}

	bool eyeUnderWater() const;

private:
	typedef BW::list< const Water* > OwnerList;

	bool								dynamic_;
	//bool								drawingReflection_;	
	float								waterHeight_;
	float								waterWaveHeight_;
	Vector4								camPos_;
	OwnerList							owners_;

	WaterSceneRenderer*					reflectionScene_;
//	WaterSceneRenderer*					refractionScene_;	
};


/**
 * TODO: to be documented.
 */
class WaterSceneRenderer : public TextureRenderer
{
public:
	WaterSceneRenderer( WaterScene* scene, bool reflect = false, D3DFORMAT format = D3DFMT_A8R8G8B8 );
	virtual ~WaterSceneRenderer();

	virtual void renderSelf( float dTime );

	bool recreate( );
	void deleteUnmanagedObjects();

	void setTexture( Moo::RenderTargetPtr texture ) { pRT_ = texture; }
	Moo::RenderTargetPtr getTexture() { return pRT_; }

	virtual bool shouldDraw() const;

	static void setPlayerModel( PyModel* pModel ) { playerModel_ = pModel; }
	static PyModel* playerModel(  ) { return playerModel_; }

	static void setSkyBoxModel( const PyModel * pModel ) { skyBoxModel_ = pModel; }
	static const PyModel* skyBoxModel(  ) { return skyBoxModel_; }

	static void incReflectionCount() { reflectionCount_++; }
	static uint32 reflectionCount() { return reflectionCount_; }
	void clearReflectionCount() { reflectionCount_=0; }

	static WaterScene* currentScene() { return currentScene_; }
	static void currentScene( WaterScene * scene ){ currentScene_ = scene; }

	static float currentCamHeight() { return currentScene()->cameraPos().y; }

	float waterHeight() const { return scene_->waterHeight(); }

	WaterScene* scene() { return scene_; }

	bool shouldReflect( const Vector3& pos, PyModel* model );

	static void resetSceneCounter() { drawnSceneCount_=0; }

private:
	WaterSceneRenderer( const WaterSceneRenderer& );
	WaterSceneRenderer& operator=( const WaterSceneRenderer& );

	void createRenderTarget();

	//TODO:
//	void resetDrawState();
//	void setDrawState();


	//TODO: remove useless vars	
	bool							reflection_;
	WaterScene*						scene_;

	BW::string						sceneName_;

	static PyModel*					playerModel_;
	static const PyModel*			skyBoxModel_;
	static uint32					reflectionCount_;
	static WaterScene*				currentScene_;
	static int						drawnSceneCount_;

public:
	bool							eyeUnderWater_;

	static ChunkSpacePtr			s_pSpace_;
	static Moo::LightContainerPtr	s_lighting_;

	static bool						s_drawTrees_;
	static bool						s_drawDynamics_;
	static bool						s_drawPlayer_;
	static uint						s_maxReflections_;
	static float					s_maxReflectionDistance_;
	static bool						s_simpleScene_;
	static bool						s_useClipPlane_;
	static float					s_textureSize_;

	Moo::DrawContext*				waterDrawContext_;
};

BW_END_NAMESPACE

#endif // _WATER_SCENE_RENDERER_HPP
