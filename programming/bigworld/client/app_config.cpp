#include "pch.hpp"
#include "app_config.hpp"

#ifndef CODE_INLINE
#include "app_config.ipp"
#endif

#include "resmgr/xml_section.hpp"

DECLARE_DEBUG_COMPONENT2( "App", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: AppConfig
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
AppConfig::AppConfig() :
	pRoot_( NULL )
{
}


/**
 *	Destructor.
 */
AppConfig::~AppConfig()
{
}


/**
 *	Init method
 */
bool AppConfig::init( DataSectionPtr configSection )
{
	BW_GUARD;
	if (!configSection.exists() || configSection->countChildren() == 0) 
	{
		return false;
	}

	pRoot_ = configSection;
	return true;
}


/**
 *	Instance accessor
 */
AppConfig & AppConfig::instance()
{
	static AppConfig	inst;
	return inst;
}

BW_END_NAMESPACE

// app_config.cpp
