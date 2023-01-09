#include "pch.hpp"

#include "shadows.hpp"
#include "shadow_math.hpp"

//------------------------------------------------------------------------------
// Help macroses
//------------------------------------------------------------------------------

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) \
{						\
	if(p) {				\
		(p)->Release();	\
		(p)=NULL;		\
	}					\
}
#endif

//------------------------------------------------------------------------------
// Base class
//------------------------------------------------------------------------------

bool 
Shadows::init(void* device, int splitsCount, int shadowMapSize)
{
	release();
	m_splitsCount = splitsCount <= SHADOWS_MAX_SPLITS ? splitsCount : SHADOWS_MAX_SPLITS;
	m_shadowMapSize = shadowMapSize;
	return true;
}

void 
Shadows::release()
{
}

void 
Shadows::update(const D3DXMATRIX& view,						
				float fov, float aspectRatio,
				float shadowsNearPlane, float shadowsFarPlane,
				const D3DXVECTOR3& lightDir, float lightDist)
{
	//calcSplitDistances(m_splitDistances, m_splitsCount, shadowsNearPlane, shadowsFarPlane);
	//m_view = view;
	//for(size_t i = 0; i < m_splitsCount; ++i) {
	//	calcLightViewProjectionMatrixJitteringSuppressed(
	//		m_viewMatrices[i], m_projMatrices[i], m_viewProjMatrices[i], 
	//		m_view, m_splitDistances[i], m_splitDistances[i + 1], fov, aspectRatio, 
	//		lightDir, lightDist,
	//		m_shadowMapSize);
	//}
}

//------------------------------------------------------------------------------
// DX9 implementation
//------------------------------------------------------------------------------

#ifdef SHADOWS_ENABLE_DX9_SUPPORT

ShadowsDX9::ShadowsDX9() :
	m_device(NULL),
	m_depthStencil(NULL),
	m_oldDepthStencil(NULL),
	m_oldRenderTarget(NULL)
{

}

ShadowsDX9::~ShadowsDX9() 
{
	release();
}

bool 
ShadowsDX9::init(void* device, int splitsCount, int shadowMapSize)
{
	if(!Shadows::init(device, splitsCount, shadowMapSize)) {
		return false;
	}
	m_device = (LPDIRECT3DDEVICE9) device;
	return OnDeviceReset();
}

void 
ShadowsDX9::release()
{
	OnDeviceLost();
	m_splitsCount = 0;
	m_device = NULL;
}

void 
ShadowsDX9::OnDeviceLost()
{
	for(size_t i = 0; i < m_splitsCount; ++i) {
		SAFE_RELEASE(m_shadowMaps[i]);
	}
	SAFE_RELEASE(m_depthStencil);
}

bool 
ShadowsDX9::OnDeviceReset()
{
	if(m_device) { // if initialized
		HRESULT hr = D3D_OK;

		D3DSURFACE_DESC descs[SHADOWS_MAX_SPLITS] = {};

		// depth textures
		for(size_t i = 0; i < m_splitsCount; ++i) {
			hr |= m_device->CreateTexture(
				m_shadowMapSize, m_shadowMapSize, 1, 
				D3DUSAGE_RENDERTARGET, D3DFMT_R32F,
				D3DPOOL_DEFAULT, &m_shadowMaps[i], NULL
				);

			if(SUCCEEDED(hr)) {
				m_shadowMaps[i]->GetLevelDesc(0, &descs[i]);
			}
		}

		// depth-stencil
		hr |= m_device->CreateDepthStencilSurface(
			m_shadowMapSize, m_shadowMapSize,
			D3DFMT_D24X8, // TODO: choose a format to suit the hardware
			descs[0].MultiSampleType, descs[0].MultiSampleQuality,
			TRUE, &m_depthStencil, NULL);

		if(FAILED(hr)) {
			release();
			return false;
		} else {
			return true;
		}
	}
	return false;
}

void 
ShadowsDX9::getEffectMacroDefinitions(BW::vector<D3DXMACRO>& outMacroses)
{
	// TODO: ...
	outMacroses.clear();
}

