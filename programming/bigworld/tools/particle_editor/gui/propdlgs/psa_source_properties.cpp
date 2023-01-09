// psa_source_properties.cpp : implementation file
//

#include "pch.hpp"
#include "particle_editor.hpp"
#include "main_frame.hpp"
#include "gui/gui_utilities.hpp"
#include "gui/propdlgs/psa_source_properties.hpp"
#include "particle/meta_particle_system.hpp"
#include "particle/actions/source_psa.hpp"
#include "particle/actions/vector_generator.hpp"
#include "particle/renderers/particle_system_renderer.hpp"
#include "particle/renderers/mesh_particle_renderer.hpp"
#include "particle/renderers/sprite_particle_renderer.hpp"
#include "particle/renderers/point_sprite_particle_renderer.hpp"
#include "shell/pe_module.hpp"
#include "gui/vector_generator_proxies.hpp"
#include "gizmo/general_properties.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

BW_BEGIN_NAMESPACE

bool PsaSourceProperties::s_showPos_ = true;
bool PsaSourceProperties::s_showVel_ = true;

// PsaSourceProperties

IMPLEMENT_DYNCREATE(PsaSourceProperties, PsaProperties)

PsaSourceProperties::PsaSourceProperties()
:
PsaProperties(PsaSourceProperties::IDD)
{
	BW_GUARD;

    forcedUnitSize_.SetNumericType( controls::EditNumeric::ENT_INTEGER);
    minimumSize_.SetMinimum( 0.f, false );
    minimumSize_.SetMaximum( 100.f );
    maximumSize_.SetMinimum( 0.f, false );
    maximumSize_.SetMaximum( 100.f );
    randomSpinMinRPS_.SetMinimum( 0.f );
    randomSpinMinRPS_.SetMaximum( 20.f / MATH_PI );
    randomSpinMaxRPS_.SetMinimum( 0.f );
    randomSpinMaxRPS_.SetMaximum( 20.f / MATH_PI );
    initialOrientRandomiseX_.SetMinimum( 0.f );
    initialOrientRandomiseX_.SetMaximum( 180.f);
    initialOrientRandomiseY_.SetMinimum( 0.f );
    initialOrientRandomiseY_.SetMaximum( 180.f );
    spriteInitialOrient_.SetMinimum( 0.f );
    spriteInitialOrient_.SetMaximum( 360.f );
    spriteSpin_.SetMinimum( -2.f );
    spriteSpin_.SetMaximum( 2.f );
    spriteInitialOrientRand_.SetMinimum( 0.f );
    spriteInitialOrientRand_.SetMaximum( 180.f );
    spriteSpinRand_.SetMinimum( 0.f );
    spriteSpinRand_.SetMaximum( 2.f );
    inheritVelocity_.SetMinimum(0.0f);
    inheritVelocity_.SetMaximum(1.0f);
    maxSpeed_.SetMinimum(0.0f);
	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

PsaSourceProperties::~PsaSourceProperties()
{
}


void PsaSourceProperties::OnInitialUpdate()
{
	BW_GUARD;

    positionGeneratorCustodian_.setComboBox(& positionGenerator_);
    positionGeneratorCustodian_.setVectorGeneratorSetFn(&SourcePSA::setPositionSource);
    positionGeneratorCustodian_.setVectorGeneratorGetFn(&SourcePSA::getPositionSource);
    positionGeneratorCustodian_.setVectorGeneratorOwner(action());
    positionGeneratorCustodian_.setGizmoProperties(0xFFFF0000);
    positionGeneratorCustodian_.setPositionControls(
                &positionGeneratorX_, &positionGeneratorY_, &positionGeneratorZ_,
                &positionGeneratorX2_, &positionGeneratorY2_, &positionGeneratorZ2_ );

    velocityGeneratorCustodian_.setComboBox(& velocityGenerator_);
    velocityGeneratorCustodian_.setVectorGeneratorSetFn(&SourcePSA::setVelocitySource);
    velocityGeneratorCustodian_.setVectorGeneratorGetFn(&SourcePSA::getVelocitySource);
    velocityGeneratorCustodian_.setVectorGeneratorOwner(action());
    velocityGeneratorCustodian_.setGizmoProperties(0xFF00FF00, true, true);
    velocityGeneratorCustodian_.setPositionControls(
                &velocityGeneratorX_, &velocityGeneratorY_, &velocityGeneratorZ_,
                &velocityGeneratorX2_, &velocityGeneratorY2_, &velocityGeneratorZ2_ );

    // initialise dependencies
    positionGeneratorCustodian_.setParentVectorGeneratorCustodian(NULL);
    positionGeneratorCustodian_.setChildVectorGeneratorCustodian(&velocityGeneratorCustodian_);
    velocityGeneratorCustodian_.setParentVectorGeneratorCustodian(&positionGeneratorCustodian_);
    velocityGeneratorCustodian_.setChildVectorGeneratorCustodian(NULL);

    // set the initial values of the controls
    PsaProperties::OnInitialUpdate();
    
    // set these tick boxes and gizmo visibility for the  generators
    positionGeneratorShow_.SetCheck(s_showPos_);
    positionGeneratorCustodian_.doDraw(s_showPos_);
    velocityGeneratorShow_.SetCheck(s_showVel_);
    velocityGeneratorCustodian_.doDraw(s_showVel_);

    // see what sort of renderer is being used
    ParticleSystemPtr pSystem = MainFrame::instance()->GetCurrentParticleSystem();
    bool mesh = pSystem->pRenderer()->isMeshStyle();
    sizeBox_.EnableWindow(!mesh);
    minimumSize_.EnableWindow(!mesh);
    maximumSize_.EnableWindow(!mesh);

    randomSpin_.EnableWindow(mesh);
    if (!mesh)
    {
        randomSpin_.SetCheck( BST_UNCHECKED );
    }

    bool RandomSpin = mesh && (randomSpin_.GetCheck() == BST_CHECKED);
    
    randomSpinMinRPS_.EnableWindow(RandomSpin);
    randomSpinMaxRPS_.EnableWindow(RandomSpin);
    
    initialOrientRandomise_.EnableWindow(mesh);
    if (!mesh)
    {
        initialOrientRandomise_.SetCheck( BST_UNCHECKED );
    }
    
    bool Randomise = mesh && (initialOrientRandomise_.GetCheck() == BST_CHECKED);
    
    initialOrientRandomiseX_.EnableWindow(Randomise);
    initialOrientRandomiseY_.EnableWindow(Randomise);

    initialOrientX_.EnableWindow(mesh);
    initialOrientY_.EnableWindow(mesh);

    pSystem = MainFrame::instance()->GetCurrentParticleSystem();
    bool sprite =
        (
            pSystem->pRenderer()->nameID() 
            == 
            SpriteParticleRenderer::nameID_
        );
    // Note that these are not enabled for point sprite:
    spriteInitialOrientRand_.EnableWindow(sprite);
    spriteInitialOrient_.EnableWindow(sprite);
    spriteSpinRand_.EnableWindow(sprite);
    spriteSpin_.EnableWindow(sprite);
}


void PsaSourceProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

    ASSERT(action_);

    SET_CHECK_PARAMETER(task, motionTriggered);
    SET_CHECK_PARAMETER(task, timeTriggered  );
    SET_CHECK_PARAMETER(task, grounded       );
    SET_FLOAT_PARAMETER(task, dropDistance   );
    SET_FLOAT_PARAMETER(task, rate           );
    SET_FLOAT_PARAMETER(task, sensitivity    );
    SET_FLOAT_PARAMETER(task, inheritVelocity);
    SET_FLOAT_PARAMETER(task, activePeriod   );
    SET_INT_PARAMETER  (task, forcedUnitSize );
    SET_CHECK_PARAMETER(task, randomSpin     );
    SET_FLOAT_PARAMETER(task, minimumSize    );
    SET_FLOAT_PARAMETER(task, maximumSize    );
    SET_FLOAT_PARAMETER(task, sleepPeriodMax );
    SET_FLOAT_PARAMETER(task, sleepPeriod    );
    SET_FLOAT_PARAMETER(task, allowedTimeInSeconds    );
    SET_FLOAT_PARAMETER(task, maxSpeed    );
	SET_FLOAT_PARAMETER(task, delay);

    if (task == SET_CONTROL)
    {
        // generators
        positionGeneratorCustodian_.updateControl();
        velocityGeneratorCustodian_.updateControl();

        // spin unit conversion
        randomSpinMinRPS_.SetValue( action()->minSpin() * 20.f / MATH_PI );
        randomSpinMaxRPS_.SetValue( action()->maxSpin() * 20.f / MATH_PI );

        ParticleSystemPtr pSystem = MainFrame::instance()->GetCurrentParticleSystem();
        bool sprite =
            (
                pSystem->pRenderer()->nameID() 
                == 
                SpriteParticleRenderer::nameID_
            )
            ||
            (
                pSystem->pRenderer()->nameID() 
                == 
                PointSpriteParticleRenderer::nameID_
            );

        if (sprite)
        {
            Vector2 rotation = action()->initialRotation();
            // initial orient
            spriteInitialOrient_.SetValue( RAD_TO_DEG( rotation.y ) );
            // spin rate
            float sp = rotation.x;
            if (sp > MATH_PI)
                sp -= MATH_2PI;

            sp = Math::clamp( -2.f, sp, 2.f );
            spriteSpin_.SetValue( sp );

            Vector2 randRotation = action()->randomInitialRotation();

			sp = randRotation.x;
            if (sp > MATH_PI)
                sp -= MATH_2PI;

			sp = Math::clamp( 0.f, sp, 2.f );

            spriteSpinRand_.SetValue( sp );
            spriteInitialOrientRand_.SetValue( RAD_TO_DEG( randRotation.y ) );
        }
        else
        {
            // assume mesh (value not actually used by system if not)
            Vector2 rotation = action()->initialRotation();
            initialOrientX_.SetValue( RAD_TO_DEG( rotation.x ) );
            initialOrientY_.SetValue( RAD_TO_DEG( rotation.y ) );

            Vector2 randRotation = action()->randomInitialRotation();
            initialOrientRandomiseX_.SetValue( RAD_TO_DEG( randRotation.x ) );
            initialOrientRandomiseY_.SetValue( RAD_TO_DEG( randRotation.y ) );
            initialOrientRandomise_.SetCheck( (randRotation != Vector2::zero()) ? BST_CHECKED : BST_UNCHECKED);
        }
    }
    else    // set the psa
    {
        // generators

        //Bug 5105 Fix: Added checks for the "show" checkbox for drawing of gizmo.
        positionGeneratorCustodian_.updateOwner( positionGeneratorShow_.GetCheck() & BST_CHECKED );
        velocityGeneratorCustodian_.updateOwner( velocityGeneratorShow_.GetCheck() & BST_CHECKED );

        // spin unit conversion
        const float minSpin = randomSpinMinRPS_.GetValue();
        float maxSpin = randomSpinMaxRPS_.GetValue();
        if (maxSpin < minSpin)
        {
            maxSpin = minSpin;
            randomSpinMaxRPS_.SetValue(maxSpin);
        }

        action()->minSpin( minSpin * MATH_PI / 20.f );
        action()->maxSpin( maxSpin * MATH_PI / 20.f );

        ParticleSystemPtr pSystem = MainFrame::instance()->GetCurrentParticleSystem();
        bool sprite =
            (
                pSystem->pRenderer()->nameID() 
                == 
                SpriteParticleRenderer::nameID_
            )
            ||
            (
                pSystem->pRenderer()->nameID() 
                == 
                PointSpriteParticleRenderer::nameID_
            );
        if (sprite)
        {
            float sp = spriteSpin_.GetValue();
            if (sp < 0)
                sp += MATH_2PI;
            Vector2 rotation( sp, DEG_TO_RAD( spriteInitialOrient_.GetValue() ) );
            action()->initialRotation( rotation );

            Vector2 randRotation = Vector2( spriteSpinRand_.GetValue(),
                                    DEG_TO_RAD( spriteInitialOrientRand_.GetValue() ) );
            action()->randomInitialRotation( randRotation );

            // tell the renderer whether sprites are rotating
            SpriteParticleRenderer* sRenderer = static_cast<SpriteParticleRenderer*>(&*MainFrame::instance()->GetCurrentParticleSystem()->pRenderer());
            //Bug 5147 fix: the rotated flag didn't take into account the initial rotation
            sRenderer->rotated( (rotation + randRotation) != Vector2::zero() );
        }
        else
        {
            // assume mesh (value not actually used by system if not)
            Vector2 rotation( DEG_TO_RAD( initialOrientX_.GetValue() ),
                                DEG_TO_RAD( initialOrientY_.GetValue() ) );
            action()->initialRotation( rotation );

            Vector2 randRotation = Vector2::zero();
            if (initialOrientRandomise_.GetState() & 0x0003)
            {
                randRotation = Vector2( DEG_TO_RAD( initialOrientRandomiseX_.GetValue() ),
                                    DEG_TO_RAD( initialOrientRandomiseY_.GetValue() ) );
            }
            action()->randomInitialRotation( randRotation );
        }
    }

    setControlEnableStates();
}


