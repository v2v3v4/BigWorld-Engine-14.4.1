#include "pch.hpp"
#include "particle_editor.hpp"
#include "main_frame.hpp"
#include "gui/propdlgs/psa_collide_properties.hpp"
#include "controls/dir_dialog.hpp"
#include "controls/utils.hpp"
#include "particle/actions/collide_psa.hpp"
#include "resmgr/string_provider.hpp"
#include "fmodsound/sound_manager.hpp"

DECLARE_DEBUG_COMPONENT2("GUI", 0)
BW_BEGIN_NAMESPACE

namespace 
{
	bool validSoundTag( const BW::string & filename )
    {
        return true;
    }
}

IMPLEMENT_DYNCREATE(PsaCollideProperties, PsaProperties)

PsaCollideProperties::PsaCollideProperties()
:
PsaProperties(PsaCollideProperties::IDD)
{
	BW_GUARD;

	delay_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	delay_.SetMinimum( 0.0f );
}

PsaCollideProperties::~PsaCollideProperties()
{
}

void PsaCollideProperties::OnInitialUpdate()
{
	BW_GUARD;

    PsaProperties::OnInitialUpdate();
}

void PsaCollideProperties::DoDataExchange(CDataExchange* pDX)
{
    PsaProperties::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PSA_COLLIDE_ELASTICITY, elasticity_);
	DDX_Control(pDX, IDC_PSA_DELAY, delay_);
}

void PsaCollideProperties::SetParameters(SetOperation task)
{
	BW_GUARD;

    ASSERT(action_);

    SET_FLOAT_PARAMETER( task, elasticity  );
	SET_FLOAT_PARAMETER( task, delay );
}

CollidePSA * PsaCollideProperties::action()
{
	return static_cast<CollidePSA *>(action_.get());
}

BW_END_NAMESPACE

