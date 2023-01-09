#include "pch.hpp"
#include "particle_editor.hpp"
#include "gui/propdlgs/psa_barrier_properties.hpp"
#include "gizmo/general_properties.hpp"
#include "gui/vector_generator_proxies.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNCREATE(PsaBarrierProperties, PsaProperties)

PsaBarrierProperties::PsaBarrierProperties()
	: PsaProperties(PsaBarrierProperties::IDD)
	, gizmo_(NULL)
	, populated_(false)
{
	BW_GUARD;

	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

PsaBarrierProperties::~PsaBarrierProperties()
{
	BW_GUARD;

	if (gizmo_)
		GizmoManager::instance().removeGizmo(gizmo_);
}

void PsaBarrierProperties::OnInitialUpdate()
{
	BW_GUARD;

    PsaProperties::OnInitialUpdate();
}

void PsaBarrierProperties::populate()
{
	BW_GUARD;

	barrierShape_.ResetContent();
	for (int i = 0; i < BarrierPSA::SHAPE_MAX; i++)
		barrierShape_.AddString(bw_utf8tow( BarrierPSA::shapeTypeNames_[i] ).c_str());

	barrierReaction_.ResetContent();
	for (int i = 0; i < BarrierPSA::REACTION_MAX; i++)
		barrierReaction_.AddString(bw_utf8tow( BarrierPSA::reactionTypeNames_[i] ).c_str());

	populated_ = true;
}

void PsaBarrierProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

	ASSERT(action_);

	if (!populated_)
		populate();

	SET_FLOAT_PARAMETER(task, delay);

	if (task == SET_CONTROL)
	{
		// the reaction
		barrierReaction_.SetCurSel(action()->reaction());

		// the shape
		if (action()->shape() != barrierShape_.GetCurSel())
		{
			barrierShape_.SetCurSel(action()->shape());

			if (action()->shape() != BarrierPSA::NONE)
				setGizmo((BarrierPSA::Shape)barrierShape_.GetCurSel());
		}
	}
	else
	{
		// change the BarrierPSA
		// the reaction
		switch (barrierReaction_.GetCurSel())
		{
		case BarrierPSA::BOUNCE:
			{
				action()->bounceParticles();
				break;
			}
		case BarrierPSA::REMOVE:
			{
				action()->removeParticles();
				break;
			}
		case BarrierPSA::ALLOW:
			{
				action()->allowParticles();
				break;
			}
		case BarrierPSA::WRAP:
			{
				action()->wrapParticles();
				break;
			}
		default:
			{
				MF_ASSERT(0);
			}
		}

		// the shape
		if (barrierShape_.GetCurSel() == -1)
			return;

		if (action()->shape() != barrierShape_.GetCurSel())
		{
			switch (barrierShape_.GetCurSel())
			{
			case BarrierPSA::VERTICAL_CYLINDER:
				{
					action()->verticalCylinder(Vector3(0.f, 0.f, 0.f), 1.f);
					break;
				}
			case BarrierPSA::BOX:
				{
					action()->box(Vector3(-.5f, -.5f, -.5f), Vector3(.5f, .5f, .5f));
					break;
				}
			case BarrierPSA::SPHERE:
				{
					action()->sphere(Vector3(0.f, 0.f, 0.f), 1.f);
					break;
				}
			default:
				{
					action()->none();
					break;
				}
			}

			setGizmo((BarrierPSA::Shape)barrierShape_.GetCurSel());
		}
	}
}


