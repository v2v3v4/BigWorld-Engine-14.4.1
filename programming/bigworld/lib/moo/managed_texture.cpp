#include "pch.hpp"

#include "managed_texture.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/bwresource.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/slot_tracker.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/file_streamer.hpp"
#include "texture_manager.hpp"

#include "render_context.hpp"

#include "resource_load_context.hpp"

#include "dds.h"
#include "texture_loader.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "managed_texture.ipp"
#endif

extern SIZE_T textureMemSum;

MEMTRACKER_DECLARE( ManagedTexture, "ManagedTexture", 0);

namespace Moo
{

uint32 ManagedTexture::frameTimestamp_ = -1;
uint32 ManagedTexture::totalFrameTexmem_ = 0;

/*static*/ bool					ManagedTexture::s_accErrs = false;
/*static*/ BW::string			ManagedTexture::s_accErrStr = "";
/*static*/ void					ManagedTexture::accErrs( bool acc ) { s_accErrs = acc; s_accErrStr = ""; }
/*static*/ const BW::string&	ManagedTexture::accErrStr() { return s_accErrStr; } 


ManagedTexture::ManagedTexture(
	const BW::StringRef& resourceID,
	const FailurePolicy& notFoundPolicy,
	D3DFORMAT format, int mipSkip, 
	bool noResize, bool noFilter,
	const BW::StringRef& allocator )
: resourceID_( canonicalTextureName(resourceID) ),
  qualifiedResourceID_( resourceID.data(), resourceID.size() ),
  width_( 0 ),
  height_( 0 ),
  format_( format ),
  mipSkip_( mipSkip ),
  textureMemoryUsed_( 0 ),
  localFrameTimestamp_( 0 ),
  noResize_( noResize ),
  noFilter_( noFilter ),
  texture_( NULL ),
  reuseOnRelease_( false ),
  loadedFromDisk_( true )
#if ENABLE_RESOURCE_COUNTERS
  , BaseTexture(allocator)
#endif
  ,cubemap_( false )
{
	BW_GUARD;
	this->load( notFoundPolicy );
}


ManagedTexture::ManagedTexture( 
		const BW::StringRef& resourceID, uint32 w, uint32 h, int nLevels,
		DWORD usage, D3DFORMAT fmt, const BW::StringRef& allocator ):
	width_( w ),
	height_( h ),
	format_( fmt ),
	mipSkip_( 0 ),
	textureMemoryUsed_( 0 ),
#if ENABLE_RESOURCE_COUNTERS
	BaseTexture(allocator),
#endif
	resourceID_( canonicalTextureName(resourceID) ),
	qualifiedResourceID_( resourceID.data(), resourceID.size() ),
	noResize_( false ),
	noFilter_( true ),
	reuseOnRelease_( false ),
	loadedFromDisk_( false ),
	cubemap_( false ),
	localFrameTimestamp_( 0 )
{
	tex_ = Moo::rc().createTexture( w, h, nLevels, usage, fmt,
									D3DPOOL_MANAGED, resourceID );
	texture_ = tex_.pComObject();
	if (tex_.hasComObject())
	{
		this->status( STATUS_READY );
	}
	this->BaseTexture::addToManager();
}


ManagedTexture::ManagedTexture
( 
	DataSectionPtr		data, 
	BW::StringRef			const& resourceID, 
	const FailurePolicy& notFoundPolicy, 
	int					mipSkip /*= 0*/,
	bool				noResize,
	bool				noFilter, 
	const BW::StringRef&	allocator
):
	width_( 0 ),
	height_( 0 ),
	format_( D3DFMT_UNKNOWN ),
	mipSkip_( mipSkip ),
	textureMemoryUsed_( 0 ),
#if ENABLE_RESOURCE_COUNTERS
	BaseTexture(allocator),
#endif
	resourceID_( canonicalTextureName(resourceID) ),
	qualifiedResourceID_( resourceID.data(), resourceID.size() ),
	noResize_( noResize ),
	noFilter_( noFilter ),
	reuseOnRelease_( false ),
	loadedFromDisk_( true ),
	cubemap_( false ),
	localFrameTimestamp_( 0 )
{
	BW_GUARD;
	MEMTRACKER_SCOPED( ManagedTexture );
	this->status( STATUS_NOT_READY );
	if (data != NULL && Moo::rc().device() != NULL)
	{
		BinaryPtr bin = data->asBinary();
		if (bin != NULL && bin->len() >= 4)
		{
			SimpleMutexHolder smh( loadingLock_ );

			DWORD *binData = (DWORD *)bin->data();
			uint length = bin->len();
			// skip header if necessary
			if (*binData != DDS_MAGIC)
			{
				binData += 1;
				length += 4;
			}

			TextureLoader::InputOptions input;
			input.mipSkip_ = mipSkip_;
			input.upscaleToNextPow2_ = !noResize_;
			input.mipFilter_ = noFilter_ ? D3DX_FILTER_POINT : D3DX_FILTER_LINEAR;

			TextureLoader tl;
			tl.loadTextureFromMemory( binData, length,
									input, "texture/data managed texture" );

			this->processLoaderOutput( tl, notFoundPolicy );
			MF_ASSERT_DEV( this->status() == STATUS_READY);
		}
	}
}

ManagedTexture::~ManagedTexture()
{
	BW_GUARD;

	// let the texture manager know we're gone
	// reuse it if reuse flag is set
	if (reuseOnRelease_ && tex_.hasComObject())
	{
		rc().putTextureToReuseList( tex_ );
	}
}


void ManagedTexture::destroy() const
{
	// attempt to safety delete the texture
	this->tryDestroy();
}


void ManagedTexture::processLoaderOutput( const TextureLoader& texLoader,
	const FailurePolicy& notFoundPolicy )
{
	BW_GUARD;

	TextureLoader::Output result;
	BaseTexture::processLoaderOutput( texLoader,
		notFoundPolicy,
		qualifiedResourceID_,
		result );

	tex_ = result.texture2D_;
	cubeTex_ = result.cubeTexture_;
	width_ = result.width_;
	height_ = result.height_;
	format_ = result.format_;

	if (tex_)
	{
		texture_ = tex_.pComObject();
		// put texture to reuse list on release if this is a proper .dds texture
		static BW::string ddsExt = "dds";
		if (BWResource::getExtension(qualifiedResourceID_) == ddsExt)
		{
			reuseOnRelease_ = true;
		}
	}
	else if (cubeTex_)
	{
		cubemap_ = true;
		texture_ = cubeTex_.pComObject();
	}
	else
	{
		texture_ = NULL;
	}

	if (texture_)
	{
		textureMemoryUsed_ = BaseTexture::textureMemoryUsed( texture_);
		rc().addPreloadResource( texture_ );
		this->status( STATUS_READY );
	}
	else
	{
		textureMemoryUsed_ = 0;
		this->status( STATUS_FAILED_TO_LOAD );
	}
}


/**
 *	Print out a missing texture message.
 *	If ManagedTexture::s_accErrs is true, then failure messages will
 *	accumulated so they are only printed once.
 *	@param qualifiedResourceID the name of the texture that is missing.
 */
void ManagedTexture::reportMissingTexture(
	const BW::StringRef& qualifiedResourceID ) const
{
	BW_GUARD;
	if (!ManagedTexture::s_accErrs)
	{
		WARNING_MSG( "Moo::ManagedTexture::load: "
			"Can't read texture file '%s'%s.\n",
			qualifiedResourceID.to_string().c_str(),
			ResourceLoadContext::formattedRequesterString().c_str() );
	}
	else if (ManagedTexture::s_accErrStr.find( qualifiedResourceID.data(), 0, qualifiedResourceID.length() ) ==
		BW::string::npos)
	{
		ManagedTexture::s_accErrStr =
			ManagedTexture::s_accErrStr + "\n * " +
			qualifiedResourceID + " (invalid format?)";
	}
}


bool ManagedTexture::resize( uint32 newWidth, uint32 newHeight,
						int nLevels, DWORD usage, D3DFORMAT fmt )
{
	BW_GUARD;
	MEMTRACKER_SCOPED( ManagedTexture );
	ComObjectWrap< DX::Texture > tex;

	tex = Moo::rc().createTexture( newWidth, newHeight, nLevels, usage,
					fmt, D3DPOOL_MANAGED, resourceID_.c_str() );
	if (tex.hasComObject())
	{
		this->release();
		width_ = newWidth;
		height_ = newHeight;
		tex_ = tex;
		texture_ = tex_.pComObject();
		this->status( STATUS_READY );
		return true;
	}

	return false;
}

/** 
 * This task reloads managed texture data on the background thread
 * by creating a new texture and and replacing original texture data
 * using doMainThreadTask part of the task
*/
class ManagedTexture::ReloadTextureTask : public BackgroundTask
{
public:
	ReloadTextureTask( const ManagedTexturePtr& texture,
		const BW::StringRef& resourceID ):
	BackgroundTask( "ReloadTextureTask" ),
	texture_( texture ),
	resourceID_( resourceID.data(), resourceID.size() ),
	mipSkip_( texture->mipSkip_ ),
	noResize_( texture->noResize_ ),
	noFilter_( texture->noFilter_ ),
	format_( texture->format_ ),
#if ENABLE_RESOURCE_COUNTERS
	allocator_( texture_->allocator_.c_str() )
#else
	allocator_( NULL )
#endif
	{
	}

protected:
	void doBackgroundTask( TaskManager & mgr )
	{
		BW_GUARD;
		MEMTRACKER_SCOPED( ManagedTexture );

		// we cannot just call reload on the background thread because this will
		// break the main thread which might be accessing managed texture data

		// what we going to do instead is to create a new Managed Texture
		// load data into it using BackgroundTask, release the DX::Texture
		// resource of the original texture and copy all fields from the new
		// texture to the original texture we were reloading

		newTexture_ = new ManagedTexture( resourceID_,
			BaseTexture::FAIL_ATTEMPT_LOAD_PLACEHOLDER,
			format_, mipSkip_, noResize_, noFilter_
#if ENABLE_RESOURCE_COUNTERS
			, allocator_
#endif
			);
		mgr.addMainThreadTask( this );
	}

