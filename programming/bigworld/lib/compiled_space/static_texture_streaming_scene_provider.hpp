#ifndef STATIC_TEXTURE_STREAMING_SCENE_PROVIDER_HPP
#define STATIC_TEXTURE_STREAMING_SCENE_PROVIDER_HPP

#include "binary_format.hpp"
#include "static_texture_streaming_types.hpp"

#include "compiled_space/loader.hpp"
#include "scene/scene_provider.hpp"
#include "moo/texture_streaming_scene_view.hpp"

namespace BW {
namespace CompiledSpace {

class StaticTextureStreamingSceneProvider :
	public ILoader,
	public SceneProvider,
	public Moo::BaseTextureStreamingSceneViewProvider
{
public:
	StaticTextureStreamingSceneProvider();
	virtual ~StaticTextureStreamingSceneProvider();

protected:
	// ILoader interface
	virtual bool doLoadFromSpace( ClientSpace * pSpace,
		BinaryFormat & reader,
		const DataSectionPtr & pSpaceSettings,
		const Matrix & transform,
		const StringTable & strings );

	virtual bool doBind();
	virtual void doUnload();
	virtual float percentLoaded() const;

	bool isValid() const;

	// Scene view implementations
	virtual void * getView(
		const SceneTypeSystem::RuntimeTypeID & sceneInterfaceTypeID);

	virtual void getModelUsage( 
		const Moo::ModelTextureUsage ** pUsages, 
		size_t * numUsage );

	virtual void drawDebug( uint32 debugMode );

private:
	bool loadUsages( const StringTable & strings );
	void freeUsages();
	
	BinaryFormat * pReader_;
	BinaryFormat::Stream * pStream_;

	ExternalArray<StaticTextureStreamingTypes::Usage> usageDefs_;
	BW::vector<Moo::ModelTextureUsage> usages_;
};

} // namespace CompiledSpace
} // namespace BW

#endif // STATIC_TEXTURE_STREAMING_SCENE_PROVIDER_HPP
