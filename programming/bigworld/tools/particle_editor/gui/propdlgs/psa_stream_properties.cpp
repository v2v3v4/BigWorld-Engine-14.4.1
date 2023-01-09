#include "pch.hpp"
#include "particle_editor.hpp"
#include "gui/propdlgs/psa_stream_properties.hpp"
#include "particle/actions/stream_psa.hpp"
#include "gizmo/general_editor.hpp"
#include "gizmo/combination_gizmos.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNCREATE(PsaStreamProperties, PsaProperties)

PsaStreamProperties::PsaStreamProperties()
: 
PsaProperties(PsaStreamProperties::IDD),
positionGizmo_(NULL),
positionMatrixProxy_(NULL)
{
	BW_GUARD;

	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

PsaStreamProperties::~PsaStreamProperties()
{
	BW_GUARD;

	removePositionGizmo();
}

void PsaStreamProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

	ASSERT(action_);

	SET_VECTOR3_PARAMETER(task, vector, x, y, z);
	SET_FLOAT_PARAMETER(task, halfLife);
	SET_FLOAT_PARAMETER(task, delay);

	// tell the gizmo of the new position
	addPositionGizmo();
	Matrix mat;
	mat.setTranslate(position());
	positionMatrixProxy_->setMatrixAlone(mat);
}

void PsaStreamProperties::position(const Vector3 & position)
{
	BW_GUARD;

	// set the values into the edit and notify the psa
	x_.SetValue(position.x);
	y_.SetValue(position.y);
	z_.SetValue(position.z);

    // notify the psa
	SetParameters(SET_PSA);

    // note that this function is ultimately called by a gizmo change, and so
    // the undo/redo history is set up elsewhere (in PeModule).
}

Vector3 PsaStreamProperties::position() const
{
	// should get this from the psa directly, but there are const issues
	return Vector3(x_.GetValue(), y_.GetValue(), z_.GetValue());
}

void PsaStreamProperties::addPositionGizmo()
{
	BW_GUARD;

	if (positionGizmo_)
		return;	// already been created

	GeneralEditorPtr generalsDaughter = new GeneralEditor();
	positionMatrixProxy_ = new VectorGeneratorMatrixProxy<PsaStreamProperties>(this, 
		&PsaStreamProperties::position, 
		&PsaStreamProperties::position);
	generalsDaughter->addProperty(new GenPositionProperty(Name( "vector" ), positionMatrixProxy_));
	positionGizmo_ = new VectorGizmo(MODIFIER_ALT, positionMatrixProxy_, 0xFFFFFF00, 0.015f, NULL, 0.1f);
	GizmoManager::instance().addGizmo(positionGizmo_);
	GeneralEditor::Editors newEditors;
	newEditors.push_back(generalsDaughter);
	GeneralEditor::currentEditors(newEditors);
}

void PsaStreamProperties::removePositionGizmo()
{
	BW_GUARD;

	GizmoManager::instance().removeGizmo(positionGizmo_);
	positionGizmo_ = NULL;
}


void PsaStreamProperties::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PSA_STREAM_HALFLIFE, halfLife_);
	DDX_Control(pDX, IDC_PSA_STREAM_VECTORZ, z_);
	DDX_Control(pDX, IDC_PSA_STREAM_VECTORY, y_);
	DDX_Control(pDX, IDC_PSA_STREAM_VECTORX, x_);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}


StreamPSA *	PsaStreamProperties::action()
{
	return static_cast<StreamPSA *>(action_.get());
}

BEGIN_MESSAGE_MAP(PsaStreamProperties, PsaProperties)
END_MESSAGE_MAP()


// PsaStreamProperties diagnostics

#ifdef _DEBUG
void PsaStreamProperties::AssertValid() const
{
	CFormView::AssertValid();
}

void PsaStreamProperties::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG


BW_END_NAMESPACE
// PsaStreamProperties message handlers
