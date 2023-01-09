#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "cstdmf/bw_string.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/bw_util.hpp"

#include "config_reader.hpp"

#define TRIM_LEADING_WHITESPACE( str ) \
	while ( isspace( *str ) ) ++str;

#define TRIM_TRAILING_WHITESPACE( start, end ) \
	while ( (end > start) && isspace( *end ) ) --end;


BW_BEGIN_NAMESPACE

ConfigReader::ConfigReader( const char *filename ) :
	filename_( filename ),
	separator_( '=' )
{
}


ConfigReader::~ConfigReader()
{
	Sections::iterator it = sections_.begin();
	while (it != sections_.end())
	{
		delete it->second;
		it++;
	}
}


/**
 * This method reads the contents of the configuration file in.
 */
bool ConfigReader::read()
{
	FILE *fp = NULL;

	if ((fp = bw_fopen( filename_.c_str(), "r" )) == NULL)
	{
		ERROR_MSG( "ConfigReader::read: Unable to open file "
			"'%s' in mode 'r': %s\n", filename_.c_str(), strerror( errno ) );
		return false;
	}


	char *line = NULL;
	size_t len = 0;
	bool ok = true;

	if (fseek( fp, 0, 0 ))
	{
		ERROR_MSG( "ConfigReader::read: Couldn't rewind '%s': %s\n",
			filename_.c_str(), strerror( errno ) );
		fclose( fp );
		return false;
	}

	SectionEntries *currSection = NULL;

	while (getline( &line, &len, fp ) != -1)
	{
		// Chomp where necessary
		size_t slen = strlen( line );
		if (line[ slen-1 ] == '\n')
			line[ slen-1 ] = '\0';

		if (!this->handleLine( line, currSection ))
		{
			ok = false;
			ERROR_MSG( "ConfigReader::read: "
				"Aborting due to failure in handleLine()\n" );
			break;
		}
	}

	if (line)
		free( line );

	fclose( fp );

	return ok;
}


/**
 * Handles processing of a line from the config file.
 */
bool ConfigReader::handleLine( const char *line, SectionEntries* & currSection )
{
	size_t len = strlen( line );

	if ((line[0] == '[') && (line[ len - 1 ] == ']'))
	{
		BW::string sectionName;

		// 1         The starting point of the section name after the [
		// len - 2   The length of the string without the surrounding [ ]
		sectionName.assign( line, 1, len-2 );

		Sections::iterator section = sections_.find( sectionName );
		if (section == sections_.end())
		{
			currSection = new SectionEntries;
			sections_[ sectionName ] = currSection;
		}
		else
		{
			currSection = section->second;
		}
	}
	else
	{
		const char *sep = strchr( line, separator_ );
		BW::string key;

		const char *keyStart = line;
		const char *keyEnd = sep;

		// If no separator was found, the entire string will be the key,
		// and there will be no value.
		if (sep == NULL)
		{
			keyStart = line;
			keyEnd = keyStart + strlen( line );
		}

		TRIM_LEADING_WHITESPACE( keyStart );

		// Ignore comment lines
		if (keyStart[0] == '#')
		{
			return true;
		}

		keyEnd--;
		TRIM_TRAILING_WHITESPACE( keyStart, keyEnd );

		// If it's an empty string don't bother processing further
		if (keyEnd < keyStart)
		{
			return true;
		}

		// If this is to be supported, use an arbitrary section name
		// eg: "undefined" or ""
		if (currSection == NULL)
		{
			ERROR_MSG( "ConfigReader::handleLine: Entry found that is not "
				"contained in a section.\n" );
			return true;
		}

		key.assign( keyStart, 0, (keyEnd - keyStart) + 1 );


		BW::string value;
		if (sep != NULL)
		{
			const char *valueStart = sep + 1;
			const char *valueEnd = (line + strlen( line ));

			TRIM_LEADING_WHITESPACE( valueStart );
			valueEnd--;
			TRIM_TRAILING_WHITESPACE( valueStart, valueEnd );

			value.assign( valueStart, 0, (valueEnd - valueStart) + 1 );
		}

		(*currSection)[ key ] = value;
	}

	return true;
}


/**
 * Retrieves a value from the requested section and key.
 *
 * @param sectionName The section name to search for the key in.
 * @param key         The key to which the value is associated.
 * @param value       Location to store the value if found.
 *
 * @returns true if the section / key exists, false if not.
 */
bool ConfigReader::getValue( const BW::string sectionName,
	const BW::string key, BW::string &value ) const
{
	Sections::const_iterator sectionIter = sections_.find( sectionName );
	SectionEntries *section = NULL;
	if (sectionIter == sections_.end())
	{
		ERROR_MSG( "ConfigReader::getValue: Section not found '%s'.\n",
			sectionName.c_str() );
		return false;
	}

	section = sectionIter->second;

	SectionEntries::const_iterator entriesIter = section->find( key );
	if (entriesIter == section->end())
	{
		return false;
	}

	value = entriesIter->second;

	return true;
}


/**
 * Separates the provided line using the specified separator.
 *
 * Results are placed into the provided resultList which is cleared of
 * any existing content.
 *
 * @param line        The line to separate.
 * @param sep         The character to use as the separator.
 * @param resultList  The list to store the separated strings into.
 */
void ConfigReader::separateLine( const BW::string &line, char sep,
	BW::vector< BW::string > &resultList )
{
	// Get rid of anything in the current list
	resultList.clear();

	if (strlen( line.c_str() ) == 0)
		return;

	const char *entryStart = line.c_str();
	const char *lineEnd = entryStart + strlen( entryStart );

	const char *match = NULL;

	while ((match = strchr( entryStart, sep )) != NULL)
	{
		const char *entryEnd = match - 1;

		TRIM_LEADING_WHITESPACE( entryStart );
		TRIM_TRAILING_WHITESPACE( entryStart, entryEnd );

		if (entryEnd > entryStart)
		{

			BW::string entry( entryStart, (entryEnd - entryStart) + 1 );
			if (!entry.empty())
			{
				resultList.push_back( entry );
			}
		}

		entryStart = ++match;
	}

	// extract the final entry
	TRIM_LEADING_WHITESPACE( entryStart );
	TRIM_TRAILING_WHITESPACE( entryStart, lineEnd );

	BW::string entry( entryStart, (lineEnd - entryStart) );
	if (!entry.empty())
	{
		resultList.push_back( entry );
	}

	return;
}

BW_END_NAMESPACE

// config_reader.cpp

