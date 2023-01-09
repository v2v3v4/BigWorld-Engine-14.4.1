#include "pch.hpp"

#include "static_texture_streaming_scene_provider.hpp"

#include "string_table.hpp"

#include "scene/scene.hpp"
#include "space/client_space.hpp"
#include "moo/texture_usage.hpp"
#include "moo/texture_manager.hpp"
#include "moo/streaming_texture.hpp"
#include "moo/texture_streaming_manager.hpp"

namespace BW {
namespace CompiledSpace {


StaticTextureStreamingSceneProvider::StaticTextureStreamingSceneProvider() :
	pReader_( NULL ),
	pStream_( NULL )
{

}


StaticTextureStreamingSceneProvider::~StaticTextureStreamingSceneProvider()
{

}


bool StaticTextureStreamingSceneProvider::doLoadFromSpace( 
	ClientSpace * pSpace,
	BinaryFormat & reader,
	const DataSectionPtr & pSpaceSettings,
	const Matrix & transform,
	const StringTable & strings )
{
	PROFILER_SCOPED( StaticTextureStreamingSceneProvider_doLoadFromSpace );

	MF_ASSERT( pReader_ == NULL );
	pReader_ = &reader;

	pStream_ = pReader_->findAndOpenSection(
		StaticTextureStreamingTypes::FORMAT_MAGIC,
		StaticTextureStreamingTypes::FORMAT_VERSION,
		"StaticTextureStreamingSceneProvider" );
	if (!pStream_)
	{
		ASSET_MSG( "Failed to map '%s' into memory.\n",
			pReader_->resourceID() );
		this->unload();
		return false;
	}

	if (!pStream_->read( usageDefs_ ))
	{
		ASSET_MSG( "StaticTextureStreamingSceneProvider in '%s' has "
			"incomplete data.\n",
			pReader_->resourceID() );
		this->unload();
		return false;
	}

	if (!this->loadUsages( strings ))
	{
		this->unload();
		return false;
	}

	return true;
}


bool StaticTextureStreamingSceneProvider::doBind()
{
	if (isValid())
	{
		space().scene().addProvider( this );
	}

	return true;
}


void StaticTextureStreamingSceneProvider::doUnload()
{
	space().scene().removeProvider( this );

	this->freeUsages();
	
	if (pStream_ && pReader_)
	{
		pReader_->closeSection( pStream_ );
	}

	pReader_ = NULL;
	pStream_ = NULL;
}


float StaticTextureStreamingSceneProvider::percentLoaded() const
{
	return 1.0f;
}


bool StaticTextureStreamingSceneProvider::isValid() const
{
	return usageDefs_.valid();
}


void * StaticTextureStreamingSceneProvider::getView(
	const SceneTypeSystem::RuntimeTypeID & viewTypeID)
{
	void * result = NULL;

	exposeView<ITextureStreamingSceneViewProvider>( this, viewTypeID, result );
	
	return result;
}


void StaticTextureStreamingSceneProvider::getModelUsage( 
	const Moo::ModelTextureUsage ** pUsages, 
	size_t * numUsage )
{
	*numUsage = usages_.size();
	if (*numUsage)
	{
		*pUsages = &usages_.front();
	}
}


bool StaticTextureStreamingSceneProvider::loadUsages( 
	const StringTable & strings )
{
	// Iterate over all usage defs and instantiate a model usage for 
	// each texture
	usages_.reserve( usageDefs_.size() );

	uint32 previousTextureResourceID = uint32(~0);
	Moo::BaseTexturePtr pTexture = NULL;

	for (auto iter = usageDefs_.begin(); iter != usageDefs_.end(); ++iter)
	{
		StaticTextureStreamingTypes::Usage & def = *iter;
		
		Moo::ModelTextureUsage usage;
		usage.bounds_ = def.bounds_;
		usage.uvDensity_ = def.uvDensity_;
		usage.worldScale_ = def.worldScale_;

		// Extract the texture
		if (def.resourceID_ != previousTextureResourceID)
		{
			previousTextureResourceID = def.resourceID_;
			const char * resource = strings.entry( def.resourceID_ );
			MF_ASSERT( resource );
			pTexture = Moo::TextureManager::instance()->get( resource );
		}
		
		usage.pTexture_ = pTexture;

		if (pTexture && 
			pTexture->type() == Moo::BaseTexture::STREAMING)
		{
			usages_.push_back( usage );
		}
	}

	return true;
}


void StaticTextureStreamingSceneProvider::freeUsages()
{
	// Usage destructors will let go of all the relevant textures
	// involved here
	usages_.clear();
}


void StaticTextureStreamingSceneProvider::drawDebug( uint32 debugMode )
{
	if (debugMode != Moo::DebugTextures::DEBUG_TS_OBJECT_BOUNDS)
	{
		// Only supporting drawing object bounds for now
		return;
	}

	// Draw using a dark blue color
	drawDebugUsages( 0x80000080 );
}


} // namespace CompiledSpace
} // namespace BW
