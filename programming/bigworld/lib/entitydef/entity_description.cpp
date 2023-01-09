#include "pch.hpp"

#include "entity_description.hpp"

#include "constants.hpp"
#include "data_type.hpp"
#include "script_data_sink.hpp"
#include "script_data_source.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/watcher.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"

#include "float.h"

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "entity_description.ipp"
#endif


// -----------------------------------------------------------------------------
// Section: EntityDescriptionHasPythonScript
// -----------------------------------------------------------------------------

namespace
{

/**
 *	Static helper function to see if the named python file exists
 */
bool pythonScriptExists( const BW::string & path )
{
	MultiFileSystemPtr pFS = BWResource::instance().fileSystem();

	const char * exts[] = { ".py", ".pyc", ".pyo", ".pyd" };
	for (unsigned int i = 0; i < sizeof(exts)/sizeof(exts[0]); i++)
	{
		IFileSystem::FileInfo finfo;
		if (pFS->getFileType( path + exts[i], &finfo ) == pFS->FT_FILE &&
			finfo.size != 0)
				return true;
	}

	return false;
}

} // anonymous namespace


EntityDistribution::EntityDistribution( const DataSectionPtr & pDistribution ) :
	baseTag_( Unspecified ),
	cellTag_( Unspecified ),
	clientTag_( Unspecified )
{
	if (pDistribution)
	{
		cellTag_ = pDistribution->readBool( "Cell" ) ? True : False;
		baseTag_ = pDistribution->readBool( "Base" ) ? True : False;
		clientTag_ = pDistribution->readBool( "Client" ) ? True : False;
	}
}

bool HasScriptOrTagDistributionDecider::canBeOnCell(
	const BW::string & name, const EntityDistribution & distr ) const
{
	if (distr.cellTag() != EntityDistribution::Unspecified)
	{
		return distr.cellTag() == EntityDistribution::True;
	}

#if defined( MF_SERVER ) || defined( EDITOR_ENABLED )
	return this->fileExists( EntityDef::Constants::entitiesCellPath(), name );
#else
	// In the client, don't check for existence of script files, just
	// assume entities have cell and base scripts. This has to be done
	// because the final game should not have the server scripts shipped.
	return true;
#endif
}

bool HasScriptOrTagDistributionDecider::canBeOnBase(
	const BW::string & name, const EntityDistribution & distr ) const
{
	if (distr.baseTag() != EntityDistribution::Unspecified)
	{
		return distr.baseTag() == EntityDistribution::True;
	}

#if defined( MF_SERVER ) || defined( EDITOR_ENABLED )
	return this->fileExists( EntityDef::Constants::entitiesBasePath(), name );
#else
	return true;
#endif
}

bool HasScriptOrTagDistributionDecider::canBeOnClient(
	const BW::string & name, const EntityDistribution & distr ) const
{
	if (distr.clientTag() != EntityDistribution::Unspecified)
	{
		return distr.clientTag() == EntityDistribution::True;
	}

	return this->fileExists( EntityDef::Constants::entitiesClientPath(), name );
}

BW::string HasScriptOrTagDistributionDecider::getPath( const char * path,
													const BW::string & name )
{
	return BW::string( path ) + "/" + name;
}

bool HasScriptOrTagDistributionDecider::fileExists( const char * path,
												const BW::string & name )
{
	return pythonScriptExists( getPath( path, name ) );
}


// -----------------------------------------------------------------------------
// Section: EntityDescription
// -----------------------------------------------------------------------------

/**
 *	The constructor.
 */
EntityDescription::EntityDescription() :
	index_( INVALID_ENTITY_TYPE_ID ),
	clientIndex_( INVALID_ENTITY_TYPE_ID ),
	canBeOnCell_( true ),
	canBeOnBase_( true ),
	canBeOnClient_( true ),
	hasComponents_( false ),
	isService_( false ),
	isPersistent_( true ),
	forceExplicitDBID_( false ),
	volatileInfo_(),
	internalNetworkCompressionType_( BW_COMPRESSION_DEFAULT_INTERNAL ),
	externalNetworkCompressionType_( BW_COMPRESSION_DEFAULT_EXTERNAL ),
	clientServerProperties_(),
	cell_(),
	base_(),
	client_(),
	numEventStampedProperties_( 0 ),
	numLatestChangeOnlyMembers_( 0 ),
	appealRadius_( 0.f ),
	shouldSendDetailedPosition_( false ),
	isManualAoI_( false )
{
}


/**
 *	Destructor.
 */
EntityDescription::~EntityDescription()
{
}


/**
 *	This method parses an entity description from a datasection.
 *
 *	@param name		The name of the Entity type.
 *	@param pHasScriptDecider	This object is used to decide whether different
 *		components of an entity has script. This is NULL if an interface is
 *		being parsed. (i.e. this is not the final pass).
 *
 *	@return true if successful, false otherwise.
 */
