#include "server/bwconfig.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/debug_filter.hpp"
#include "cstdmf/watcher.hpp"

#include "network/network_interface.hpp"

#include "resmgr/bwresource.hpp"

#include <sys/types.h>
#include <pwd.h>

DECLARE_DEBUG_COMPONENT2( "Config", 0 )


BW_BEGIN_NAMESPACE

namespace BWConfig
{

int shouldDebug = 0;
BW::string badFilename;

Container g_sections;

bool g_inited = false;

// ----------------------------------------------------------------------------
// Section: BWConfig::SearchIterator
// ----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	Constructs an end iterator.
 */
BWConfig::SearchIterator::SearchIterator():
		iter_( DataSection::endOfSearch() ),
		chainTail_(),
		searchName_(),
		currentConfigPath_()
{
}


/**
 *	Constructor.
 *
 *	@param searchName 	The section name to search for.
 *	@param chain		The configuration section chain.
 */
BWConfig::SearchIterator::SearchIterator( const BW::string & searchName,
			const Container & chain ):
		iter_( DataSection::endOfSearch() ),
		chainTail_(),
		searchName_( searchName ),
		currentConfigPath_()
{
	for (Container::const_iterator iConfig = chain.begin();
			iConfig != chain.end();
			++iConfig)
	{
		chainTail_.push( *iConfig );
	}

	this->findNext();
}


/**
 *	Find the next matching element, iterating through each configuration data
 *	section. If none is found, then this iterator will be equivalent to the
 *	iterator returned by endOfSearch().
 */
void BWConfig::SearchIterator::findNext()
{
	do
	{
		if (iter_ != DataSection::endOfSearch())
		{
			++iter_;
		}
		else if (!chainTail_.empty())
		{
			DataSectionPtr nextConfigSection = chainTail_.front().first;
			currentConfigPath_ = chainTail_.front().second;
			chainTail_.pop();
			iter_ = nextConfigSection->beginSearch( searchName_ );
		}
	} 
	while (!chainTail_.empty() && iter_ == DataSection::endOfSearch());

	if (chainTail_.empty() && iter_ == DataSection::endOfSearch())
	{
		currentConfigPath_ = "";
	}
}


/**
 *	Copy constructor.
 *
 *	@param copy 	The other search iterator object to copy.
 */
BWConfig::SearchIterator::SearchIterator( const SearchIterator & copy ):
		iter_( copy.iter_ ),
		chainTail_( copy.chainTail_ ),
		searchName_( copy.searchName_ ),
		currentConfigPath_( copy.currentConfigPath_ )
{
}


/**
 *	Return the next matching DataSection.
 */
DataSectionPtr BWConfig::SearchIterator::operator*()
{
	return *iter_;
}


/**
 *	Pre-increment operator.
 */
SearchIterator & BWConfig::SearchIterator::operator++()
{
	this->findNext();
	return *this;
}


/**
 *	Post-increment operator.
 */
SearchIterator BWConfig::SearchIterator::operator++( int )
{
	SearchIterator searchIteratorCopy( *this );
	this->operator++();
	return searchIteratorCopy;
}


/**
 *	Assignment operator.
 *
 *	@param copy 	The other search iterator to assign from.
 */
SearchIterator & BWConfig::SearchIterator::operator=(
		const SearchIterator & copy )
{
	iter_ = copy.iter_;
	chainTail_ = copy.chainTail_;
	searchName_ = copy.searchName_;
	currentConfigPath_ = copy.currentConfigPath_;
	return *this;
}




bool isBad()
{
	if (!badFilename.empty())
	{
		BWResource::openSection( badFilename );
		ERROR_MSG( "Could not open configuration file %s\n",
				badFilename.c_str() );
		return true;
	}

	return false;
}

void init()
{
	const char * BASE_FILENAME = "server/bw.xml";

	g_inited = true;
	g_sections.clear();
	BW::string filename = BASE_FILENAME;

	// Attempt to find a personalised configuration file first. This is of the
	// form "bw_<username>.xml".
	bool isUserFilename = false;
	struct passwd * pPasswordEntry = ::getpwuid( ::getUserId() );

	if (pPasswordEntry && (pPasswordEntry->pw_name != NULL))
	{
		filename = BW::string( "server/bw_" ) +
			pPasswordEntry->pw_name + ".xml";
		isUserFilename = true;
	}

	while (!filename.empty())
	{
		// This also handles reload.
		BWResource::instance().purge( filename );
		DataSectionPtr pDS = BWResource::openSection( filename );

		if (pDS)
		{
			INFO_MSG( "BWConfig::init: Adding %s\n", filename.c_str() );
			g_sections.push_back( std::make_pair( pDS, filename ) );
			filename = pDS->readString( "parentFile" );
		}
		else if (isUserFilename && !BWResource::fileExists( filename ))
		{
			filename = "server/bw.xml";
		}
		else
		{
			if (BWResource::fileExists( filename ))
			{
				ERROR_MSG( "Could not parse %s\n", filename.c_str() );
			}
			else
			{
				ERROR_MSG( "Could not find %s\n", filename.c_str() );
			}

			badFilename = filename;
			filename = "";
		}

		isUserFilename = false;
	}

	update( "debugConfigOptions", shouldDebug );

	DebugFilter::instance().filterThreshold(
		(DebugMessagePriority)BWConfig::get< uint32 >( "outputFilterThreshold",
			DebugFilter::instance().filterThreshold() ) );

	DebugFilter::instance().hasDevelopmentAssertions(
		BWConfig::get( "hasDevelopmentAssertions",
			DebugFilter::instance().hasDevelopmentAssertions() ) );


}

bool processCommandLine( int argc, char * argv[] )
{
	if (!g_sections.empty())
	{
		DataSectionPtr pDS = g_sections.front().first;

		for (int i = 0; i < argc - 1; ++i)
		{
			if (argv[i][0] == '+')
			{
				pDS->writeString( argv[i]+1, argv[ i+1 ] );
				INFO_MSG( "BWConfig::init: "
						"Setting '%s' to '%s' from command line.\n",
					argv[i]+1, argv[ i+1 ] );

				++i;
			}
		}
	}
	else
	{
		ERROR_MSG( "BWConfig::init: No configuration section.\n" );
		return false;
	}

	return true;
}


bool init( int argc, char * argv[] )
{
	if (!g_inited)
	{
		init();
	}
	else
	{
		WARNING_MSG( "BWConfig::init: "
				"Already initialised before parse args.\n" );
	}

	return processCommandLine( argc, argv );
}



bool hijack( Container & container, int argc, char * argv[] )
{
	g_inited = true;
	badFilename.clear();
	g_sections.swap( container );
	processCommandLine( argc, argv );
	return true;
}


DataSectionPtr getSection( const char * path, DataSectionPtr defaultValue )
{
	if (!g_inited)
	{
		init();
	}

	Container::iterator iter = g_sections.begin();

	while (iter != g_sections.end())
	{
		DataSectionPtr pResult = iter->first->openSection( path );

		if (pResult)
		{
			if (shouldDebug > 1)
			{
				DEBUG_MSG( "BWConfig::getSection: %s = %s (from %s)\n",
					path, pResult->asString().c_str(),
					iter->second.c_str() );
			}

			return pResult;
		}

		++iter;
	}

	if (shouldDebug > 1)
	{
		DEBUG_MSG( "BWConfig::getSection: %s was not set.\n", path );
	}

	return defaultValue;
}


/**
 *	Return a search iterator that searches the given config path.
 *
 *	@param sectionName 	Name of the section to open
 *
 *	@see BWConfig::endSearch
 */
SearchIterator beginSearchSections( const char * sectionName )
{
	return SearchIterator( sectionName, g_sections );
}


/**
 *	Return an end iterator for searches.
 *
 *	@see BWConfig::beginSearchSections
 */
const SearchIterator & endSearch()
{
	static SearchIterator end;
	return end;
}


void getChildrenNames( BW::vector< BW::string >& childrenNames,
		const char * parentPath )
{
	for ( Container::iterator iter = g_sections.begin();
			iter != g_sections.end(); ++iter )
	{
		DataSectionPtr pParent = iter->first->openSection( parentPath );

		if (pParent)
		{
			int numChildren = pParent->countChildren();
			for ( int i = 0; i < numChildren; ++i )
			{
				childrenNames.push_back( pParent->childSectionName( i ) );
			}
		}
	}
}

BW::string get( const char * path, const char * defaultValue )
{
	BW::string defaultValueStr( defaultValue );
	return get( path, defaultValueStr );
}


void prettyPrint( const char * path,
		const BW::string & oldValue, const BW::string & newValue,
		const char * typeStr, bool isHit )
{
#ifndef _WIN32
	const char * sectionName = basename( path );
#else
	const char * sectionName = path;
	const char * pCurr = path;
	while (*pCurr != '\0')
	{
		if (*pCurr == '/')
		{
			sectionName = pCurr + 1;
		}
		++pCurr;
	}
#endif

	BW::string sorting = (sectionName == path) ? "aatoplevel/" : "";
	sorting += path;

	BW::string changedValue = BW::string( " " ) + newValue;

	if (oldValue == newValue)
	{
		changedValue = "";
	}

	DEBUG_MSG( "BWConfig: %-40s <%s> %s </%s> "
				"<!-- Type: %s%s%s -->\n",
			sorting.c_str(),
			sectionName,
			oldValue.c_str(),
			sectionName,
			typeStr,
			changedValue.c_str(),
			isHit ? "" : " NOT READ" );
}

} // namespace BWConfig


// TODO: This should really be in BWService but causes link problems with
// bwmachined. It doesn't link against resmgr.


/**
 *	This method returns the internalInterface setting from bw.xml for a
 *	specific app.
 */
BW::string getBWInternalInterfaceSetting( const char * configPath )
{
	char buffer[ BUFSIZ ];
	snprintf( buffer, sizeof( buffer ), "%s/internalInterface", configPath );

	return BWConfig::get( buffer,
			BWConfig::get( "internalInterface",
				Mercury::NetworkInterface::USE_BWMACHINED ) );
}

BW_END_NAMESPACE

// bwconfig.cpp
