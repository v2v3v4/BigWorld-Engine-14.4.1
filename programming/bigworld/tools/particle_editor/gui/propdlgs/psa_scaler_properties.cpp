#include "pch.hpp"
#include "particle_editor.hpp"
#include "gui/propdlgs/psa_scaler_properties.hpp"
#include "particle/actions/scaler_psa.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

BW_BEGIN_NAMESPACE

// PsaScalerProperties

IMPLEMENT_DYNCREATE(PsaScalerProperties, PsaProperties)

PsaScalerProperties::PsaScalerProperties()
	: PsaProperties(PsaScalerProperties::IDD)
{
	BW_GUARD;

	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

PsaScalerProperties::~PsaScalerProperties()
{
}

void PsaScalerProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

	ASSERT(action_);

	SET_FLOAT_PARAMETER(task, size);
	SET_FLOAT_PARAMETER(task, rate);
	SET_FLOAT_PARAMETER(task, delay);

	size_.SetAllowNegative( false );
	rate_.SetAllowNegative( false );
}

void PsaScalerProperties::DoDataExchange(CDataExchange* pDX)
{
	PsaProperties::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PSA_SCALER_SIZE, size_);
	DDX_Control(pDX, IDC_PSA_SCALER_RATE, rate_);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}


ScalerPSA *	PsaScalerProperties::action()
{
	return static_cast<ScalerPSA *>(action_.get());
}

BEGIN_MESSAGE_MAP(PsaScalerProperties, PsaProperties)
END_MESSAGE_MAP()


// PsaScalerProperties diagnostics

#ifdef _DEBUG
void PsaScalerProperties::AssertValid() const
{
	PsaProperties::AssertValid();
}

void PsaScalerProperties::Dump(CDumpContext& dc) const
{
	PsaProperties::Dump(dc);
}
#endif //_DEBUG


BW_END_NAMESPACE

// PsaScalerProperties message handlers