void 
ShadowsDX9::beginCastPass(int splitNumber)
{
	if(m_device) {
		// store old RT and depth stencil
		m_device->GetRenderTarget(0, &m_oldRenderTarget);
		m_device->GetDepthStencilSurface(&m_oldDepthStencil);

		// set our RT and depth stencil
		LPDIRECT3DSURFACE9 renderTarget;
		m_shadowMaps[splitNumber]->GetSurfaceLevel(0, &renderTarget);

		m_device->SetRenderTarget(0, renderTarget);
		m_device->SetDepthStencilSurface(m_depthStencil);

		SAFE_RELEASE(renderTarget);

		// clear device
		m_device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFFFFFFFF, 1.f, 0);
	}
}

void 
ShadowsDX9::endCastPass()
{
	if(m_device) {
		m_device->SetRenderTarget(0, m_oldRenderTarget);
		m_device->SetDepthStencilSurface(m_oldDepthStencil);

		SAFE_RELEASE(m_oldRenderTarget);
		SAFE_RELEASE(m_oldDepthStencil);
	}
}

void 
ShadowsDX9::beginReceivePass()
{

}

void 
ShadowsDX9::setReceiveEffectParams(void* the_effect)
{
	LPD3DXEFFECT effect = (LPD3DXEFFECT) the_effect;
	effect->SetFloatArray("g_splitPlanesArray", &m_splitDistances[1], UINT(m_splitsCount));
	effect->SetVector("g_splitPlanesVector1", (D3DXVECTOR4*) &m_splitDistances[0]);
	effect->SetVector("g_splitPlanesVector2", (D3DXVECTOR4*) &m_splitDistances[1]);		
	effect->SetMatrixArray("g_textureMatrices", &m_viewProjMatrices[0], UINT(m_splitsCount));
	effect->SetMatrix("g_viewMatrix", &m_view);
	for(unsigned int i = 0; i < m_splitsCount; ++i) {
		char tmp[16];
		sprintf(tmp, "g_shadowMap%u", i);
		effect->SetTexture(tmp, m_shadowMaps[i]);
	}
}

void 
ShadowsDX9::endReceivePass()
{

}

void 
ShadowsDX9::drawDebugInfo(float x, float y, float size)
{
	// 1 - 3
	// | \ |
	// 0 - 2

	if(m_device) {
		m_device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
		m_device->SetRenderState(D3DRS_FOGENABLE, FALSE);
		m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
		m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		m_device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		m_device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

		const float offset = 10.0f;

		struct {
			float x, y, z;
			float rhw;
			float tu, tv;
		} v[4];

		for(size_t i = 0; i < m_splitsCount; ++i) {
			v[0].x = x + i * (offset + size);
			v[0].y = y + size;
			v[0].z = 0.0f;
			v[0].rhw = 1.0f;
			v[0].tu = 0.0f;
			v[0].tv = 1.0f;

			v[1].x = x + i * (offset + size);
			v[1].y = y;
			v[1].z = 0.0f;
			v[1].rhw = 1.0f;
			v[1].tu = 0.0f;
			v[1].tv = 0.0f;

			v[2].x = x + size + i * (offset + size);
			v[2].y = y + size;
			v[2].z = 0.0f;
			v[2].rhw = 1.0f;
			v[2].tu = 1.0f;
			v[2].tv = 1.0f;

			v[3].x = x + size + i * (offset + size);
			v[3].y = y;
			v[3].z = 0.0f;
			v[3].rhw = 1.0f;
			v[3].tu = 1.0f;
			v[3].tv = 0.0f;

			m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
			m_device->SetTexture(0, m_shadowMaps[i]);	
			m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(v[0]));
		}
	}
}

#endif // SHADOWS_ENABLE_DX9_SUPPORT

//------------------------------------------------------------------------------
// DX10 implementation
//------------------------------------------------------------------------------

#ifdef SHADOWS_ENABLE_DX10_SUPPORT



#endif // SHADOWS_ENABLE_DX9_SUPPORT

//------------------------------------------------------------------------------
// End
//------------------------------------------------------------------------------