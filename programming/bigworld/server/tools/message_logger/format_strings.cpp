#include "format_strings.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Destructor.
 */
FormatStrings::~FormatStrings()
{
	FormatMap::iterator it = formatMap_.begin();
	while (it != formatMap_.end())
	{
		delete it->second;
		++it;
	}
}


/**
 * This method is to clear our current self knowledge.
 */
void FormatStrings::clear()
{
	FormatMap::iterator it = formatMap_.begin();
	while (it != formatMap_.end())
	{
		delete it->second;
		++it;
	}

	formatMap_.clear();
}


/**
 * If we're in write mode and the fmt string passed in does not currently exist
 * in the mapping, it will be added to db.
 */
LogStringInterpolator* FormatStrings::resolve( const BW::string &fmt )
{
	FormatMap::iterator it = formatMap_.find( fmt );
	if (it != formatMap_.end())
	{
		return it->second;
	}

	if (!this->canAppendToDB())
	{
		return NULL;
	}

	LogStringInterpolator *pHandler = new LogStringInterpolator( fmt );

	if (!pHandler->isOk())
	{
		ERROR_MSG( "FormatStrings::resolve: "
			"Failed to create LogStringInterpolator for: %s. This might be "
			"because the format string is invalid.\n", fmt.c_str() );

		delete pHandler;
		return NULL;
	}

	// TODO: should potentially consider failing here if we were unable to
	//       write to db.
	this->writeFormatStringToDB( pHandler );

	this->addFormatStringToMap( fmt, pHandler );

	return pHandler;
}


/**
 * This method returns a list of all the format available format strings.
 *
 * @returns A BW::vector of BW::strings.
 */
FormatStringList FormatStrings::getFormatStrings() const
{
	FormatStringList stringsList;

	FormatMap::const_iterator it = formatMap_.begin();
	while (it != formatMap_.end())
	{
		stringsList.push_back( it->first );
		++it;
	}

	return stringsList;
}


/**
 * This method add the new format string to the map.
 */
void FormatStrings::addFormatStringToMap( const BW::string & fmt,
		LogStringInterpolator *pHandler )
{
	formatMap_[ fmt ] = pHandler;
}


BW_END_NAMESPACE

// format_strings.cpp
