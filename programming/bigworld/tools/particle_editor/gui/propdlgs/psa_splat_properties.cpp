#include "pch.hpp"
#include "psa_splat_properties.hpp"
#include "particle/actions/splat_psa.hpp"

DECLARE_DEBUG_COMPONENT2("GUI", 0)

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNCREATE(PsaSplatProperties, PsaProperties)

PsaSplatProperties::PsaSplatProperties()
:
PsaProperties(IDD)
{
	BW_GUARD;

	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

/*virtual*/ void PsaSplatProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

	SET_FLOAT_PARAMETER(task, delay);
}


void PsaSplatProperties::DoDataExchange(CDataExchange* pDX)
{
	PsaProperties::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}


SplatPSA * PsaSplatProperties::action()
{
	return static_cast<SplatPSA *>(action_.get());
}

BEGIN_MESSAGE_MAP(PsaSplatProperties, PsaProperties)
END_MESSAGE_MAP()
BW_END_NAMESPACE

