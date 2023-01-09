#include "pch.hpp"
#include "particle_editor.hpp"
#include "main_frame.hpp"
#include "gui/propdlgs/ps_properties.hpp"
#include "particle/meta_particle_system.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

BW_BEGIN_NAMESPACE

BEGIN_MESSAGE_MAP(PsProperties, CFormView)
    ON_MESSAGE(WM_EDITNUMERIC_CHANGE, OnUpdatePsProperties)
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(PsProperties, CFormView)

PsProperties::PsProperties(): 
	CFormView(PsProperties::IDD),
	initialised_(false),
	nameInvalidMessage_(),
	capacity_(),
	windFactor_(),
	maxLod_(),
	tooltips_()
{
	BW_GUARD;

    capacity_.SetNumericType( controls::EditNumeric::ENT_INTEGER);
    capacity_.SetMinimum(1);
    capacity_.SetMaximum(65536);

	windFactor_.SetMinimum(0);
	windFactor_.SetMaximum(1);

	maxLod_.SetAllowNegative(false);
}

PsProperties::~PsProperties()
{
}

void PsProperties::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PS_CAPACITY    , capacity_  );
    DDX_Control(pDX, IDC_PS_WINDHALFLIFE, windFactor_);
    DDX_Control(pDX, IDC_PS_MAXLOD      , maxLod_    );
}

ParticleSystemPtr PsProperties::action()
{
	BW_GUARD;

    return MainFrame::instance()->GetCurrentParticleSystem();
}

void PsProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

    if (!action())
        return;

    if (task == SET_PSA)
    {
        MainFrame::instance()->PotentiallyDirty
        (
            true,
            UndoRedoOp::AK_PARAMETER,
            LocaliseUTF8(L"PARTICLEEDITOR/GUI/PS_PROPERTIES/SET_PARAM")
        );
    }

    SET_INT_PARAMETER(task, capacity);
    SET_FLOAT_PARAMETER(task, windFactor);
    SET_FLOAT_PARAMETER(task, maxLod);
}

afx_msg LRESULT PsProperties::OnUpdatePsProperties(WPARAM mParam, LPARAM lParam)
{
	BW_GUARD;

    if (initialised_)
        SetParameters(SET_PSA);
    
    return 0;
}

void PsProperties::OnInitialUpdate()
{
	BW_GUARD;

    CFormView::OnInitialUpdate();
    SetParameters(SET_CONTROL);
    tooltips_.CreateEx(this, TTS_ALWAYSTIP, WS_EX_TOPMOST);
    tooltips_.AddTool(&capacity_  , IDS_TT_PROP_CAPACITY);
    tooltips_.AddTool(&windFactor_, IDS_TT_PROP_WINDHL  );
    tooltips_.AddTool(&maxLod_    , IDS_TT_PROP_MAXLOD  );
    tooltips_.Activate(TRUE);
    tooltips_.SetWindowPos(&CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    initialised_ = true;
    INIT_AUTO_TOOLTIP();
}

void PsProperties::OnBnClickedPsButton()
{
	BW_GUARD;

    SetParameters(SET_PSA);
}
BW_END_NAMESPACE

