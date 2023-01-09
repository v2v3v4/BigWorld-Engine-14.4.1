#include "pch.hpp"
#include "particle_editor.hpp"
#include "gui/propdlgs/psa_force_properties.hpp"
#include "gui/vector_generator_proxies.hpp"
#include "particle/actions/force_psa.hpp"
#include "gizmo/general_editor.hpp"
#include "gizmo/combination_gizmos.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNCREATE(PsaForceProperties, PsaProperties)

PsaForceProperties::PsaForceProperties()
: 
PsaProperties(PsaForceProperties::IDD),
positionGizmo_(NULL),
positionMatrixProxy_(NULL)
{
	BW_GUARD;

	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

PsaForceProperties::~PsaForceProperties()
{
	BW_GUARD;

	removePositionGizmo();
}

void PsaForceProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

	ASSERT(action_);

	SET_VECTOR3_PARAMETER(task, vector, x, y, z);
	SET_FLOAT_PARAMETER(task, delay);

	// tell the gizmo of the new position
	addPositionGizmo();
	Matrix mat;
	mat.setTranslate(position());
	positionMatrixProxy_->setMatrixAlone(mat);
}

void PsaForceProperties::position(const Vector3 & position)
{
	BW_GUARD;

	// set the values into the edit
	x_.SetValue(position.x);
	y_.SetValue(position.y);
	z_.SetValue(position.z);

    // notify the psa
	SetParameters(SET_PSA);

    // note that this function is ultimately called by a gizmo change, and so
    // the undo/redo history is set up elsewhere (in PeModule).
}

Vector3 PsaForceProperties::position() const
{
	BW_GUARD;

	// should get this from the psa directly, but there are const issues
	return Vector3(x_.GetValue(), y_.GetValue(), z_.GetValue());
}

void PsaForceProperties::addPositionGizmo()
{
	BW_GUARD;

	if (positionGizmo_)
		return;	// already been created

	GeneralEditorPtr generalsDaughter = new GeneralEditor();
	positionMatrixProxy_ = new VectorGeneratorMatrixProxy<PsaForceProperties>(this, 
		&PsaForceProperties::position, 
		&PsaForceProperties::position);
	generalsDaughter->addProperty(new GenPositionProperty(Name( "vector" ), positionMatrixProxy_));
	positionGizmo_ = new VectorGizmo(MODIFIER_ALT, positionMatrixProxy_, 0xFFFFFF00, 0.015f, NULL, 0.1f);
	GizmoManager::instance().addGizmo(positionGizmo_);
	GeneralEditor::Editors newEditors;
	newEditors.push_back(generalsDaughter);
	GeneralEditor::currentEditors(newEditors);
}

void PsaForceProperties::removePositionGizmo()
{
	BW_GUARD;

	GizmoManager::instance().removeGizmo(positionGizmo_);
	positionGizmo_ = NULL;
}

void PsaForceProperties::DoDataExchange(CDataExchange* pDX)
{
	PsaProperties::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PSA_FORCE_X, x_);
	DDX_Control(pDX, IDC_PSA_FORCE_Y, y_);
	DDX_Control(pDX, IDC_PSA_FORCE_Z, z_);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}


ForcePSA * PsaForceProperties::action()
{
	return static_cast<ForcePSA *>(action_.get());
}

BEGIN_MESSAGE_MAP(PsaForceProperties, PsaProperties)
END_MESSAGE_MAP()


// PsaForceProperties diagnostics

#ifdef _DEBUG
void PsaForceProperties::AssertValid() const
{
	PsaProperties::AssertValid();
}

void PsaForceProperties::Dump(CDumpContext& dc) const
{
	PsaProperties::Dump(dc);
}
#endif //_DEBUG


BW_END_NAMESPACE

// PsaForceProperties message handlers
