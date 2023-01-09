#include "pch.hpp"
#include "hdr_settings.hpp"


BW_BEGIN_NAMESPACE

//--------------------------------------------------------------------------------------------------
HDRSettings::Bloom::Bloom()
    :   m_enabled(true),
        m_brightThreshold(2.0f),
        m_brightOffset(1.0f),
        m_factor(1.5f)
{

}

//--------------------------------------------------------------------------------------------------
HDRSettings::ToneMapping::ToneMapping()
    :   m_eyeDarkLimit(0.05f),
        m_eyeLightLimit(1.5f),
        m_middleGray(0.5f),
        m_whitePoint(1.5f)
{

}

//--------------------------------------------------------------------------------------------------
HDRSettings::Environment::Environment()
    :   m_skyLumMultiplier(5.0f),
        m_sunLumMultiplier(4.5f),
        m_ambientLumMultiplier(1.0f),
        m_fogLumMultiplier(1.0f)
{

}

//--------------------------------------------------------------------------------------------------
HDRSettings::GammaCorrection::GammaCorrection()
    :   m_enabled(false),
        m_gamma(2.2f)
{

}

//--------------------------------------------------------------------------------------------------
HDRSettings::HDRSettings()
    :   m_enabled(true),
        m_version(1),
        m_useLogAvgLuminance(false),
        m_adaptationSpeed(60.0f),
        m_avgLumUpdateInterval(0.25f)
{

}

//--------------------------------------------------------------------------------------------------
HDRSettings::~HDRSettings()
{

}

//--------------------------------------------------------------------------------------------------
bool HDRSettings::load(const DataSectionPtr &iData)
{
    BW_GUARD;

    //-- if data section is empty use default parameters.
    if (!iData.exists())
        return false;

    //-- common parameters.
    m_version                       = iData->readInt    ("version", m_version);
    m_enabled                       = iData->readBool   ("enable", m_enabled);
    m_adaptationSpeed               = iData->readFloat  ("adaptationSpeed", m_adaptationSpeed);
    m_avgLumUpdateInterval          = iData->readFloat  ("avgLumUpdateInterval", m_avgLumUpdateInterval);

    //-- bloom parameters.
    {
        Bloom& b = m_bloom;
        b.m_enabled                 = iData->readBool   ("bloom/enable", b.m_enabled);
        b.m_brightThreshold         = iData->readFloat  ("bloom/brightThreshold", b.m_brightThreshold);
        b.m_brightOffset            = iData->readFloat  ("bloom/brightOffset", b.m_brightOffset);
        b.m_factor                  = iData->readFloat  ("bloom/factor", b.m_factor);
    }
    
    //-- tone mapping parameters.
    {
        ToneMapping& t = m_tonemapping;

        t.m_eyeDarkLimit            = iData->readFloat  ("tonemapping/eyeDarkLimit", t.m_eyeDarkLimit);
        t.m_eyeLightLimit           = iData->readFloat  ("tonemapping/eyeLightLimit", t.m_eyeLightLimit);
        t.m_middleGray              = iData->readFloat  ("tonemapping/middleGray", t.m_middleGray);
        t.m_whitePoint              = iData->readFloat  ("tonemapping/whitePoint", t.m_whitePoint);
    }

    //-- environment lighting parameters.
    {
        Environment& e = m_environment;

        e.m_skyLumMultiplier        = iData->readFloat  ("environment/skyLumMultiplier", e.m_skyLumMultiplier);
        e.m_sunLumMultiplier        = iData->readFloat  ("environment/sunLumMultiplier", e.m_sunLumMultiplier);
        e.m_ambientLumMultiplier    = iData->readFloat  ("environment/ambientLumMultiplier", e.m_ambientLumMultiplier);
        //-- Note: for backward capability we use sun luminance multiplier as fallback, in case if 
        //--       fog luminance multiplier is not presented in cfg.
        e.m_fogLumMultiplier        = iData->readFloat  ("environment/fogLumMultiplier", m_environment.m_sunLumMultiplier);
    }

    //-- gamma correction.
    {
        GammaCorrection& gc = m_gammaCorrection;

        gc.m_enabled                = iData->readBool   ("gammaCorrection/enabled", gc.m_enabled);
        gc.m_gamma                  = iData->readFloat  ("gammaCorrection/gamma", gc.m_gamma);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool HDRSettings::save(DataSectionPtr& oData) const
{
    BW_GUARD;
    MF_ASSERT(oData);
    oData->delChildren();

    //-- common parameters.
    oData->writeInt     ("version", m_version);
    oData->writeBool    ("enable", m_enabled);
    oData->writeFloat   ("adaptationSpeed", m_adaptationSpeed);
    oData->writeFloat   ("avgLumUpdateInterval", m_avgLumUpdateInterval);

    //-- bloom parameters.
    oData->writeBool    ("bloom/enable", m_bloom.m_enabled);
    oData->writeFloat   ("bloom/brightThreshold", m_bloom.m_brightThreshold);
    oData->writeFloat   ("bloom/brightOffset", m_bloom.m_brightOffset);
    oData->writeFloat   ("bloom/factor", m_bloom.m_factor);

    //-- tone mapping parameters.
    oData->writeFloat   ("tonemapping/eyeDarkLimit", m_tonemapping.m_eyeDarkLimit);
    oData->writeFloat   ("tonemapping/eyeLightLimit", m_tonemapping.m_eyeLightLimit);
    oData->writeFloat   ("tonemapping/middleGray", m_tonemapping.m_middleGray);
    oData->writeFloat   ("tonemapping/whitePoint", m_tonemapping.m_whitePoint);

    //-- environment lighting parameters.
    oData->writeFloat   ("environment/skyLumMultiplier", m_environment.m_skyLumMultiplier);
    oData->writeFloat   ("environment/sunLumMultiplier", m_environment.m_sunLumMultiplier);
    oData->writeFloat   ("environment/ambientLumMultiplier", m_environment.m_ambientLumMultiplier);
    oData->writeFloat   ("environment/fogLumMultiplier", m_environment.m_fogLumMultiplier);

    //-- gamma correction.
    oData->writeBool    ("gammaCorrection/enabled", m_gammaCorrection.m_enabled);
    oData->writeFloat   ("gammaCorrection/gamma", m_gammaCorrection.m_gamma);
    
    return true;
}

BW_END_NAMESPACE
