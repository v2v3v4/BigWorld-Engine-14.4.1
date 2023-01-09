#include "pch.hpp"

#include <algorithm>
#include "cstdmf/bw_string.hpp"

#include "moo/convert_texture_tool.hpp"

#include "base_texture.hpp"
#include "texture_manager.hpp"
#include "texture_helpers.hpp"
#include "texture_streaming_manager.hpp"
#include "streaming_texture.hpp"
#include "texture_compressor.hpp"
#include "animating_texture.hpp"
#include "render_context.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "format_name_map.hpp"
#include "sys_mem_texture.hpp"

#include "cstdmf/binaryfile.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "resmgr/auto_config.hpp"
#include "managed_texture.hpp"
#include "resource_load_context.hpp"

#include "cstdmf/bw_util.hpp"
#include "cstdmf/singleton_manager.hpp"

#if ENABLE_ASSET_PIPE
#include "asset_pipeline/asset_client.hpp"
#endif

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

static AutoConfigString s_notFoundBmp( "system/notFoundBmp" );


// Helper functions
namespace
{
	char toLower( char ch )
	{
		if (ch >= 'A' && ch <= 'Z')
		{
			return ch - 'A' + 'a';
		}
		return ch;
	}


	int calcMipSkip(int qualitySetting, int lodMode)
	{	
		int result = 0;

		switch (lodMode)
		{
			case 0: // disabled
				result = 0;
				break;
			case 1: // normal
				result = qualitySetting;
				break;
			case 2: // low bias
				result = std::min(qualitySetting, 1);
				break;
			case 3: // high bias
				result = std::max(qualitySetting-1, 0);
				break;
		}
		return result;
	}
}

namespace Moo
{

/**
 * Default constructor.
 */
TextureManager::TextureManager()
	: fullHouse_( false )
	, useDummyTexture_( false )
	, pDetailLevels_( NULL )
#if ENABLE_ASSET_PIPE
	, pAssetClient_(NULL)
#endif
{
	BW_GUARD;

	streamingManager_.reset( new TextureStreamingManager( this ) );

	// texture quality settings
	this->qualitySettings_ = 
		Moo::makeCallbackGraphicsSetting(
			"TEXTURE_QUALITY", "Texture Quality", *this, 
			&TextureManager::setQualityOption, 
			0, true, false);

	this->qualitySettings_->addOption("HIGH", "High", true);
	this->qualitySettings_->addOption("MEDIUM", "Medium", true);
	this->qualitySettings_->addOption("LOW", "Low", true);
	Moo::GraphicsSetting::add(this->qualitySettings_);

	// texture compression settings
	this->compressionSettings_ = 
		Moo::makeCallbackGraphicsSetting(
			"TEXTURE_COMPRESSION", "Texture Compression", *this, 
			&TextureManager::setCompressionOption, 
			0, true, false);

	this->compressionSettings_->addOption("ON", "On", true);
	this->compressionSettings_->addOption("OFF", "Off", true);
	Moo::GraphicsSetting::add(this->compressionSettings_);

	// texture filtering settings
	this->filterSettings_ = 
		Moo::makeCallbackGraphicsSetting(
			"TEXTURE_FILTERING", "Texture Filtering", *this, 
			&TextureManager::setFilterOption, 
			0, false, false);

	this->filterSettings_->addOption("ANISOTROPIC_16X", "Anisotropic (16x)", Moo::rc().maxAnisotropy() >= 16);
	this->filterSettings_->addOption("ANISOTROPIC_8X",  "Anisotropic (8x)", Moo::rc().maxAnisotropy() >= 8);
	this->filterSettings_->addOption("ANISOTROPIC_4X",  "Anisotropic (4x)", Moo::rc().maxAnisotropy() >= 4);
	this->filterSettings_->addOption("ANISOTROPIC_2X",  "Anisotropic (2x)", Moo::rc().maxAnisotropy() >= 2);
	this->filterSettings_->addOption("TRILINEAR", "Trilinear", true);
	this->filterSettings_->addOption("BILINEAR", "Bilinear", true);
	this->filterSettings_->addOption("POINT", "Point", true);
	Moo::GraphicsSetting::add(this->filterSettings_);

	// saved settings may differ from the ones set 
	// in the constructor call (0). If that is the case,
	// both settings will be pending. Commit them now.
	Moo::GraphicsSetting::commitPending();
	BWResource::instance().addModificationListener( this );

	pDetailLevels_ = new TextureDetailLevelManager();
	MF_ASSERT( pDetailLevels_ );
	pDetailLevels_->init();
}

/**
 * Destructor.
 */
TextureManager::~TextureManager()
{
	BW_GUARD;

	// Check if BWResource singleton exists as this singleton could
	// be destroyed afterwards
	if (BWResource::pInstance() != NULL)
	{
		BWResource::instance().removeModificationListener( this );
	}

	while (!textures_.empty())
	{
		INFO_MSG( "[TextureManager] -- Texture not properly deleted on exit: %s\n",
			textures_.begin()->first.to_string().c_str() );

		textures_.erase(textures_.begin());
	}

	
	MF_ASSERT( pDetailLevels_ );
	pDetailLevels_->fini();
	bw_safe_delete( pDetailLevels_ );
}

/**
 * Static instance accessor.
 */
TextureManager* TextureManager::instance()
{
	SINGLETON_MANAGER_WRAPPER( TextureManager )
	return pInstance_;
}

/**
 *	Static method to return the default texture used as a fallback for missing
 *	textures.
 */
/*static*/ const BW::string & TextureManager::notFoundBmp()
{
	return s_notFoundBmp.value();
}


/**
 * Load all contained textures.
 */
HRESULT TextureManager::initTextures( )
{
	BW_GUARD;
	HRESULT res = S_OK;
	TextureMap::iterator it = textures_.begin();

	while( it != textures_.end() )
	{
		(it->second)->load();
		it++;
	}
	return res;
}

/**
 * Release all textures.
 */
void TextureManager::releaseTextures( )
{
	BW_GUARD;
	for (TextureMap::iterator it = textures_.begin();
		it != textures_.end(); ++it )
	{
		(*it).second->release();
	}
}

/**
 * Release all DX managed resources.
 */
void TextureManager::deleteManagedObjects( )
{
	BW_GUARD;
	DEBUG_MSG( "Moo::TextureManager - Used texture memory %d\n", textureMemoryUsed() );
	releaseTextures();
}

/**
 * Create all DX managed resources.
 */
void TextureManager::createManagedObjects( )
{
	BW_GUARD;
	initTextures();
}

/**
 * This method returns total texture memory used in bytes.
 */
uint32 TextureManager::textureMemoryUsed( ) const
{
	BW_GUARD;
	const BW::string animated( ".texanim" );

	TextureMap::const_iterator it = textures_.begin();
	TextureMap::const_iterator end = textures_.end();
	uint32 tm = 0;
	while( it != end )
	{
		const BW::string& id = it->second->resourceID();
		if (id.length() < animated.length() ||
            id.substr( id.length() - animated.length(), animated.length() ) != animated )
		{
			tm += (it->second)->textureMemoryUsed( );
		}
		++it;
	}
	return tm;
}


/**
 *	Set whether or not we have a full house
 *	(and thus cannot load any new textures)
 */
void TextureManager::fullHouse( bool noMoreEntries )
{
	fullHouse_ = noMoreEntries;
}



/**
 *	This method saves a dds file from a texture.
 *
 *	@param texture the DX::BaseTexture to save out (important has to be 32bit colour depth.
 *	@param ddsName the filename to save the file to.
 *	@param format the TF_format to save the file in.
 *	@param	numDestMipLevels	Number of Mip levels to save in the DDS.
 *
 *	@return true if the function succeeds.
 */
bool TextureManager::writeDDS( DX::BaseTexture* texture, const BW::StringRef& ddsName,
								D3DFORMAT format, int numDestMipLevels )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( texture )
	{
		return false;
	}

