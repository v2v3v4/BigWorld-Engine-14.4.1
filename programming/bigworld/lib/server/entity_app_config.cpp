#include "entity_app_config.hpp"

#include "network/compression_stream.hpp"
#include "server/common.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EntityAppConfig
// -----------------------------------------------------------------------------

bool EntityAppConfig::postInit()
{
	if (!ServerAppConfig::postInit())
	{
		return false;
	}

	if (!CompressionOStream::initDefaults(
				BWConfig::getSection( "networkCompression" ) ))
	{
		ERROR_MSG( "EntityAppConfig::postInit: "
				"Failed to initialise networkCompression settings.\n" );
		return false;
	}

	return true;
}

BW_END_NAMESPACE

// entity_app_config.cpp
