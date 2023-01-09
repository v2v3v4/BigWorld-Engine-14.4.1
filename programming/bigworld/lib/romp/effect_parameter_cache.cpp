#include "pch.hpp"
#include "effect_parameter_cache.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

EffectParameterCache::~EffectParameterCache()
{
	BW_GUARD;
	this->spEffect_ = NULL;
}

void EffectParameterCache::cache(const BW::string& name )
{		
	BW_GUARD;
	D3DXHANDLE handle = spEffect_->GetParameterByName(NULL,name.c_str());
	if (handle)
		parameters_[name] = handle;
	else
		ERROR_MSG( "Could not find parameter %s in EffectMaterial\n", name.c_str() );
}

BW_END_NAMESPACE

// effect_parameter_cache.cpp