	bool ret = false;

	// write out the dds
	D3DRESOURCETYPE resType = texture->GetType();
	if (resType == D3DRTYPE_TEXTURE)
	{
		TextureCompressor tc( (DX::Texture*)texture );

		ret = tc.save( ddsName, format, numDestMipLevels );
	}

	return ret;
}



#define DEFAULT_TGA_FORMAT D3DFMT_DXT3
#define DEFAULT_BMP_FORMAT D3DFMT_DXT1




/**
 *	This method reloads all the textures used from disk.
 */
void TextureManager::reloadAllTextures( ProgressListener* pCallBack )
{
	BW_GUARD;
	int finishedCount = 0;
	TextureMap::iterator it = textures_.begin();
	TextureMap::iterator end = textures_.end();
	while (it != end)
	{
		char resourceName[BW_MAX_PATH];
		BaseTexture * pTex = it->second;
		bw_str_copy( resourceName, ARRAY_SIZE( resourceName ),
			pTex->resourceID() );
		if (BWResource::getExtension(resourceName) == "dds")
		{
			BWResource::instance().purge( pTex->resourceID() );
			prepareResource( resourceName, resourceName,
				ARRAY_SIZE( resourceName ), true );
		}		
		it++;
		finishedCount++;
		if (pCallBack)
		{
			pCallBack->onTextureManagerProgress( ProgressListener::PURGE, 
				static_cast<int>(textures_.size()), finishedCount );
		}
	}	

	finishedCount = 0;
	it = textures_.begin();
	end = textures_.end();
	while (it != end)
	{
		BaseTexture *pTex = it->second;
		if ( pTex->resourceID() == s_notFoundBmp.value() )
		{
			char tempBuffer[BW_MAX_PATH];
			pTex->reload( prepareResource( it->first, tempBuffer,
				ARRAY_SIZE( tempBuffer ) ) );
		}
		else
		{
			pTex->reload();
		}
		it++;
		finishedCount++;
		if (pCallBack)
		{
			pCallBack->onTextureManagerProgress( ProgressListener::RELOAD, 
				static_cast<int>(textures_.size()), finishedCount );
		}
	}	
}




/**
 *	This method gets the given texture resource.
 *
 *  @param name				The name of the resource texture to get.
 *  @param allowAnimation	Can the texture be animated?
 *  @param mustExist		Attempt to load the not-found texture on failure.
 *  @param loadIfMissing	Load the texture if missing?
 *	@return the resource if it loaded, otherwise NULL.
 */
