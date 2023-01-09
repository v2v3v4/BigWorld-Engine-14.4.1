#ifndef EFFECT_STATE_MANAGER_HPP
#define EFFECT_STATE_MANAGER_HPP

#include "cstdmf/vectornodest.hpp"
#include "resmgr/bwresource.hpp"
#include "com_object_wrap.hpp"
#include "texture_manager.hpp"
#include "render_context_callback.hpp"
#include <d3dx9.h>


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This class implements the d3d effect state manager.
 *	It lets us use the render context state manager when using effects.
 *
 */
class StateManager : public ID3DXEffectStateManager, public SafeReferenceCount
{
public:
	StateManager() {}
	virtual ~StateManager() {}

	STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

	HRESULT __stdcall LightEnable( DWORD index, BOOL enable );
	HRESULT __stdcall SetFVF( DWORD fvf );
	HRESULT __stdcall SetLight( DWORD index, CONST D3DLIGHT9* pLight );
	HRESULT __stdcall SetMaterial( CONST D3DMATERIAL9* pMaterial );
	HRESULT __stdcall SetNPatchMode( FLOAT nSegments );
	HRESULT __stdcall SetPixelShader( LPDIRECT3DPIXELSHADER9 pShader );	
	HRESULT __stdcall SetPixelShaderConstantB( UINT reg, CONST BOOL* pData, UINT count );
	HRESULT __stdcall SetPixelShaderConstantF( UINT reg, CONST FLOAT* pData, UINT count );
	HRESULT __stdcall SetPixelShaderConstantI( UINT reg, CONST INT* pData, UINT count );
	HRESULT __stdcall SetVertexShader( LPDIRECT3DVERTEXSHADER9 pShader );
	HRESULT __stdcall SetVertexShaderConstantB( UINT reg, CONST BOOL* pData, UINT count );
	HRESULT __stdcall SetVertexShaderConstantF( UINT reg, CONST FLOAT* pData, UINT count );
	HRESULT __stdcall SetVertexShaderConstantI( UINT reg, CONST INT* pData, UINT count );
	HRESULT __stdcall SetRenderState( D3DRENDERSTATETYPE state, DWORD value );
	HRESULT __stdcall SetSamplerState( DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value );
	HRESULT __stdcall SetTexture( DWORD stage, LPDIRECT3DBASETEXTURE9 pTexture);
	HRESULT __stdcall SetTextureStageState( DWORD stage, D3DTEXTURESTAGESTATETYPE type, 
		DWORD value );
	HRESULT __stdcall SetTransform( D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* pMatrix );
};

} // namespace Moo

BW_END_NAMESPACE

#endif // EFFECT_STATE_MANAGER_HPP
