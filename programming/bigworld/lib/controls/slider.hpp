#pragma once

#include "controls/defs.hpp"
#include "controls/fwd.hpp"

BW_BEGIN_NAMESPACE

/////////////////////////////////////////////////////////////////////////////
// Slider

namespace controls
{
	class CONTROLS_DLL Slider : public CSliderCtrl
	{
	
	public:
		Slider();
		virtual ~Slider();
	
		bool doUpdate()
		{
			bool temp = released_;
			released_ = false;
			return temp;
		}
	
	private:
		bool lButtonDown_;
		bool released_;
	
	protected:
		DECLARE_MESSAGE_MAP()
	public:
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	};
}

BW_END_NAMESPACE