void PsaBarrierProperties::setGizmo(BarrierPSA::Shape shapeType)
{
	BW_GUARD;

	if (gizmo_)
		GizmoManager::instance().removeGizmo(gizmo_);

	if (shapeType == BarrierPSA::NONE)
		return;

	GeneralEditorPtr generalsDaughter = new GeneralEditor();

	if (shapeType == BarrierPSA::VERTICAL_CYLINDER)
	{
		// position
		VectorGeneratorMatrixProxy<BarrierPSA> * matrixProxy = new VectorGeneratorMatrixProxy<BarrierPSA>(action(), 
			&BarrierPSA::verticalCylinderPointOnAxis, 
			&BarrierPSA::verticalCylinderPointOnAxis);
		generalsDaughter->addProperty(new GenPositionProperty("position", matrixProxy));

		// radius
		FloatProxyPtr radiusProxy = new AccessorDataProxy<BarrierPSA, FloatProxy>(
									action(), 
									&BarrierPSA::verticalCylinderRadius, 
									&BarrierPSA::verticalCylinderRadius);
		generalsDaughter->addProperty(new GenRadiusProperty("radius", radiusProxy, matrixProxy));

		gizmo_ = new CylinderGizmo(matrixProxy, NULL, radiusProxy, 0xFFFF0000, false, NULL, 4.5, MODIFIER_ALT, true);
	}
	else if (shapeType == BarrierPSA::BOX)
	{
		// corner
		VectorGeneratorMatrixProxy<BarrierPSA> * matrixProxy1 = new VectorGeneratorMatrixProxy<BarrierPSA>(action(), 
			&BarrierPSA::boxCorner, 
			&BarrierPSA::boxCorner);
		generalsDaughter->addProperty(new GenPositionProperty("corner", matrixProxy1));

		// opposite corner
		VectorGeneratorMatrixProxy<BarrierPSA> * matrixProxy2 = new VectorGeneratorMatrixProxy<BarrierPSA>(action(), 
			&BarrierPSA::boxOppositeCorner, 
			&BarrierPSA::boxOppositeCorner);
		generalsDaughter->addProperty(new GenPositionProperty("opposite corner", matrixProxy2));

		gizmo_ = new BoxGizmo(matrixProxy1, matrixProxy2);
	}
	else if (shapeType == BarrierPSA::SPHERE)
	{
		// position
		VectorGeneratorMatrixProxy<BarrierPSA> * matrixProxy = new VectorGeneratorMatrixProxy<BarrierPSA>(action(), 
			&BarrierPSA::sphereCentre, 
			&BarrierPSA::sphereCentre);
		generalsDaughter->addProperty(new GenPositionProperty("position", matrixProxy));

		// radius
		FloatProxyPtr radiusProxy1 = new AccessorDataProxy<BarrierPSA, FloatProxy>(
									action(), 
									&BarrierPSA::sphereRadius, 
									&BarrierPSA::sphereRadius);
		generalsDaughter->addProperty(new GenRadiusProperty("minRadius", radiusProxy1, matrixProxy));

		// gizmo
		gizmo_ = new SphereGizmo(matrixProxy, radiusProxy1, NULL);
	}

	GeneralEditor::Editors newEditors;
	GizmoManager::instance().addGizmo(gizmo_);
	newEditors.push_back(generalsDaughter);
	GeneralEditor::currentEditors(newEditors);
}

void PsaBarrierProperties::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	PsaProperties::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PSA_BARRIER_SHAPE, barrierShape_);
	DDX_Control(pDX, IDC_PSA_BARRIER_REACTION, barrierReaction_);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}

BEGIN_MESSAGE_MAP(PsaBarrierProperties, PsaProperties)
	ON_CBN_SELCHANGE(IDC_PSA_BARRIER_SHAPE, OnCbnSelchangePsaBarrierCombo)
	ON_CBN_SELCHANGE(IDC_PSA_BARRIER_REACTION, OnCbnSelchangePsaBarrierCombo)
END_MESSAGE_MAP()


// PsaBarrierProperties diagnostics

#ifdef _DEBUG
void PsaBarrierProperties::AssertValid() const
{
	PsaProperties::AssertValid();
}

void PsaBarrierProperties::Dump(CDumpContext& dc) const
{
	PsaProperties::Dump(dc);
}
#endif //_DEBUG


// PsaBarrierProperties message handlers

void PsaBarrierProperties::OnCbnSelchangePsaBarrierCombo()
{
	BW_GUARD;

	CopyDataToPSA();
}
BW_END_NAMESPACE

