
#include "pch.hpp"
#include "sys_mem_texture.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/file_streamer.hpp"
#include "resmgr/multi_file_system.hpp"
#include "texture_loader.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

namespace Moo
{

// -----------------------------------------------------------------------------
// Section: SysMemTexture
// -----------------------------------------------------------------------------

/**
 *  Constructor.
 */
SysMemTexture::SysMemTexture( const BW::StringRef & resourceID, D3DFORMAT format ) :
    width_( 0 ),
    height_( 0 ),
    format_(format),
    resourceID_( canonicalTextureName(resourceID) ),
    qualifiedResourceID_( resourceID.data(), resourceID.size() ),
    reuseOnRelease_ ( false )
{
    load();
}

SysMemTexture::SysMemTexture( BinaryPtr data, D3DFORMAT format ) :
    width_( 0 ),
    height_( 0 ),
    format_(format),
    resourceID_( "" ),
    qualifiedResourceID_( "" )
{
    load(data);
}

SysMemTexture::SysMemTexture( DX::Texture* pTex ):
    width_( 0 ),
    height_( 0 ),
    format_( D3DFMT_UNKNOWN )
{
    D3DSURFACE_DESC desc;
    if ( SUCCEEDED( pTex->GetLevelDesc( 0, &desc ) ) )
    {
        pTexture_ = pTex;
        width_ = desc.Width;
        height_ = desc.Height;
        format_ = desc.Format;
        this->status( STATUS_READY );
    }
}

SysMemTexture::~SysMemTexture()
{
    // let the texture manager know we're gone
    // reuse it if reuse flag is set
    // DISABLED reuse for system memory textures till we get some stats on it
    /*if (reuseOnRelease_ && pTexture_.hasComObject())
    {
        rc().putTextureToReuseList( pTexture_ );
    }*/
}


void SysMemTexture::destroy() const
{
    // attempt to safety delete the texture
    this->tryDestroy();
}


void SysMemTexture::load()
{
    load( NULL );
}

void SysMemTexture::load(BinaryPtr data)
{
    BW_GUARD;
    if (status() == STATUS_FAILED_TO_LOAD)
    {
        return;
    }
    status( STATUS_LOADING );

    if (pTexture_.pComObject())
    {
        status( STATUS_READY );
        return;
    }

    if (rc().device() == NULL)
    {
        status( STATUS_FAILED_TO_LOAD );
        return;
    }

    HRESULT hr = E_FAIL;

    // Open the texture file resource.
    if (data == NULL)
        data = BWResource::instance().rootSection()->readBinary( qualifiedResourceID_ );

    if( data.hasObject() )
    {
        ComObjectWrap<DX::Texture> tex;
        static PALETTEENTRY palette[256];

        // Create the texture and the mipmaps.
        tex = Moo::rc().createTextureFromFileInMemoryEx( data->data(), data->len(),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, format_, 
            D3DPOOL_SYSTEMMEM, D3DX_FILTER_BOX|D3DX_FILTER_MIRROR, 
            D3DX_FILTER_BOX|D3DX_FILTER_MIRROR, 0, NULL, palette );

        if( tex )
        {
            DX::Surface* surface;
            if( SUCCEEDED( tex->GetSurfaceLevel( 0, &surface ) ) )
            {
                D3DSURFACE_DESC desc;
                surface->GetDesc( &desc );

                width_ = desc.Width;
                height_ = desc.Height;
                format_ = desc.Format;

                surface->Release();
                surface = NULL;
            }
            // Assign the texture to the smartpointer.
            pTexture_ = tex;
        }
        else
        {
            ERROR_MSG( "SysMemTexture::load(%s): Unknown texture format\n",
                qualifiedResourceID_.c_str() );
        }
    }

    if (pTexture_)
    {
        this->status( STATUS_READY );
        reuseOnRelease_ = true;
    }
    else
    {
        this->status( STATUS_FAILED_TO_LOAD );
    }
}

void    SysMemTexture::reload()
{ 
    release();
    load();
}
void    SysMemTexture::reload( const BW::StringRef & resourceID )
{ 
    release();

    resourceID_   = canonicalTextureName(resourceID);
    qualifiedResourceID_ = resourceID.to_string();

    load();
}

void                SysMemTexture::release()
{
    pTexture_ = NULL;
    height_ = 0;
    width_ = 0;
}


} // namespace Moo

BW_END_NAMESPACE

// sys_mem_texture.cpp

