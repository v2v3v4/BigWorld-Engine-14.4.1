#include "pch.hpp"
#include "particle_editor.hpp"
#include "gui/propdlgs/psa_magnet_properties.hpp"
#include "particle/actions/magnet_psa.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNCREATE(PsaMagnetProperties, PsaProperties)

PsaMagnetProperties::PsaMagnetProperties()
: 
PsaProperties(PsaMagnetProperties::IDD)
{
	BW_GUARD;

	minDist_.SetMinimum(0.f, false);
	minDist_.SetAllowNegative(false);
	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

PsaMagnetProperties::~PsaMagnetProperties()
{
}

void PsaMagnetProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

	ASSERT(action_);

	SET_FLOAT_PARAMETER(task, strength);
	SET_FLOAT_PARAMETER(task, minDist);
	SET_FLOAT_PARAMETER(task, delay);
}


void PsaMagnetProperties::DoDataExchange(CDataExchange* pDX)
{
	PsaProperties::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PSA_MAGNET_STRENGTH, strength_);
	DDX_Control(pDX, IDC_PSA_MAGNET_MINDIST, minDist_);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}


MagnetPSA *	PsaMagnetProperties::action()
{
	return static_cast<MagnetPSA *>(action_.get());
}

BEGIN_MESSAGE_MAP(PsaMagnetProperties, PsaProperties)
END_MESSAGE_MAP()


// PsaMagnetProperties diagnostics

#ifdef _DEBUG
void PsaMagnetProperties::AssertValid() const
{
	PsaProperties::AssertValid();
}

void PsaMagnetProperties::Dump(CDumpContext& dc) const
{
	PsaProperties::Dump(dc);
}
#endif //_DEBUG


BW_END_NAMESPACE

// PsaMagnetProperties message handlers
