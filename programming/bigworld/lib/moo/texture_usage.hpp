#ifndef TEXTURE_USAGE_HPP
#define TEXTURE_USAGE_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/object_pool.hpp"
#include "moo/base_texture.hpp"
#include "math/boundbox.hpp"
#include "math/sphere.hpp"
#include "texture_streaming_scene_view.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

class StreamingTexture;

class ModelTextureUsage
{
public:
	static const float INVALID_DENSITY;
	static const float MIN_DENSITY;

	ModelTextureUsage();

	void initialise();

	Sphere bounds_;
	SmartPointer< BaseTexture > pTexture_;
	float worldScale_;
	float uvDensity_;
};

class ModelTextureUsageContext
{
public:
	typedef Handle< 20 > Handle;

	void lock();
	void unlock();

	Handle registerUsage( const BaseTexturePtr& pTexture );
	void unregisterUsage( Handle handle );
	ModelTextureUsage * getUsage( Handle handle );

	size_t size();
	void getUsages( const ModelTextureUsage** pUsages, 
		size_t* numUsage );

private:
	mutable SimpleMutex textureUsageLock_;
	typedef PackedObjectPool< ModelTextureUsage, Handle > 
		ModelTextureUsageArray;
	ModelTextureUsageArray textureUsage_;
};

class ModelTextureUsageGroup
{
public:
	typedef ModelTextureUsageContext Context;
	typedef Context::Handle Handle;
	typedef BW::map< BaseTexture *, Handle > ModelTextureUsageMap;

	class Data
	{
	public:
		float worldScale_;
		Sphere worldBoundSphere_;
		ModelTextureUsageMap usages_;
	};

	ModelTextureUsageGroup( Data& data, Context& provider );
	~ModelTextureUsageGroup();

	void setWorldScale_FromTransform( const Matrix & xform );
	void setWorldScale( float scale );
	void setWorldBoundSphere( const Sphere & sphere );
	void setTextureUsage( BaseTexture * pTexture, float uvDensity ); 
	void removeTextureUsage( BaseTexture * pTexture ); 
	
	void clearTextureUsage();
	void applyObjectChanges();

private:

	static Sphere calcBoundSphere( const Matrix & xform, const BoundingBox & bb );
	static float calcMaxScale( const Matrix & xform );

	bool isObjectDataDirty_;
	Data& data_;
	Context& provider_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // TEXTURE_USAGE_HPP