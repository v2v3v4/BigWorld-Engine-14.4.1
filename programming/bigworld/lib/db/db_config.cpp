#include "db_config.hpp"

#include <limits>


BW_BEGIN_NAMESPACE


namespace DBConfig
{

TopLevelConfigBlock * pTopLevelConfig = NULL;


/**
 *	Constructor.
 *
 *	@param name 	The name of this block's section in bw.xml.
 *	@param pParent 	The parent block, or NULL if none.
 */
ConfigBlock::ConfigBlock( const BW::string & name,
			ConfigBlock * pParent /* = NULL */ ) :
		name_( name ),
		path_(),
		childBlocks_(),
		options_(),
		obsoleteNames_()
{
	if (pParent)
	{
		pParent->addChildBlock( this );
	}
}


/**
 *	This method initialises a block of database configuration from bw.xml.
 *
 *	@param parentPath 	The parent block's path describing a section in bw.xml,
 *						or empty if we are a top-level block.
 *
 *	@return true on success, false otherwise.
 */
bool ConfigBlock::init( const BW::string & parentPath )
{
	bool success = true;

	path_ = !parentPath.empty() ? parentPath + "/" + name_ : name_;

	// Doing child blocks first (depth-first).
	for (Blocks::iterator iter = childBlocks_.begin();
			iter != childBlocks_.end();
			++iter)
	{
		ConfigBlock & block = **iter;

		if (!block.init( path_ ))
		{
			success = false;
			// But keep going to so we know what other options can't be
			// found.
		}
	}

	// Read options in this block.
	for (Options::iterator iter = options_.begin();
			iter != options_.end();
			++iter)
	{
		ConfigOptionBase & option = **iter;

		if (!option.init( path_ ))
		{
			success = false;
		}
	}

	// Check for obsolete entries in this block.
	for (ObsoleteNames::const_iterator iter = obsoleteNames_.begin();
			iter != obsoleteNames_.end();
			++iter)
	{
		BW::string obsoletePath = !path_.empty() ?
			path_ + "/" + iter->first :
			iter->first;

		if (BWConfig::getSection( obsoletePath.c_str() ).get() != NULL)
		{
			ERROR_MSG( "ConfigBlock::init: "
					"Obsolete option used in bw.xml: <%s>: %s\n",
				obsoletePath.c_str(), iter->second.c_str() );

			success = false;
		}
	}

	if (!success)
	{
		return false;
	}

	return this->postInit();
}


#if ENABLE_WATCHERS
/**
 *	This method constructs a watcher for this block.
 *
 *	@return 	The watcher for this block.
 */
WatcherPtr ConfigBlock::pWatcher() const
{
	WatcherPtr pWatcher = new DirectoryWatcher;

	for (Blocks::const_iterator iter = childBlocks_.begin();
			iter != childBlocks_.end();
			++iter)
	{
		ConfigBlock & block = **iter;

		pWatcher->addChild( block.name().c_str(), block.pWatcher(),
			// We need to use the offset of the member relative to this object,
			// hence the ugliness here.
			(void *)((uintptr( &block ) - uintptr( this ))) );
	}

	for (Options::const_iterator iter = options_.begin();
			iter != options_.end();
			++iter)
	{
		ConfigOptionBase & option = **iter;

		pWatcher->addChild( option.name().c_str(), option.pWatcher(),
			// We need to use the offset of the member relative to this object,
			// hence the ugliness here.
			(void *)((uintptr( &option ) - uintptr( this ))) );
	}

	return pWatcher;
}

#endif // ENABLE_WATCHERS


/**
 *	Constructor.
 *
 *	@param parent 	The parent block.
 */
UnicodeStringConfigBlock::UnicodeStringConfigBlock( ConfigBlock & parent ) :
	ConfigBlock( "unicodeString", &parent ),
	characterSet( *this, "characterSet", "utf8" ),
	collation( *this, "collation", "utf8_bin" )
{}



/**
 *	Constructor.
 *
 *	@param parent 	The parent block.
 */
MySQLConfigBlock::MySQLConfigBlock( ConfigBlock & parent ) :
		ConfigBlock( "mysql", &parent ),
		host( *this, "host", "localhost" ),
		port( *this, "port", 0 ),
		username( *this, "username", "bigworld" ),
		password( *this, "password", "bigworld" ),
		databaseName( *this, "databaseName", "bigworld" ),
		numConnections( *this, "numConnections", 5 ),
		secureAuth( *this, "secureAuth", false ),
		syncTablesToDefs( *this, "syncTablesToDefs", false ),
		maxSpaceDataSize( *this, "maxSpaceDataSize", 2048 ),
		unicodeString( *this )
{}


/*
 *	Override from ConfigBlock.
 */
bool MySQLConfigBlock::postInit() /* override */
{
	if (port() > std::numeric_limits< uint16 >::max())
	{
		ERROR_MSG( "MySQLConfigBlock::postInit: "
				"<%s>: MySQL server port out of range: %u\n",
			port.path().c_str(), port() );

		return false;
	}

	if (databaseName().empty())
	{
		ERROR_MSG( "MySQLConfigBlock::postInit: "
				"<%s> must not be empty\n",
			databaseName.path().c_str() );
		return false;
	}

	if (numConnections() == 0)
	{
		WARNING_MSG( "MySQLConfigBlock::postInit: "
				"<%s> cannot be zero, set to 1\n",
			numConnections.path().c_str() );

		numConnections.set( 1 );
	}

	if (maxSpaceDataSize() == 0)
	{
		maxSpaceDataSize.set( 1 );
	}

	return true;
}


/**
 *	Constructor.
 *
 *	@param parent 	The parent block.
 */
XMLConfigBlock::XMLConfigBlock( ConfigBlock & parent ) :
		ConfigBlock( "xml", &parent ),
		archivePeriod( *this, "archivePeriod", 3600 ),
		numArchives( *this, "numArchives", 5 ),
		savePeriod( *this, "savePeriod", 900 )
{}


/**
 *	Constructor.
 */
ConsolidationConfigBlock::ConsolidationConfigBlock( ConfigBlock & parent ) :
		ConfigBlock( "consolidation", &parent ),
		directory( *this, "directory", "/tmp/" )
{
}


/**
 *	Constructor.
 */
SecondaryDatabasesConfigBlock::SecondaryDatabasesConfigBlock(
			ConfigBlock & parent ) :
		ConfigBlock( "secondaryDB", &parent ),
		enable( *this, "enable", true ),
		maxCommitPeriod( *this, "maxCommitPeriod", 5.0 ),
		directory( *this, "directory", "server/db/secondary" ),
		consolidation( *this )
{
}


/**
 *	This method returns the maximum commit period, as read from the
 *	maxCommitPeriod option, in ticks, rather than in seconds.
 */
uint SecondaryDatabasesConfigBlock::maxCommitPeriodInTicks() const
{
	return pTopLevelConfig->secondsToTicks( this->maxCommitPeriod(),
		/* lowerBound */ 1U );
}


/**
 *	Constructor.
 */
DBConfigBlock::DBConfigBlock( ConfigBlock & parent ) :
		ConfigBlock( "db", &parent ),
		type( *this, "type", "xml" ),
		mysql( *this ),
		xml( *this ),
		secondaryDB( *this )
{
	// MySQL options need to be in the <mysql> child section.
	static const char * MYSQL_MESSAGE = 
		"<dbMgr> MySQL options have been moved to the <db/mysql> section";
	this->addObsoleteName( "host", MYSQL_MESSAGE );
	this->addObsoleteName( "port", MYSQL_MESSAGE );
	this->addObsoleteName( "username", MYSQL_MESSAGE );
	this->addObsoleteName( "password", MYSQL_MESSAGE );
	this->addObsoleteName( "databaseName", MYSQL_MESSAGE );
	this->addObsoleteName( "numConnections", MYSQL_MESSAGE );
	this->addObsoleteName( "secureAuth", MYSQL_MESSAGE );
	this->addObsoleteName( "syncTablesToDefs", MYSQL_MESSAGE );
	this->addObsoleteName( "maxSpaceDataSize", MYSQL_MESSAGE );
	this->addObsoleteName( "unicodeString", MYSQL_MESSAGE );

	this->addObsoleteName( "consolidation",
		"Secondary database consolidation options have been moved from the "
			"<dbMgr/consolidation> section to the "
			"<db/secondaryDB/consolidation> section" );

	// Some options were moved to DBApp. They are considered obsolete as well, 
	// as they should not be in this section.

	static const char * DBAPP_MESSAGE = 
		"This DBApp-specific option belongs in <dbApp>, not in <db>";
	this->addObsoleteName( "dumpEntityDescription", DBAPP_MESSAGE );
	this->addObsoleteName( "desiredBaseApps", DBAPP_MESSAGE );
	this->addObsoleteName( "desiredCellApps", DBAPP_MESSAGE );
	this->addObsoleteName( "desiredServiceApps", DBAPP_MESSAGE );
	this->addObsoleteName( "maxConcurrentEntityLoaders", DBAPP_MESSAGE );
	this->addObsoleteName( "numDBLockRetries", DBAPP_MESSAGE );
	this->addObsoleteName( "shouldCacheLogOnRecords", DBAPP_MESSAGE );
	this->addObsoleteName( "shouldDelayLookUpSend", DBAPP_MESSAGE );
}


/**
 *	Constructor.
 */
BaseAppConfigBlock::BaseAppConfigBlock( ConfigBlock & parent ) :
		ConfigBlock( "baseApp", &parent )
{
	this->addObsoleteName( "secondaryDB",
		"Secondary database options have been moved to the <db/secondaryDB> "
			"section" );
}


/**
 *	Constructor.
 */
DBAppConfigBlock::DBAppConfigBlock( ConfigBlock & parent ) :
		ConfigBlock( "dbApp", &parent )
{
	// MySQL options need to be in the <db/mysql> child section.
	static const char * MYSQL_MESSAGE = 
		"<dbMgr> MySQL options have been moved to the <db/mysql> section";
	this->addObsoleteName( "host", MYSQL_MESSAGE );
	this->addObsoleteName( "port", MYSQL_MESSAGE );
	this->addObsoleteName( "username", MYSQL_MESSAGE );
	this->addObsoleteName( "password", MYSQL_MESSAGE );
	this->addObsoleteName( "databaseName", MYSQL_MESSAGE );
	this->addObsoleteName( "numConnections", MYSQL_MESSAGE );
	this->addObsoleteName( "secureAuth", MYSQL_MESSAGE );
	this->addObsoleteName( "syncTablesToDefs", MYSQL_MESSAGE );
	this->addObsoleteName( "maxSpaceDataSize", MYSQL_MESSAGE );
	this->addObsoleteName( "unicodeString", MYSQL_MESSAGE );

	this->addObsoleteName( "consolidation",
		"Secondary database consolidation options have been moved from the "
			"<dbMgr/consolidation> section to the "
			"<db/secondaryDB/consolidation> section, and is not part of the "
			"DBApp-specific configuration under <dbApp>" );
}


/**
 *	This method returns whether the configuration initialisation succeeded.
 */
bool DBConfigBlock::isGood() const
{
	return pTopLevelConfig->isGood();
}


/**
 *	Constructor.
 */
TopLevelConfigBlock::TopLevelConfigBlock() :
		ConfigBlock( "" ),
		updateHertz( *this, "gameUpdateHertz", 10U ),
		db( *this ),
		obsoleteBaseAppConfigBlock_( *this ),
		obsoleteDBAppConfigBlock_( *this ),
		isGood_( false )
{
	this->addObsoleteName( "dbMgr",
		"<dbMgr> options have been replaced by <dbApp> and <db> sections.\n"
			"\t<dbApp> holds DBApp-specific options\n"
			"\t<db> holds general database settings shared by all DB apps and "
				"tools\n"
			"Consult the Server Operations Guide for more details." );

	isGood_ = this->init();
}


/**
 *	This method converts a time duration from seconds to ticks.
 *
 *	@param seconds The number of seconds to convert.
 *	@param lowerBound A lower bound on the resulting value.
 */
uint TopLevelConfigBlock::secondsToTicks( float seconds, 
		uint lowerBound ) const
{
	return std::max( lowerBound,
			uint( floorf( seconds * this->updateHertz() + 0.5f ) ) );
}


/**
 *	This method returns the static instance of the database configuration data.
 */
const DBConfigBlock & get()
{
	static TopLevelConfigBlock instance;
	// We hold onto the top-level object so it can be initialised to check for
	// the <dbMgr> top-level obsolete section.

	pTopLevelConfig = &instance;
	return instance.db;
}


} // end namespace DBConfig

BW_END_NAMESPACE


// db_config.cpp
