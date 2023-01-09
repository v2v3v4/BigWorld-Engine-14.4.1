#ifndef SHADOWS_H
#define SHADOWS_H

#include "config.hpp"
#include "cstdmf/bw_vector.hpp"

#ifdef SHADOWS_ENABLE_DX9_SUPPORT
#include <d3d9.h>
#include <d3dx9.h>
#endif

#ifdef SHADOWS_ENABLE_DX10_SUPPORT
#include <d3d10.h>
#include <d3dx10.h>
#endif

//------------------------------------------------------------------------------
// Base class
//------------------------------------------------------------------------------

class Shadows 
{
public:
	Shadows() {};
	virtual ~Shadows() {};

	virtual bool init(void* device, int splitsCount, int shadowMapSize) = 0;
	virtual void release() = 0;

	int getSplitsCount() const { return static_cast<int>(m_splitsCount); }
	int getShadowMapSize() const { return m_shadowMapSize; }

	virtual void getEffectMacroDefinitions(BW::vector<D3DXMACRO>& outMacroses) = 0;

	virtual void update(const D3DXMATRIX& view,						
						float fov, float aspectRatio,
						float shadowsNearPlane, float shadowsFarPlane,
						const D3DXVECTOR3& lightDir, float lightDist);

	const D3DXMATRIX& getShadowViewMatrix(int splitNumber) { return m_viewMatrices[splitNumber]; }
	const D3DXMATRIX& getShadowProjMatrix(int splitNumber) { return m_projMatrices[splitNumber]; }

	virtual void beginCastPass(int splitNumber) = 0;
//	virtual void setCastEffectParams(void* shader) = 0;
	virtual void endCastPass() = 0;

	virtual void beginReceivePass() = 0;
	virtual void setReceiveEffectParams(void* effect) = 0;
	virtual void endReceivePass() = 0;

/*
	virtual void applyScreenMap() = 0;
*/

#ifdef SHADOWS_DEBUG_ENABLED
	virtual void drawDebugInfo(float x, float y, float size) = 0;
#endif

protected:
	size_t m_splitsCount;
	int m_shadowMapSize;

	float m_splitDistances[SHADOWS_MAX_SPLITS + 1];
	
	// updates every frame (update(...) call)
	D3DXMATRIX m_view;
	D3DXMATRIX m_viewMatrices[SHADOWS_MAX_SPLITS];
	D3DXMATRIX m_projMatrices[SHADOWS_MAX_SPLITS];
	D3DXMATRIX m_viewProjMatrices[SHADOWS_MAX_SPLITS];
};

//------------------------------------------------------------------------------
// DX9 implementation
//------------------------------------------------------------------------------

#ifdef SHADOWS_ENABLE_DX9_SUPPORT

class ShadowsDX9 : public Shadows
{
public:
	ShadowsDX9();
	virtual ~ShadowsDX9();

	virtual bool init(void* device, int splitsCount, int shadowMapSize);
	virtual void release();

	void OnDeviceLost();
	bool OnDeviceReset();

	virtual void getEffectMacroDefinitions(BW::vector<D3DXMACRO>& outMacroses);

	virtual void beginCastPass(int splitNumber);
	virtual void endCastPass();

	virtual void beginReceivePass();
	virtual void setReceiveEffectParams(void* effect);
	virtual void endReceivePass();

#ifdef SHADOWS_DEBUG_ENABLED
	virtual void drawDebugInfo(float x, float y, float size);
#endif

protected:
	LPDIRECT3DDEVICE9 m_device;
	LPDIRECT3DTEXTURE9 m_shadowMaps[SHADOWS_MAX_SPLITS];
	LPDIRECT3DSURFACE9 m_depthStencil;

	// for restoring old RT and depth stencil in endCastPass()
	LPDIRECT3DSURFACE9 m_oldRenderTarget;
	LPDIRECT3DSURFACE9 m_oldDepthStencil;
};

#endif // SHADOWS_ENABLE_DX9_SUPPORT

//------------------------------------------------------------------------------
// DX10 implementation
//------------------------------------------------------------------------------

#ifdef SHADOWS_ENABLE_DX10_SUPPORT

class ShadowsDX10 : public Shadows
{
public:
protected:
};

#endif // SHADOWS_ENABLE_DX9_SUPPORT

//------------------------------------------------------------------------------
// End
//------------------------------------------------------------------------------

#endif // SHADOWS_H