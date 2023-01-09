#include "pch.hpp"
#include "particle_editor.hpp"
#include "particle/actions/jitter_psa.hpp"
#include "particle/actions/vector_generator.hpp"
#include "gui/propdlgs/psa_jitter_properties.hpp"
#include "gui/vector_generator_proxies.hpp"
#include "gizmo/general_properties.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNCREATE(PsaJitterProperties, PsaProperties)

PsaJitterProperties::PsaJitterProperties()
: 
PsaProperties(PsaJitterProperties::IDD),
filterUpdatePSA_(false)
{
	BW_GUARD;

	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

PsaJitterProperties::~PsaJitterProperties()
{
}

void PsaJitterProperties::OnInitialUpdate()
{
	BW_GUARD;

    // Add position and vector generators if they do not exist.  This should
    // only occur with hand-coded or programmatic jitter actions:
    JitterPSA *jitterAction = action();
    if (jitterAction->getPositionSource() == NULL)
    {
        jitterAction->setPositionSource(new PointVectorGenerator());
    }
    if (jitterAction->getVelocitySource() == NULL)
    {
        jitterAction->setVelocitySource
        (
            new PointVectorGenerator(Vector3(0.0f, 1.0f, 0.0f))
        );
    }

	positionGeneratorCustodian_.setComboBox(& positionGenerator_);
	positionGeneratorCustodian_.setVectorGeneratorSetFn(&JitterPSA::setPositionSource);
	positionGeneratorCustodian_.setVectorGeneratorGetFn(&JitterPSA::getPositionSource);
	positionGeneratorCustodian_.setVectorGeneratorOwner(action());
	positionGeneratorCustodian_.setGizmoProperties(0xFFFF0000);

	velocityGeneratorCustodian_.setComboBox(& velocityGenerator_);
	velocityGeneratorCustodian_.setVectorGeneratorSetFn(&JitterPSA::setVelocitySource);
	velocityGeneratorCustodian_.setVectorGeneratorGetFn(&JitterPSA::getVelocitySource);
	velocityGeneratorCustodian_.setVectorGeneratorOwner(action());
	velocityGeneratorCustodian_.setGizmoProperties(0xFF00FF00, true, true);
    velocityGeneratorCustodian_.scale(0.1f); // make less sensitive

	// initialise dependencies
	positionGeneratorCustodian_.setParentVectorGeneratorCustodian(NULL);
	positionGeneratorCustodian_.setChildVectorGeneratorCustodian(NULL);
	velocityGeneratorCustodian_.setParentVectorGeneratorCustodian(NULL);
	velocityGeneratorCustodian_.setChildVectorGeneratorCustodian(NULL);

    PsaProperties::OnInitialUpdate();

	// set initial button state
    filterUpdatePSA_ = true;
	OnBnClickedPsaJitterAffectPosition();
	OnBnClickedPsaJitterAffectVelocity();
    filterUpdatePSA_ = false;
}

void PsaJitterProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

	ASSERT(action_);

	SET_CHECK_PARAMETER(task, affectPosition);
	SET_CHECK_PARAMETER(task, affectVelocity);
	SET_FLOAT_PARAMETER(task, delay);

	// generators
	if (task == SET_CONTROL)
	{
		positionGeneratorCustodian_.updateControl();
		velocityGeneratorCustodian_.updateControl();
	}
	else
	{
		positionGeneratorCustodian_.updateOwner();
		velocityGeneratorCustodian_.updateOwner();
	}
}


void PsaJitterProperties::DoDataExchange(CDataExchange* pDX)
{
	PsaProperties::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PSA_JITTER_AFFECT_POSITION, affectPosition_);
	DDX_Control(pDX, IDC_PSA_JITTER_AFFECT_VELOCITY, affectVelocity_);
	DDX_Control(pDX, IDC_PSA_JITTER_POSITION_GENERATOR, positionGenerator_);
	DDX_Control(pDX, IDC_PSA_JITTER_VELOCITY_GENERATOR, velocityGenerator_);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}

BEGIN_MESSAGE_MAP(PsaJitterProperties, PsaProperties)
	ON_BN_CLICKED(IDC_PSA_JITTER_AFFECT_POSITION, OnBnClickedPsaJitterAffectPosition)
	ON_BN_CLICKED(IDC_PSA_JITTER_AFFECT_VELOCITY, OnBnClickedPsaJitterAffectVelocity)
	ON_CBN_SELCHANGE(IDC_PSA_JITTER_POSITION_GENERATOR, OnBnClickedPsaJitterButton)
	ON_CBN_SELCHANGE(IDC_PSA_JITTER_VELOCITY_GENERATOR, OnBnClickedPsaJitterButton)
END_MESSAGE_MAP()


// PsaJitterProperties diagnostics

#ifdef _DEBUG
void PsaJitterProperties::AssertValid() const
{
	PsaProperties::AssertValid();
}

void PsaJitterProperties::Dump(CDumpContext& dc) const
{
	PsaProperties::Dump(dc);
}
#endif //_DEBUG


// PsaJitterProperties message handlers

void PsaJitterProperties::OnBnClickedPsaJitterAffectPosition()
{
	BW_GUARD;

    if (!filterUpdatePSA_)
	    CopyDataToPSA();

	if (affectPosition_.GetState() & 0x0003)
	{
		// enabled
		positionGenerator_.EnableWindow(true);
		positionGeneratorCustodian_.doDraw(true);
	}
	else
	{
		// disabled
		positionGenerator_.EnableWindow(false);
		positionGeneratorCustodian_.doDraw(false);
	}
}

void PsaJitterProperties::OnBnClickedPsaJitterAffectVelocity()
{
	BW_GUARD;

    if (!filterUpdatePSA_)
	    CopyDataToPSA();

	if (affectVelocity_.GetState() & 0x0003)
	{
		// enabled
		velocityGenerator_.EnableWindow(true);
		velocityGeneratorCustodian_.doDraw(true);
	}
	else
	{
		// disabled
		velocityGenerator_.EnableWindow(false);
		velocityGeneratorCustodian_.doDraw(false);
	}
}

void PsaJitterProperties::OnBnClickedPsaJitterButton()
{
	BW_GUARD;

    if (!filterUpdatePSA_)
	    CopyDataToPSA();
}

JitterPSA *	PsaJitterProperties::action()
{
	return static_cast<JitterPSA *>(action_.get());
}

BW_END_NAMESPACE