void PsaSourceProperties::DoDataExchange(CDataExchange* pDX)
{
    PsaProperties::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PSA_SOURCE_MOTIONTRIGGERED, motionTriggered_);
    DDX_Control(pDX, IDC_PSA_SOURCE_TIMETRIGGERED, timeTriggered_);
    DDX_Control(pDX, IDC_PSA_SOURCE_GROUNDED, grounded_);
    DDX_Control(pDX, IDC_PSA_SOURCE_DROPDISTANCE, dropDistance_);
    DDX_Control(pDX, IDC_PSA_SOURCE_RATE, rate_);
    DDX_Control(pDX, IDC_PSA_SOURCE_SENSITIVITY, sensitivity_);
    DDX_Control(pDX, IDC_PSA_SOURCE_ACTIVEPERIOD, activePeriod_);
    DDX_Control(pDX, IDC_PSA_SOURCE_SLEEPPERIOD, sleepPeriod_);
    DDX_Control(pDX, IDC_PSA_SOURCE_MINIMUMSIZE, minimumSize_);
    DDX_Control(pDX, IDC_PSA_SOURCE_MAXIMUMSIZE, maximumSize_);
    DDX_Control(pDX, IDC_PSA_SOURCE_FORCEDUNITSIZE, forcedUnitSize_);
    DDX_Control(pDX, IDC_PSA_SOURCE_ALLOWEDTIME, allowedTimeInSeconds_);
    DDX_Control(pDX, IDC_PSA_SOURCE_POSITION_GENERATOR, positionGenerator_);
    DDX_Control(pDX, IDC_PSA_SOURCE_VELOCITY_GENERATOR, velocityGenerator_);
    DDX_Control(pDX, IDC_PSA_SOURCE_VELOCITY_GENERATOR_SHOW, velocityGeneratorShow_);
    DDX_Control(pDX, IDC_PSA_SOURCE_POSITION_GENERATOR_SHOW, positionGeneratorShow_);
    DDX_Control(pDX, IDC_PSA_SOURCE_SLEEPPERIOD_MAX, sleepPeriodMax_);
    DDX_Control(pDX, IDC_PSA_SOURCE_RANDOM_SPIN, randomSpin_);
    DDX_Control(pDX, IDC_PSA_SOURCE_RANDOM_SPIN_MIN, randomSpinMinRPS_);
    DDX_Control(pDX, IDC_PSA_SOURCE_RANDOM_SPIN_MAX, randomSpinMaxRPS_);
    DDX_Control(pDX, IDC_PSA_SOURCE_INITIALORIENT_RANDOMISE, initialOrientRandomise_);
    DDX_Control(pDX, IDC_PSA_SOURCE_INITIALORIENT_RANDOMISE_MIN, initialOrientRandomiseX_);
    DDX_Control(pDX, IDC_PSA_SOURCE_INITIALORIENT_RANDOMISE_MAX, initialOrientRandomiseY_);
    DDX_Control(pDX, IDC_PSA_SOURCE_INITIALORIENT_X, initialOrientX_);
    DDX_Control(pDX, IDC_PSA_SOURCE_INITIALORIENT_Y, initialOrientY_);
    DDX_Control(pDX, IDC_PSA_SOURCE_SIZEBOX, sizeBox_);
    DDX_Control(pDX, IDC_PSA_SOURCE_SPRITE_INITORIENT, spriteInitialOrient_);
    DDX_Control(pDX, IDC_PSA_SOURCE_SPRITE_INITORIENT_RAND, spriteInitialOrientRand_);
    DDX_Control(pDX, IDC_PSA_SOURCE_SPRITE_SPINRATE, spriteSpin_);
    DDX_Control(pDX, IDC_PSA_SOURCE_SPRITE_SPINRATE_RAND, spriteSpinRand_);
    DDX_Control(pDX, IDC_PSA_SOURCE_POSITION_GENERATOR_X, positionGeneratorX_);
    DDX_Control(pDX, IDC_PSA_SOURCE_POSITION_GENERATOR_Y, positionGeneratorY_);
    DDX_Control(pDX, IDC_PSA_SOURCE_POSITION_GENERATOR_Z, positionGeneratorZ_);
    DDX_Control(pDX, IDC_PSA_SOURCE_POSITION_GENERATOR_X2, positionGeneratorX2_);
    DDX_Control(pDX, IDC_PSA_SOURCE_POSITION_GENERATOR_Y2, positionGeneratorY2_);
    DDX_Control(pDX, IDC_PSA_SOURCE_POSITION_GENERATOR_Z2, positionGeneratorZ2_);
    DDX_Control(pDX, IDC_PSA_SOURCE_VELOCITY_GENERATOR_X, velocityGeneratorX_);
    DDX_Control(pDX, IDC_PSA_SOURCE_VELOCITY_GENERATOR_Y, velocityGeneratorY_);
    DDX_Control(pDX, IDC_PSA_SOURCE_VELOCITY_GENERATOR_Z, velocityGeneratorZ_);
    DDX_Control(pDX, IDC_PSA_SOURCE_VELOCITY_GENERATOR_X2, velocityGeneratorX2_);
    DDX_Control(pDX, IDC_PSA_SOURCE_VELOCITY_GENERATOR_Y2, velocityGeneratorY2_);
    DDX_Control(pDX, IDC_PSA_SOURCE_VELOCITY_GENERATOR_Z2, velocityGeneratorZ2_);
    DDX_Control(pDX, IDC_PSA_SOURCE_VELOCITY_INHERITANCE , inheritVelocity_);
    DDX_Control(pDX, IDC_PSA_SOURCE_MAXSPEED , maxSpeed_);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}