BaseTexturePtr TextureManager::get( const BW::StringRef& resourceID,
	bool allowAnimation, bool mustExist, bool loadIfMissing,
	const char* allocator )
{
	BW_GUARD;
	PROFILER_SCOPED( TextureManager_get );

	char nameBuffer[BW_MAX_PATH];
	const BW::StringRef name = TextureHelpers::copyNormalisedTextureName( 
		useDummyTexture_ ? s_notFoundBmp.value() : resourceID,
		nameBuffer, ARRAY_SIZE( nameBuffer ) );
	const BW::StringRef baseName = removeTextureExtension( name );


	if (name.empty())
	{
#ifndef EDITOR_ENABLED
		ASSET_MSG( "Moo::TextureManager::get: empty texture path was requested!\n" );
#endif
		return NULL;
	}

	#define DISABLE_TEXTURING 0
	#if DISABLE_TEXTURING

	BW::string saniName      = this->prepareResourceName(name);
	TextureDetailLevelPtr lod = this->pDetailLevels_->detailLevel(saniName);
	D3DFORMAT format          = lod->format();

	typedef BW::map<D3DFORMAT, BaseTexturePtr> BaseTextureMap;
	static BaseTextureMap s_debugTextureMap;
	BaseTextureMap::const_iterator textIt = s_debugTextureMap.find(format);
	if (textIt != s_debugTextureMap.end())
	{
		return textIt->second;
	}
	#endif // DISABLE_TEXTURING

	BaseTexturePtr res = NULL;

	if (name.length() > 3)
	{
		if (allowAnimation)
		{
			char tempBuffer[BW_MAX_PATH];
			res = this->find( 
				TextureHelpers::copyBaseNameAndExtension( baseName, "texanim",
					tempBuffer, ARRAY_SIZE( tempBuffer) ) );
			if (res) return res;
		}
	}

	// check the cache directly
	res = this->find( name );
	if (res) return res;

	// try it as an already loaded dds then
	{
		char ddsNameBuffer[BW_MAX_PATH];
		const BW::StringRef ddsName = name.empty() ? name : 
			TextureHelpers::copyBaseNameAndExtension( baseName, "dds",
			ddsNameBuffer, ARRAY_SIZE( ddsNameBuffer ) );
		res = this->find( ddsName );
		if (res)
		{
			return res;
		}
	}

	if (!loadIfMissing)
	{
		return NULL;
	}

	char resourceName[BW_MAX_PATH];
	prepareResource( name, resourceName, ARRAY_SIZE( resourceName ), false );

	// alright, we're ready to load something now

	// make sure we're allowed to load more stuff
	if (fullHouse_)
	{
		CRITICAL_MSG( "TextureManager::getTextureResource: "
			"Loading the texture '%s' into a full house!\n",
			resourceName );
	}

	BaseTexturePtr result = NULL;
	result = this->getUnique( name, resourceName, 
		allowAnimation, mustExist, allocator );

#if !CONSUMER_CLIENT_BUILD
	if (result)
	{
		if (BWResource::getExtension( name ) != "texanim" &&
			BWResource::getExtension( resourceName ) != "dds")
		{
			checkLoadedResourceName( name, resourceName ); 
		}
	}
#endif // !CONSUMER_CLIENT_BUILD

	#if DISABLE_TEXTURING
		s_debugTextureMap.insert(std::make_pair(result->format(), result));
	#endif // DISABLE_TEXTURING

	return result;
}

/**
 *	This method gets the given texture resource from a DataSection.
 *
 *  @param data				The DataSection to get the resource from.
 *  @param name				The name of the resource texture to get.
 *  @param mustExist		Attempt to load the not-found texture on failure.
 *  @param loadIfMissing	Load the texture if missing?
 *  @param refresh			Refresh the texture from the data section even if
 *							already in the cache?
 *	@param allocator		This parameter is unused.
 *	@return the resource if it loaded, otherwise NULL.
 */
BaseTexturePtr TextureManager::get( DataSectionPtr data, 
	const BW::StringRef& name, bool mustExist, bool loadIfMissing, bool refresh,
	const char* allocator )
{
	BW_GUARD;
	MF_ASSERT(!name.empty());

	BaseTexturePtr res;

	// Find the value in the cache, and if we are refreshing remove from the
	// cache.  If in the cache and not refreshing then return the cached value:
	res = find(name);
	if (res != NULL && refresh)
	{
		RecursiveMutexHolder lock( texturesLock_ );
		this->delInternal( res.getObject() );
		this->delFromDirtyTextures( res.getObject() );
		res = NULL;
	}
	if (res != NULL)
		return res;

	// Make sure that we're allowed to load more stuff:
	if (fullHouse_)
	{
		CRITICAL_MSG( "TextureManager::getTextureResource: "
			"Loading the texture '%s' into a full house!\n",
			name.to_string().c_str() );
	}

	// Try loading the texture and add it to the cache:
	ManagedTexture *manTex = new ManagedTexture( data, name,
		mustExist ? BaseTexture::FAIL_ATTEMPT_LOAD_PLACEHOLDER :
		BaseTexture::FAIL_RETURN_NULL );
	if (manTex->status() == BaseTexture::STATUS_READY)
	{
		res = manTex;
		addInternal(manTex);
	}
	else
	{
		delete manTex;
	}

	return res;
}

