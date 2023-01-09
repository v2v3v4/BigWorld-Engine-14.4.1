#pragma once

#include "forward_declarations.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	
	//-- Provides some simple implementation of a post-processing anti-aliasing techniques.
	//-- There are a lot of this kind techniques. For example: FXAA, SRAA, NFAA, MLAA, DLAA and so on.
	//-- PPAASupport stands for Post-processing Anti-aliasing support.
	//----------------------------------------------------------------------------------------------
	class PPAASupport : public DeviceCallback
	{
	private:
		PPAASupport(const PPAASupport&);
		PPAASupport& operator = (const PPAASupport&);

	public:
		PPAASupport();
		~PPAASupport();

		void enable(bool flag);
		bool enable() const		{ return m_enabled; }

		void mode(uint index);
		uint mode() const { return m_mode; }
		
		//-- does anti-aliasing filter.
		void apply();

		//-- interface DeviceCallback.
		virtual void deleteUnmanagedObjects();
		virtual void createUnmanagedObjects();
		virtual void deleteManagedObjects();
		virtual void createManagedObjects();
		virtual bool recreateForD3DExDevice() const;

	private:
		bool				m_inited;
		bool				m_enabled;
		uint				m_mode;
		RenderTargetPtr	  	m_rt;		    //-- back-buffer copy.
		EffectMaterialPtr	m_materials[3]; //-- LQ, MQ, HQ
	};

} //-- Moo

BW_END_NAMESPACE
