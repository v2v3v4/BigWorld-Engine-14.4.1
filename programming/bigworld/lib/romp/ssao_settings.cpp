#include "pch.hpp"
#include "ssao_settings.hpp"

BW_BEGIN_NAMESPACE

//--------------------------------------------------------------------------------------------------

SSAOSettings::SSAOSettings()
    : m_enabled(true)
    , m_radius(0.04f)
    , m_amplify(1.f)
    , m_fade(3.f)
{
}

SSAOSettings::~SSAOSettings()
{
}

//--------------------------------------------------------------------------------------------------

bool SSAOSettings::load(const DataSectionPtr &iData)
{
    BW_GUARD;

    //-- if data section is empty use default parameters.
    if (!iData.exists())
        return false;

    m_enabled = iData->readBool("enable", m_enabled);
    m_radius  = iData->readFloat("redius", m_radius);
    m_amplify = iData->readFloat("amplify", m_amplify);
    m_fade    = iData->readFloat("fade", m_fade);
    
    //-- influence factors parameters.
    {
        InfluenceFactors& i = m_influences;
        i.m_speedtree   = iData->readVector2("influences/speedtree", i.m_speedtree);
        i.m_terrain     = iData->readVector2("influences/terrain", i.m_terrain);
        i.m_objects     = iData->readVector2("influences/objects", i.m_objects);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool SSAOSettings::save(DataSectionPtr& oData) const
{
    BW_GUARD;
    MF_ASSERT(oData);
    oData->delChildren();

    oData->writeBool("enable", m_enabled);
    oData->writeFloat("redius", m_radius);
    oData->writeFloat("amplify", m_amplify);
    oData->writeFloat("fade", m_fade);

    //-- influence factors parameters.
    oData->writeVector2("influences/speedtree", m_influences.m_speedtree);
    oData->writeVector2("influences/terrain", m_influences.m_terrain);
    oData->writeVector2("influences/objects", m_influences.m_objects);

    return true;
}

BW_END_NAMESPACE