bool EntityDescription::parse( const BW::string & name,
		const BW::string & componentName,
		const IEntityDistributionDecider * pDistributionDecider )
{
	BW::string filename = this->getDefsDir() + "/" + name + ".def";
	DataSectionPtr pSection = BWResource::openSection( filename );

	if (!pSection)
	{
		ERROR_MSG( "EntityDescription::parse: Could not open %s\n",
				filename.c_str() );
		return false;
	}

	BW::string parentName = pSection->readString( "Parent" );

	if (!parentName.empty())
	{
		if (!this->parse( parentName, componentName, NULL ))
		{
			ERROR_MSG( "EntityDescription::parse: "
						"Could not parse %s, parent of %s\n",
					parentName.c_str(), name.c_str() );
			return false;
		}
	}

	name_ = name;

	if (!isService_)
	{
		// The ClientName tag is optional. It allows us to specify a different
		// class name for the client. If it is not present, it defaults to the
		// same as the server name.

		clientName_ = pSection->readString( "ClientName", clientName_ );
	}

	if (pDistributionDecider)
	{
		EntityDistribution distr( pSection->openSection( "Distribution" ) );
		
		canBeOnCell_ = pDistributionDecider->canBeOnCell( name_, distr );
		canBeOnBase_ = pDistributionDecider->canBeOnBase( name_, distr );
		canBeOnClient_ = pDistributionDecider->canBeOnClient( name_, distr );

		if (!canBeOnClient_)
		{
			clientName_ = BW::string();
		}
		else if (clientName_.empty())
		{
			clientName_ = name_;
		}
		else
		{
			NOTICE_MSG( "Support for <ClientName> is deprecated. It may be "
				"removed in BigWorld 3.0. Please contact "
				"support@bigworldtech.com if you would like this feature maintained\n" );
		}
	}

	bool result = this->parseInterface( pSection, name_.c_str(),
			componentName );

	if (pDistributionDecider && !isService_)
	{
		// If not doing an interface, pHasScriptDecider is not NULL.

		// Adjust the detail levels. Because LoDs can be defined in any order
		// and derived interfaces can add more, we may not know the actual
		// level when we first parse a property. Here we convert from the index
		// to the actual level.

		int levels[ MAX_DATA_LOD_LEVELS + 1 ];

		for (int i = 0; i < MAX_DATA_LOD_LEVELS + 1; ++i)
		{
			levels[i] = lodLevels_.getLevel( i ).index();
		}

		for (unsigned int i = 0; i < this->propertyCount(); ++i)
		{
			DataDescription * pDD = this->property( i );

			switch (pDD->detailLevel())
			{
				case DataLoDLevel::NO_LEVEL:
					break;

				case DataLoDLevel::OUTER_LEVEL:
					pDD->detailLevel( lodLevels_.size() - 1 );
					break;

				default:
					pDD->detailLevel( levels[ pDD->detailLevel() ] );
					break;
			}
		}
	}

	// Check that entities without cell scripts don't have cell properties
	// This is important because DBApp puts all persistent data (including
	// cell data) onto the stream regardless of whether they have cell script
	// or not. While other parts of the code skips the streaming of cell data
	// if it doesn't have a script.
	// But if it's client-only then we don't care. Important because client-only
	// entities have ALL_CLIENTS, OTHER_CLIENTS or OWN_CLIENT flags which
	// are also cell data.
	if (!this->canBeOnCell() && !this->isClientOnlyType())
	{
		for ( Properties::const_iterator i = properties_.begin();
			i != properties_.end(); ++i )
		{
			if (i->isCellData())
			{
				WARNING_MSG( "Entity '%s' does not have a cell script but "
					"has cell property '%s'.\n", name_.c_str(),
					i->name().c_str() );
				// result = false;
				break;
			}
		}
	}

	if (result && !this->canBeOnCell() && (this->cell().size() > 0))
	{
		WARNING_MSG( "Entitiy '%s' does not have a cell script but "
					"has cell method '%s'\n",
				name_.c_str(),
				this->cell().internalMethod( 0 )->name().c_str() );
		// result = false;
	}

	if (result && !this->canBeOnBase())
	{
		if (isService_)
		{
			ERROR_MSG( "Service '%s' does not have a service script.\n",
					name_.c_str() );
			result = false;
		}
		else if (this->base().size() > 0)
		{
			WARNING_MSG( "Entity '%s' does not have a base script but "
						"has base method '%s'\n",
				name_.c_str(),
				this->base().internalMethod( 0 )->name().c_str() );
			// result = false;
		}
	}

	if (result && !this->canBeOnClient() && (this->client().size() > 0))
	{
		ERROR_MSG( "Entity '%s' does not have a client script but "
					"has client methods\n",
				name_.c_str() );
		result = false;
	}

	if (this->isService() || this->isClientOnlyType())
	{
		isPersistent_ = false;
	}

	result &= initCompressionTypes( pSection->findChild( "NetworkCompression" ),
			internalNetworkCompressionType_, externalNetworkCompressionType_ );

	if ((appealRadius_ > 0.f) && volatileInfo_.shouldSendPosition() &&
		!shouldSendDetailedPosition_)
	{
		INFO_MSG( "EntityDescription::parse: Entity type %s has an "
				"AppealRadius of %.2f and Volatile position.\n"
				"If an entity's aoiRadius plus this AppealRadius (%.2f) "
				"exceeds maxAoIRadius, the position sent to the client "
				"will be sent with full-precision, consuming twice as much "
				"bandwidth as normal volatile position updates.\n",
			name_.c_str(), appealRadius_, appealRadius_ );
	}

	return result;
}


/**
 *
 */
bool EntityDescription::parseService( const BW::string & name,
		const BW::string & componentName,
		const IEntityDistributionDecider * pDistributionDecider )
{
	isService_ = true;

	return this->parse( name, componentName, pDistributionDecider );
}


/**
 *	This method parses a data section for the properties and methods associated
 *	with this entity description.
 */
bool EntityDescription::parseInterface( DataSectionPtr pSection,
		const char * interfaceName, const BW::string & componentName )
{
	if (!pSection)
	{
		return false;
	}

	bool result = true;

	forceExplicitDBID_ = pSection->readBool( "ExplicitDatabaseID",
		 forceExplicitDBID_ );

	if (!isService_)
	{
		isPersistent_ = pSection->readBool( "Persistent", isPersistent_ );

		result &=
			lodLevels_.addLevels( pSection->openSection( "LoDLevels" ) );

		result &=
			this->BaseUserDataObjectDescription::parseInterface( pSection, 
				interfaceName, componentName );

		result &=
			volatileInfo_.parse( pSection->openSection( "Volatile" ) );

		appealRadius_ = pSection->readFloat( "AppealRadius", appealRadius_ );

		shouldSendDetailedPosition_ = pSection->readBool(
			"ShouldSendDetailedVolatilePosition", shouldSendDetailedPosition_ );

		DataSectionPtr pManualAOISection = pSection->findChild( "IsManualAOI" );
		if (pManualAOISection.exists())
		{
			WARNING_MSG( "EntityDescription::parse: For entity type %s, "
					"IsManualAOI is deprecated, use IsManualAoI instead\n",
				name_.c_str() );
			isManualAoI_ = pManualAOISection->asBool( false );
		}

		// This will override any potential setting from the deprecated
		// isManualAOI above.
		isManualAoI_ = pSection->readBool( "IsManualAoI", isManualAoI_ );
	}
	else
	{
		result &=
			this->parseImplements( pSection->openSection( "Implements" ),
					componentName );
	}

	result &=
		this->parseMethods( pSection, interfaceName, componentName );

	result &=
		this->parseTempProperties( pSection->openSection( "TempProperties" ),
			interfaceName, componentName );

	result &=
		this->parseComponents( pSection->openSection( "Components" ),
			interfaceName );

	return result;
}


/**
 * This method parses a data section for the set of components that make up
 * this entity's functionality.
 */
