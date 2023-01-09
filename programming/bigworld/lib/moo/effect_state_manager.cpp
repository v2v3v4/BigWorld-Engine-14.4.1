#include "pch.hpp"
#include "effect_state_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 );


BW_BEGIN_NAMESPACE

extern void * gRenderTgtTexture;

namespace Moo
{

/*
 * Com QueryInterface method
 */
HRESULT __stdcall StateManager::QueryInterface( REFIID iid, LPVOID *ppv)
{
    if (iid == IID_IUnknown || iid == IID_ID3DXEffectStateManager)
    {
        *ppv = static_cast<ID3DXEffectStateManager*>(this);
    } 
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}

/*
 * Com AddRef method
 */
ULONG __stdcall StateManager::AddRef()
{
    return this->incRef();
}

/*
 * Com Release method
 */
ULONG __stdcall StateManager::Release()
{
    return this->decRef();
}

/*
 *	LightEnable
 */
HRESULT __stdcall StateManager::LightEnable( DWORD index, BOOL enable )
{
	return rc().device()->LightEnable( index, enable );
}

/*
 *	SetFVF
 */
HRESULT __stdcall StateManager::SetFVF( DWORD fvf )
{
	return rc().setFVF( fvf );
}

/*
 *	SetLight
 */
HRESULT __stdcall StateManager::SetLight( DWORD index, CONST D3DLIGHT9* pLight )
{
	return rc().device()->SetLight( index, pLight );
}

/*
 *	SetMaterial
 */
HRESULT __stdcall StateManager::SetMaterial( CONST D3DMATERIAL9* pMaterial )
{
	return rc().device()->SetMaterial( pMaterial );
}

/*
 *	SetNPatchMode
 */
HRESULT __stdcall StateManager::SetNPatchMode( FLOAT nSegments )
{
	return rc().device()->SetNPatchMode( nSegments );
}

/*
 *	SetPixelShader
 */
HRESULT __stdcall StateManager::SetPixelShader( LPDIRECT3DPIXELSHADER9 pShader )
{
//	PROFILER_SCOPED( StateManager_SetPixelShader );
	return rc().device()->SetPixelShader( pShader );
}

/*
 *	SetPixelShaderConstantB
 */
HRESULT __stdcall StateManager::SetPixelShaderConstantB( UINT reg, CONST BOOL* pData, UINT count )
{
//	PROFILER_SCOPED( StateManager_SetPixelShaderConstant );
	return rc().device()->SetPixelShaderConstantB( reg, pData, count );
}

/*
 *	SetPixelShaderConstantF
 */
HRESULT __stdcall StateManager::SetPixelShaderConstantF( UINT reg, CONST FLOAT* pData, UINT count )
{
//	PROFILER_SCOPED( StateManager_SetPixelShaderConstant );
	return rc().device()->SetPixelShaderConstantF( reg, pData, count );

}

/*
 *	SetPixelShaderConstantI
 */
HRESULT __stdcall StateManager::SetPixelShaderConstantI( UINT reg, CONST INT* pData, UINT count )
{
//	PROFILER_SCOPED( StateManager_SetPixelShaderConstant );
	return rc().device()->SetPixelShaderConstantI( reg, pData, count );
}

/*
 *	SetVertexShader
 */
HRESULT __stdcall StateManager::SetVertexShader( LPDIRECT3DVERTEXSHADER9 pShader )
{
	//PROFILER_SCOPED( StateManager_SetVertexShader );
	return rc().setVertexShader( pShader );
}

/*
 *	SetVertexShaderConstantB
 */
HRESULT __stdcall StateManager::SetVertexShaderConstantB( UINT reg, CONST BOOL* pData, UINT count )
{
//	PROFILER_SCOPED( StateManager_SetVertexShaderConstant );
	return rc().device()->SetVertexShaderConstantB( reg, pData, count );
}

/*
 *	SetVertexShaderConstantF
 */
HRESULT __stdcall StateManager::SetVertexShaderConstantF( UINT reg, CONST FLOAT* pData, UINT count )
{
//	PROFILER_SCOPED( StateManager_SetVertexShaderConstant );
	HRESULT hr;
	hr = rc().device()->SetVertexShaderConstantF( reg, pData, count );
	return hr;
}

/*
 *	SetVertexShaderConstantI
 */
HRESULT __stdcall StateManager::SetVertexShaderConstantI( UINT reg, CONST INT* pData, UINT count )
{
//	PROFILER_SCOPED( StateManager_SetVertexShaderConstant );
	return rc().device()->SetVertexShaderConstantI( reg, pData, count );
}

/*
 *	SetRenderState
 */
HRESULT __stdcall StateManager::SetRenderState( D3DRENDERSTATETYPE state, DWORD value )
{
	if (Moo::rc().mirroredTransform() && state == D3DRS_CULLMODE)
		value = value ^ (value >> 1);
	return rc().setRenderState( state, value );
}

/*
 *	SetVertexSamplerState
 */
HRESULT __stdcall StateManager::SetSamplerState( DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value )
{
	return rc().setSamplerState( sampler, type, value );
}

/*
 *	SetTexture
 */
HRESULT __stdcall StateManager::SetTexture( DWORD stage, LPDIRECT3DBASETEXTURE9 pTexture)
{
	return rc().setTexture( stage, pTexture );
}

/*
 *	SetTextureStageState
 */
HRESULT __stdcall StateManager::SetTextureStageState( DWORD stage, D3DTEXTURESTAGESTATETYPE type,
	DWORD value )
{
	return rc().setTextureStageState( stage, type, value );
}

/*
 *	SetTransform
 */
HRESULT __stdcall StateManager::SetTransform( D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* pMatrix )
{
	return rc().device()->SetTransform( state, pMatrix );
}


} // namespace Moo

BW_END_NAMESPACE

// effect_state_manager.cpp
