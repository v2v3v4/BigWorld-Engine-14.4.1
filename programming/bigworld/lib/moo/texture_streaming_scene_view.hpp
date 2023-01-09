#ifndef TEXTURE_STREAMING_SCENE_VIEW_HPP
#define TEXTURE_STREAMING_SCENE_VIEW_HPP

#include "texture_streaming_manager.hpp"
#include "scene/scene_provider.hpp"
#include "scene/scene_object.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

class ModelTextureUsage;
class ModelTextureUsageGroup;
class StreamingTexture;
class ModelTextureUsageProxy;

class ITextureStreamingObjectOperationTypeHandler
{
public:
	virtual ~ITextureStreamingObjectOperationTypeHandler() {}

	virtual bool registerTextureUsage( 
		const SceneObject& object,
		ModelTextureUsageGroup& usageGroup ) = 0;
	virtual bool updateTextureUsage( 
		const SceneObject& object, 
		ModelTextureUsageGroup& usageGroup ) = 0;
};


class TextureStreamingObjectOperation
	: public SceneObjectOperation<ITextureStreamingObjectOperationTypeHandler>
{
public:

	bool registerTextureUsage( const SceneObject& object,
		ModelTextureUsageGroup& usageGroup );
	bool updateTextureUsage( const SceneObject& object, 
		ModelTextureUsageGroup& usageGroup );
};


class ITextureStreamingSceneViewProvider
{
public:
	typedef TextureStreamingManager::TextureStreamingUsageCollector 
		TextureStreamingUsageCollector;
	typedef TextureStreamingManager::ViewpointData ViewpointData;

	virtual ~ITextureStreamingSceneViewProvider() {}

	virtual void calculateIdealMips( 
		TextureStreamingUsageCollector& usageCollector, 
		const ViewpointData & viewpoint ) = 0;
	virtual void drawDebug( uint32 debugMode ) {};
	virtual uint32 usageCount() {return 0;};
};


class BaseTextureStreamingSceneViewProvider :
	public ITextureStreamingSceneViewProvider
{
public:
	virtual void beginProcessing();
	virtual void endProcessing();
	virtual void getModelUsage( 
		const ModelTextureUsage ** pUsages, 
		size_t* numUsage ) = 0;

	virtual void calculateIdealMips( 
		TextureStreamingUsageCollector& usageCollector,
		const ViewpointData & viewpoint );
	virtual void drawDebug( uint32 debugMode );
	virtual uint32 usageCount();

protected:
	void drawDebugUsages( uint32 color );
};


class TextureStreamingSceneView : 
	public SceneView<ITextureStreamingSceneViewProvider>
{
public:
	typedef TextureStreamingManager::TextureStreamingUsageCollector 
		TextureStreamingUsageCollector;
	typedef TextureStreamingManager::ViewpointData 
		ViewpointData;
	
	TextureStreamingSceneView();
	~TextureStreamingSceneView();

	void calculateIdealMips( TextureStreamingUsageCollector& usageCollector,
		const ViewpointData & viewpoint );
	void drawDebug( uint32 debugMode );
	uint32 usageCount();

	void ignoreObjectsFrom( SceneProvider* pProvider );
	void unignoreObjectsFrom( SceneProvider* pProvider );

protected:
	
	virtual void onSetScene( Scene* pOldScene, Scene* pNewScene );

	class RuntimeProvider;
	RuntimeProvider* pRuntimeProvider_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // TEXTURE_USAGE_HPP