bool EntityDescription::parseComponents( DataSectionPtr pComponents,
		const char * interfaceName )
{
	if (!pComponents)
	{
		return true;
	}

	// cache for component definition sections
	typedef BW::unordered_map<BW::string, DataSectionPtr> DataSectionMap;
	DataSectionMap componentSections;

	for (DataSectionIterator itComponent = pComponents->begin();
		 itComponent != pComponents->end();
		 ++itComponent)
	{
		DataSectionPtr pComponent = *itComponent;
		const BW::string componentType = pComponent->sectionName();

		DataSectionMap::iterator itComponentDef = 
			componentSections.find(componentType);

		if (componentSections.end() == itComponentDef)
		{
			BW::string filename( EntityDef::Constants::componentsDefsPath() );
			filename += "/" + componentType + ".def";

			DataSectionPtr pComponentFile = BWResource::openSection( filename );

			if (pComponentFile == NULL)
			{
				ERROR_MSG( "EntityDescription::parseComponents: "
							"Failed to open component def file '%s'.\n",
						filename.c_str() );
				return false;
			}

			itComponentDef = 
				componentSections.insert(
					std::make_pair( componentType, pComponentFile ) ).first;
		}
		// optional alias for component
		const BW::string componentName = 
			 pComponent->readString("name", componentType, /*flags*/0);

		if (!componentNames_.insert( componentName.c_str() ).second)
		{
			ERROR_MSG( "EntityDescription::parseComponents: "
							"Component with name '%s' is duplicated.\n",
						componentName.c_str() );
			return false;
		}

		if (!this->parseComponent( itComponentDef->second, interfaceName,
					componentName.c_str() ))
		{
			ERROR_MSG( "EntityDescription::parseComponents: "
						"Failed to parse component '%s' for entity '%s'.\n",
					componentName.c_str(), interfaceName );
			return false;
		}
	}

	return true;
}


/**
 * This method parses a data section for a component (a reusable set of
 * functionality for an entity, made up of properties and methods).
 */
bool EntityDescription::parseComponent( DataSectionPtr pSection,
		const char * interfaceName, const BW::string & componentName )
{
	INFO_MSG( "EntityDescription::parseComponent: "
				"adding component '%s' to definition for entity '%s'.\n",
			componentName.c_str(), interfaceName );

	hasComponents_ = true;

	bool result = true;

	result &=
		this->parseProperties( pSection->openSection( "Properties" ),
				componentName );

	result &=
		this->parseTempProperties( pSection->openSection( "TempProperties" ),
				interfaceName, componentName );

	result &=
		this->parseMethods( pSection, interfaceName, componentName );

	return result;
}


/**
 * This method parses a data section for the methods associated with this
 * entity description.
 */
bool EntityDescription::parseMethods( DataSectionPtr pSection,
	   const char * interfaceName, const BW::string & componentName )
{
	bool result = true;

	result &=
		this->parseBaseMethods(
				pSection->openSection( isService_ ? "Methods" : "BaseMethods" ),
				interfaceName, componentName );

	if (!isService_)
	{
		result &=
			this->parseCellMethods(
					pSection->openSection( "CellMethods" ), interfaceName,
					componentName );

		result &=
			this->parseClientMethods(
					pSection->openSection( "ClientMethods" ), interfaceName,
					componentName );
	}

	return result;
}


/**
 *	This method parses a data section for the properties associated with this
 *	entity description.
 *
 *	@param pProperties	The datasection containing the properties.
 *
 *	@return true if successful, false otherwise.
 */
bool EntityDescription::parseProperties( DataSectionPtr pProperties,
		const BW::string & componentName )
{
	if (!pProperties)
	{
		return true;
	}

	if (isService_)
	{
		WARNING_MSG( "EntityDescription::parseProperties: %s: "
							"Services do not support properties.\n",
						name_.c_str() );

		return true;
	}

	bool propertiesOk = true;

	for (DataSectionIterator iter = pProperties->begin();
			iter != pProperties->end();
			++iter)
	{
		DataDescription dataDescription;

		if (!dataDescription.parse( *iter, name_, componentName,
				&numLatestChangeOnlyMembers_ ))
		{
			WARNING_MSG( "Error parsing properties for %s\n",
					name_.c_str() );
			return false;
		}

#ifndef EDITOR_ENABLED
		if (dataDescription.isEditorOnly())
		{
			continue;
		}
#endif

		int index = static_cast<int>(properties_.size());
		int clientServerIndex = -1;

		PropertyMap & propertyMap 
				= this->getComponentProperties( componentName.c_str() );
		PropertyMap::const_iterator propIter =
				propertyMap.find( dataDescription.name().c_str() );

		if (propIter != propertyMap.end())
		{
			INFO_MSG( "EntityDescription::parseProperties: "
					"property %s%s.%s is being overridden.\n",
				name_.c_str(),
				componentName.empty() ? "" : ( "." + componentName ).c_str(),
				dataDescription.name().c_str() );
			index = propIter->second;
			if (dataDescription.isClientServerData())
			{
				clientServerIndex = 
					properties_[ index ].clientServerFullIndex();
			}
		}

		dataDescription.index( index );
		propertyMap[dataDescription.name().c_str()] = dataDescription.index();

#ifdef EDITOR_ENABLED
		DataSectionPtr widget = (*iter)->openSection( "Widget" );
		if (!!widget)
		{
			dataDescription.widget( widget );
		}
#endif

		if (dataDescription.isClientServerData())
		{
			if (clientServerIndex != -1)
			{
				dataDescription.clientServerFullIndex( clientServerIndex );
				clientServerProperties_[ clientServerIndex ] =
					dataDescription.index();
			}
			else
			{
				dataDescription.clientServerFullIndex(
							static_cast<int>(clientServerProperties_.size()) );
				clientServerProperties_.push_back(
								dataDescription.index() );
			}

			int clientSafety = dataDescription.clientSafety();

			if (clientSafety & DataType::CLIENT_UNSAFE)
			{
				WARNING_MSG( "%s.%s is a client-server property that "
								"has a PYTHON member "
								"(potential security hole)\n",
							name_.c_str(), dataDescription.name().c_str() );
			}

			if (clientSafety & DataType::CLIENT_UNUSABLE)
			{
				ERROR_MSG( "%s.%s is a client-server property that "
								"has a MAILBOX member "
								"(type is not able to be sent to or from client)\n",
							name_.c_str(), dataDescription.name().c_str() );

				propertiesOk = false;
			}

		}

		if (dataDescription.isOtherClientData())
		{
			int detailLevel;

			if (lodLevels_.findLevel( detailLevel,
						(*iter)->openSection( "DetailLevel" ) ))
			{
				dataDescription.detailLevel( detailLevel );
			}
			else
			{
				ERROR_MSG( "EntityDescription::parseProperties: "
										"Invalid detail level for %s.\n",
								dataDescription.name().c_str() );

				return false;
			}

			dataDescription.eventStampIndex( numEventStampedProperties_ );
			numEventStampedProperties_++;
		}

		if (index == int(properties_.size()))
		{
			properties_.push_back( dataDescription );
		}
		else
		{
			properties_[index] = dataDescription;
		}
	}

	if (!propertiesOk)
	{
		return false;
	}

	this->allocateClientServerFullIndexes();

	return true;
}


