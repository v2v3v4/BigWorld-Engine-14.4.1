#pragma once

#include "cstdmf/singleton.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/vectornodest.hpp"
#include "moo/forward_declarations.hpp"
#include "moo/device_callback.hpp"
#include "moo/moo_math.hpp"
#include "moo/base_texture.hpp"
#include "moo/vertex_buffer.hpp"
#include "moo/light_container.hpp"

BW_BEGIN_NAMESPACE

//-- Manages rendering process of lights in the deferred shading pipeline. This manager doesn't
//-- perform lights visibility culling that task is for ChunkManager. It just gather all visible
//-- lights from ChunkManager sorts them and draw.
//--------------------------------------------------------------------------------------------------
class LightsManager : public Moo::DeviceCallback, public Singleton<LightsManager>
{
private:
	LightsManager(LightsManager&);
	LightsManager& operator = (const LightsManager&);

public:
	LightsManager();
	~LightsManager();

	bool			init();
	void			tick(float dt);
	void			draw();

	//-- gather visible lights.
	void			gatherVisibleLights(const Moo::LightContainerPtr& lights);

	//-- interface DeviceCallback.
	virtual bool 	recreateForD3DExDevice() const;
	virtual void	createUnmanagedObjects();
	virtual void	deleteUnmanagedObjects();
	virtual void	createManagedObjects();
	virtual void	deleteManagedObjects();

private:
	void			prepareLights();
	void			sortLights();
	void			drawSunLight();
	void			drawOmnis();
	void			drawSpots();

private:
	typedef VectorNoDestructor<Moo::OmniLight::GPU> GPUOmniLights;
	typedef VectorNoDestructor<Moo::SpotLight::GPU>	GPUSpotLights;

	Moo::LightContainer		m_lights;
	GPUOmniLights			m_gpuOmnisInstanced;
	GPUOmniLights			m_gpuOmnisFallbacks;
	GPUSpotLights			m_gpuSpotsInstanced;
	GPUSpotLights			m_gpuSpotsFallbacks;

	//-- lighting pass shaders set.
	Moo::EffectMaterialPtr	m_material;

	//-- helper meshes needed for optimization lighting pass.
	Moo::VisualPtr			m_sphere;
	Moo::VisualPtr			m_cone;

	Moo::VertexBuffer		m_instancedVB;
};

BW_END_NAMESPACE