void TextureManager::getAnimationSet( const BW::string & pathWithPrefix, Moo::BaseTexturePtr** dst, uint32 number, bool allowAnimation /*= true*/, bool mustExist /*= true*/, bool loadIfMissing /*= true*/ )
{
	char buf[50];
	BaseTexturePtr* textures = (*dst);
	if(textures)
		bw_safe_delete_array(textures);
	textures = new Moo::BaseTexturePtr[number];
	for(uint32 i = 0; i < number; ++i)
	{
		itoa(i, buf, 10 );
		BW::string name = pathWithPrefix;
		if(i < 10)
			name += "0";
		if(i < 100)
			name += "0";
		name += buf;
		name += ".dds";
		textures[i] = this->get(name, allowAnimation, mustExist, loadIfMissing);
	}
	(*dst) = textures;
}
/**
 *	This method gets a unique copy of a texture resource
 *	@param resourceID the resourceID of the texture
 *	@param sanitisedResourceID the optional sanitised resource id,
 *		the sanitised resource id is the resource id that is used 
 *		for loading the texture (i.e. the .dds version of the resource)
 *  @param allowAnimation can the texture be animated?
 *  @param mustExist Attempt to load the not-found texture on failure.
 *	@param allocator the owner of this texture
 *	@return pointer to the loaded texture if it loaded, otherwise NULL.
 */
BaseTexturePtr TextureManager::getUnique(
	const BW::StringRef& resourceID, const BW::StringRef& sanitisedResourceID, 
	bool allowAnimation, bool mustExist, const char* allocator )
{

	BW_GUARD;
	// Make sure we have a proper resource id to load
	const BW::StringRef sanitisedID = 
		sanitisedResourceID.length() ? sanitisedResourceID : resourceID;

#ifdef EDITOR_ENABLED
	if (sanitisedID.empty())
	{
		return NULL;
	}
#endif

	BaseTexturePtr res = NULL;

	// load the texture
	TextureDetailLevelPtr lod = this->pDetailLevels_->detailLevel( sanitisedID );

	if (allowAnimation)
	{
		char tempBuffer[BW_MAX_PATH];
		const BW::StringRef animTextureName =
			TextureHelpers::copyBaseNameAndExtension( 
			removeTextureExtension( sanitisedID ), "texanim", tempBuffer,
			ARRAY_SIZE( tempBuffer ) );
		DataSectionPtr animSect = BWResource::instance().openSection(
			animTextureName );
		if (animSect.exists())
		{
			res = new AnimatingTexture( animTextureName );
			this->addInternal( res.get() );
		}
	}
	if (!res.exists())
	{
		int lodMode = lod->lodMode();
		int qualitySetting = this->qualitySettings_->activeOption();

		D3DFORMAT format = lod->format();
		if(TextureManager::instance()->isCompressed( lod ))
		{
			format = lod->formatCompressed();
		}

		// Request a texture from the streaming system
		res = streamingManager()->createStreamingTexture( lod.get(), sanitisedID,
			mustExist ? BaseTexture::FAIL_ATTEMPT_LOAD_PLACEHOLDER :
			BaseTexture::FAIL_RETURN_NULL, 
			format,
			calcMipSkip(qualitySetting, lodMode),
			lod->noResize(), lod->noFilter(), allocator );
		
		// Did the streaming system supply a texture?
		if (!res)
		{
			res = new ManagedTexture( sanitisedID,
				mustExist ? BaseTexture::FAIL_ATTEMPT_LOAD_PLACEHOLDER :
				BaseTexture::FAIL_RETURN_NULL, 
				format,
				calcMipSkip(qualitySetting, lodMode),
				lod->noResize(), lod->noFilter(), allocator );
		}
		
		char internalIDBuffer[BW_MAX_PATH];
		const BW::StringRef internalID = resourceID.length() ? 
			TextureHelpers::copyBaseNameAndExtension( 
				removeTextureExtension( resourceID ), "dds", internalIDBuffer,
				ARRAY_SIZE( internalIDBuffer ) ) 
			: sanitisedID;

		if (res->status() == BaseTexture::STATUS_READY &&
			internalID.length() )
		{
			this->addInternal( res.getObject(), internalID );
		}
		else
		{
			res = NULL;
		}
	}

	return res;
}

/** 
 *	This method does a quick check to see if the resource is a texture file.
 *	It does this by checking that the file exists and has a valid texture
 *	extension.
 *
 *	@param resourceID	The name of the texture to test.
 *  @param checkExists  if set to false, faile does not need to exist to be a texture file.
 *
 *	@return				True if the resource is a texture, false otherwise.
 */
bool TextureManager::isTextureFile(const BW::StringRef & resourceID, bool checkExists) const
{
	BW_GUARD;
	// The file must exist:
	if (checkExists && !BWResource::fileExists(resourceID))
		return false;

	// Only recognised extensions are textures:
	BW::StringRef extension = BWResource::getExtension(resourceID);
	for (size_t i = 0; TextureHelpers::textureExts[i] != NULL; ++i)
	{
		if (extension.equals_lower( TextureHelpers::textureExts[i] ))
			return true;
	}

	// Extension of "texanim" is a special case:
	if (extension.equals_lower( "texanim" ))
		return true;

	return false;
}