/**
 *	This method parses a data section for the methods associated with this
 *	entity description that are implemented by the client.
 *
 *	@param pMethods The datasection containing the methods.
 *
 *	@return true if successful, false otherwise.
 */
bool EntityDescription::parseClientMethods( DataSectionPtr pMethods,
		const char * interfaceName, const BW::string & componentName )
{
	if (pMethods)
	{
		return client_.init( pMethods, MethodDescription::CLIENT,
			interfaceName, componentName, &numLatestChangeOnlyMembers_ );
	}

	return true;
}


/**
 *	This method parses a data section for the methods associated with this
 *	entity description that are implemented by the cell.
 *
 *	@param pMethods	The data section containing the methods.
 *
 *	@return true if successful, false otherwise.
 */
bool EntityDescription::parseCellMethods( DataSectionPtr pMethods,
		const char * interfaceName, const BW::string & componentName )
{
	if (!pMethods) return true;

	return cell_.init( pMethods, MethodDescription::CELL, interfaceName,
			componentName );
}


/**
 *	This method parses a data section for the methods associated with this
 *	entity description that are implemented by the base.
 *
 *	@param pMethods	The data section containing the methods.
 *
 *	@return true if successful, false otherwise.
 */
bool EntityDescription::parseBaseMethods( DataSectionPtr pMethods,
		const char * interfaceName, const BW::string & componentName )
{
	if (!pMethods) return true;

	return base_.init( pMethods, MethodDescription::BASE, interfaceName,
			componentName );
}


/**
 *	This method parses a data section for temporary properties.
 *
 *	@param pTempProps The data section containing the temporary properties.
 *
 *	@return true if successful, false otherwise.
 */
bool EntityDescription::parseTempProperties( DataSectionPtr pTempProps,
		const char * interfaceName, const BW::string & componentName )
{
	if (!pTempProps)
	{
		return true;
	}


	int numProps = pTempProps->countChildren();

	for (int i = 0; i < numProps; ++i)
	{
		tempProperties_.insert( pTempProps->childSectionName( i ) );
	}

	return true;
}


namespace {
/**
 *	Sort client-server properties list helper class
 */
class ClientServerPropertiesSortHelper
{
	// Matches BaseUserDataObjectDescription::Properties
	typedef BW::vector< DataDescription > Properties;

public:
	ClientServerPropertiesSortHelper( const Properties & properties ) :
		properties_( properties ) {}

	bool operator()( const unsigned int &propertyIndex1,
					 const unsigned int &propertyIndex2 )
	{
		int16 size1 = properties_[ propertyIndex1 ].streamSize();
		int16 size2 = properties_[ propertyIndex2 ].streamSize();
		// Both variable size
		if (size1 < 0 && size2 < 0)
		{
			return -size1 < -size2;
		}
		// Both fixed-size
		if (size1 >=0 && size2 >= 0)
		{
			return size1 < size2;
		}
		// Otherwise, one fixed and one variable.
		// Fixed (+ve) is always lesser.
		return size1 > size2;
	}

private:
	const Properties & properties_;
};
}


/**
 *	Helper method to allocate clientServerFullIndex values in increasing
 *	order of bandwidth usage/efficiency.
 */
// The goal is to sort by fixed-size, followed by variable size, so that
// in the event that excess properties are multiplexed through
// PROPERTY_CHANGE_ID_SINGLE, those properties will already be large
// fixed-size properties or have a variable size, saving bandwidth usage.

// It's a stable sort, so that equal-sized methods remain in the same
// order as their declarations.
void EntityDescription::allocateClientServerFullIndexes()
{
	std::stable_sort( clientServerProperties_.begin(),
		clientServerProperties_.end(),
		ClientServerPropertiesSortHelper( properties_ ) );
	for (unsigned int i = 0; i < clientServerProperties_.size(); ++i )
	{
		properties_[ clientServerProperties_[ i ] ].clientServerFullIndex( i );
	}
}


/**
 *	This method supersedes this entity description with a newer one.
 */
void EntityDescription::supersede( MethodDescription::Component component )
{
	if (component == MethodDescription::BASE)
	{
		base_.supersede();
	}
	else if (component == MethodDescription::CELL)
	{
		cell_.supersede();
	}
	else
	{
		WARNING_MSG("only baseApp and cellApp can call supersede method. Ignored\n");
	}
}

/**
  * Tell entity description which directory it should try read
  * the .def files from
  */
const BW::string EntityDescription::getDefsDir() const
{
	return isService_ ?
			EntityDef::Constants::servicesDefsPath() :
			EntityDef::Constants::entitiesDefsPath();
}


/**
 *	This method is used for error checking. It checks whether the input class
 *	supports all of the necessary methods. Methods that are handled by
 *	compnents are not checked, as they will not be implemented in the scripts.
 */
bool EntityDescription::checkMethods( const MethodDescriptionList & methods,
		ScriptObject pClass, bool warnOnMissing ) const
{
	MethodDescriptionList::const_iterator iter = methods.begin();
	bool isOkay = true;

	while (iter != methods.end())
	{
		if (!iter->isComponentised() && !iter->isMethodHandledBy( pClass ))
		{
			if (warnOnMissing)
			{
				ERROR_MSG( "EntityDescription::checkMethods: "
					"class %s does not have method %s\n",
					this->name().c_str(), iter->name().c_str() );
			}

			isOkay = false;
		}

		iter++;
	}

	return isOkay;
}

/**
 *	This method checks whether the input class
 *	has correct .
 */
