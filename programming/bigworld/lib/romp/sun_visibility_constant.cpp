#include "pch.hpp"
#include "sun_visibility_constant.hpp"
#include "lens_effect_manager.hpp"
#include "time_of_day.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/effect_constant_value.hpp"
#include "moo/effect_visual_context.hpp"

BW_BEGIN_NAMESPACE


static AutoConfigString s_sunVisibilityTestXML( "environment/sunVisibilityTestXML" );

//-- Sun visibility shared auto-constant.
//--------------------------------------------------------------------------------------------------
class SunVisibilityConstant : public Moo::EffectConstantValue
{
public:
    SunVisibilityConstant(LensEffect& sunVisibilityTestEffect) : 
      m_sunVisibilityTestEffect(sunVisibilityTestEffect)
    {
    }

    bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
    {
        BW_GUARD;
        pEffect->SetFloat(constantHandle, LensEffectManager::instance().sunVisibility());
        return true;
    }

private:
    LensEffect& m_sunVisibilityTestEffect;
};

SunVisibilityConstantSupport::SunVisibilityConstantSupport():
m_added(false)
{

}

void SunVisibilityConstantSupport::load()
{
    //Sun flare
    DataSectionPtr pLensEffectsSection = BWResource::openSection( s_sunVisibilityTestXML );
    if ( pLensEffectsSection )
    {
        m_sunVisibilityTestEffect.load( pLensEffectsSection );
        m_sunVisibilityTestEffect.clampToFarPlane( true );
    }
    else
    {
        WARNING_MSG( "Could not find lens effects definitions for the Sun\n" );
    }
    createManagedObjects();
}

void SunVisibilityConstantSupport::createManagedObjects()
{
    BW_GUARD;
    if(m_added)
        return;

    //-- register shared auto-constant.
    Moo::rc().effectVisualContext().registerAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "SunVisibility",
        new SunVisibilityConstant(m_sunVisibilityTestEffect)
        );
    m_added = true;
}

void SunVisibilityConstantSupport::deleteManagedObjects()
{
    BW_GUARD;
    m_added = false;

    //-- unregister shared auto-constant.
    Moo::rc().effectVisualContext().unregisterAutoConstant(
        Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "SunVisibility"
        );
}

BW_END_NAMESPACE