/**
 *	This method loads a texture into a system memory resource.
 *	@param resourceID the name of the texture in the res tree.
 *	@return the pointer to the system memory base texture.
 */
BaseTexturePtr	TextureManager::getSystemMemoryTexture(
	const BW::StringRef & resourceID )
{
	BW_GUARD;

	// load it
	char resourceName[BW_MAX_PATH];
	bw_str_copy( resourceName, ARRAY_SIZE( resourceName ),
		resourceID );
	std::for_each( resourceName, resourceName +
		bw_str_len( resourceName ), toLower );

	prepareResource( resourceName, resourceName,
		ARRAY_SIZE( resourceName ) );

	char systemName[BW_MAX_PATH];
	TextureHelpers::copyBaseNameAndExtension( resourceName, "_system",
		systemName, ARRAY_SIZE( systemName ) );

	//search cache first
	BaseTexturePtr res = this->find( systemName );
	if (res)
	{
		return res;
	}

	SmartPointer< SysMemTexture > sysTexture = new SysMemTexture( resourceName );

    if (sysTexture->status() != BaseTexture::STATUS_READY)
	{
		return NULL;
	}

	// add to the cache
	this->addInternal( sysTexture.get(), systemName );

	return sysTexture;
}


void TextureManager::prepareResource( const BW::StringRef & resourceName ) const
{
	char buffer[BW_MAX_PATH];
	prepareResource( resourceName, buffer, ARRAY_SIZE( buffer ) );
}


BW::StringRef TextureManager::prepareResource( 
	const BW::StringRef & resourceName, char * outPtr, size_t maxSize,
	bool forceConvert /*= false*/ ) const
{
	BW_GUARD;

	BW::StringRef resource = TextureHelpers::copyNormalisedTextureName(
		resourceName, outPtr, maxSize );
	TextureDetailLevelPtr pDetailLevel = 
		this->pDetailLevels_->detailLevel( resource );
	resource = TextureHelpers::copyBaseNameAndExtension( 
		BWResource::removeExtension( resource ),
		isCompressed( pDetailLevel ) ? "c.dds" : "dds", outPtr, maxSize );

#if ENABLE_ASSET_PIPE
	BW::StringRef ext = BWResource::getExtension( resourceName );
	if (!ext.equals_lower( "texanim" ) && !BWResource::fileExists( resource ))
	{
		if (pAssetClient_ != NULL)
		{
			pAssetClient_->requestAsset( resource, false );
		}
		return this->prepareResourceName( resourceName, outPtr, maxSize );
	}
#endif // ENABLE_ASSET_PIPE

	return resource;
}


BW::StringRef TextureManager::prepareResourceName( 
	const BW::StringRef & resourceName, char * outPtr, size_t maxSize ) const
{
	BW_GUARD;	

	const BW::StringRef result = TextureHelpers::copyNormalisedTextureName(
		resourceName, outPtr, maxSize );

#if ENABLE_ASSET_PIPE

	if (BWResource::getExtension( result ).equals_lower( "texanim" )) 
	{
		return TextureHelpers::copyBaseNameAndExtension( 
			removeTextureExtension( result ), "dds", outPtr, maxSize );
	}

	// If the source texture is a dds or the source texture can not be found
	// Check if one with a different extension can be found
#if ENABLE_FILE_CASE_CHECKING
	BW::string caseCorrected;
#endif
	const BW::StringRef baseName = removeTextureExtension( result );
	for (uint32 i = 0; TextureHelpers::textureExts[i] != NULL; ++i)
	{
		BW::StringRef fileName = 
			TextureHelpers::copyBaseNameAndExtension( baseName,
			TextureHelpers::textureExts[i], outPtr, maxSize );
#if ENABLE_FILE_CASE_CHECKING
		if (BWResource::checkCaseOfPaths())
		{
			// This ensures that the extension is in the correct case
			// the rest of the filename is discarded in this scope.
			caseCorrected = BWResource::correctCaseOfPath( fileName );
			TextureHelpers::copyNormalisedTextureName( caseCorrected,
				outPtr, maxSize );
		}
#endif
		if ( BWResource::fileExists( fileName ) )
		{
			return fileName;
		}
	}

	TextureHelpers::copyNormalisedTextureName(
		resourceName, outPtr, maxSize );

	return result;

#else // ENABLE_ASSET_PIPE

	if (result.rfind("dds") == result.size() - 3)
		return result;
	TextureDetailLevelPtr pDetailLevel = this->pDetailLevels_->detailLevel(result);
	if (isCompressed( pDetailLevel ))
		return BWResource::changeExtension( result, ".c.dds" );
	else
		return BWResource::changeExtension( result, ".dds" );

#endif // ENABLE_ASSET_PIPE
}


void TextureManager::checkLoadedResourceName( const BW::StringRef & requested,
											  const BW::StringRef & found ) const
{
	if (requested != found)
	{
		WARNING_MSG( "Texture '%s'%s was not found, using '%s' instead.\n",
			requested.to_string().c_str(),
			ResourceLoadContext::formattedRequesterString().c_str(),
			found.to_string().c_str() );
	}
}


