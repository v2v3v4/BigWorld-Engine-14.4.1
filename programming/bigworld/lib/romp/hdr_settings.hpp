#pragma once

#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

//-- Describes some HDR properties unique for each individual chunk space. So this object belongs
//-- to the ChunkSpace class and is written and read from *.xml file of chunk space.
//--------------------------------------------------------------------------------------------------
struct HDRSettings
{
	HDRSettings();
	~HDRSettings();

	//-- load/save settings.
	bool load(const DataSectionPtr& iData);
	bool save(DataSectionPtr& oData) const;

	//-- bloom parameters.
	struct Bloom
	{
		Bloom();

		bool  m_enabled;
		float m_brightThreshold;
		float m_brightOffset;
		float m_factor;
	};

	//-- Reinhard's tone mapping parameters.
	struct ToneMapping
	{
		ToneMapping();

		float m_eyeDarkLimit;
		float m_eyeLightLimit;
		float m_middleGray;
		float m_whitePoint;
	};

	//-- environment lighting parameters.
	struct Environment
	{
		Environment();

		float m_skyLumMultiplier;
		float m_sunLumMultiplier;
		float m_ambientLumMultiplier;
		float m_fogLumMultiplier;
	};

	//-- linear space lighting.
	struct GammaCorrection
	{
		GammaCorrection();
		
		bool  m_enabled;
		float m_gamma;

	};

	bool			m_enabled;
	uint			m_version;
	bool			m_useLogAvgLuminance;
	float			m_adaptationSpeed;
	float			m_avgLumUpdateInterval;

	Bloom			m_bloom;
	ToneMapping		m_tonemapping;
	Environment		m_environment;
	GammaCorrection	m_gammaCorrection;
};

BW_END_NAMESPACE
