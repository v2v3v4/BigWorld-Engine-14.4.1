#pragma once

#include "forward_declarations.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/custom_mesh.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{	
	
	//-- Represents full-screen quad, which is commonly used for post-processing's tasks.
	//-- It was automatically adjusted by a half pixel offset for the current screen resolution.
	//----------------------------------------------------------------------------------------------
	class FullscreenQuad : public DeviceCallback
	{
	private:
		FullscreenQuad(const FullscreenQuad&);
		FullscreenQuad& operator = (const FullscreenQuad&);

	public:
		FullscreenQuad();
		~FullscreenQuad();
		
		//-- draw full screen quad with desired material.
		void draw(const EffectMaterial& material);
		void draw(const EffectMaterial& material, uint width, uint height);

		//-- clear
		void clearChannel( const Vector4 & color, uint32 colorWriteEnable );
		void clearChannel( const Vector4 & color, uint32 colorWriteEnable, uint width, uint height );

		//-- interface DeviceCallback.
		virtual void deleteUnmanagedObjects();
		virtual void createUnmanagedObjects();
		virtual void deleteManagedObjects();
		virtual void createManagedObjects();
		virtual bool recreateForD3DExDevice() const;

	private:
		Moo::EffectMaterialPtr			m_clearFM;
		Moo::VertexDeclaration*			m_vecDclr;
		StaticMesh						m_FSQuadMesh;
	};

} //-- Moo

BW_END_NAMESPACE
