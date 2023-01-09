#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "custom.hpp"

#include "dbapp.hpp"

#include "db_storage/db_entitydefs.hpp"
#include "resmgr/xml_section.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This method is used to create a new "unknown" entity. All its properties
 * 	should be default values except its name property which will be set to
 * 	the login name.
 */
DataSectionPtr createNewEntity( EntityTypeID typeID, const BW::string& name )
{
	const EntityDefs & entityDefs = DBApp::instance().getEntityDefs();
	DataSectionPtr pSection = 
			new XMLSection( entityDefs.getEntityDescription( typeID ).name() );

	const DataDescription *pIdentifier = entityDefs.getIdentifierProperty(
														typeID );

	if (pIdentifier)
	{
		if (entityDefs.getPropertyType( typeID, pIdentifier->name() ) == "BLOB" )
		{
			pSection->writeBlob( pIdentifier->name(), name );
		}
		else
		{
			pSection->writeString( pIdentifier->name(), name );
		}
	}

	return pSection;
}

BW_END_NAMESPACE

// custom.cpp
