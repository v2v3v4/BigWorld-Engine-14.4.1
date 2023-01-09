#include "pch.hpp"

#include "resmgr/multi_file_system.hpp"

#include "base_texture.hpp"
#include "moo/resource_load_context.hpp"
#include "texture_manager.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "base_texture.ipp"
#endif

namespace Moo
{

BaseTexture::BaseTexture(const BW::StringRef& allocator):
status_( STATUS_NOT_READY ),
statusHandler_( NULL )
{
	BW_GUARD;

#if ENABLE_RESOURCE_COUNTERS
	allocator.copy_to( allocator_ );
#endif
}

BaseTexture::~BaseTexture()
{
}

/**
 *	This method implements a stub accessor for the resource id
 *	of the texture.
 *	@return an empty string.
 */
const BW::string& BaseTexture::resourceID( ) const
{
	static BW::string s;
	return s;
}

/**
 *	This method saves the texture to disk in DDS format using
 *	the given filename.
 *	@return weather or not the save operation succeeded
 */
bool BaseTexture::save( const BW::string& filename )
{
	BW_GUARD;
	DX::BaseTexture* pTex = this->pTexture();
	if (!pTex)
	{
		return false;
	}

	ComObjectWrap<ID3DXBuffer> buffer;
	HRESULT hr = D3DXSaveTextureToFileInMemory( &buffer, D3DXIFF_DDS, pTex, NULL );
	if (FAILED(hr))
	{
		ASSET_MSG( "BaseTexture::save: failed to save '%s' (hr=%s).\n",
			filename.c_str(),
			DX::errorAsString( hr ).c_str() );
		return false;
	}

	BinaryPtr bin = new BinaryBlock( buffer->GetBufferPointer(),
		buffer->GetBufferSize(), "BinaryBlock/BaseTextureSave" );

	return BWResource::instance().fileSystem()->writeFile( filename, bin, true );
}


namespace
{
	DX::Surface*	getTextureSurfaceLevel(uint level, DX::BaseTexture* pTex )
	{
		DX::Surface* surface = NULL;
		if (pTex->GetType() == D3DRTYPE_CUBETEXTURE)
		{
			DX::CubeTexture* cubeTex = static_cast<DX::CubeTexture*>( pTex );
			cubeTex->GetCubeMapSurface( D3DCUBEMAP_FACE_POSITIVE_X, level, &surface );
		}
		else if (pTex->GetType() == D3DRTYPE_TEXTURE)
		{
			DX::Texture* tex2d = static_cast<DX::Texture*>( pTex );
			tex2d->GetSurfaceLevel( level, &surface );
		}
		return surface;
	}
}
/**
 *	This method calculates the amount of memory used by a texture in bytes.
 */
uint32 BaseTexture::textureMemoryUsed( DX::BaseTexture* pTex )
{
	BW_GUARD;
	
	if (pTex == NULL)
	{
		return 0;
	}

	return DX::textureSize( pTex, ResourceCounters::MT_GPU );
}


/**
 *	This method adds this texture to the texture manager
 */
void BaseTexture::addToManager()
{
	BW_GUARD;
	TextureManager::add( this );
}


void BaseTexture::tryDestroy() const
{
	BW_GUARD;
	TextureManager::tryDestroy( this );
}

/**
 *	This method does any final processing/error checking to the result of a
 *	texture loader.
 *	@param texLoader the texture loader to process.
 *	@param notFoundPolicy this policy specifies what to do if the texture
 *		loader failed to load a texture. eg. replacing failed textures with
 *		a placeholder texture.
 *	@param inoutQualifiedResourceID [in] the resource ID of the texture that
 *		the texture loader was loading.
 *		[out] The final resource ID, this may have changed if a placeholder
 *		texture has been swapped in.
 *	@param outResult [out] The final result of the texutre loader after
 *		processing. Note that the resulting texture may be NULL on failure.
 */
void BaseTexture::processLoaderOutput( const TextureLoader& texLoader,
	const FailurePolicy& notFoundPolicy,
	BW::string& inoutQualifiedResourceID,
	TextureLoader::Output& outResult ) const
{
	BW_GUARD;

	const bool useFallback = !texLoader.output().texture2D_ &&
		!texLoader.output().cubeTexture_ &&
		(notFoundPolicy == FAIL_ATTEMPT_LOAD_PLACEHOLDER);

	// Fallback to using the "not found" bmp if we failed to load the
	// original texture and the resource must exist
	if (useFallback)
	{
		this->reportMissingTexture( inoutQualifiedResourceID );

		inoutQualifiedResourceID = TextureManager::notFoundBmp();

		// Default input for notFound.bmp
		TextureLoader::InputOptions input;
		TextureLoader backupTexLoader;
		backupTexLoader.loadTextureFromFile( inoutQualifiedResourceID, input );

		// Failed to load the "not found" bmp
		if (!backupTexLoader.output().texture2D_ &&
			!backupTexLoader.output().cubeTexture_)
		{
			this->reportMissingTexture( inoutQualifiedResourceID );
		}
		outResult.texture2D_ = backupTexLoader.output().texture2D_;
		outResult.cubeTexture_ = backupTexLoader.output().cubeTexture_;
		outResult.width_ = backupTexLoader.output().width_;
		outResult.height_ = backupTexLoader.output().height_;
		outResult.format_ = backupTexLoader.output().format_;
	}
	else
	{
		outResult.texture2D_ = texLoader.output().texture2D_;
		outResult.cubeTexture_	= texLoader.output().cubeTexture_;
		outResult.width_ = texLoader.output().width_;
		outResult.height_ = texLoader.output().height_;
		outResult.format_ = texLoader.output().format_;
	}
}


/**
 *	Print out a missing texture message.
 *	@param qualifiedResourceID the name of the texture that is missing.
 */
void BaseTexture::reportMissingTexture(
	const BW::StringRef& qualifiedResourceID ) const
{
	BW_GUARD;
	WARNING_MSG( "Moo::BaseTexture::reportMissingTexture: "
		"Can't read texture file '%s'%s.\n",
		qualifiedResourceID.to_string().c_str(),
		ResourceLoadContext::formattedRequesterString().c_str() );
}


void	BaseTexture::load()
{ 
	status( STATUS_READY );
}

void	BaseTexture::release()
{
	status( STATUS_NOT_READY );
}

void	BaseTexture::reload( )
{
	status( STATUS_READY );
}

void	BaseTexture::reload( const BW::StringRef & resourceID )
{
	status( STATUS_READY );
}

void BaseTexture::setStatusHandler( TextureStatusHandler* pHandler )
{
	BW_GUARD;
	statusHandler_ = pHandler;
}

void BaseTexture::onStatusChange( Status newStatus )
{
	BW_GUARD;
	if (statusHandler_)
	{
		statusHandler_->onStatusChange( newStatus );
	}
}

/**
 * This function removes file extension from given name, and a ".c" extension 
 * if it exists.
 */
BW::StringRef removeTextureExtension(const BW::StringRef & resourceName)
{
	BW_GUARD;
	BW::StringRef baseName = BWResource::removeExtension(resourceName);
	if (BWResource::getExtension(baseName) == "c")
	{
		baseName = BWResource::removeExtension(baseName);
	}
	return baseName;
}


/**
 * This function removes a ".c" extension from a given texture name.
 */
BW::string canonicalTextureName(const BW::StringRef & resourceName)
{
	if (resourceName.empty())
	{
		return resourceName.to_string();
	}

	BW::StringRef baseName  = removeTextureExtension(resourceName);
	BW::StringRef extension = BWResource::getExtension(resourceName);

	return baseName + "." + extension;
}

} // namespace Moo

BW_END_NAMESPACE

// base_texture.cpp
