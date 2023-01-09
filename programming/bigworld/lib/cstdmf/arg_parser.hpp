#ifndef ARG_PARSER_HPP
#define ARG_PARSER_HPP

#include "bw_string.hpp"
#include "stdmf.hpp"
#include "stringmap.hpp"

BW_BEGIN_NAMESPACE


/**
 *	This class is an argument parser.
 *
 *	Programs add arguements, parse the command line, and then get values.
 *
 *	Example usage:
 *
 *	ArgParser parser( "AddressBook" );
 *
 *	parser.add( "name", "Contact Name", false );
 *	parser.add( "age", "Contact Age" );
 *
 *	if (!parser.parse( argv, argc ))
 *	{
 *		return -1;
 *	}
 *
 *	BW::string name = parser.get( "name", "Unknown" );
 *	unsigned int age = parser.get( "age", 0 );
 */
class ArgParser
{
public:
	ArgParser( const char * name );
	~ArgParser();

	void add( const char * name, const char * help, bool isOptional = true );

	bool parse( int argc, char * argv[] );

	void printUsage();

	BW::string		get( const char * name, const BW::string & defaultValue );
	unsigned int	get( const char * name, const unsigned int & defaultValue );
	bool			get( const char * name, const bool & defaultValue );

	bool isPresent( const char * name );

private:

	struct Arg
	{
		Arg( const char * n, const char * h, bool o ) :
			name( n ),
			help( h ),
			value( NULL ),
			isOptional( o ),
			isPresent( false )
		{ }

		const char * name;
		const char * help;
		char * value;
		bool isOptional;
		bool isPresent;
	};

	Arg * find( const char * name );

	char * value( const char * name );

	typedef StringHashMap< Arg * > ArgMap;
	ArgMap args_;

	const char * programName_;
};

BW_END_NAMESPACE

#endif // ARG_PARSER_HPP
