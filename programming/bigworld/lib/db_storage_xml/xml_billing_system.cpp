#include "script/first_include.hpp"

#include "xml_billing_system.hpp"

#include "xml_database.hpp"

#include "db_storage/db_entitydefs.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: XMLBillingSystem
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
XMLBillingSystem::XMLBillingSystem( DataSectionPtr pLogonMapSection,
			const EntityDefs & entityDefs,
	 		XMLDatabase * pDatabase	) :
		BillingSystem( entityDefs ),
		pLogonMapSection_( pLogonMapSection ),
		entityDefs_( entityDefs )
{
	this->initLogOnMapping( pDatabase );
}


/**
 *
 */
void XMLBillingSystem::initLogOnMapping( XMLDatabase * pDatabase )
{
	for (DataSection::iterator it = pLogonMapSection_->begin();
			it != pLogonMapSection_->end(); ++it)
	{
		DataSectionPtr pSection = *it;

		EntityTypeID typeID =
			entityDefs_.getEntityType( pSection->readString( "type" ) );

		if (typeID != INVALID_ENTITY_TYPE_ID)
		{
			DatabaseID entityID =
				pSection->read( "entityID", DatabaseID( 0 ) );

			if (entityID == 0)
			{
				entityID = pDatabase->findEntityByName( typeID,
						pSection->readString( "entityName" ) );
			}

			logonMap_[ pSection->readString( "logOnName" ) ] =
					LogOnMapping( EntityKey( typeID, entityID ),
							pSection->readString( "password" ) );
		}
		else
		{
			WARNING_MSG( "XMLDatabase::intLogOnMapping: "
					"Logon mapping ignored because '%s' is not a "
					"valid entity type\n",
				pSection->readString( "type" ).c_str() );
		}
	}
}


/**
 *	Override from BillingSystem.
 */
void XMLBillingSystem::getEntityKeyForAccount(
	const BW::string & logOnName, const BW::string & password,
	const Mercury::Address & clientAddr,
	IGetEntityKeyForAccountHandler & handler )
{
	if (authenticateViaBaseEntity_)
	{
		handler.onGetEntityKeyForAccountLoadFromUsername(
			entityTypeIDForUnknownUsers_, logOnName,
			/* shouldCreateNewOnLoadFailure */ true,
			/* shouldRememberNewOnLoadFailure */ false,
			BW::string(), password );
		return;
	}

	LogonMap::const_iterator it = logonMap_.find( logOnName );

	if (it != logonMap_.end())
	{
		if (it->second.matchesPassword( password ))
		{
			handler.onGetEntityKeyForAccountSuccess( it->second.entityKey() );
		}
		else
		{
			handler.onGetEntityKeyForAccountFailure(
					LogOnStatus::LOGIN_REJECTED_INVALID_PASSWORD );
		}
	}
	else if (shouldAcceptUnknownUsers_)
	{
		handler.onGetEntityKeyForAccountCreateNew( entityTypeIDForUnknownUsers_,
				shouldRememberUnknownUsers_ );
	}
	else
	{
		handler.onGetEntityKeyForAccountFailure(
				LogOnStatus::LOGIN_REJECTED_NO_SUCH_USER );
	}
}


/*
 *	Override from BillingSystem
 */
void XMLBillingSystem::setEntityKeyForAccount( const BW::string & username,
		const BW::string & password, const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler )
{
	// Try to find existing section.
	DataSectionPtr pSection = NULL;

	if (logonMap_.find( username ) != logonMap_.end())
	{
		// Using linear search... yuk but this should occur rarely.
		for (DataSection::iterator it = pLogonMapSection_->begin();
				it != pLogonMapSection_->end(); ++it)
		{
			if ((*it)->readString( "logOnName" ) == username)
			{
				pSection = *it;
				break;
			}
		}
	}

	logonMap_[ username ] = LogOnMapping( ekey, password );

	const BW::string & typeName =
			entityDefs_.getEntityDescription( ekey.typeID ).name();

	if (!pSection)
	{
		pSection = pLogonMapSection_->newSection( "item" );
	}

	pSection->writeString( "logOnName", username );
	pSection->writeString( "password", password );
	pSection->writeString( "type", typeName );
	pSection->write( "entityID", ekey.dbID );

	handler.onSetEntityKeyForAccountSuccess( ekey );
}

BW_END_NAMESPACE

// xml_billing_system.cpp

