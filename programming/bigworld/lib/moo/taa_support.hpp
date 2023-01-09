#pragma once

#include "forward_declarations.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	//-- forward declaration.
	class Camera;

	//-- The central point in the Temporal Anti Aliasing technique.
	//-- It provides backuped color and depth buffer from the previous frame, which may be reused
	//-- in the shader to provide it additional sub-pixel (or per sample) information about the
	//-- pixel currently in progress.
	//-- Every frame the camera's projection matrix jitters by specific sub-pixel pattern to provide
	//-- additional info about pixel much so like as MSAA does that. Then every frame we backup color
	//-- and depth buffer (in our case we backup the second MRT target). At the next frame we reuse
	//-- this buffer by accessing it from the pixel shader to fetch the next sample color.
	//-- To eliminate situations when the camera or some objects move very fast we use cache miss
	//-- philosophy. That means that before reuse the sample color we check depth changes of the
	//-- current pixel and the fetched. And reuse it only if changes don't exceed some predefined
	//-- lambda value.
	//-- The temporal AA (TAA) uses assumption that the aliasing is more noticeable when the camera
	//-- moves slowly or just doesn't move at all, it this case TAA can greatly eliminate aliasing
	//-- over time.
	//----------------------------------------------------------------------------------------------
	class TemporalAASupport : public DeviceCallback
	{
	private:
		//-- make non-copyable.
		TemporalAASupport(const TemporalAASupport&);
		TemporalAASupport& operator = (const TemporalAASupport&);

	public:
		TemporalAASupport();
		~TemporalAASupport();
		
		bool enable() const;
		void enable(bool flag);
		
		//-- Go to the next frame.
		void nextFrame();

		//-- returns shaked camera projection matrix for desired sub-pixel pattern.
		Matrix jitteredProjMatrix(const Moo::Camera& cam);

		//-- Do full screen post-processing pass and apply TAA to the final image.
		//-- Warning: Do that before any final back buffer image processing.
		void resolve();

		//-- interface DeviceCallback.
		virtual void deleteUnmanagedObjects();
		virtual void createUnmanagedObjects();
		virtual void deleteManagedObjects();
		virtual void createManagedObjects();
		
	private:
		enum { NUM_TEMPORAL_RTS = 4 };

		//-- Describes each individual sample of some anti-aliasing pattern.
		struct SampleDesc
		{
			SampleDesc() : m_number(0), m_weight(0), m_offset(0,0) { }
			SampleDesc(uint number, float weight, const Vector2& offset)
				: m_number(number), m_weight(weight), m_offset(offset) { }

			uint    m_number;
			float   m_weight;
			Vector2 m_offset;
		};

		bool				m_isEnabled;
		bool				m_samplePatternChanged;
		uint				m_sampleCounter;
		Vector2				m_lastJitterOffset;
		SampleDesc			m_curSample;
		Vector2				m_screenRes;
		RenderTargetPtr		m_colorRTCopies[NUM_TEMPORAL_RTS];
		RenderTargetPtr 	m_depthRTCopy;
		EffectMaterialPtr	m_resolveDepthMaterial;
		EffectMaterialPtr	m_resolveTAAx2Material;
		EffectMaterialPtr	m_resolveTAAx4Material;
	};

} //-- Moo

BW_END_NAMESPACE