BEGIN_MESSAGE_MAP(PsaSourceProperties, PsaProperties)
    ON_BN_CLICKED(IDC_PSA_SOURCE_MOTIONTRIGGERED, OnBnClickedPsaSourceButton)
    ON_BN_CLICKED(IDC_PSA_SOURCE_TIMETRIGGERED, OnBnClickedPsaSourceButton)
    ON_BN_CLICKED(IDC_PSA_SOURCE_GROUNDED, OnBnClickedPsaSourceButton)
    ON_CBN_SELCHANGE(IDC_PSA_SOURCE_POSITION_GENERATOR, OnBnClickedPsaSourceButton)
    ON_CBN_SELCHANGE(IDC_PSA_SOURCE_VELOCITY_GENERATOR, OnBnClickedPsaSourceButton)
    ON_BN_CLICKED(IDC_PSA_SOURCE_POSITION_GENERATOR_SHOW, OnBnClickedPsaSourcePositionGeneratorShow)
    ON_BN_CLICKED(IDC_PSA_SOURCE_VELOCITY_GENERATOR_SHOW, OnBnClickedPsaSourceVelocityGeneratorShow)
    ON_BN_CLICKED(IDC_PSA_SOURCE_RANDOM_SPIN, OnBnClickedPsaSourceButton)
    ON_BN_CLICKED(IDC_PSA_SOURCE_INITIALORIENT_RANDOMISE, OnBnClickedPsaSourceButton)
    ON_BN_CLICKED(IDC_PSA_SOURCE_SPRITE_INITORIENT_RAND, OnBnClickedPsaSourceButton)
    ON_BN_CLICKED(IDC_PSA_SOURCE_SPRITE_SPINRATE_RAND, OnBnClickedPsaSourceButton)
    ON_EN_CHANGE(IDC_PSA_SOURCE_SPRITE_SPINRATE_RAND, OnEnChangePsaSourceSpriteSpinrateRand)
