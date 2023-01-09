#include "pch.hpp"
#include "fog_helper.hpp"

BW_BEGIN_NAMESPACE

Moo::FogHelper* Moo::FogHelper::s_pInstance = NULL;

const Moo::FogParams& Moo::FogHelper::fogParams() const
{
	return m_fogParams;
}

void Moo::FogHelper::fogParams( const Moo::FogParams& params)
{
	m_fogParams = params;
}

void Moo::FogHelper::fogEnable(bool enable)
{
	m_fogParams.m_enabled = enable;
}

bool Moo::FogHelper::fogEnabled() const
{
	return m_fogParams.m_enabled != 0.0f;
}

Moo::FogHelper* Moo::FogHelper::pInstance()
{
	MF_ASSERT(s_pInstance && "Moo::FogHelper uninitialised!");
	return s_pInstance;
}

Moo::FogHelper::FogHelper()
{
	s_pInstance = this;
}
Moo::FogHelper::~FogHelper()
{
	s_pInstance = NULL;
}


BW_END_NAMESPACE

// fog_helper.cpp
