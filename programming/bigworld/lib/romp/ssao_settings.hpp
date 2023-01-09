#pragma once

#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

//-- Describes some SSAO properties unique for each individual chunk space. So this object belongs
//-- to the ChunkSpace class and is written and read from *.xml file of chunk space.
//--------------------------------------------------------------------------------------------------
struct SSAOSettings
{
    SSAOSettings();
    ~SSAOSettings();

    //-- load/save settings.
    bool load(const DataSectionPtr& iData);
    bool save(DataSectionPtr& oData) const;

	//-- defines how much SSAO affects different sub-systems and different lighting terms.
	struct InfluenceFactors
	{
		InfluenceFactors() : m_speedtree(1.0f, 0.0f), m_terrain(1.0f, 0.0f), m_objects(1.0f, 0.0f) { }

		Vector2 m_speedtree; //-- x - ambient term, y - lighting (diffuse + specular) term.
		Vector2 m_terrain;
		Vector2 m_objects;
	};

    bool				m_enabled;
    float				m_radius;
    float				m_amplify;
    float				m_fade;
	InfluenceFactors	m_influences;
};

BW_END_NAMESPACE