void TextureManager::tryDestroy( const BaseTexture * pTexture )
{
	BW_GUARD;
	if (pInstance_)
	{
		RecursiveMutexHolder lock( pInstance_->texturesLock_ );
		if (pTexture->refCount() == 0)
		{
			pInstance_->delInternal( pTexture );
			pInstance_->delFromDirtyTextures( pTexture );
			delete pTexture;
		}
	}
	else
	{
		MF_ASSERT(pTexture->refCount() == 0);
		delete pTexture;
	}
}


void TextureManager::add( BaseTexture * pTexture )
{
	BW_GUARD;
	if (pInstance_)
	{
		pInstance_->addInternal( pTexture );
	}
}


/**
 *	Add this texture to the map of those already loaded
 */
void TextureManager::addInternal( BaseTexture * pTexture,
	const BW::StringRef & resourceID )
{
	BW_GUARD;

	MF_ASSERT( !resourceID.empty() || !pTexture->resourceID().empty() );

	RecursiveMutexHolder lock( texturesLock_ );	

	// convert texture name to lowercase and fix slashes
	char resourceBuffer[BW_MAX_PATH];
	const BW::StringRef lowercaseResourceID = 
		TextureHelpers::copyNormalisedTextureName(
		resourceID.empty() ? pTexture->resourceID() : resourceID,
		resourceBuffer, ARRAY_SIZE( resourceBuffer ) );

	// transform buffer to lower case
	const size_t len = lowercaseResourceID.size();
	std::transform( resourceBuffer, resourceBuffer + len, resourceBuffer,
		tolower );

	if ( textures_.find( lowercaseResourceID ) != textures_.end() )
	{
		WARNING_MSG( "TextureManager::addInternal - replacing %s\n",
			resourceBuffer );
	}

	textures_.insert( std::make_pair( lowercaseResourceID, pTexture ) );
}


/**
 *	Remove this texture from the map of those already loaded
 */
void TextureManager::delInternal( const BaseTexture * pTexture )
{
	BW_GUARD;
	MF_ASSERT( texturesLock_.lockCount() > 0 );

	for (TextureMap::iterator it = textures_.begin(); it != textures_.end(); it++)
	{
		if (it->second == pTexture)
		{
			//DEBUG_MSG( "TextureManager::del: %s\n", pTexture->resourceID().c_str() );
			textures_.erase( it );
			return;
		}
	}

	INFO_MSG( "TextureManager::del: "
		"Could not find texture '%s' at 0x%p to delete it\n",
		pTexture->resourceID().c_str(), pTexture );
}


/**
 *	Remove the given texture from the dirty textures list.
 *	@param pTexture the texture to remove.
 *	@return true if the texture was removed, false if not found.
 */
bool TextureManager::delFromDirtyTextures( const BaseTexture * pTexture )
{
	BW_GUARD;
	MF_ASSERT( texturesLock_.lockCount() > 0 );

	// Find texture in dirty textures list
	TextureStringVector::iterator itr( dirtyTextures_.end() );
	for (itr = dirtyTextures_.begin(); itr != dirtyTextures_.end(); ++itr)
	{
		if (itr->first == pTexture->resourceID())
		{
			// Remove if found
			dirtyTextures_.erase( itr );
			return true;
		}
	}

	return false;
}


/**
 *	Find this texture in the map of those already loaded
 */
BaseTexturePtr TextureManager::find( const BW::StringRef & resourceID )
{
	BW_GUARD;

	if (resourceID.empty())
	{
		return NULL;
	}

	// create a lower case resource id for comparison
	// TODO: lowercase_compare
	char lowercaseResourceID[BW_MAX_PATH];
	MF_ASSERT( resourceID.size() < ARRAY_SIZE( lowercaseResourceID ) );
	const size_t size = std::min( resourceID.size(), 
		ARRAY_SIZE( lowercaseResourceID ) - 1 );
	std::transform( resourceID.begin(), resourceID.begin() + size,
		lowercaseResourceID,
		tolower );
	// null terminate
	lowercaseResourceID[size] = 0;

	RecursiveMutexHolder lock( texturesLock_ );

	TextureMap::iterator it = textures_.find( BW::StringRef( lowercaseResourceID,
		size ) );
	if (it == textures_.end())
		return NULL;

	return it->second;
}

/**
 * This function returns true if the texture is compressed
 */
bool TextureManager::isCompressed(TextureDetailLevelPtr lod) const
{
	int comprSettings = this->compressionSettings_->activeOption();
	return comprSettings == 0 && lod->isCompressed();
}

/**
 *	Sets the texture quality setting. Implicitly called 
 *	whenever the user changes the TEXTURE_QUALITY setting.
 *
 *	Texture quality can vary from 0 (highest) to 2 (lowest). Lowering texture
 *	quality is done by skipping the topmost mipmap levels (normally two in the 
 *	lowest quality setting, one in the medium and none in the highest). 
 *
 *	Artists can tweak how each texture responds to the quality setting using the 
 *	<lodMode> field in the ".texformat" file or global texture_detail_level.xml. 
 *	The table bellow shows the number of lods skipped according to the texture's
 *	lodMode and the current quality setting.
 *
 *					texture quality setting
 *	lodMode			0		1		2
 *	0 (disabled)	0		0		0
 *	1 (normal)		0		1		2
 *	2 (low bias)	0		1		1
 *	3 (high bias)	0		0		1
 */
