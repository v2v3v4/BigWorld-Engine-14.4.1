#pragma once

#include <memory>

BW_BEGIN_NAMESPACE

namespace Moo
{
	class FSAASupport;
	class PPAASupport;
	
	//-- This class is a manager of the all existing custom anti-aliasing techniques.
	//-- Some custom AA techniques may be combined together to get better final result.
	//-- So it enables or disables desired modes based on the incoming mode ID. It doesn't try to
	//-- provide some basic interface to all existing AA techniques, because they all have very
	//-- different approaches, some techniques much more invasive than another, so their interfaces
	//-- may be very different. Instead it provides straight-forward access to them all.
	//----------------------------------------------------------------------------------------------
	class CustomAA
	{
	private:
		CustomAA(const CustomAA&);
		CustomAA& operator = (const CustomAA&);

	public:
		CustomAA();
		~CustomAA();
		
		//-- returns number of available modes.
		uint modesCount() const { return m_modesCount; }
		
		//-- set/get current mode.
		void mode(uint id);
		uint mode() const	{ return m_mode; }
		
		//-- apply mode.
		void apply();
		
		PPAASupport& ppaa()	{ return *m_ppaa.get(); }

	private:
		std::auto_ptr<PPAASupport>	m_ppaa;
		uint						m_mode;
		uint						m_modesCount;
	};

} //-- Moo

BW_END_NAMESPACE