bool EntityDescription::checkExposedMethods( bool isProxy ) const
{
	bool isOkay = true;
	for (uint i = 0; i < this->base().exposedSize(); ++i)
	{
		const MethodDescription * pMethod = this->base().exposedMethod( i );
		if (!isProxy)
		{
			WARNING_MSG( "EntityDescription::checkExposedMethods: "
				"Base method %s in non-proxy entity %s "
				"cannot be exposed\n",
				pMethod->name().c_str(), this->name().c_str() );
			isOkay = false;
		}
		else
		{
			if (pMethod->isExposedToAllClientsOnly())
			{
				WARNING_MSG( "EntityDescription::checkExposedMethods: "
					"Base method %s in entity %s "
					"cannot be exposed with ALL_CLIENTS flag\n",
					pMethod->name().c_str(), this->name().c_str() );
				isOkay = false;
			}
		}
	}

	for (uint i = 0; i < this->cell().exposedSize(); ++i)
	{
		const MethodDescription *pMethod = this->cell().exposedMethod( i );
		if (!isProxy)
		{
			if (pMethod->isExposedToOwnClientOnly())
			{
				WARNING_MSG( "EntityDescription::checkExposedMethods: "
					"Cell method %s in non-proxy entity %s "
					"cannot be exposed with OWN_CLIENT flag\n",
					pMethod->name().c_str(), this->name().c_str() );
				isOkay = false;
			}
		}
		else
		{
			if (pMethod->isExposedToDefault())
			{
				WARNING_MSG( "EntityDescription::checkExposedMethods: "
					"Cell method %s in proxy entity %s "
					"must be exposed explicitly with ALL_CLIENTS "
					"or OWN_CLIENT flag\n",
					pMethod->name().c_str(), this->name().c_str() );
				isOkay = false;
			}
		}
	}
	return isOkay;
}


/**
 *	This method checks if the entity description has a specific component
 */
bool EntityDescription::hasComponent( const BW::string & componentName ) const
{
	return componentNames_.find( componentName ) != componentNames_.end();
}


/**
 *	This method converts a StreamContentType (an enum describing the type of
 *	data in the stream) into a set of flags usable by EntityDescription to
 *	filter that type of data. The purpose of StreamContentType is to keep the
 *	details of the flags abstracted away from what they represent.
 *	TODO Might need to populate data for 'tagged'
 *	TODO Set domains for two unimplemented content types below.
 */
/*static*/ int EntityDescription::getDataDomains(
		StreamContentType contentType )
{
	int dataDomains = 0;

	switch(contentType)
	{
		case BASE_ENTITY_BACKUP:
			dataDomains = BASE_DATA;
			break;

		case BASE_ENTITY_OFFLOAD:
			dataDomains = BASE_DATA;
			break;

		case CELL_CREATION:
			dataDomains = CELL_DATA;
			break;

		case CELL_GHOST_CREATION:
			dataDomains = CELL_DATA | ONLY_OTHER_CLIENT_DATA;
			break;

		case CELL_ENTITY_BACKUP:
			dataDomains = CELL_DATA;
			break;

		case CELL_ENTITY_OFFLOAD:
			dataDomains = CELL_DATA;
			break;

		case CLIENT_ENTITY_CELL_CREATION:
			CRITICAL_MSG( "Unimplemented\n" );
			break;

		case CLIENT_PLAYER_CREATION:
			dataDomains = FROM_BASE_TO_CLIENT_DATA;
			break;

		case CLIENT_PLAYER_CELL_CREATION:
			dataDomains = FROM_CELL_TO_CLIENT_DATA;
			break;

		case CLIENT_ENTITY_LOD_DATA:
			CRITICAL_MSG( "Unimplemented\n" );
			break;

		case DATABASE_ENTITY:
			dataDomains = BASE_DATA | ONLY_PERSISTENT_DATA;
			break;
	}

	return dataDomains;
}


/**
 *	This simple static helper function decides whether data should be sent based
 *	on the current pass.
 */
/*static*/ bool EntityDescription::shouldConsiderData( int pass,
		const DataDescription * pDD, int dataDomains )
{
	// This array is used to identify what data to add on each pass.
	const bool PASS_FILTER[4][2] =
	{
		{ true,  false },	// Base and not client
		{ true,  true },	// Base and client
		{ false, true },	// Cell and client
		{ false, false },	// Cell and not client
	};

	return (PASS_FILTER[ pass ][0] == pDD->isBaseData()) &&
			(PASS_FILTER[ pass ][1] == pDD->isClientServerData()) &&
			(pDD->isOtherClientData() ||
								!(dataDomains & ONLY_OTHER_CLIENT_DATA)) &&
			(pDD->isPersistent() || !(dataDomains & ONLY_PERSISTENT_DATA));
}


/**
 *	This simple static helper function decides whether a pass should be skipped
 *	based on the desired data domains that want to be streamed.
 *
 *	If the EXACT_MATCH flag is set, the pass is skipped unless all flags match
 *	exactly. If the EXACT_MATCH is not set, only one of the set flags passed in
 *	needs to match a flag for that pass.
 *
 *	The passes are as follows:
 *		0. Base data and not client data
 *		1. Base data and client data
 *		2. Cell data and client data
 *		3. Cell data and not client data
 */
/*static*/ bool EntityDescription::shouldSkipPass( int pass, int dataDomains )
{
	// This array is used to indicate whether or not we should skip this pass.
	// If one of the data domains is not set, we do not do that pass.
	const int PASS_JUMPER[4] =
	{
		EXACT_MATCH | BASE_DATA,				// done in pass 0
		EXACT_MATCH | BASE_DATA | CLIENT_DATA,	// done in pass 1
		EXACT_MATCH | CELL_DATA | CLIENT_DATA,	// done in pass 2
		EXACT_MATCH | CELL_DATA					// done in pass 3
	};

	// If the EXACT_MATCH flag is set, all of the flags must match.
	if (dataDomains & EXACT_MATCH)
	{
		return (dataDomains != PASS_JUMPER[ pass ]);
	}
	else
	{
		return ((dataDomains & PASS_JUMPER[ pass ]) == 0);
	}
}


static int NUM_PASSES = 4;


/**
 *	This interface is used by EntityDescription::addToStream.
 */
class AddToStreamVisitor
{
public:
	AddToStreamVisitor( const EntityDescription & entityDescription ):
			entityDescription_( entityDescription )
	{}

	virtual ~AddToStreamVisitor() {};

	virtual ScriptObject getData( DataDescription & /*dataDesc*/ ) const
	{
		// Derived classes should implement either this method or addToStream.
		// If they derive from addToStream, it should no longer call getData.
		MF_EXIT( "getData not implemented or invalid call to getData" );
		return ScriptObject();
	}