END_MESSAGE_MAP()

void PsaSourceProperties::setControlEnableStates()
{
	BW_GUARD;

    {
        bool setEnabled = false;
        if (motionTriggered_.GetState() & 0x0003)
            setEnabled = true;

        sensitivity_.EnableWindow(setEnabled);
    }

    {
        bool setEnabled = false;
        if (timeTriggered_.GetState() & 0x0003)
            setEnabled = true;

        rate_.EnableWindow(setEnabled);
        activePeriod_.EnableWindow(setEnabled);
        sleepPeriod_.EnableWindow(setEnabled);
        sleepPeriodMax_.EnableWindow(setEnabled);
    }

    {
        bool setEnabled = false;
        if (grounded_.GetState() & 0x0003)
            setEnabled = true;

        dropDistance_.EnableWindow(setEnabled);
    }

    {
        bool setEnabled = false;
        if (randomSpin_.GetState() & 0x0003)
            setEnabled = true;

        randomSpinMinRPS_.EnableWindow(setEnabled);
        randomSpinMaxRPS_.EnableWindow(setEnabled);
    }

    {
        bool setEnabled = false;
        if (initialOrientRandomise_.GetState() & 0x0003)
            setEnabled = true;

        initialOrientRandomiseX_.EnableWindow(setEnabled);
        initialOrientRandomiseY_.EnableWindow(setEnabled);
    }
}