	void doMainThreadTask( TaskManager & mgr )
	{
		BW_GUARD;

		SimpleMutexHolder smh( texture_->loadingLock_ );
		if (newTexture_->status() == BaseTexture::STATUS_READY)
		{
			texture_->release();
			// copy all fields to the original texture
			texture_->width_			= newTexture_->width_ ;
			texture_->height_			= newTexture_->height_ ;
			texture_->format_			= newTexture_->format_ ;
			texture_->mipSkip_			= newTexture_->mipSkip_ ;
			texture_->loadedFromDisk_	= newTexture_->loadedFromDisk_ ;
			texture_->noResize_			= newTexture_->noResize_ ;
			texture_->noFilter_			= newTexture_->noFilter_ ;
			texture_->textureMemoryUsed_ = newTexture_->textureMemoryUsed_ ;
			texture_->texture_			= newTexture_->texture_ ;
			texture_->tex_				= newTexture_->tex_ ;
			texture_->cubeTex_			= newTexture_->cubeTex_ ;
			texture_->resourceID_		= newTexture_->resourceID_ ;
			texture_->qualifiedResourceID_ = newTexture_->qualifiedResourceID_ ;
			texture_->status( newTexture_->status() ) ;
			texture_->reuseOnRelease_	= newTexture_->reuseOnRelease_ ;
			texture_->cubemap_ = newTexture_->cubemap_ ;
			texture_->localFrameTimestamp_ = newTexture_->localFrameTimestamp_ ;
			texture_->frameTimestamp_ = newTexture_->frameTimestamp_ ;
			// set resource ID to be empty to prevent deleting it from the TextureManager

			newTexture_->resourceID_ = "";
			newTexture_->qualifiedResourceID_ = "";
			newTexture_->reuseOnRelease_ = false;
			// make sure no one holds a reference to the new texture
			MF_ASSERT( newTexture_->refCount() == 1 );
			MF_ASSERT( texture_->status() == BaseTexture::STATUS_READY );

			TRACE_MSG( "ManagedTexture: reloaded '%s'.\n",
				resourceID_.c_str() );
		}
		else
		{
			WARNING_MSG( "ManagedTexture - Failed to reload %s\n", resourceID_.c_str() );
		}
	}

private:
	ManagedTexturePtr	texture_;
	uint				mipSkip_;
	bool				noResize_;
	bool				noFilter_;
	D3DFORMAT			format_;
	const char*			allocator_;
	ManagedTexturePtr	newTexture_;
	BW::string			resourceID_;
};

void ManagedTexture::reload()
{
	BW_GUARD;
	this->reload( qualifiedResourceID_ );
}

void ManagedTexture::reload( const BW::StringRef & resourceID )
{
	BW_GUARD;
	if ( !loadedFromDisk_ )
	{
		return;
	}

	// use async reload
	FileIOTaskManager::instance().addBackgroundTask(
		new ReloadTextureTask( this,  resourceID ),
		BgTaskManager::HIGH );
}

void ManagedTexture::load( const FailurePolicy& notFoundPolicy )
{
	BW_GUARD;
	MEMTRACKER_SCOPED( ManagedTexture );

	SimpleMutexHolder smh( loadingLock_ );
	// do not attempt to reload if it failed to load before ??
	// TODO: alexanderr - change this ?
	// also do not load texture if it's already being initialised
	if (this->status() == STATUS_FAILED_TO_LOAD || 
		this->status() == STATUS_READY )
	{
		return;
	}

	this->status( STATUS_LOADING );

	TextureLoader::InputOptions input;
	input.mipSkip_ = mipSkip_;
	input.upscaleToNextPow2_ = !noResize_;
	input.format_ = format_;
	input.mipFilter_ = noFilter_ ? D3DX_FILTER_POINT : D3DX_FILTER_LINEAR;

#if ENABLE_RESOURCE_COUNTERS
	const BW::string& allocator = allocator_;
#else
	static BW::string allocator = "texture/unknown managed texture";
#endif

	TextureLoader tl;
	tl.loadTextureFromFile( qualifiedResourceID_,
							input, allocator );
	this->processLoaderOutput( tl, notFoundPolicy );
}

void ManagedTexture::release( )
{
	BW_GUARD;
	// release our reference to the texture
	texture_ = NULL;

	if (reuseOnRelease_ && tex_.hasComObject())
	{
		rc().putTextureToReuseList( tex_ );
	}
	tex_.pComObject( NULL );
	cubeTex_.pComObject( NULL );

	this->status( STATUS_NOT_READY );

	textureMemoryUsed_ = 0;
	width_ = 0;
	height_ = 0;
}

DX::BaseTexture* ManagedTexture::pTexture()
{
	BW_GUARD;
	if( this->status() !=  STATUS_READY)
	{
		this->load( BaseTexture::FAIL_ATTEMPT_LOAD_PLACEHOLDER );
	}

	if (!rc().frameDrawn(frameTimestamp_))
	{
		totalFrameTexmem_ = 0;
	}

	if (frameTimestamp_ != localFrameTimestamp_)
	{
		localFrameTimestamp_ = frameTimestamp_;
		totalFrameTexmem_ += textureMemoryUsed_;
	}
	return texture_;
}

std::ostream& operator<<(std::ostream& o, const ManagedTexture& t)
{
	o << "ManagedTexture\n";
	return o;
}

} // namespace Moo

BW_END_NAMESPACE

// managed_texture.cpp
