#include "arg_parser.hpp"

#include <iomanip>

BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 *
 * 	@param programName	Name of the program.
 */
ArgParser::ArgParser( const char * programName ) :
	args_(),
	programName_( programName )
{
	// Always helpful
	this->add( "help", "Program usage" );
}


/**
 * 	Destructor.
 */
ArgParser::~ArgParser()
{
	ArgMap::iterator it = args_.begin();

	while (it != args_.end())
	{
		bw_safe_delete( it->second );
		++it;
	}

	args_.clear();
}


/**
 *	This method adds a named argument to the parser. Existing arguments with
 *	the	same name are replaced.
 *
 * 	@param name			Name of the argument
 * 	@param help			Description of the argument
 * 	@param isOptional	Flags if the argument is optional.
 *
 * 	@return Pointer to the added argument.
 */
void ArgParser::add( const char * name, const char * help, bool isOptional )
{
	MF_ASSERT( name );
	MF_ASSERT( help );

	MF_ASSERT( args_.find( name ) ==  args_.end() );

	args_[ name ] = new Arg( name, help, isOptional );
}


/**
 *	This method consumes and command line arguements. Presence of non optional
 *	arguements are checked. Program usage is output to console if the help
 *	argument is present or a non option argument is not present.
 *
 * 	@param argc		Argument count from the command line.
 * 	@param argv		Argument values from the command line.
 *
 * 	@return	True if all non optional arguements are present.
 */
bool ArgParser::parse( int argc, char * argv[] )
{
	for (int i = 1; i < argc; ++i)
	{
		// Consume named arguement, e.g. $ a.out --name
		if (strncmp( argv[i], "--", 2 ) == 0)
		{
			// Check for seperator
			char * pSeperator = strchr( argv[i], '=' );

			// Parse name, left strip "--" and right strip '='
			BW::string name = pSeperator ?
				BW::string( argv[i] + 2, pSeperator - argv[i] - 2 ) :
				BW::string( argv[i] + 2 );

			// Check for interest
			ArgMap::iterator it = args_.find( name.c_str() );
			if (it == args_.end())
			{
				continue;
			}

			Arg * pArg = it->second;
			pArg->isPresent = true;

			// Value is seperated on the same argument
			// e.g. --argName=argValue
			if (pSeperator)
			{
				pArg->value = pSeperator + 1;
			}

			// Value is the next argument
			// e.g. --argName argValue
			else if ((i < argc-1) && (strncmp( argv[i+1], "--", 2 ) != 0))
			{
				pArg->value = argv[++i];
			}

			// No value
			// e.g. --argName
			else
			{
				pArg->value = NULL;
			}
		}

		// TODO: Consume flag arguments,  e.g. $ a.out -f
		// TODO: Consume other arguments, e.g. $ a.out arg1 arg2 arg3
		// TODO: Make Windows compatible
	}

	// Check for non optional
	ArgMap::iterator presenceIt = args_.begin();
	while (presenceIt != args_.end())
	{
		if (!presenceIt->second->isOptional &&
			!presenceIt->second->isPresent)
		{
			break;
		}
		++presenceIt;
	}

	ArgMap::iterator helpIt = args_.find( "help" );

	if (presenceIt != args_.end() || this->isPresent( "help" ))
	{
		return false;
	}

	return true;
}


/**
 *	This method prints optional and mandatory argument usage.
 */
void ArgParser::printUsage()
{
	BW::stringstream usage;
	usage << "Usage: " << programName_ << " [OPTIONS]";

	BW::stringstream options;
	options << "\n\nOPTIONS:";

	ArgMap::iterator it = args_.begin();

	while (it != args_.end())
	{
		Arg * pArg = it->second;
		++it;

		if (pArg->isOptional)
		{
			options << "\n\t--" <<
				std::left << std::setw( 15 ) << pArg->name <<
				std::setfill( ' ' ) << std::setw( 30 ) << pArg->help;
		}
		else
		{
			usage << " --" << pArg->name << " " << pArg->help;
		}
	}

	printf( usage.str().c_str() );
	printf( "%s\n", options.str().c_str() );
}


char * ArgParser::value( const char * name )
{
	Arg * pArg = this->find( name );
	return pArg ? pArg->value : NULL;
}


bool ArgParser::isPresent( const char * name )
{
	Arg * pArg = this->find( name );
	return pArg ? pArg->isPresent : false;
}


ArgParser::Arg * ArgParser::find( const char * name )
{
	if (!name)
	{
		return NULL;
	}

	ArgMap::iterator it = args_.find( name );

	return (it == args_.end()) ? NULL : it->second;
}


/**
 * 	This method returns the argument value as a BW::string. The default value
 * 	is returned if argument or value is not present.
 *
 * 	@param name			The arguement name
 * 	@param defaultValue	The default value.
 *
 * 	@return	Arguement value if found, default value otherwise.
 */
BW::string ArgParser::get( const char * name, const BW::string & defaultValue )
{
	char * value = this->value( name );
	return value ? BW::string( value ) : defaultValue;
}


/**
 * 	This method returns the argument value as an unsigned int. The default value
 * 	is returned if argument or value is not present.
 *
 * 	@param name			The arguement name
 * 	@param defaultValue	The default value.
 *
 * 	@return	Arguement value if found, default value otherwise.
 */
unsigned int ArgParser::get( const char * name,
		const unsigned int & defaultValue )
{
	char * value = this->value( name );
	return value ? atoi( value ) : defaultValue;
}


/**
 * 	This method returns the argument value as a bool. The default value is
 * 	returned if argument or value is not present.
 *
 * 	@param name			The arguement name
 * 	@param defaultValue	The default value.
 *
 * 	@return	Arguement value if found, default value otherwise.
 */
bool ArgParser::get( const char * name, const bool & defaultValue )
{
	Arg * pArg = this->find( name );

	if (!pArg || !pArg->isPresent)
	{
		return defaultValue;
	}

	if (pArg->value)
	{
		const char * trueCStr = "true";
		size_t len = strlen( trueCStr );
		return strncmp( pArg->value, trueCStr, len ) == 0;
	}
	else
	{
		return true;
	}
}


BW_END_NAMESPACE

// arg_parser.cpp