	virtual bool addToStream( DataDescription & dataDesc,
			BinaryOStream & stream, bool isPersistentOnly ) const
	{
		bool result = true;
		ScriptObject pValue = this->getData( dataDesc );

		if (!pValue)
		{
			// TODO: Could have a flag indicating whether there
			// should be an error here.
			pValue = dataDesc.pInitialValue();
			result = !this->isErrorOnNULL();
		}

		if (!dataDesc.isCorrectType( pValue ))
		{
			ERROR_MSG( "EntityDescription::addToStream(%s): "
					"Data for %s (%s) is wrong type (%s)\n", 
				entityDescription_.name().c_str(),
				dataDesc.name().c_str(),
				dataDesc.dataType()->typeName().c_str(),
				pValue.typeNameOfObject() );
			pValue = dataDesc.pInitialValue();
			result = false;
		}
		else
		{
			ScriptDataSource source( pValue );

			if (!dataDesc.addToStream( source, stream, isPersistentOnly ))
			{
				ERROR_MSG( "EntityDescription::addToStream(%s): "
						"Failed to write property %s (%s) to stream\n",
					entityDescription_.name().c_str(),
					dataDesc.name().c_str(), 
					dataDesc.dataType()->typeName().c_str() );
				result = false;
			}
		}

		return result;
	}

	virtual bool isErrorOnNULL() const
	{
		return true;
	}

private:
	const EntityDescription & entityDescription_;
};


/**
 *	This class is used by EntityDescription::addSectionToStream.
 */
class AddToStreamSectionVisitor : public AddToStreamVisitor
{
public:
	AddToStreamSectionVisitor( const EntityDescription & entityDescription,
			DataSection * pSection ) : 
		AddToStreamVisitor( entityDescription ),
		pSection_( pSection )
	{}

protected:
	virtual bool addToStream( DataDescription & dataDesc,
			BinaryOStream & stream, bool isPersistentOnly ) const
	{
		DataSectionPtr pCurr = pSection_->openSection( dataDesc.fullName() );

		// pCurr == NULL is not an error.
		return dataDesc.fromSectionToStream( pCurr, stream, isPersistentOnly );
	}

private:
	DataSection * pSection_;
};



/**
 *	This class is used by EntityDescription::addDictionaryToStream.
 */
class AddToStreamMapVisitor : public AddToStreamVisitor
{
public:
	AddToStreamMapVisitor( const EntityDescription & entityDescription,
			const ScriptMapping & map ) :
		AddToStreamVisitor( entityDescription ),
		map_( map )
	{
	}

protected:
	ScriptObject getData( DataDescription & dataDesc ) const
	{
		ScriptObject object = dataDesc.getItemFrom( map_ );

		if (!object)
		{
			Script::clearError();
		}

		return object;
	}

	virtual bool isErrorOnNULL() const	{ return false; }

private:
	const ScriptMapping & map_;
};




/**
 *	This class is used by EntityDescription::addAttributesToStream.
 */
class AddToStreamAttributeVisitor : public AddToStreamVisitor
{
public:
	AddToStreamAttributeVisitor( const EntityDescription & entityDescription,
			const ScriptObject & object ) : 
		AddToStreamVisitor( entityDescription ),
		object_( object )
	{
		// Not ref counting pObj since the life of this object is short.
	}

protected:
	ScriptObject getData( DataDescription & dataDesc ) const
	{
		ScriptObject ret = dataDesc.getAttrFrom( object_ );

		if (!ret)
		{
			Script::printError();
		}

		return ret;
	}

private:
	const ScriptObject & object_;
};



/**
 *	This method adds information from the input section to the input stream.
 *	It may include base, client or cell data or any combination of these. This
 *	is specified by the dataDomains argument.
 *
 *	@param pSection		The data section containing the values to use. If any
 *						value is not in the data section, it gets the default.
 *	@param stream		The stream to add the data to.
 *	@param dataDomains	Indicates the type of data to be added to the stream.
 *						This is a bitwise combination of BASE_DATA, CLIENT_DATA
 *						and CELL_DATA.
 *
 *	@return True on success, otherwise false.
 */
bool EntityDescription::addSectionToStream( DataSectionPtr pSection,
		BinaryOStream & stream,
		int dataDomains ) const
{
	AddToStreamSectionVisitor visitor( *this, pSection.get() );

	return this->addToStream( visitor, stream, dataDomains );
}



/**
 *	This method adds information from the input section to the input PyDict.
 *	It may include base, client or cell data or any combination of these. This
 *	is specified by the dataDomains argument.
 *
 *	@param pSection		The data section containing the values to use. If any
 *						value is not in the data section, it gets the default.
 *	@param pDict		The Python dict to add the data to.
 *	@param dataDomains	Indicates the type of data to be added to the stream.
 *						This is a bitwise combination of BASE_DATA, CLIENT_DATA
 *						and CELL_DATA.
 *
 *	@return True on success, otherwise false.
 */
bool EntityDescription::addSectionToDictionary( DataSectionPtr pSection,
			ScriptObject pDict,
			int dataDomains ) const
{

	class AddToDictSectionVisitor : public IDataDescriptionVisitor
	{
	public:
		AddToDictSectionVisitor( DataSectionPtr pSection, ScriptObject pDict ) :
			pSection_( pSection ),
			pDict_( pDict )
		{};

		virtual bool visit( const DataDescription& propDesc )
		{
			DataSectionPtr pValueSection =
				pSection_->findChild( propDesc.name() );
			if (pValueSection)
			{
				ScriptDataSink sink;
				if (propDesc.createFromSection( pValueSection, sink ))
				{
					propDesc.insertItemInto( ScriptDict( pDict_ ),
						sink.finalise() );
				}
				else
				{
					WARNING_MSG( "EntityDescription::addSectionToDictionary: "
							"Could not add %s\n", propDesc.name().c_str() );
				}
			}

			return true;
		}

	private:
		DataSectionPtr pSection_;
		ScriptObject pDict_;
	};

	AddToDictSectionVisitor visitor( pSection, pDict );
	this->visit( dataDomains, visitor );

	return true;
}



/**
 *	This method adds information from the input mapping to the input stream.
 *	It may include base, client or cell data or any combination of these. This
 *	is specified by the dataDomains argument.
 *
 *	@param map			The map containing the values to use. If any
 *						value is not in the map, it gets the default.
 *	@param stream		The stream to add the data to.
 *	@param dataDomains	Indicates the type of data to be added to the stream.
 *						This is a bitwise combination of BASE_DATA, CLIENT_DATA
 *						and CELL_DATA.
 *
 *	@return True on success, otherwise false.
 */
bool EntityDescription::addDictionaryToStream( const ScriptMapping & map,
		BinaryOStream & stream,
		int dataDomains,
		int32 * pDataSizes,
		int numDataSizes ) const
{
	if (!map)
	{
		ERROR_MSG( "EntityDescription::addDictionaryToStream: "
				"invalid map object.\n" );
		return false;
	}

	AddToStreamMapVisitor visitor( *this, map );

	return this->addToStream( visitor, stream, dataDomains,
			pDataSizes, numDataSizes );
}


