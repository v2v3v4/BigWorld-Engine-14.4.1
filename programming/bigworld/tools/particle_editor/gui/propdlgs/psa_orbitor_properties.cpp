#include "pch.hpp"
#include "particle_editor.hpp"
#include "gui/vector_generator_proxies.hpp"
#include "gui/propdlgs/psa_orbitor_properties.hpp"
#include "particle/actions/orbitor_psa.hpp"
#include "gizmo/general_editor.hpp"
#include "gizmo/combination_gizmos.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNCREATE(PsaOrbitorProperties, PsaProperties)

PsaOrbitorProperties::PsaOrbitorProperties()
: 
PsaProperties(PsaOrbitorProperties::IDD),
positionGizmo_(NULL),
positionMatrixProxy_(NULL)
{
	BW_GUARD;

	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

PsaOrbitorProperties::~PsaOrbitorProperties()
{
	BW_GUARD;

	removePositionGizmo();
}

void PsaOrbitorProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

	ASSERT(action_);

	SET_FLOAT_PARAMETER(task, delay);

	if (task == SET_CONTROL)
    {
        x_.SetValue(action()->point().x);
        z_.SetValue(action()->point().z);
    }
    else if (task == SET_PSA)
    {
        Vector3 newVector( x_.GetValue(), 0.f, z_.GetValue() );
        action()->point(newVector);
    }

	SET_FLOAT_PARAMETER(task, angularVelocity);
	SET_CHECK_PARAMETER(task, affectVelocity);

	// tell the gizmo of the new position
	addPositionGizmo();
	Matrix mat;
	mat.setTranslate(position());
	positionMatrixProxy_->setMatrixAlone(mat);
}

void PsaOrbitorProperties::position(const Vector3 & position)
{
	BW_GUARD;

	// set the values into the edit and notify the psa
	x_.SetValue(position.x);
	z_.SetValue(position.z);

    // notify the psa
	SetParameters(SET_PSA);

    // note that this function is ultimately called by a gizmo change, and so
    // the undo/redo history is set up elsewhere (in PeModule).
}

Vector3 PsaOrbitorProperties::position() const
{
	BW_GUARD;

	// should get this from the psa directly, but there are const issues
	return Vector3(x_.GetValue(), 0.f, z_.GetValue());
}

void PsaOrbitorProperties::addPositionGizmo()
{
	BW_GUARD;

	if (positionGizmo_)
		return;	// already been created

	GeneralEditorPtr generalsDaughter = new GeneralEditor();
	positionMatrixProxy_ = new VectorGeneratorMatrixProxy<PsaOrbitorProperties>(this, 
		&PsaOrbitorProperties::position, 
		&PsaOrbitorProperties::position);
	generalsDaughter->addProperty(new GenPositionProperty(Name( "vector" ), positionMatrixProxy_));
	positionGizmo_ = new PositionGizmo(MODIFIER_ALT, positionMatrixProxy_, NULL, 0.1f, true);
	GizmoManager::instance().addGizmo(positionGizmo_);
	GeneralEditor::Editors newEditors;
	newEditors.push_back(generalsDaughter);
	GeneralEditor::currentEditors(newEditors);
}

void PsaOrbitorProperties::removePositionGizmo()
{
	BW_GUARD;

	GizmoManager::instance().removeGizmo(positionGizmo_);
	positionGizmo_ = NULL;
}


void PsaOrbitorProperties::DoDataExchange(CDataExchange* pDX)
{
	PsaProperties::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PSA_ORBITOR_POINTX, x_);
	DDX_Control(pDX, IDC_PSA_ORBITOR_POINTZ, z_);
	DDX_Control(pDX, IDC_PSA_ORBITOR_ANGULARVELOCITY, angularVelocity_);
	DDX_Control(pDX, IDC_PSA_ORBITOR_AFFECTVELOCITY, affectVelocity_);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}

BEGIN_MESSAGE_MAP(PsaOrbitorProperties, PsaProperties)
	ON_BN_CLICKED(IDC_PSA_ORBITOR_AFFECTVELOCITY, OnBnClickedPsaOrbitorButton)
END_MESSAGE_MAP()


// PsaOrbitorProperties diagnostics

#ifdef _DEBUG
void PsaOrbitorProperties::AssertValid() const
{
	PsaProperties::AssertValid();
}

void PsaOrbitorProperties::Dump(CDumpContext& dc) const
{
	PsaProperties::Dump(dc);
}
#endif //_DEBUG


// PsaOrbitorProperties message handlers

void PsaOrbitorProperties::OnBnClickedPsaOrbitorButton()
{
	BW_GUARD;

	CopyDataToPSA();
}

OrbitorPSA * PsaOrbitorProperties::action()
{
	return static_cast<OrbitorPSA *>(action_.get());
}

BW_END_NAMESPACE

