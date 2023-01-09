#include "pch.hpp"

#include "entity_description_map.hpp"

#include "entitydef/constants.hpp"

#include "cstdmf/debug.hpp"
#include "network/exposed_message_range.hpp"
#include "resmgr/bwresource.hpp"
#include "script/init_time_job.hpp"

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Static objects
// -----------------------------------------------------------------------------

/**
 *	This class is used to clear entity description state before Script::fini()
 	completes.
 */
class EntityDefFiniTimeJob : public Script::FiniTimeJob
{
protected:
	void fini()
	{
		DataType::clearAllScript();
	}
};

static EntityDefFiniTimeJob s_entityDefFiniTimeJob;


// -----------------------------------------------------------------------------
// Section: IEntityDescriptionHasScript
// -----------------------------------------------------------------------------



/**
 *
 */
class EntityDescriptionHasClientScript :
	public HasScriptOrTagDistributionDecider
{
public:
	EntityDescriptionHasClientScript( bool hasClientScript ) :
		hasClientScript_( hasClientScript )
	{
	}

	bool canBeOnClient( 
		const BW::string & name, const EntityDistribution & distr ) const
	{
		return hasClientScript_;
	}

private:
	bool hasClientScript_;
};


/**
 *	This class returns whether a service has script for specific entity
 *	components.
 */
class ServiceDescriptionHasPythonScript :
	public HasScriptOrTagDistributionDecider
{
public:
	bool canBeOnClient( 
		const BW::string & name, const EntityDistribution & distr ) const	
	{ 
		return false; 
	}

	bool canBeOnCell( 
		const BW::string & name, const EntityDistribution & distr ) const
	{ 
		return false; 
	}

	bool canBeOnBase( 
		const BW::string & name, const EntityDistribution & distr ) const
	{
		if (distr.baseTag() != EntityDistribution::Unspecified)
		{
			return distr.baseTag() == EntityDistribution::True;
		}

#if defined( MF_SERVER ) || defined( EDITOR_ENABLED )
		return this->fileExists( EntityDef::Constants::entitiesServicePath(), name );
#else
		return true;
#endif
	}
};




// -----------------------------------------------------------------------------
// Section: EntityDescriptionMap
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EntityDescriptionMap::EntityDescriptionMap() :
	maxClientServerPropertyCount_( 0 ),
	maxExposedClientMethodCount_( 0 ),
	maxExposedBaseMethodCount_( 0 ),
	maxExposedCellMethodCount_( 0 )
{
}


/**
 *	Destructor.
 */
EntityDescriptionMap::~EntityDescriptionMap()
{
}


/**
 *	This method parses the entity description map from a datasection.
 *
 *	@param pSection	Datasection containing the entity descriptions.
 *	@param pClientPropertyMessageRange	A pointer to the client property range.
 *	@param pClientMessageRange	A pointer to the client message ID ranges.
 *	@param pBaseMessageRange	A pointer to the baseapp message ID ranges.
 *	@param pCellMessageRange	A pointer to the cellapp message ID ranges.
 *
 *	@return true if successful, false otherwise.
 */