/**
 *	This method adds information from the input entity to the input stream.
 *	It may include base, client or cell data or any combination of these. This
 *	is specified by the dataDomains argument.
 *
 *	@see EntityDescription::addAttributesToStream
 */
bool EntityDescription::addAttributesToStream( const ScriptObject & object,
		BinaryOStream & stream,
		int dataDomains,
		int32 * pDataSizes,
		int numDataSizes ) const
{
	if (!object)
	{
		ERROR_MSG( "EntityDescription::addAttributesToStream: "
				"object is NULL\n" );
		return false;
	}

	AddToStreamAttributeVisitor visitor( *this, object );

	return this->addToStream( visitor, stream, dataDomains,
			pDataSizes, numDataSizes );
}


/**
 *	This helper method is used by EntityDescription::addSectionToStream and
 *	EntityDescription::addDictionaryToStream.
 */
bool EntityDescription::addToStream( const AddToStreamVisitor & visitor,
		BinaryOStream & stream,
		int dataDomains,
		int32 * pDataSizes,
		int numDataSizes ) const
{
	int actualPass = 0;

	for (int pass = 0; pass < NUM_PASSES; pass++)
	{
		if (!EntityDescription::shouldSkipPass( pass, dataDomains ))
		{
			int initialStreamSize = stream.size();

			for (uint i = 0; i < this->propertyCount(); i++)
			{
				DataDescription * pDD = this->property( i );

				if (EntityDescription::shouldConsiderData( pass, pDD,
							dataDomains ))
				{
					// TRACE_MSG( "EntityDescription::addToStream: "
					//			"Adding property = %s\n", pDD->name().c_str() );

					if (!visitor.addToStream( *pDD, stream,
						(dataDomains & ONLY_PERSISTENT_DATA) != 0 ))
					{
						// TODO: Make sure that every caller handles the false
						// case correctly. The stream is now in an invalid state
						// so if it is still used it may cause problems remotely
						ERROR_MSG( "EntityDescription::addToStream(%s): "
								"Failed to add to stream while adding %s(%s). "
								"STREAM NOW INVALID!!\n",
							this->name().c_str(),
							pDD->name().c_str(),
							pDD->dataType()->typeName().c_str() );
						return false;
					}
				}
			}

			if ((pDataSizes != NULL) && (actualPass < numDataSizes))
			{
				pDataSizes[actualPass] = stream.size() - initialStreamSize;
			}

			actualPass++;
		}
	}

	MF_ASSERT_DEV( (numDataSizes == 0) || (numDataSizes == actualPass) ||
			(numDataSizes == actualPass - 1) );

	return true;
}


/**
 *	This method calls the visitor's visit method for each DataDescription
 *	matching dataDomains.
 *
 *	@param visitor	The object to have visit called on.
 *	@param dataDomains	Indicates the type of data to be added to the stream.
 *						This is a bitwise combination of BASE_DATA, CLIENT_DATA
 *						and CELL_DATA.
 */
bool EntityDescription::visit( int dataDomains,
		IDataDescriptionVisitor & visitor ) const
{
	for ( int pass = 0; pass < NUM_PASSES; pass++ )
	{
		if (!EntityDescription::shouldSkipPass( pass, dataDomains ))
		{
			for (uint i = 0; i < this->propertyCount(); i++)
			{
				DataDescription * pDD = this->property( i );

				if (EntityDescription::shouldConsiderData( pass, pDD,
					dataDomains ))
				{
					if (!visitor.visit( *pDD ))
						return false;
				}
			}
		}
	}

	return true;
}


/**
 *	This method removes the data on the input stream and sets values on the
 *	input dictionary.
 */
bool EntityDescription::readStreamToDict( BinaryIStream & stream,
	int dataDomains, const ScriptMapping & dict ) const
{
	class Visitor : public IDataDescriptionVisitor
	{
	public:
		Visitor( BinaryIStream & stream, const ScriptMapping & map, bool onlyPersistent ) :
			stream_( stream ),
			map_( map ),
			onlyPersistent_( onlyPersistent ) {}

		bool visit( const DataDescription & dataDesc )
		{
			//TRACE_MSG( "EntityDescription::readStreamToDict: Reading "
			//			"property=%s %s\n", dataDesc.name().c_str(), dataDesc.dataType()->typeName().c_str() );

			ScriptDataSink sink;
			bool result = dataDesc.createFromStream( stream_, sink,
									onlyPersistent_ );

			ScriptObject pValue = sink.finalise();
			IF_NOT_MF_ASSERT_DEV( result && pValue )
			{
				ERROR_MSG( "EntityDescription::readStream: "
							"Could not create %s from stream.\n",
						dataDesc.name().c_str() );
				return false;
			}

			if (!dataDesc.insertItemInto( map_, pValue ))
			{
				ERROR_MSG( "EntityDescription::readStream: "
						"Failed to set %s\n", dataDesc.name().c_str() );
				Script::printError();
			}

			return !stream_.error();
		}

	private:
		BinaryIStream & stream_;
		const ScriptMapping & map_;
		bool onlyPersistent_;
	};

	Visitor visitor( stream, dict,
			((dataDomains & ONLY_PERSISTENT_DATA) != 0) );
	return this->visit( dataDomains, visitor );
}


/**
 *	This method reads a stream of properties sent from the cell to the client.
 *	The stream contains the property id before each property.
 */
bool EntityDescription::readTaggedClientStreamToDict( BinaryIStream & stream,
	const ScriptMapping & dict, bool allowOwnClientData ) const
{
	uint8 size;
	stream >> size;

	for (uint8 i = 0; i < size; i++)
	{
		uint8 index;
		stream >> index;

		const DataDescription * pDD = this->clientServerProperty( index );

		MF_ASSERT_DEV( pDD && (pDD->isOtherClientData() ||
			(allowOwnClientData && pDD->isOwnClientData())) );

		if (pDD != NULL && (pDD->isOtherClientData() ||
			(allowOwnClientData && pDD->isOwnClientData())))
		{
			ScriptDataSink sink;
			bool result = pDD->createFromStream( stream, sink,
				/* isPersistentOnly */ false );
			ScriptObject value = sink.finalise();
			MF_ASSERT_DEV( value && result );

			if (!pDD->insertItemInto( dict, value ))
			{
				ERROR_MSG( "EntityDescription::readTaggedClientStreamToDict: "
						"Failed to set %s\n", pDD->name().c_str() );
				Script::printError();
				// TODO: Should this return false
			}
		}
	}

	return true;
}


/**
 *	This method adds the data on a stream to the input DataSection.
 */
