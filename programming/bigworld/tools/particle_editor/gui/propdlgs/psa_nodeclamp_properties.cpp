#include "pch.hpp"
#include "psa_nodeclamp_properties.hpp"
#include "particle/actions/node_clamp_psa.hpp"

DECLARE_DEBUG_COMPONENT2("GUI", 0)

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNCREATE(PsaNodeClampProperties, PsaProperties)

PsaNodeClampProperties::PsaNodeClampProperties()
:
PsaProperties(IDD)
{
	BW_GUARD;

	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

/*virtual*/ void PsaNodeClampProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

	SET_FLOAT_PARAMETER(task, delay);
}


void PsaNodeClampProperties::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	PsaProperties::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}


NodeClampPSA * PsaNodeClampProperties::action()
{
	return static_cast<NodeClampPSA *>(action_.get());
}

BEGIN_MESSAGE_MAP(PsaNodeClampProperties, PsaProperties)
END_MESSAGE_MAP()
BW_END_NAMESPACE

