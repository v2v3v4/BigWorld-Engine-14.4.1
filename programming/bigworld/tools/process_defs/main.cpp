// This program parses the BigWorld .def files and generates a Python object
// that describes all of the entity types. A Python module is then load and a
// function is called with the description object.

const char * USAGE =
"usage: %s [OPTION] scriptArgs\n"
"\n"
"This program parses the BigWorld entity definition files. A Python object is\n"
"then created representing the entity descriptions. This is passed to a Python\n"
"callback moduleName.functionName. By default, this is ProcessDefs.process.\n"
"\n"
"Options:\n"
"  -r             directory     Specifies a resource path. Can be used\n"
"                               multiple times, in order of decreasing\n"
#if defined( __APPLE__ )
"                               priority.\n"
#else
"                               priority, replacing the paths provided in\n"
#if defined( WIN32 )
"                               paths.xml\n"
#else // defined( WIN32 )
"                               ~/.bwmachined.conf\n"
#endif // defined( WIN32 )
#endif // defined( __APPLE__ )
"  -f, --function funcName      The Python function to call. \"process\" by\n"
"                               default.\n"
"  -m, --module   moduleName    The Python module to load. \"ProcessDefs\"\n"
"                               by default.\n"
"  -p, --path     directory     The directory to find the module.\n"
"                               \"tools/process_defs/resources/scripts\" in \n"
"                               the resource tree is the default.\n"
"  --use-stdout                 Send messages to stdout rather then stderr\n"
"  -v, --verbose                Displays more verbose output.\n"
"  -h, --help                   Displays this message.\n"
;

#include "script/first_include.hpp"

#include "connection/baseapp_ext_interface.hpp"
#include "connection/client_interface.hpp"

#include "cstdmf/bw_util.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/debug_filter.hpp"
#include "cstdmf/md5.hpp"

#include "entitydef/constants.hpp"
#include "entitydef/entity_description_debug.hpp"
#include "entitydef/entity_description_map.hpp"
#include "entitydef/meta_data_type.hpp"

#include "pyscript/personality.hpp"
#include "pyscript/py_import_paths.hpp"

#include "resmgr/bwresource.hpp"

#include "help_msg_handler.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

const char PROCESS_NAME[] = "process_defs";

const char * FLAGS_HELP[] = {"-h", "--help", NULL};
const char * FLAGS_VERBOSE[] = {"-v", "--verbose", NULL};

const char * FLAGS_PATH[] = {"-p", "--path", NULL};
const char * FLAGS_MODULE[] = {"-m", "--module", NULL};
const char * FLAGS_FUNCTION[] = {"-f", "--function", NULL};

const char * FLAGS_USE_STDOUT[] = {"--use-stdout", NULL};

extern int force_link_UDO_REF;
extern int ResMgr_token;
extern int PyScript_token;

namespace
{
int s_moduleTokens = ResMgr_token | PyScript_token | force_link_UDO_REF;
}

/**
 *	This function prints the usage.
 */
void printUsage()
{
	fprintf( stderr, USAGE, PROCESS_NAME );
}


/**
 *	This function creates a dictionary that describes the given method.
 */