bool EntityDescriptionMap::parse( DataSectionPtr pSection,
		const ExposedPropertyMessageRange * pClientPropertyMessageRange,
		const ExposedMethodMessageRange * pClientMessageRange,
		const ExposedMethodMessageRange * pBaseMessageRange,
		const ExposedMethodMessageRange * pCellMessageRange )
{
	if (!pSection)
	{
		ERROR_MSG( "EntityDescriptionMap::parse: pSection is NULL\n" );
		return false;
	}

	DataSectionPtr pServicesSection;

#ifdef MF_SERVER
	pServicesSection = BWResource::openSection( "scripts/services.xml" );
#endif

	bool isOkay = true;

	int capacity =
			(pServicesSection ? pServicesSection->countChildren() : 0);

	DataSectionPtr pClientServerEntities =
		pSection->findChild( "ClientServerEntities" );
	DataSectionPtr pServerOnlyEntities =
		pSection->findChild( "ServerOnlyEntities" );

	if (pClientServerEntities)
	{
		capacity += pClientServerEntities->countChildren();

		if (pServerOnlyEntities)
		{
			capacity += pServerOnlyEntities->countChildren();
		}

		vector_.reserve( capacity );

		isOkay &= this->parseInternal( pClientServerEntities,
				EntityDescriptionHasClientScript( true ) );

		if (pServerOnlyEntities)
		{
			isOkay &= this->parseInternal( pServerOnlyEntities,
					EntityDescriptionHasClientScript( false ) );
		}
		else
		{
			WARNING_MSG( "EntityDescriptionMap::parse: scripts/entities.xml "
				"does not have a <ServerOnlyEntities> section.\n" );

		}
	}
	else
	{
		capacity += pSection->countChildren();

		NOTICE_MSG( "EntityDescriptionMap::parse: scripts/entities.xml does not "
			"have a <ClientServerEntities> section. Reverting to old-style "
			"parsing where server-only entities types were detected by "
			"non-existence of client Python script file.\n" );
		vector_.reserve( capacity );

		DescriptionVector serverOnlyDescriptions;
		isOkay &= this->parseInternal( pSection,
				HasScriptOrTagDistributionDecider(),
				&serverOnlyDescriptions );

		vector_.insert( vector_.end(),
				serverOnlyDescriptions.begin(),
				serverOnlyDescriptions.end() );
	}

	size_t size = vector_.size();

	for (size_t i = 0; i < size; ++i)
	{
		EntityDescription & desc = vector_[ i ];

		desc.index( static_cast<EntityTypeID>(i) );
		map_[ desc.name() ] = desc.index();

		if (desc.isClientType())
		{
			desc.clientIndex( static_cast<EntityTypeID>(i) );
		}
	}

#ifdef MF_SERVER
	if (isOkay && pServicesSection)
	{
		isOkay &= this->parseServices( pServicesSection, vector_.size() );
	}
#endif

	// TODO: The ClientName feature is deprecated. Removed in BW3.0
	isOkay &= this->adjustForClientName();

	this->setExposedMessageIDs( pClientMessageRange, pBaseMessageRange,
			pCellMessageRange );

	if (isOkay)
	{
		if (pClientPropertyMessageRange || pClientMessageRange ||
				pBaseMessageRange || pCellMessageRange)
		{
			INFO_MSG( "Highest exposed counts:\n" );
		}

		if (pClientPropertyMessageRange)
		{
			isOkay &= this->checkCount( "client top-level property",
							&EntityDescription::clientServerPropertyCount,
							pClientPropertyMessageRange->numSlots(), 256);
		}

		if (pClientMessageRange)
		{
			int numSlots = pClientMessageRange->numSlots();

			isOkay &= this->checkCount( "client method",
							&EntityDescription::clientMethodCount,
							numSlots, 256 * numSlots );
		}

		if (pBaseMessageRange)
		{
			int numSlots = pBaseMessageRange->numSlots();

			isOkay &= this->checkCount( "base method",
							&EntityDescription::exposedBaseMethodCount,
							numSlots, 256 * numSlots );
		}

		if (pCellMessageRange)
		{
			int numSlots = pCellMessageRange->numSlots();

			isOkay &= this->checkCount( "cell method",
							&EntityDescription::exposedCellMethodCount,
							numSlots, 256 * numSlots );
		}

		INFO_MSG( "\n" );
	}

	MD5 md5;
	this->addToMD5( md5 );
	md5.getDigest( digest_ );

	return isOkay;
}


/**
 *	This method calculates the maximum counts for exposed methods and updates
 *	exposed message ids appropriately.
 */
void EntityDescriptionMap::setExposedMessageIDs(
				const ExposedMethodMessageRange * pClientMessageRange,
				const ExposedMethodMessageRange * pBaseMessageRange,
				const ExposedMethodMessageRange * pCellMessageRange )
{
	DescriptionVector::iterator iter = vector_.begin();

	while (iter != vector_.end())
	{
		maxClientServerPropertyCount_ = std::max( maxClientServerPropertyCount_,
					iter->clientServerPropertyCount() );

		maxExposedClientMethodCount_ = std::max( maxExposedClientMethodCount_,
					iter->client().exposedSize() );

		maxExposedBaseMethodCount_ = std::max( maxExposedBaseMethodCount_,
					iter->base().exposedSize() );

		maxExposedCellMethodCount_ = std::max( maxExposedCellMethodCount_,
					iter->cell().exposedSize() );

		++iter;
	}

	iter = vector_.begin();

	while (iter != vector_.end())
	{
		iter->setExposedMsgIDs(
				maxExposedClientMethodCount_, pClientMessageRange,
				maxExposedBaseMethodCount_, pBaseMessageRange,
				maxExposedCellMethodCount_, pCellMessageRange );

		++iter;
	}
}


