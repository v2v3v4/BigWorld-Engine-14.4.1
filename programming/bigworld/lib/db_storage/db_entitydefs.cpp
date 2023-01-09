#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "db_entitydefs.hpp"

#include "connection/baseapp_ext_interface.hpp"
#include "connection/client_interface.hpp"

#include "entitydef/entity_description_debug.hpp"
#include "entitydef/constants.hpp"
#include "cstdmf/md5.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Constants
// -----------------------------------------------------------------------------
static const BW::string EMPTY_STR;


namespace
{

/**
 *	This function validates the data type of an identifer property.
 *
 *	@param dataDesc        The data description of the property that has an
 *	                       Identifier tag.
 *	@param entityTypeName  The name of the Entity type the property belongs to.
 *
 *	@returns true if the identifier is valid, false otherwise.
 */
bool checkIdentifierPropertyType( const DataDescription & dataDesc,
		const BW::string & entityTypeName )
{
	const char * pPropTypeName = dataDesc.dataType()->pMetaDataType()->name();
	bool isValid = true;

	if ((strcmp( pPropTypeName, "STRING" ) != 0) &&
		(strcmp( pPropTypeName, "UNICODE_STRING" ) != 0) &&
		(strcmp( pPropTypeName, "BLOB" ) != 0))
	{
		ERROR_MSG( "EntityDefs::findIdentifier: "
					"Identifier must be of type STRING, UNICODE_STRING or "
					"BLOB. Identifier '%s' for entity type '%s' is ignored.\n",
				dataDesc.name().c_str(),
				entityTypeName.c_str() );
		isValid = false;
	}

	return isValid;
}

} // anonymous namespace

// -----------------------------------------------------------------------------
// Section: EntityDefs
// -----------------------------------------------------------------------------
/**
 * 	This function initialises EntityDefs. Must be called once and only once for
 * 	each instance of EntityDefs.
 *
 * 	@param	pEntitiesSection	The entities.xml data section.
 *
 * 	@return	True if successful.
 */
bool EntityDefs::init( DataSectionPtr pEntitiesSection )
{
	if (pEntitiesSection == NULL)
	{
		ERROR_MSG( "EntityDefs::init: Failed to open '%s'\n",
			EntityDef::Constants::entitiesFile() );
		return false;
	}

	MF_ASSERT( entityDescriptionMap_.size() == 0 );

	if (!entityDescriptionMap_.parse( pEntitiesSection,
		&ClientInterface::Range::entityPropertyRange,
		&ClientInterface::Range::entityMethodRange,
		&BaseAppExtInterface::Range::baseEntityMethodRange,
		&BaseAppExtInterface::Range::cellEntityMethodRange ))
	{
		ERROR_MSG( "EntityDefs::init: "
				"Failed to initialise EntityDescriptionMap.\n" );
		return false;
	}

	bool isOkay = true;

	// Set up some entity def stuff specific to DBApp

	for ( EntityTypeID i = 0; i < entityDescriptionMap_.size(); ++i )
	{
		const EntityDescription & entityDesc = this->getEntityDescription( i );

		isOkay &= this->findIdentifier( entityDesc, /*entityTypeID*/ i );
	}

	MD5 md5;
	entityDescriptionMap_.addToMD5( md5 );
	md5.getDigest( md5Digest_ );

	MD5 persistentPropertiesMD5;
	entityDescriptionMap_.addPersistentPropertiesToMD5(
			persistentPropertiesMD5 );
	persistentPropertiesMD5.getDigest( persistentPropertiesMD5Digest_ );

	return isOkay;
}


bool EntityDefs::findIdentifier( const EntityDescription & entityDesc,
		EntityTypeID entityTypeID )
{
	bool isOkay = true;
	bool hasIdentifier = false;

	for ( uint i = 0; i < entityDesc.propertyCount(); ++i)
	{
		const DataDescription * pDataDesc = entityDesc.property( i );
		if (pDataDesc->isIdentifier())
		{
			if (!hasIdentifier)
			{
				isOkay &= checkIdentifierPropertyType( *pDataDesc,
										entityDesc.name() );
			}
			else // We don't support having multiple to name columns.
			{
				ERROR_MSG( "EntityDefs::findIdentifier: "
							"Multiple identifiers for one entity type are not "
							"supported. Identifier '%s' for entity type '%s' "
							"has been ignored.\n",
						pDataDesc->name().c_str(),
						entityDesc.name().c_str() );
				isOkay = false;
			}

			hasIdentifier = true;
		}
	}

	return isOkay;
}


/**
 *	This function returns the type name of the given property.
 */
BW::string EntityDefs::getPropertyType( EntityTypeID typeID,
	const BW::string & propName ) const
{
	const EntityDescription & entityDesc = this->getEntityDescription( typeID );
	DataDescription * pDataDesc = entityDesc.findProperty( propName.c_str() );
	return ( pDataDesc ) ? pDataDesc->dataType()->typeName() : BW::string();
}


/**
 *	This function prints out information about the entity defs.
 */
void EntityDefs::debugDump( int detailLevel ) const
{
	EntityDescriptionDebug::dump( entityDescriptionMap_, detailLevel );
}

BW_END_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Mercury
// -----------------------------------------------------------------------------

#define DEFINE_INTERFACE_HERE
#include "connection/baseapp_ext_interface.hpp"

#define DEFINE_INTERFACE_HERE
#include "connection/client_interface.hpp"

// db_entitydefs.cpp