void TextureManager::setQualityOption(int optionIndex)
{
	BW_GUARD;
	TextureMap::iterator it  = textures_.begin();
	TextureMap::iterator end = textures_.end();
	while (it != end)
	{
		BaseTexture *pTex = it->second;
		TextureDetailLevelPtr lod = this->pDetailLevels_->detailLevel( pTex->resourceID() );
		int lodMode    = lod->lodMode();
		int newMipSkip = calcMipSkip(optionIndex, lodMode);

		// check if texture is compressed
		BW::StringRef baseName      = removeTextureExtension( pTex->resourceID() );
		BW::string comprSuffix   = "";
		if (this->isCompressed(lod))
		{
			comprSuffix = ".c";
		}
		BW::string ddsName = baseName + comprSuffix + ".dds";

		if (lodMode != 2 && newMipSkip != pTex->mipSkip())
		{
			pTex->mipSkip(newMipSkip);
			this->dirtyTextures_.push_back( 
				std::make_pair( ddsName, ddsName ) );
		}
		it++;
	}		

	int pendingOption = 0;
	if (!Moo::GraphicsSetting::isPending(
			this->compressionSettings_, pendingOption))
	{
		Moo::rc().clearTextureReuseList();
		this->reloadDirtyTextures();
	}
}

/**
 *	Sets the texture compression setting. Implicitly called 
 *	whenever the user changes the TEXTURE_COMPRESSION setting.
 *
 *	Only textures with <formatCompressed> specified in their ".texformat" 
 *	or the global texture_detail_level.xml file. The format specified in 
 *	formatCompressed will be used whenever texture compressio is enabled. 
 *	If texture compression is disabled or formatCompressed has not been 
 *	specified, <format> will be used.
 */
void TextureManager::setCompressionOption(int optionIndex)
{
	BW_GUARD;
	TextureMap::iterator it  = textures_.begin();
	TextureMap::iterator end = textures_.end();
	while (it != end)
	{
		BaseTexture *pTex = it->second;
		BW::string resourceName = pTex->resourceID();
		TextureDetailLevelPtr lod = this->pDetailLevels_->detailLevel( resourceName );
		if (lod->isCompressed() &&
			BWResource::getExtension(resourceName) == "dds")
		{
			char preparedName[BW_MAX_PATH];
			prepareResource( resourceName, preparedName,
				ARRAY_SIZE( preparedName ) );
			this->dirtyTextures_.push_back( 
				std::make_pair( 
					resourceName,	 // old name
					preparedName	 // new name
				) );
		}		
		it++;
	}

	int pendingOption = 0;
	if (!Moo::GraphicsSetting::isPending(
			this->qualitySettings_, pendingOption))
	{
		this->reloadDirtyTextures();
	}
}

/**
 *	Triggered by the graphics settings registry when the user changes 
 *	the active TEXTURE_FILTER option. This method is left empty as
 *	we get the option from the actual graphics setting object.
 */
void TextureManager::setFilterOption(int)
{}

/**
 *	Returns the currently selected mipmapping filter. 
 *	This is defined by the active TEXTURE_FILTER option.
 */
int TextureManager::configurableMipFilter() const
{
	BW_GUARD;
	static D3DTEXTUREFILTERTYPE s_mipFilters[] = {
		D3DTEXF_LINEAR, D3DTEXF_LINEAR,
		D3DTEXF_LINEAR, D3DTEXF_LINEAR,
		D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_NONE};

	return s_mipFilters[this->filterSettings_->activeOption()];
}

/**
 *	Returns the currently selected min/mag mapping filter. 
 *	This is defined by the active TEXTURE_FILTER option.
 */
int TextureManager::configurableMinMagFilter() const
{
	BW_GUARD;
	static D3DTEXTUREFILTERTYPE s_minMagFilters[] = {
		D3DTEXF_ANISOTROPIC, D3DTEXF_ANISOTROPIC,
		D3DTEXF_ANISOTROPIC, D3DTEXF_ANISOTROPIC,
		D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_POINT};

	return s_minMagFilters[this->filterSettings_->activeOption()];
}

/**
 *	Returns the currently selected max anisotropy value. 
 *	This is defined by the active TEXTURE_FILTER option.
 */
int TextureManager::configurableMaxAnisotropy() const
{
	BW_GUARD;
	static int s_maxAnisotropy[] = {16, 8, 4, 2, 1, 1, 1};
	return s_maxAnisotropy[this->filterSettings_->activeOption()];
}

/**
 *	Reloads all textures in the dirty textures list. Textures are 
 *	added to the dirty list whenever the texture settings change,
 *	either quality or compression. 
 */
void TextureManager::reloadDirtyTextures()
{
	BW_GUARD;

	// Since AnimatingTexture::reload can delete a texture
	// therefore delete it from dirtyTextures_, so we need check
	// every one if it still exist. it's a work around solution,
	// we should probably make AnimatingTexture::reload not delete the old 
	// ones but reload.
	TextureStringVector dirtyTexturesCopy = dirtyTextures_;
	TextureStringVector::const_iterator texIt  = dirtyTexturesCopy.begin();
	TextureStringVector::const_iterator texEnd = dirtyTexturesCopy.end();
	for (; texIt != texEnd; ++texIt)
	{
		// because maybe by far this texture is already
		// deleted by previous AnimatingTexture::reload,
		// so we need check if it still exist.
		BaseTexturePtr pTex ( this->find( texIt->first ) );
		if (pTex)
		{
			pTex->reload( texIt->second ); // reload with new name
		}
	}
	this->dirtyTextures_.clear();
}