// PsaSourceProperties diagnostics

#ifdef _DEBUG
void PsaSourceProperties::AssertValid() const
{
    PsaProperties::AssertValid();
}

void PsaSourceProperties::Dump(CDumpContext& dc) const
{
    PsaProperties::Dump(dc);
}
#endif //_DEBUG

void PsaSourceProperties::OnBnClickedPsaSourceButton()
{
	BW_GUARD;

    CopyDataToPSA();
}

void PsaSourceProperties::OnBnClickedPsaSourcePositionGeneratorShow()
{
	BW_GUARD;

    s_showPos_ = (positionGeneratorShow_.GetCheck() & BST_CHECKED);

    positionGeneratorCustodian_.doDraw(s_showPos_);
}

void PsaSourceProperties::OnBnClickedPsaSourceVelocityGeneratorShow()
{
	BW_GUARD;

    s_showVel_ = (velocityGeneratorShow_.GetCheck() & BST_CHECKED);

    velocityGeneratorCustodian_.doDraw(s_showVel_);
}

void PsaSourceProperties::OnEnChangePsaSourceSpriteSpinrateRand()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the PsaProperties::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}


SourcePSA * PsaSourceProperties::action()
{
	return static_cast<SourcePSA *>(action_.get());
}

BW_END_NAMESPACE

