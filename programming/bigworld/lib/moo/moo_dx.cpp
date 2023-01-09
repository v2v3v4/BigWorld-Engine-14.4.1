#include "pch.hpp"
#include "moo_dx.hpp"
#include "moo/render_context.hpp"
#include "math/math_extra.hpp"

#ifdef ENABLE_DX_TRACKER
#include "cstdmf/stack_tracker.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#else
#undef MEMTRACKER_SCOPED
#undef PROFILER_SCOPED
#define MEMTRACKER_SCOPED(slot)
#define PROFILER_SCOPED(slot)
#endif


DECLARE_DEBUG_COMPONENT2( "Moo", 0 )



namespace
{
    /**
    *   This function gets the pool & format of a texture
    */
    D3DSURFACE_DESC GetLevelDesc( BW_NAMESPACE uint level, DX::BaseTexture* base )
    {
        DX::Texture* tex;
        DX::CubeTexture* cubeTex;
        D3DSURFACE_DESC desc = { D3DFMT_UNKNOWN };
        if( SUCCEEDED( base->QueryInterface( __uuidof( DX::Texture ), (LPVOID*)&tex ) ) )
        {
            tex->GetLevelDesc( level, &desc );
            tex->Release();
        }
        else if( SUCCEEDED( base->QueryInterface( __uuidof( DX::CubeTexture ), (LPVOID*)&cubeTex ) ) )
        {
            cubeTex->GetLevelDesc( level, &desc );
            cubeTex->Release();
        }
        else
            throw "if the code reaches here, please refine this function";
        return desc;
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

BW_NAMESPACE uint32 DX::surfaceSize( BW_NAMESPACE uint width, BW_NAMESPACE uint height,
    D3DFORMAT format )
{
    BW_USE_NAMESPACE

    const uint32 pitchAlignment = 64;           // What to round the pitch up to in bytes.
    uint32 bytesPerPixel = 0;
    uint32 textureSize = 0;

    // Round up height to a multiple of 4. Width is determined by pitch.
    height = roundUpToPow2Multiple( height, 4 );

    // Handle per format.
    switch ( format )
    {
        case D3DFMT_R8G8B8:         bytesPerPixel =  3; break;
        case D3DFMT_A8R8G8B8:       bytesPerPixel =  4; break;
        case D3DFMT_X8R8G8B8:       bytesPerPixel =  4; break;
        case D3DFMT_R5G6B5:         bytesPerPixel =  2; break;
        case D3DFMT_X1R5G5B5:       bytesPerPixel =  2; break;
        case D3DFMT_A1R5G5B5:       bytesPerPixel =  2; break;
        case D3DFMT_A4R4G4B4:       bytesPerPixel =  2; break;
        case D3DFMT_R3G3B2:         bytesPerPixel =  1; break;
        case D3DFMT_A8:             bytesPerPixel =  1; break;
        case D3DFMT_A8R3G3B2:       bytesPerPixel =  2; break;
        case D3DFMT_X4R4G4B4:       bytesPerPixel =  2; break;
        case D3DFMT_A2B10G10R10:    bytesPerPixel =  4; break;
        case D3DFMT_A8B8G8R8:       bytesPerPixel =  4; break;
        case D3DFMT_X8B8G8R8:       bytesPerPixel =  4; break;
        case D3DFMT_G16R16:         bytesPerPixel =  4; break;
        case D3DFMT_A2R10G10B10:    bytesPerPixel =  4; break;
        case D3DFMT_A16B16G16R16:   bytesPerPixel =  8; break;
        case D3DFMT_A8P8:           bytesPerPixel =  2; break;
        case D3DFMT_P8:             bytesPerPixel =  1; break;
        case D3DFMT_L8:             bytesPerPixel =  1; break;
        case D3DFMT_A8L8:           bytesPerPixel =  2; break;
        case D3DFMT_A4L4:           bytesPerPixel =  1; break;
        case D3DFMT_V8U8:           bytesPerPixel =  2; break;
        case D3DFMT_L6V5U5:         bytesPerPixel =  2; break;
        case D3DFMT_X8L8V8U8:       bytesPerPixel =  4; break;
        case D3DFMT_Q8W8V8U8:       bytesPerPixel =  4; break;
        case D3DFMT_V16U16:         bytesPerPixel =  4; break;
        case D3DFMT_A2W10V10U10:    bytesPerPixel =  4; break;
        case D3DFMT_UYVY:           bytesPerPixel =  1; break;
        case D3DFMT_R8G8_B8G8:      bytesPerPixel =  2; break;
        case D3DFMT_YUY2:           bytesPerPixel =  1; break;
        case D3DFMT_G8R8_G8B8:      bytesPerPixel =  2; break;
        case D3DFMT_D16_LOCKABLE:   bytesPerPixel =  2; break;
        case D3DFMT_D32:            bytesPerPixel =  4; break;
        case D3DFMT_D15S1:          bytesPerPixel =  2; break;
        case D3DFMT_D24S8:          bytesPerPixel =  4; break;
        case D3DFMT_D24X8:          bytesPerPixel =  4; break;
        case D3DFMT_D24X4S4:        bytesPerPixel =  4; break;
        case D3DFMT_D16:            bytesPerPixel =  2; break;
        case D3DFMT_D32F_LOCKABLE:  bytesPerPixel =  4; break;
        case D3DFMT_D24FS8:         bytesPerPixel =  4; break;
        case D3DFMT_L16:            bytesPerPixel =  2; break;
        case D3DFMT_Q16W16V16U16:   bytesPerPixel =  8; break;
        case D3DFMT_MULTI2_ARGB8:   bytesPerPixel =  0; break;      // neilr: I'm not sure this is right. Perhaps we want to assert here?
        case D3DFMT_R16F:           bytesPerPixel =  2; break;
        case D3DFMT_G16R16F:        bytesPerPixel =  4; break;
        case D3DFMT_A16B16G16R16F:  bytesPerPixel =  8; break;
        case D3DFMT_R32F:           bytesPerPixel =  4; break;
        case D3DFMT_G32R32F:        bytesPerPixel =  8; break;
        case D3DFMT_A32B32G32R32F:  bytesPerPixel = 16; break;
        case D3DFMT_CxV8U8:         bytesPerPixel =  2; break;
        case D3DFMT_DXT1:           
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
        {
            int numBlocksWide = 0;
            if( width > 0 )
                numBlocksWide = std::max<UINT>( 1, width / 4 );
            int numBlocksHigh = 0;
            if( height > 0 )
                numBlocksHigh = std::max<UINT>( 1, height / 4 );
            int numBytesPerBlock = ( format == D3DFMT_DXT1 ? 8 : 16 );
            uint32 rowBytes = numBlocksWide * numBytesPerBlock;
            uint32 numRows = numBlocksHigh;
            return uint32( rowBytes * numRows );
        }
        default:
            MF_ASSERT( !"Unhandled D3DFMT" );
            break;
    }

    // General case calculation. Must round up to pitch size, and then to total
    uint pitchBytes = roundUpToPow2Multiple( width * bytesPerPixel, pitchAlignment );
    return ( pitchBytes * height );
}

BW_NAMESPACE uint32 DX::surfaceSize( const D3DSURFACE_DESC& desc )
{
    return surfaceSize(desc.Width, desc.Height, desc.Format);
}

BW_NAMESPACE uint32 DX::textureSize( const BaseTexture *constTexture, BW_NAMESPACE ResourceCounters::MemoryType infoType )
{
    BW_USE_NAMESPACE
    BW_GUARD;
    if (constTexture == NULL)
        return 0;
    uint32 baseSize = 0;
    uint32 overheadSize = 0;

    // The operations on the texture are really const operations
    BaseTexture *texture = const_cast<BaseTexture *>(constTexture);
    D3DSURFACE_DESC baseSurfaceDesc = GetLevelDesc(0, texture);

    // Get a surface to determine the width, height, and format
    uint32 levelCount = texture->GetLevelCount();

    // Data size.
    for(uint32 levelIdx = 0; levelIdx < levelCount; ++levelIdx)
    {
        D3DSURFACE_DESC surfaceDesc = GetLevelDesc(levelIdx, texture);

        // Accumulate surface size.
        baseSize += DX::surfaceSize(surfaceDesc);
    }

    // Track memory usage
    if(baseSize > (1024*1024))
    {
        baseSize = roundUpToPow2Multiple(baseSize, 65536);
    }
    else
    {
        baseSize = roundUpToPow2Multiple(baseSize, 4096);
    }

    // Overhead.
    if(levelCount > 1)
    {
        overheadSize += 4096 + (levelCount * 1024);
    }
    else
    {
        overheadSize += 3072;
    }

    // Return appropriate value for combination of device type, memory type, pool, usage, etc.
    // NOTE: Check  d3dpool_default with static textures in D3D9. Mapping may be the same as D3D9ex.
    if(infoType == BW_NAMESPACE ResourceCounters::MT_MAIN)
    {
        if(!Moo::rc().usingD3DDeviceEx())
        {
            // D3D9.
            switch(baseSurfaceDesc.Pool)
            {
            case D3DPOOL_DEFAULT:

                return baseSize + overheadSize;
                break;
            case D3DPOOL_MANAGED:
                return baseSize + overheadSize;
                break;
            case D3DPOOL_SYSTEMMEM:
                return baseSize + overheadSize;
                break;
            default:
                assert(0);
                break;
            }
        }
        else
        {
            // D3D9ex.
            switch(baseSurfaceDesc.Pool)
            {
            case D3DPOOL_DEFAULT:
                if((baseSurfaceDesc.Usage & D3DUSAGE_DYNAMIC) != 0)
                {
                    return baseSize + overheadSize;
                }
                else
                {
                    return overheadSize;
                }
                break;
            case D3DPOOL_SYSTEMMEM:
                return baseSize + overheadSize;
                break;
            default:
                assert(0);
                break;
            }
        }
    }
    else if(infoType == BW_NAMESPACE ResourceCounters::MT_GPU)
    {
        if(!Moo::rc().usingD3DDeviceEx())
        {
            // D3D9.
            switch(baseSurfaceDesc.Pool)
            {
            case D3DPOOL_DEFAULT:
                return baseSize;
                break;
            case D3DPOOL_MANAGED:
                return 0;
                break;
            case D3DPOOL_SYSTEMMEM:
                return 0;
                break;
            default:
                assert(0);
                break;
            }
        }
        else
        {
            // D3D9ex.
            switch(baseSurfaceDesc.Pool)
            {
            case D3DPOOL_DEFAULT:
                return baseSize;
            case D3DPOOL_SYSTEMMEM:
                return 0;
                break;
            default:
                assert(0);
                break;
            }
        }
    }

    return 0;
}

BW_NAMESPACE uint32 DX::vertexBufferSize( const VertexBuffer *constVertexBuffer,
    BW_NAMESPACE ResourceCounters::MemoryType infoType )
{
    BW_USE_NAMESPACE
    BW_GUARD;
    if (constVertexBuffer == NULL)
        return 0;
    // Get a vertex buffer description
    VertexBuffer* vertexBuffer = const_cast<VertexBuffer*>(constVertexBuffer);
    D3DVERTEXBUFFER_DESC vbDesc;
    vertexBuffer->GetDesc(&vbDesc);

    uint32 baseSize = roundUpToPow2Multiple(vbDesc.Size, 4096);
    uint32 overheadSize = 2048;
    
    // NOTE: Take a look into the dynamic flags here.
    if(infoType == BW_NAMESPACE ResourceCounters::MT_MAIN)
    {
        if(!Moo::rc().usingD3DDeviceEx())
        {
            // D3D9.
            switch(vbDesc.Pool)
            {
            case D3DPOOL_DEFAULT:
            case D3DPOOL_MANAGED:
            case D3DPOOL_SYSTEMMEM:
                return baseSize + overheadSize;
                break;
            default:
                assert(0);
                break;
            }
        }
        else
        {
            // D3D9ex.
            switch(vbDesc.Pool)
            {
            case D3DPOOL_DEFAULT:
            case D3DPOOL_SYSTEMMEM:
                return baseSize + overheadSize;
                break;
            default:
                assert(0);
                break;
            }
        }
    }
    else if(infoType == BW_NAMESPACE ResourceCounters::MT_GPU)
    {
        if(!Moo::rc().usingD3DDeviceEx())
        {
            // D3D9.
            switch(vbDesc.Pool)
            {
            case D3DPOOL_DEFAULT:
                return baseSize;
                break;
            case D3DPOOL_MANAGED:
            case D3DPOOL_SYSTEMMEM:
                return 0;
                break;
            default:
                assert(0);
                break;
            }
        }
        else
        {
            // D3D9ex.
            switch(vbDesc.Pool)
            {
            case D3DPOOL_DEFAULT:
                return baseSize;
            case D3DPOOL_SYSTEMMEM:
                return 0;
                break;
            default:
                assert(0);
                break;
            }
        }
    }

    return 0;
}

BW_NAMESPACE uint32 DX::indexBufferSize( const IndexBuffer * constIndexBuffer,
    BW_NAMESPACE ResourceCounters::MemoryType infoType )
{
    BW_USE_NAMESPACE
    BW_GUARD;
    if (constIndexBuffer == NULL)
        return 0;
    // Get a vertex buffer description
    IndexBuffer* indexBuffer = const_cast<IndexBuffer*>(constIndexBuffer);
    D3DINDEXBUFFER_DESC ibDesc;
    indexBuffer->GetDesc(&ibDesc);

    uint32 baseSize = roundUpToPow2Multiple(ibDesc.Size, 4096);
    uint32 overheadSize = 2048;
    
    if(infoType == BW_NAMESPACE ResourceCounters::MT_MAIN)
    {
        if(!Moo::rc().usingD3DDeviceEx())
        {
            // D3D9.
            switch(ibDesc.Pool)
            {
            case D3DPOOL_DEFAULT:
            case D3DPOOL_MANAGED:
            case D3DPOOL_SYSTEMMEM:
                return baseSize + overheadSize;
                break;
            default:
                assert(0);
                break;
            }
        }
        else
        {
            // D3D9ex.
            switch(ibDesc.Pool)
            {
            case D3DPOOL_DEFAULT:
            case D3DPOOL_SYSTEMMEM:
                return baseSize + overheadSize;
                break;
            default:
                assert(0);
                break;
            }
        }
    }
    else if(infoType == BW_NAMESPACE ResourceCounters::MT_GPU)
    {
        if(!Moo::rc().usingD3DDeviceEx())
        {
            // D3D9.
            switch(ibDesc.Pool)
            {
            case D3DPOOL_DEFAULT:
                return baseSize;
                break;
            case D3DPOOL_MANAGED:
            case D3DPOOL_SYSTEMMEM:
                return 0;
                break;
            default:
                assert(0);
                break;
            }
        }
        else
        {
            // D3D9ex.
            switch(ibDesc.Pool)
            {
            case D3DPOOL_DEFAULT:
                return baseSize;
            case D3DPOOL_SYSTEMMEM:
                return 0;
                break;
            default:
                assert(0);
                break;
            }
        }
    }

    return 0;
}

#define ERRORSTRING( code, ext )                                            \
    if (hr == code)                                                         \
    {                                                                       \
        bw_snprintf( res, sizeof(res), #code "(0x%08x) : %s", code, ext );  \
    }


BW::string DX::errorAsString( HRESULT hr )
{
    char res[1024];
         ERRORSTRING( D3D_OK, "No error occurred." )
    else ERRORSTRING( D3DOK_NOAUTOGEN, "This is a success code. However, the autogeneration of mipmaps is not supported for this format. This means that resource creation will succeed but the mipmap levels will not be automatically generated." )
    else ERRORSTRING( D3DERR_CONFLICTINGRENDERSTATE, "The currently set render states cannot be used together." )
    else ERRORSTRING( D3DERR_CONFLICTINGTEXTUREFILTER, "The current texture filters cannot be used together." )
    else ERRORSTRING( D3DERR_CONFLICTINGTEXTUREPALETTE, "The current textures cannot be used simultaneously." )
    else ERRORSTRING( D3DERR_DEVICELOST, "The device has been lost but cannot be reset at this time. Therefore, rendering is not possible." )
    else ERRORSTRING( D3DERR_DEVICENOTRESET, "The device has been lost but can be reset at this time." )
    else ERRORSTRING( D3DERR_DRIVERINTERNALERROR, "Internal driver error. Applications should destroy and recreate the device when receiving this error. For hints on debugging this error, see Driver Internal Errors (Direct3D 9)." )
    else ERRORSTRING( D3DERR_DRIVERINVALIDCALL, "Not used." )
    else ERRORSTRING( D3DERR_INVALIDCALL, "The method call is invalid. For example, a method's parameter may not be a valid pointer." )
    else ERRORSTRING( D3DERR_INVALIDDEVICE, "The requested device type is not valid." )
    else ERRORSTRING( D3DERR_MOREDATA, "There is more data available than the specified buffer size can hold." )
    else ERRORSTRING( D3DERR_NOTAVAILABLE, "This device does not support the queried technique." )
    else ERRORSTRING( D3DERR_NOTFOUND, "The requested item was not found." )
    else ERRORSTRING( D3DERR_OUTOFVIDEOMEMORY, "Direct3D does not have enough display memory to perform the operation." )
    else ERRORSTRING( D3DERR_TOOMANYOPERATIONS, "The application is requesting more texture-filtering operations than the device supports." )
    else ERRORSTRING( D3DERR_UNSUPPORTEDALPHAARG, "The device does not support a specified texture-blending argument for the alpha channel." )
    else ERRORSTRING( D3DERR_UNSUPPORTEDALPHAOPERATION, "The device does not support a specified texture-blending operation for the alpha channel." )
    else ERRORSTRING( D3DERR_UNSUPPORTEDCOLORARG, "The device does not support a specified texture-blending argument for color values." )
    else ERRORSTRING( D3DERR_UNSUPPORTEDCOLOROPERATION, "The device does not support a specified texture-blending operation for color values." )
    else ERRORSTRING( D3DERR_UNSUPPORTEDFACTORVALUE, "The device does not support the specified texture factor value. Not used; provided only to support older drivers." )
    else ERRORSTRING( D3DERR_UNSUPPORTEDTEXTUREFILTER, "The device does not support the specified texture filter." )
    else ERRORSTRING( D3DERR_WASSTILLDRAWING, "The previous blit operation that is transferring information to or from this surface is incomplete." )
    else ERRORSTRING( D3DERR_WRONGTEXTUREFORMAT, "The pixel format of the texture surface is not valid." )

    else ERRORSTRING( D3DXERR_CANNOTMODIFYINDEXBUFFER, "The index buffer cannot be modified." )
    else ERRORSTRING( D3DXERR_INVALIDMESH, "The mesh is invalid." )
    else ERRORSTRING( D3DXERR_CANNOTATTRSORT, "Attribute sort (D3DXMESHOPT_ATTRSORT) is not supported as an optimization technique." )
    else ERRORSTRING( D3DXERR_SKINNINGNOTSUPPORTED, "Skinning is not supported." )
    else ERRORSTRING( D3DXERR_TOOMANYINFLUENCES, "Too many influences specified." )
    else ERRORSTRING( D3DXERR_INVALIDDATA, "The data is invalid." )
    else ERRORSTRING( D3DXERR_LOADEDMESHASNODATA, "The mesh has no data." )
    else ERRORSTRING( D3DXERR_DUPLICATENAMEDFRAGMENT, "A fragment with that name already exists." )
    else ERRORSTRING( D3DXERR_CANNOTREMOVELASTITEM, "The last item cannot be deleted." )

    else ERRORSTRING( E_FAIL, "An undetermined error occurred inside the Direct3D subsystem." )
    else ERRORSTRING( E_INVALIDARG, "An invalid parameter was passed to the returning function." )
//  else ERRORSTRING( E_INVALIDCALL, "The method call is invalid. For example, a method's parameter may have an invalid value." )
    else ERRORSTRING( E_NOINTERFACE, "No object interface is available." )
    else ERRORSTRING( E_NOTIMPL, "Not implemented." )
    else ERRORSTRING( E_OUTOFMEMORY, "Direct3D could not allocate sufficient memory to complete the call." )
    else ERRORSTRING( S_OK, "No error occurred." )
    else
    {
        bw_snprintf( res, sizeof(res), "Unknown(0x%08x)", hr );
    }

    return BW::string(res);
}
