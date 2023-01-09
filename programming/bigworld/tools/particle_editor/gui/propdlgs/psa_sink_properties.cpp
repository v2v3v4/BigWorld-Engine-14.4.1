#include "pch.hpp"
#include "particle_editor.hpp"
#include "main_frame.hpp"
#include "gui/gui_utilities.hpp"
#include "gui/propdlgs/psa_sink_properties.hpp"
#include "particle/actions/sink_psa.hpp"
#include "resmgr/string_provider.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

BW_BEGIN_NAMESPACE

namespace
{
    const float SliderScale = 3.3333f; // I know... magic numbers are bad.
}

IMPLEMENT_DYNCREATE(PsaSinkProperties, PsaProperties)

BEGIN_MESSAGE_MAP(PsaSinkProperties, PsaProperties)
	ON_BN_CLICKED(IDC_PSA_SINK_OUTSIDE_ONLY, &PsaSinkProperties::OnBnClickedPsaSinkOutsideOnly)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

PsaSinkProperties::PsaSinkProperties()
: 
PsaProperties(PsaSinkProperties::IDD)
{
	BW_GUARD;

    maximumAge_.SetMaximum( 30.f );
	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

PsaSinkProperties::~PsaSinkProperties()
{
}

void PsaSinkProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

    ASSERT(action_);

    SET_FLOAT_PARAMETER(task, maximumAge);
    SET_FLOAT_PARAMETER(task, minimumSpeed);
    SET_CHECK_PARAMETER(task, outsideOnly);
	SET_FLOAT_PARAMETER(task, delay);

    // set the slider with the new position
    prevSliderPos_ = max((int)(action()->maximumAge() * SliderScale), 0);
    maximumAgeSlider_.SetPos(prevSliderPos_);
}

void PsaSinkProperties::OnInitialUpdate()
{
	BW_GUARD;

    PsaProperties::OnInitialUpdate();    
}

void PsaSinkProperties::DoDataExchange(CDataExchange* pDX)
{
    PsaProperties::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PSA_SINK_MAXIMUMAGE, maximumAge_);
    DDX_Control(pDX, IDC_PSA_SINK_MINIMUMSPEED, minimumSpeed_);
    DDX_Control(pDX, IDC_PSA_SINK_MAXIMUMAGE_SLIDER, maximumAgeSlider_);
    DDX_Control(pDX, IDC_PSA_SINK_OUTSIDE_ONLY, outsideOnly_);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}


void PsaSinkProperties::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	BW_GUARD;

	if (pScrollBar == (CScrollBar*)&maximumAgeSlider_)
	{
		int pos = maximumAgeSlider_.GetPos();

		// if slider has moved, update the action and then update the CEdit control
		if (pos != prevSliderPos_)
		{
			MainFrame::instance()->PotentiallyDirty
			(
				true,
				UndoRedoOp::AK_PARAMETER,
				LocaliseUTF8(L"PARTICLEEDITOR/GUI/PSA_SINK_PROPERTIES/SET_SINK"),
				true	/*wait for the OnHScroll-TB_ENDTRACK event*/,
				false	/*don't add barrier yet*/
			);

			if (pos == 0)
				action()->maximumAge(-1.0f);
			else
				action()->maximumAge(pos / SliderScale);

			SET_FLOAT_PARAMETER(SET_CONTROL, maximumAge);
			prevSliderPos_ = pos;
		}

		if (nSBCode == TB_ENDTRACK)
		{
			MainFrame::instance()->PotentiallyDirty
			(
				true,
				UndoRedoOp::AK_PARAMETER,
				LocaliseUTF8(L"PARTICLEEDITOR/GUI/PSA_SINK_PROPERTIES/SET_SINK")
			);
		}
	}
}


void PsaSinkProperties::OnBnClickedPsaSinkOutsideOnly()
{
	BW_GUARD;

	CopyDataToPSA();
}

SinkPSA * PsaSinkProperties::action()
{
	return static_cast<SinkPSA *>(action_.get());
}

BW_END_NAMESPACE

