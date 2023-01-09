#pragma once

#include "moo/forward_declarations.hpp"
#include "moo/device_callback.hpp"
#include "cstdmf/singleton.hpp"
#include "math/vector2.hpp"

BW_BEGIN_NAMESPACE

//-- forward declaration.
struct HDRSettings;

//-- Provides support of HDR Rendering and HDR blooming.
//-- Notice: AA is performed after tone mapping.
//--------------------------------------------------------------------------------------------------
class HDRSupport : public Moo::DeviceCallback
{
private:
	//-- make non-copyable.
	HDRSupport(const HDRSupport&);
	HDRSupport& operator = (const HDRSupport&);

public:
	HDRSupport();
	~HDRSupport();

	//-- set settings for HDR or NULL if we want default settings to be applied.
	void					settings(const HDRSettings* cfg);
	const HDRSettings&		settings() const;

	//-- set HDR rendering quality.
	void					setQualityOption(int option);
	
	//-- enabled/disable HDR Rendering.
	bool					enable() const;
	void					enable(bool flag);

	//-- do smooth changing of average luminance.
	void					tick(float dt);
	
	//-- set HDR rt to as a current render target and intercept all the drawing operation for
	//-- the back buffer.
	void					intercept();

	//-- resolve HDR render target to the back buffer with the custom filter shader.
	void					resolve();
	
	//-- interface DeviceCallback.
	virtual void			deleteUnmanagedObjects();
	virtual void			createUnmanagedObjects();
	virtual void			deleteManagedObjects();
	virtual void			createManagedObjects();
	virtual bool			recreateForD3DExDevice() const;

private:
	void					calcAvgLum();
	void					calcBloom();
	float					readAvgLum();
	void					calcAvgLumOnGPU();
	void					doTonemapping();

private:
	typedef std::pair<Moo::RenderTargetPtr, Moo::RenderTargetPtr> BloomMapPair;

	bool			  					m_successed;
	// Only valid when all buffers we require have been created
	bool								m_valid; 
	Moo::RenderTargetPtr	  			m_hdrRT;		  //-- HDR render target.
	Moo::EffectMaterialPtr				m_toneMappingMat;
	Moo::EffectMaterialPtr				m_averageLuminanceMat;
	Moo::EffectMaterialPtr				m_bloomMat;
	Moo::EffectMaterialPtr				m_gaussianBlurMat;

	BW::vector<DX::Viewport>			m_viewPorts;

	bool								m_update;
	float								m_curTime;
	Vector2								m_avgLumValues;
	BW::vector<BloomMapPair>			m_bloomMaps;
	BW::vector<Moo::RenderTargetPtr>	m_avgLumRTs;
	Moo::RenderTargetPtr				m_avgLumRT;
	Moo::RenderTargetPtr				m_finalAvgLumRT;
	ComObjectWrap<DX::Texture>			m_inmemTexture;
};

BW_END_NAMESPACE