bool EntityDescription::readStreamToSection( BinaryIStream & stream,
	int dataDomains, DataSectionPtr pSection ) const
{
	class Visitor : public IDataDescriptionVisitor
	{
		const EntityDescription & entityDescription_;
		BinaryIStream & stream_;
		DataSection * pSection_;
		bool onlyPersistent_;

	public:
		Visitor( const EntityDescription & entityDescription,
				BinaryIStream & stream,
				DataSectionPtr pSection, bool onlyPersistent ) :
			entityDescription_( entityDescription ),
			stream_( stream ),
			pSection_( pSection.get() ),
			onlyPersistent_( onlyPersistent ) {}

		bool visit( const DataDescription & dataDesc )
		{
			DataSectionPtr pCurr = pSection_->openSection( dataDesc.fullName(),
					true );

			MF_ASSERT_DEV( pCurr );

			if (pCurr)
			{
				if (!dataDesc.fromStreamToSection( stream_, pCurr, 
						onlyPersistent_ ))
				{
					ERROR_MSG( "EntityDescription::readStreamToSection(%s): "
							"Could not convert from stream to section for "
							"property %s (%s)\n",
						entityDescription_.name().c_str(),
						dataDesc.name().c_str(),
						dataDesc.dataType()->typeName().c_str() );

					return false;
				}
			}
			return true;
		}
	};

	Visitor visitor( *this, stream, pSection,
			((dataDomains & ONLY_PERSISTENT_DATA) != 0) );
	return this->visit( dataDomains, visitor );
}


/**
 *	This method returns the property that is the identifier for this Entity
 *	type.
 */
const DataDescription * EntityDescription::pIdentifier() const
{
	Properties::const_iterator iter = properties_.begin();

	while (iter != properties_.end())
	{
		if (iter->isIdentifier())
		{
			return &*iter;
		}

		++iter;
	}

	return NULL;
}


/**
 *	This method adds this object to the input MD5 object.
 */
void EntityDescription::addToMD5( MD5 & md5 ) const
{
	md5.append( name_.c_str(), static_cast<int>(name_.size()) );

	Properties::const_iterator propertyIter = properties_.begin();

	while (propertyIter != properties_.end())
	{
		// Ignore the server side only ones.
		if (propertyIter->isClientServerData())
		{
			int csi = propertyIter->clientServerFullIndex();
			md5.append( &csi, sizeof( csi ) );
			propertyIter->addToMD5( md5 );
		}

		propertyIter++;
	}

	int count;
	MethodDescriptionList::const_iterator methodIter;

	count = 0;
	methodIter = client_.internalMethods().begin();

	while (methodIter != client_.internalMethods().end())
	{
		methodIter->addToMD5( md5, count );
		count++;

		methodIter++;
	}

	count = 0;
	const MethodDescriptionList & baseMethods = base_.internalMethods();
	methodIter = baseMethods.begin();

	while (methodIter != baseMethods.end())
	{
		if (methodIter->isExposed())
		{
			methodIter->addToMD5( md5, count );
			count++;
		}

		methodIter++;
	}

	count = 0;
	const MethodDescriptionList & cellMethods = cell_.internalMethods();
	methodIter = cellMethods.begin();

	while (methodIter != cellMethods.end())
	{
		if (methodIter->isExposed())
		{
			methodIter->addToMD5( md5, count );
			count++;
		}

		methodIter++;
	}
}


/**
 *	This method adds this object's persistent properties to the input MD5
 *	object.
 */
void EntityDescription::addPersistentPropertiesToMD5( MD5 & md5 ) const
{
	md5.append( name_.c_str(), static_cast<int>(name_.size()) );

	Properties::const_iterator propertyIter = properties_.begin();

	while (propertyIter != properties_.end())
	{
		if (propertyIter->isPersistent())
		{
			propertyIter->addToMD5( md5 );
		}

		propertyIter++;
	}
}


/**
 *	This method exposes the exposed message ids for methods of this entity.
 */
void EntityDescription::setExposedMsgIDs(
	int maxClientMethodCount, const ExposedMethodMessageRange * pClientRange,
	int maxBaseMethodCount, const ExposedMethodMessageRange * pBaseRange,
	int maxCellMethodCount, const ExposedMethodMessageRange * pCellRange )
{
	client_.setExposedMsgIDs( maxClientMethodCount, pClientRange );
	base_.setExposedMsgIDs( maxBaseMethodCount, pBaseRange );
	cell_.setExposedMsgIDs( maxCellMethodCount, pCellRange );
}


// -----------------------------------------------------------------------------
// Section: Property related.
// -----------------------------------------------------------------------------

/**
 *	This method returns the number of client/server data properties of this
 *	entity class. Client/server data properties are those properties that can be
 *	sent between the server and the client.
 */
unsigned int EntityDescription::clientServerPropertyCount() const
{
	return static_cast<uint>(clientServerProperties_.size());
}


/**
 *	This method returns a given data property for this entity class.
 */
const DataDescription *
	EntityDescription::clientServerProperty( unsigned int n ) const
{
	IF_NOT_MF_ASSERT_DEV(n < clientServerProperties_.size())
	{
		MF_EXIT( "invalid property requested" );
	}

	return this->property( clientServerProperties_[n] );
}


// -----------------------------------------------------------------------------
// Section: Method related.
// -----------------------------------------------------------------------------

/**
 *	This method returns the number of client methods associated with this
 *	entity. "Client methods" refers to the methods implemented by the entity on
 *	the client that can be called by the server.
 *
 *	@return The number of client methods associated with this entity.
 */
unsigned int EntityDescription::clientMethodCount() const
{
	return static_cast<uint>(client_.internalMethods().size());
}

/**
 *	This method returns whether a name is a temporary property for this entity
 *	type.
 */
bool EntityDescription::isTempProperty( const BW::string & name ) const
{
	return tempProperties_.find( name ) != tempProperties_.end();
}


#if ENABLE_WATCHERS
WatcherPtr EntityDescription::pWatcher()
{
	static WatcherPtr watchMe = NULL;

	if (watchMe == NULL)
	{
		watchMe = new DirectoryWatcher();
		EntityDescription *pNull = NULL;
		watchMe->addChild( "cellMethods", 
						   EntityMethodDescriptions::pWatcher(), 
						   &pNull->cell_ );
		watchMe->addChild( "baseMethods", 
						   EntityMethodDescriptions::pWatcher(), 
						   &pNull->base_ );
		watchMe->addChild( "clientMethods", 
						   EntityMethodDescriptions::pWatcher(), 
						   &pNull->client_ );
	}

	return watchMe;
}

#endif // ENABLE_WATCHERS

BW_END_NAMESPACE

// entity_description.cpp