namespace
{
class TextureReloadContext
{
public:
	TextureReloadContext( const BaseTexturePtr& tex ) :
		pTex_( tex )
	{
	}

	void reload( ResourceModificationListener::ReloadTask& task )
	{
		BWResource::WatchAccessFromCallingThreadHolder holder( false );

		BWResource::instance().purge( task.resourceID(), true );
		pTex_->reload( task.resourceID() );
	}

private:
	BaseTexturePtr pTex_;
};

class TextureDetailReloadContext
{
public:
	void reload( ResourceModificationListener::ReloadTask& task )
	{
		BWResource::WatchAccessFromCallingThreadHolder holder( false );

		BWResource::instance().purge( task.resourceID(), true );
		TextureManager* pTexManager = TextureManager::instance();
		if (pTexManager)
		{
			pTexManager->detailManager()->init();
		}
	}
};
} // End anon namespace


void TextureManager::onResourceModified( const BW::StringRef& basePath,
									const BW::StringRef& resourceID,
									ResourceModificationListener::Action modType )
{
	if (modType == ResourceModificationListener::ACTION_DELETED)
	{
		return;
	}

	BWResource::WatchAccessFromCallingThreadHolder holder( false );

	char baseNameBuffer[BW_MAX_PATH];
	const BW::StringRef baseName = TextureHelpers::copyNormalisedTextureName(
		removeTextureExtension( resourceID ), baseNameBuffer,
		ARRAY_SIZE( baseNameBuffer ) );

	const BW::StringRef ext = BWResource::getExtension( resourceID );
	if (ext.equals_lower( "texanim" ))
	{
		BaseTexturePtr texture = this->find( resourceID );
		if (NULL != texture)
		{
			queueReloadTask( TextureReloadContext( texture ), 
				basePath, resourceID );
		}
	}
	else if (ext.equals_lower( "dds" ))
	{
		char ddsName[BW_MAX_PATH];
		BaseTexturePtr texture = this->find( 
			TextureHelpers::copyBaseNameAndExtension( baseName, "dds", ddsName,
				ARRAY_SIZE( ddsName ) ) );
		char preparedName[BW_MAX_PATH];
		if (NULL != texture &&
			resourceID == prepareResource( baseName, preparedName,
				ARRAY_SIZE( preparedName) ))
		{
			queueReloadTask( TextureReloadContext( texture ), 
				basePath, resourceID );
		}
	}
	else if (this->pDetailLevels_->isSourceFile( resourceID ))
	{
		queueReloadTask( TextureDetailReloadContext(), 
			basePath, resourceID );
	}
}


/**
 * This method converts from a D3DFormat enum to a string representation.
 */
const BW::string& TextureManager::getFormatName( D3DFORMAT format )
{
	return FormatNameMap::formatString( format );
}


/**
 * Initialise the manager.
 */
void TextureManager::init()
{
	BW_GUARD;
	pInstance_ = new TextureManager;
	REGISTER_SINGLETON( TextureManager );
}

/**
 * Finalise the manager.
 */
void TextureManager::fini()
{
	BW_GUARD;
	bw_safe_delete(pInstance_);
}

/**
 *	Saves list of all active d3d textures to disk
 *
 */
namespace
{

bool TexMemSorter(BaseTexture*& d1, BaseTexture*& d2)
{
	return d1->textureMemoryUsed() > d2->textureMemoryUsed();
}

}

void TextureManager::saveMemoryUsagePerTexture( const char* filename )
{
	RecursiveMutexHolder lock( texturesLock_ );

	FILE* f = fopen( filename, "wb+" );

	if (f)
	{
		fprintf( f, "Width;Height;MemUsage;Name\n" );

		BW::vector<BaseTexture*> texvec;

		for (TextureMap::iterator it = textures_.begin();
			it != textures_.end(); ++it)
		{
			texvec.push_back( it->second );
		}

		std::sort( texvec.begin(), texvec.end(), TexMemSorter );

		for (BW::vector<BaseTexture*>::iterator it = texvec.begin();
			it != texvec.end(); ++it)
		{
			fprintf( f, "%d;%d;%d;%s\n",
				(*it)->width(), (*it)->height(),
				(*it)->textureMemoryUsed(), (*it)->resourceID().c_str() );
		}
		fclose( f );
	}
}
/**
 *	This gets a NULL-terminated list of valid textures.
 *
 *	@return			A NULL-terminated list of valid texture extensions.
 */
/*static*/ const char ** TextureManager::validTextureExtensions()
{
	return TextureHelpers::textureExts;
}
void TextureManager::setTextureMemoryBlock( int minMemory )
{
	minMemoryForHighTextures = minMemory;
}


TextureManager* TextureManager::pInstance_ = NULL;

size_t TextureManager::minMemoryForHighTextures = 0;

} // namespace Moo

BW_END_NAMESPACE

// texture_manager.cpp