ScriptObject createMethodDescriptions(
		const EntityMethodDescriptions & methodDescriptions )
{
	ScriptTuple scriptMethodDescriptions =
		ScriptTuple::create( methodDescriptions.size() );

	for (unsigned int i = 0; i < methodDescriptions.size(); ++i)
	{
		const MethodDescription * pMethodDescription =
			methodDescriptions.internalMethod( i );

		ScriptDict methodDescription = ScriptDict::create();

#define ADD_PROPERTY( PROP_NAME ) 											\
	methodDescription.setItem( #PROP_NAME,							\
			ScriptObject::createFrom( pMethodDescription->PROP_NAME() ),	\
			ScriptErrorPrint( "createMethodDescriptions("#PROP_NAME"):") );

		ADD_PROPERTY( name )
		ADD_PROPERTY( isExposed )
		ADD_PROPERTY( internalIndex )
		ADD_PROPERTY( exposedIndex )

#undef ADD_PROPERTY

		methodDescription.setItem( "args",
			pMethodDescription->argumentTypesAsScript(),
			ScriptErrorPrint( "createMethodDescriptions(args):" ) );

		if (pMethodDescription->hasReturnValues())
		{
			methodDescription.setItem( "returnValues",
				pMethodDescription->returnValueTypesAsScript(),
				ScriptErrorPrint( "createMethodDescriptions(returnValues):" ) );

		}

		methodDescription.setItem( "streamSize",
			ScriptObject::createFrom(
				pMethodDescription->streamSize( true ) ),
			ScriptErrorPrint( "createMethodDescriptions(streamSize):" ) );

		scriptMethodDescriptions.setItem( i, methodDescription );
	}

	return scriptMethodDescriptions;
}


/**
 *	This function creates a dictionary that describes the given property.
 */
ScriptObject createPropertyDescription( const DataDescription & desc )
{
	ScriptDict scriptDesc = ScriptDict::create();

	scriptDesc.setItem( "type",
			ScriptObject::createFrom( desc.dataType()->typeName() ),
			ScriptErrorPrint( "createPropertyDescription" ) );

#define ADD_PROPERTY( PROP_NAME ) 											\
	scriptDesc.setItem( #PROP_NAME,									\
		ScriptObject::createFrom( desc.PROP_NAME() ),						\
		ScriptErrorPrint( "createPropertyDescription("#PROP_NAME"):" ) ); 	\

	ADD_PROPERTY( name )
	ADD_PROPERTY( index )
	ADD_PROPERTY( clientServerFullIndex )

	ADD_PROPERTY( isGhostedData )
	ADD_PROPERTY( isOtherClientData )
	ADD_PROPERTY( isOwnClientData )
	ADD_PROPERTY( isCellData )
	ADD_PROPERTY( isBaseData )
	ADD_PROPERTY( isClientServerData )
	ADD_PROPERTY( isPersistent )
	ADD_PROPERTY( isIdentifier )
	ADD_PROPERTY( isIndexed )
	ADD_PROPERTY( isUnique )
	ADD_PROPERTY( streamSize )

	scriptDesc.setItem( "isConst",
			ScriptObject::createFrom( desc.dataType()->isConst() ),
			ScriptErrorPrint( "createPropertyDescription(isConst):" ) );

#undef ADD_PROPERTY

	return scriptDesc;
}


/**
 *	This function creates a tuple of all property descriptions.
 */
ScriptObject createAllPropertyDescriptions(
		const EntityDescription & entityDescription )
{
	ScriptTuple propertyDescriptions =
		ScriptTuple::create( entityDescription.propertyCount() );

	for (unsigned int i = 0; i < entityDescription.propertyCount(); ++i)
	{
		DataDescription * pDesc = entityDescription.property( i );

		propertyDescriptions.setItem( i, createPropertyDescription( *pDesc ) );
	}

	return propertyDescriptions;
}


/**
 *	This function creates a tuple of ordered property descriptions for the given
 *	data domain.
 */
ScriptList createOrderedProperties( int dataDomains,
		const EntityDescription & entityDescription )
{
	class Visitor : public IDataDescriptionVisitor
	{
	public:
		Visitor() : descriptions_( ScriptList::create() ) {}

		bool visit( const DataDescription & dataDesc )
		{
			descriptions_.append( createPropertyDescription( dataDesc ) );

			return true;
		}

		const ScriptList & descriptions() const	{ return descriptions_; }

	private:
		ScriptList descriptions_;
	};

	Visitor visitor;

	entityDescription.visit( dataDomains, visitor );

	return visitor.descriptions();
}


/**
 *	This function creates a script dictionary containing a description of an
 *	entity type.
 */
ScriptObject createEntityDescription(
		const EntityDescription & entityDescription )
{
	ScriptDict scriptDescription = ScriptDict::create();

#define ADD_PROPERTY_NAMED( PROP_NAME, C_GETTER_NAME ) 						\
	scriptDescription.setItem( #PROP_NAME,									\
	ScriptObject::createFrom( entityDescription.C_GETTER_NAME() ),			\
	ScriptErrorPrint( "createEntityDescription("#PROP_NAME"):" ) );

#define ADD_PROPERTY( PROP_NAME ) 											\
	ADD_PROPERTY_NAMED( PROP_NAME, PROP_NAME )

	ADD_PROPERTY( name )
	ADD_PROPERTY( index )
	ADD_PROPERTY( clientIndex )
	ADD_PROPERTY_NAMED( hasClientScript, canBeOnClient )
	ADD_PROPERTY_NAMED( hasBaseScript, canBeOnBase )
	ADD_PROPERTY_NAMED( hasCellScript, canBeOnCell )
	ADD_PROPERTY( canBeOnClient )
	ADD_PROPERTY( canBeOnBase )
	ADD_PROPERTY( canBeOnCell )

	ADD_PROPERTY( isService )
	ADD_PROPERTY( isPersistent )

#undef ADD_PROPERTY_NAMED
#undef ADD_PROPERTY

	scriptDescription.setItem( "clientMethods",
			createMethodDescriptions( entityDescription.client() ),
			ScriptErrorPrint( "createEntityDescription(clientMethods)" ) );
	scriptDescription.setItem( "baseMethods",
			createMethodDescriptions( entityDescription.base() ),
			ScriptErrorPrint( "createEntityDescription(baseMethods)" ) );
	scriptDescription.setItem( "cellMethods",
			createMethodDescriptions( entityDescription.cell() ),
			ScriptErrorPrint( "createEntityDescription(cellMethods)" ) );

	scriptDescription.setItem( "allProperties",
			createAllPropertyDescriptions( entityDescription ),
			ScriptErrorPrint( "createEntityDescription(allProperties)" ) );

	scriptDescription.setItem( "clientProperties",
			createOrderedProperties(
				EntityDescription::CLIENT_DATA,
				entityDescription ),
			ScriptErrorPrint( "createEntityDescription(clientProperties)" ) );

	scriptDescription.setItem( "baseToClientProperties",
			createOrderedProperties(
				EntityDescription::FROM_BASE_TO_CLIENT_DATA,
				entityDescription ),
			ScriptErrorPrint( "createEntityDescription(baseToClientProps)" ) );

	scriptDescription.setItem( "cellToClientProperties",
			createOrderedProperties(
				EntityDescription::FROM_CELL_TO_CLIENT_DATA,
				entityDescription ),
			ScriptErrorPrint( "createEntityDescription(cellToClientProps)" ) );

	return scriptDescription;
}


/**
 *	This function creates a Script tuple contain a description of each entity
 *	type.
 */
ScriptObject createEntityDescriptions(
		const EntityDescriptionMap & entityDescriptionMap )
{
	ScriptTuple entityDescriptions =
		ScriptTuple::create( entityDescriptionMap.size() );

	EntityDescriptionMap::DescriptionMap::const_iterator iter =
		entityDescriptionMap.begin();

	int i = 0;

	while (iter != entityDescriptionMap.end())
	{
		const EntityDescription & entityDescription =
			entityDescriptionMap.entityDescription( iter->second );

		entityDescriptions.setItem( i,
				createEntityDescription( entityDescription ) );

		++i;
		++iter;
	}

	return entityDescriptions;
}


/**
 *	This function creates a string object from the digest string.
 */
ScriptObject createDigest( const EntityDescriptionMap & entityDescriptionMap )
{
	MD5 md5;
	entityDescriptionMap.addToMD5( md5 );
	MD5::Digest digest;
	md5.getDigest( digest );

	return ScriptObject::createFrom( digest.quote() );
}


/**
 *	This function parses the command line arguments and attempts to return the
 *	value associated with a give flags. The argc and argv arguments are updated
 *	to removed the matched flag and value (if any).
 *
 *	@param argc		A reference to the number of arguments. Note that this is
 *					updated if flags are matched.
 *	@param argv		An array of command line arguments. Note: this is updated if
 *					flags are matched.
 *	@param flags	A NULL terminated array of flags to attempt to match.
 *	@param defaultValue	The value to return if no matches are found.
 */
const char * getOption( int & argc, const char * argv[],
		const char * flags[], const char * defaultValue )
{
	// NOTE: Deliberately stop one short
	for (int i = 0; i < argc - 1; ++i)
	{
		bool isMatch = false;

		const char ** ppFlag = flags;

		while (*ppFlag != NULL)
		{
			isMatch |= (strcmp( argv[i], *ppFlag ) == 0);
			++ppFlag;
		}

		if (isMatch)
		{
			const char * pMatch = argv[i+1];
			argv[i] = NULL;
			argv[i+1] = NULL;
			BWUtil::compressArgs( argc, argv );

			return pMatch;
		}
	}

	return defaultValue;
}


/**
 *	This method returns whether the command line has one of the desired flags.
 */
bool hasFlag( int & argc, const char * argv[], const char * flags[] )
{
	for (int i = 0; i < argc; ++i)
	{
		bool isMatch = false;

		const char ** ppFlag = flags;

		while (*ppFlag != NULL)
		{
			isMatch |= (strcmp( argv[i], *ppFlag ) == 0);
			++ppFlag;
		}

		if (isMatch)
		{
			argv[i] = NULL;
			BWUtil::compressArgs( argc, argv );

			return true;
		}
	}

	return false;
}


/**
 *	This function calls a script function in a module with arguments.
 */
bool callFunction( const char * moduleName, const char * functionName,
		ScriptObject argument, bool shouldPrintError = true )
{
	ScriptModule module;

	module = Personality::import( moduleName );

	if (!module)
	{
		return false;
	}

	ScriptObject ret;

	if (shouldPrintError)
	{
		BW::string errorStr = "Failed to call method ";
		errorStr += functionName;

		ret = module.callMethod( functionName,
			ScriptArgs::create( argument ),
			ScriptErrorPrint( errorStr.c_str() ),
			/*allowNullMethod*/ true );
	}
	else
	{
		ret = module.callMethod( functionName,
			ScriptArgs::create( argument ),
			ScriptErrorClear(),
			/*allowNullMethod*/ true );
	}

	return ret && ret.isTrue( ScriptErrorClear() );
}

BW_END_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Network interfaces
// -----------------------------------------------------------------------------

#include "connection/baseapp_ext_interface.hpp"

#define DEFINE_INTERFACE_HERE
#include "connection/baseapp_ext_interface.hpp"

#include "connection/client_interface.hpp"

#define DEFINE_INTERFACE_HERE
#include "connection/client_interface.hpp"


BW_USE_NAMESPACE

bool process( const char * moduleName, 
		const char * functionName,
		EntityDescriptionMap& entityDescriptionMap )
{
	ScriptDict description = ScriptDict::create();

	description.setItem( "entityTypes",
		createEntityDescriptions( entityDescriptionMap ),
		ScriptErrorPrint( "main: description.entityTypes" ) );

	ScriptDict constants = ScriptDict::create();

	constants.setItem( "digest", createDigest( entityDescriptionMap ),
		ScriptErrorPrint( "main: constants.digest" ) );

	constants.setItem( "maxExposedClientMethodCount",
		ScriptObject::createFrom(
			entityDescriptionMap.maxExposedClientMethodCount() ),
		ScriptErrorPrint( "main: constants.maxExposedClientMethodCount" ) );


	constants.setItem( "maxExposedBaseMethodCount",
		ScriptObject::createFrom(
			entityDescriptionMap.maxExposedBaseMethodCount() ),
		ScriptErrorPrint( "main: constants.maxExposedBaseMethodCount" ) );

	constants.setItem( "maxExposedCellMethodCount",
		ScriptObject::createFrom(
			entityDescriptionMap.maxExposedCellMethodCount() ),
		ScriptErrorPrint( "main: constants.maxExposedCellMethodCount" ) );

	constants.setItem( "maxClientServerPropertyCount",
		ScriptObject::createFrom(
			entityDescriptionMap.maxClientServerPropertyCount() ),
		ScriptErrorPrint( "main: constants.maxClientServerPropertyCount" ) );

	description.setItem( "constants", constants,
		ScriptErrorPrint( "main: constants" ) );
 
	return callFunction( moduleName, functionName, description );
}


/**
 *	The main function.
 */
int main( int argc, const char *argv[] )
{
	BW_SYSTEMSTAGE_MAIN();

	DebugFilter::shouldWriteToConsole( true );

	const char * moduleName =
		getOption( argc, argv, FLAGS_MODULE, "ProcessDefs" );
	const char * functionName =
		getOption( argc, argv, FLAGS_FUNCTION, "process" );

	bool shouldPrintHelp = hasFlag( argc, argv, FLAGS_HELP );

	bool isVerbose = hasFlag( argc, argv, FLAGS_VERBOSE );

	BWUtil::compressArgs( argc, argv );

	if (hasFlag( argc, argv, FLAGS_USE_STDOUT ))
	{
		DebugFilter::consoleOutputFile( stdout );
	}

	if (!isVerbose)
	{
		DebugFilter::instance().filterThreshold( MESSAGE_PRIORITY_NOTICE );
	}

	BWResource bwResource;
	BWResource::init( argc, argv, true );

	BWResource::addSubPaths( "tools/process_defs" );
	
	PyImportPaths importPaths;
	importPaths.addNonResPath( getOption( argc, argv, FLAGS_PATH, "." ) );

	// This should probably always be done in Script::init.
	importPaths.addResPath( "resources/scripts" );
	importPaths.addResPath( EntityDef::Constants::serverCommonPath() );

	if (!Script::init( importPaths, PROCESS_NAME ))
	{
		ERROR_MSG( "Failed to initialise Python\n" );
		return EXIT_FAILURE;
	}

	if (shouldPrintHelp)
	{
		printUsage();

		// Increase the filter threshold to INFO so that the output from the 
		// script is not filtered out by the DebugFilter. 
		DebugFilter::instance().filterThreshold( MESSAGE_PRIORITY_INFO );

		// Instantiate the message handler below to redirect the
		// script output to stderr
		ProcessDefsHelpMsgHandler msgHandler;

		callFunction( moduleName, "help", ScriptObject(), isVerbose );
		return EXIT_SUCCESS;
	}

	PySys_SetArgv( argc, const_cast< char ** >( argv ) );

	EntityDescriptionMap entityDescriptionMap;

	if (!entityDescriptionMap.parse(
			BWResource::openSection( EntityDef::Constants::entitiesFile() ),
			&ClientInterface::Range::entityPropertyRange,
			&ClientInterface::Range::entityMethodRange,
			&BaseAppExtInterface::Range::baseEntityMethodRange,
			&BaseAppExtInterface::Range::cellEntityMethodRange ))
	{
		ERROR_MSG( "Failed to parse .def files\n" );
		return EXIT_FAILURE;
	}

	//fprintf( stderr, "Running method %s.%s\n", moduleName, functionName );

	bool isOkay = process( moduleName, functionName, entityDescriptionMap );

	Script::fini();

	// Clean up MetaDataType static registrations
	MetaDataType::fini();

	return isOkay ? EXIT_SUCCESS : EXIT_FAILURE;
}


// main.cpp
