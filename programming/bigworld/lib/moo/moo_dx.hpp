#ifndef MOO_DX_HPP
#define MOO_DX_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/list_node.hpp"
#include "cstdmf/smartpointer.hpp"

#include <d3d9.h>
#include <d3d9types.h>
#include <d3dx9effect.h>
#include "cstdmf/bw_string.hpp"
#include "cstdmf/resource_counters.hpp"
#include "math/math_extra.hpp"

// If we have a stack tracker we can also enable DX tracking.
// Should normally be off as it requires a mutex when maintaining its lists.
#if ENABLE_STACK_TRACKER
//#define ENABLE_DX_TRACKER
#endif


#ifndef D3DRS_MAX
// Must be larger than the maximum valid value of the enum D3DRENDERSTATETYPE
#define D3DRS_MAX       210
#endif // D3DRS_MAX

#ifndef D3DTSS_MAX
// Must be larger than the maximum valid value of the enum D3DTEXTURESTAGESTATETYPE
#define D3DTSS_MAX      33
#endif // D3DTSS_MAX

#ifndef D3DSAMP_MAX
// Must be larger than the maximum valid value of the enum D3DSAMPLERSTATETYPE
#define D3DSAMP_MAX		14
#endif // D3DSAMP_MAX

#ifndef D3DFFSTAGES_MAX
// Always 8, the maximum number of stages supported by the fixed function pipeline
#define D3DFFSTAGES_MAX	8
#endif // D3DFFSTAGES_MAX

#ifndef D3DSAMPSTAGES_MAX
// Must be larger than the maximum valid sampler stage, ie. D3DVERTEXTEXTURESAMPLER3
#define D3DSAMPSTAGES_MAX	261
#endif // D3DSAMPSTAGES_MAX

namespace DX
{
	typedef IDirect3D9				Interface;
	typedef IDirect3D9Ex			InterfaceEx;

	typedef IDirect3DDevice9		Device;
	typedef IDirect3DDevice9Ex		DeviceEx;
	typedef IDirect3DResource9		Resource;
	typedef IDirect3DBaseTexture9	BaseTexture;
	typedef IDirect3DTexture9		Texture;
	typedef IDirect3DCubeTexture9	CubeTexture;
	typedef IDirect3DSurface9		Surface;
	typedef IDirect3DVertexBuffer9	VertexBuffer;
	typedef IDirect3DIndexBuffer9	IndexBuffer;
	typedef IDirect3DPixelShader9	PixelShader;
	typedef IDirect3DVertexShader9	VertexShader;
	typedef IDirect3DVertexDeclaration9 VertexDeclaration;
	typedef IDirect3DQuery9			Query;

	typedef D3DLIGHT9				Light;
	typedef D3DVIEWPORT9			Viewport;
	typedef D3DMATERIAL9			Material;

	BW_NAMESPACE uint32	surfaceSize( BW_NAMESPACE uint width, 
									BW_NAMESPACE uint height,
									D3DFORMAT format );
	BW_NAMESPACE uint32	surfaceSize( const D3DSURFACE_DESC& desc );
	
	BW_NAMESPACE uint32	textureSize( const BaseTexture *texture, BW_NAMESPACE ResourceCounters::MemoryType infoType );
	BW_NAMESPACE uint32 vertexBufferSize( const VertexBuffer *vertexBuffer, BW_NAMESPACE ResourceCounters::MemoryType infoType );
	BW_NAMESPACE uint32 indexBufferSize( const IndexBuffer *vertexBuffer, BW_NAMESPACE ResourceCounters::MemoryType infoType );

	BW::string errorAsString( HRESULT hr );

	enum
	{
		WRAPPER_FLAG_IMMEDIATE_LOCK		= ( 1 << 0 ),
		WRAPPER_FLAG_DEFERRED_LOCK		= ( 1 << 1 ),
		WRAPPER_FLAG_ZERO_TEXTURE_LOCK	= ( 1 << 2 ),
		WRAPPER_FLAG_QUERY_ISSUE_FLUSH	= ( 1 << 3 ),
	};
}
#endif // MOO_DX_HPP