/**
 *	This method is used by EntityDescription::parse(), to parse the different
 *	sections of entities.xml.
 *
 *	@param pSection A section containing the entity names.
 *	@param hasScriptDecider An object that decides whether the different
 *		component of an entity has script associated with it.
 *	@param pServerOnlyEntities If not NULL, server-only entity types are
 *		appended here instead.
 */
bool EntityDescriptionMap::parseInternal( DataSectionPtr pSection,
		const IEntityDistributionDecider & hasScriptDecider,
		DescriptionVector * pServerOnlyEntities )
{
	bool isOkay = true;

	DataSection::iterator iter = pSection->begin();

	while (iter != pSection->end())
	{
		DataSectionPtr pSubSection = *iter;
		EntityDescription desc;

		BW::string typeName = pSubSection->sectionName();

		if (desc.parse( typeName, /*componentName*/ BW::string(),
					&hasScriptDecider ))
		{
			if (!desc.isClientType() && (pServerOnlyEntities != NULL))
			{
				pServerOnlyEntities->push_back( desc );
			}
			else
			{
				vector_.push_back( desc );
			}
		}
		else
		{
			ERROR_MSG( "EntityDescriptionMap: "
				"Failed to load or parse def for entity type %s\n",
				typeName.c_str() );

			isOkay = false;
		}

		++iter;
	}

	return isOkay;
}


/**
 *
 */
bool EntityDescriptionMap::parseServices( DataSectionPtr pSection,
		EntityTypeID initialTypeID )
{
	ServiceDescriptionHasPythonScript hasScriptDecider;

	int size = pSection->countChildren();

	MF_ASSERT( initialTypeID == vector_.size() );
	vector_.resize( vector_.size() + size );

	for (int i = 0; i < size; ++i)
	{
		EntityTypeID typeID = initialTypeID + i;

		DataSectionPtr pSubSection = pSection->openChild( i );
		EntityDescription & desc = vector_[ typeID ];

		BW::string serviceName = pSubSection->sectionName();

		if (!desc.parseService( serviceName, /*componentName*/ BW::string(),
					&hasScriptDecider ))
		{
			ERROR_MSG( "EntityDescriptionMap::parseServices: "
						"Failed to load or parse def for service %s\n",
					serviceName.c_str() );

			return false;
		}

		BW::string	activeOnServerModes = pSubSection->readString( "activeOnServerModes" );
		if (activeOnServerModes != "any")
		{
			desc.setActiveOnServerModes( activeOnServerModes );
		}

		desc.index( typeID );
		map_[ desc.name() ] = desc.index();
	}

	return true;
}


/**
 *	This method adjusts the clientIndex if the ClientName setting is different
 *	from the server's entity type.
 *
 *	NOTE: This feature is deprecated and will likely be removed.
 */
bool EntityDescriptionMap::adjustForClientName()
{
	bool isOkay = true;

	DescriptionVector::iterator iter = vector_.begin();

	while (iter != vector_.end())
	{
		// if name_ != clientName_
		if (iter->canBeOnClient() && !iter->isClientType())
		{
			EntityTypeID id;

			if (this->nameToIndex( iter->clientName(), id ))
			{
				const EntityDescription & alias = this->entityDescription( id );

				if (alias.clientIndex() != INVALID_ENTITY_TYPE_ID)
				{
					if ((alias.clientServerPropertyCount() ==
							iter->clientServerPropertyCount()) &&
						(alias.clientMethodCount() ==
						 	iter->clientMethodCount()))
					{
						iter->clientIndex( alias.clientIndex() );
						INFO_MSG( "%s is aliased as %s\n",
								iter->name().c_str(),
								alias.name().c_str() );
					}
					else
					{
						ERROR_MSG( "EntityDescriptionMap::parse: "
									"%s has mismatched ClientName %s\n",
								iter->name().c_str(),
								iter->clientName().c_str() );
						ERROR_MSG( "EntityDescriptionMap::parse: "
								"There are %d methods and %d props instead of "
								"%d and %d\n",
							alias.clientMethodCount(),
							alias.clientServerPropertyCount(),
						 	iter->clientMethodCount(),
							iter->clientServerPropertyCount() );
						isOkay = false;
					}
				}
				else
				{
					ERROR_MSG( "EntityDescriptionMap::parse: "
								"%s has server-only ClientName %s\n",
							iter->name().c_str(),
							iter->clientName().c_str() );
					isOkay = false;
				}
			}
			else
			{
				ERROR_MSG( "EntityDescriptionMap::parse: "
							"%s has invalid ClientName %s\n",
						iter->name().c_str(),
						iter->clientName().c_str() );
				isOkay = false;
			}
		}

		++iter;
	}

	return isOkay;
}


/**
 *	This method is used to check whether we have exceeded any property or
 *	method limits.
 */
bool EntityDescriptionMap::checkCount( const char * description,
		unsigned int (EntityDescription::*fn)() const,
		int maxEfficient, int maxAllowed ) const
{
	if (!vector_.empty())
	{
		const EntityDescription * pBest = &vector_[0];
		unsigned int maxCount = 0;

		for (EntityTypeID typeID = 0; typeID < this->size(); ++typeID)
		{
			const EntityDescription & eDesc = this->entityDescription( typeID );

			if ((eDesc.*fn)() > maxCount)
			{
				pBest = &eDesc;
				maxCount = (eDesc.*fn)();
			}
		}

		if (maxCount <= (unsigned int)maxAllowed)
		{
			INFO_MSG( "\t%s: %s count = %d. Efficient to %d (limit is %d)\n",
					pBest->name().c_str(), description, maxCount,
					maxEfficient, maxAllowed );
		}
		else
		{
			ERROR_MSG( "EntityDescriptionMap::checkCount: "
					"%s exposed %s count of %d is more than allowed (%d)\n",
				pBest->name().c_str(), description, maxCount, maxAllowed );
			return false;
		}
	}

	return true;
}


/**
 *	This method returns the number of entities.
 *
 *	@return Number of entities.
 */
int EntityDescriptionMap::size() const
{
	return static_cast<int>(vector_.size());
}


/**
 *	This method returns the entity description with the given index.
 */
const EntityDescription& EntityDescriptionMap::entityDescription(
		EntityTypeID index ) const
{
	IF_NOT_MF_ASSERT_DEV( index < EntityTypeID(vector_.size()) )
	{
		MF_EXIT( "invalid entity type id index" );
	}

	return vector_[index];
}


/**
 *	This method maps an entity class name to an index.
 *
 *	@param name		Name of the entity class.
 *	@param index	The index is returned here.
 *
 *	@return true if found, false otherwise.
 */
bool EntityDescriptionMap::nameToIndex(const BW::string& name,
	   	EntityTypeID& index) const
{
	DescriptionMap::const_iterator it = map_.find(name);

	if(it != map_.end())
	{
		index = it->second;
		return true;
	}

	return false;
}


/**
 *	This method adds this object to the input MD5 object.
 */
void EntityDescriptionMap::addToMD5( MD5 & md5 ) const
{
	DescriptionVector::const_iterator iter = vector_.begin();

	while (iter != vector_.end())
	{
		// Ignore the server side only ones.
		if (iter->isClientType())
		{
			iter->addToMD5( md5 );
		}

		iter++;
	}
}


/**
 *	This method adds this object's persistent properties to the input MD5
 *	object.	
 */
void EntityDescriptionMap::addPersistentPropertiesToMD5( MD5 & md5 ) const
{
	DescriptionVector::const_iterator iter = vector_.begin();

	// Assumes typeID is its order in the vector.
	while (iter != vector_.end())
	{
		if (iter->isPersistent())
		{
			iter->addPersistentPropertiesToMD5( md5 );
		}

		iter++;
	}
}


/**
 *	This method clears all the entity descriptions stored in this object.
 */
void EntityDescriptionMap::clear()
{
	vector_.clear();
	map_.clear();
}

bool EntityDescriptionMap::isEntity( const BW::string& name ) const
{
	return map_.find( name ) != map_.end();
}

void EntityDescriptionMap::getNames( BW::vector< BW::string > & names ) const
{
	DescriptionMap::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		if (!vector_[ iter->second ].isService())
		{
			names.push_back( iter->first );
		}

		++iter;
	}
}

BW_END_NAMESPACE

/* entity_description_map.cpp */
