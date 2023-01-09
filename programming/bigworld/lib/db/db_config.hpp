#ifndef DB_CONFIG_HPP
#define DB_CONFIG_HPP

#include "cstdmf/bw_vector.hpp"

#if ENABLE_WATCHERS
#include "cstdmf/watcher.hpp"
#endif // ENABLE_WATCHERS

#include "server/bwconfig.hpp"


BW_BEGIN_NAMESPACE

namespace DBConfig
{

class DBConfigBlock;
typedef DBConfigBlock Config;

class ConfigBlock;


const DBConfigBlock & get();



/**
 *	This is the base class for configuration options used with
 *	ConfigBlocks to read individual options from bw.xml.
 */
class ConfigOptionBase
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param name 	The name of the configuration option.
	 */
	ConfigOptionBase( const BW::string & name ) :
		name_( name ),
		path_()
	{}

	virtual ~ConfigOptionBase() {}

	virtual bool init( const BW::string & parentPath ) = 0;

#if ENABLE_WATCHERS
	/**
	 *	This method returns a watcher suitable for viewing this option.
	 */
	virtual WatcherPtr pWatcher() = 0;
#endif // ENABLE_WATCHERS

	/**
	 *	This method returns the name of the configuration option.
	 */
	const BW::string & name() const { return name_; }

	/**
	 *	This method returns the path of the configuration option, based on the
	 *	parent path when it was initialised.
	 */
	const BW::string & path() const { return path_; }

protected:
	BW::string name_;
	BW::string path_;
};


/**
 *	This is the templated option classes specific to a particular type.
 */
template< typename TYPE >
class ConfigOption : public ConfigOptionBase
{
public:
	typedef ConfigOption< TYPE > Self;

	ConfigOption( ConfigBlock & block, const BW::string & name, 
		const TYPE & defaultValue );

#if ENABLE_WATCHERS
	// Override from WatcherProvider
	WatcherPtr pWatcher() /* override */;
#endif // ENABLE_WATCHERS

	bool init( const BW::string & parentPath ) /* override */;


	/**
	 *	This method returns the configured value of the option.
	 */
	const TYPE & operator()() const { return value_; }

	/**
	 *	This method sets the option to the given value.
	 */
	void set( const TYPE & value ) { value_ = value; }

private:
	TYPE value_;
};


/**
 *	This class holds a section of options read from bw.xml. A configuration
 *	block can hold options and child configuration blocks.
 */
class ConfigBlock
{
public:
	ConfigBlock( const BW::string & name, ConfigBlock * pParent = NULL );

	bool init( const BW::string & parentPath = BW::string() );

	/**
	 *	This method returns the name of the configuration section for this
	 *	block.
	 */
	const BW::string & name() const { return name_; }

#if ENABLE_WATCHERS
	WatcherPtr pWatcher() const;
#endif // ENABLE_WATCHERS

	/**
	 *	This method adds an option that is read in this block.
	 */
	void addOption( ConfigOptionBase * pOption )
	{
		options_.push_back( pOption );
	}


	/**
	 *	This method adds a child block of this one.
	 */
	void addChildBlock( ConfigBlock * pBlock )
	{
		childBlocks_.push_back( pBlock );
	}

	/**
	 *	This method adds an entry for an obsolete block or option for which it
	 *	is considered an error if they exist in this block.
	 *
	 *	This is useful when configuration options change, and developers are
	 *	required to upgrade their configuration files to a new set of options.
	 *
	 *	@param name 	The name of the obsolete block.
	 *	@param message 	A message to be appended to the error log output. This
	 *					is intended to explain how to resolve the issue.
	 */
	void addObsoleteName( const BW::string & name, 
			const BW::string & message )
	{
		obsoleteNames_.push_back( ObsoleteNameEntry( name, message ) );
	}

protected:

	/**
	 *	This is called after this block and any child blocks have been
	 *	initialised successfully. This is used to validate and adjust option
	 *	values.
	 */
	virtual bool postInit() { return true; }

	BW::string name_;
	BW::string path_;

	// These need to be declared first before any of the public option members.
	typedef BW::vector< ConfigBlock * > Blocks;
	Blocks childBlocks_;

	typedef BW::vector< ConfigOptionBase * > Options;
	Options options_;

	typedef std::pair< BW::string, BW::string > ObsoleteNameEntry;
	typedef BW::vector< ObsoleteNameEntry > ObsoleteNames;
	ObsoleteNames obsoleteNames_;
	
};


/**
 *	Constructor.
 *
 *	@param block			The block this option belongs to.
 *	@param name				The name of the option.
 *	@param defaultValue 	The default value of the option.
 */
template< typename TYPE >
ConfigOption< TYPE >::ConfigOption( ConfigBlock & block, 
			const BW::string & name, const TYPE & defaultValue ) :
		ConfigOptionBase( name ),
		value_( defaultValue )
{
	block.addOption( this );
}


/**
 *	This method initialises the configuration option from bw.xml.
 *
 *	@param parentPath 		The path to the parent block containing this
 *							option.
 *
 *	@return true on success, false otherwise.
 */
template< typename TYPE >
bool ConfigOption< TYPE >::init( const BW::string & parentPath ) /* override */
{
	path_ = !parentPath.empty() ? parentPath + "/" + name_ : name_;

	if (!BWConfig::update( path_.c_str(), value_ ))
	{
		ERROR_MSG( "DB::ConfigOption::init: Failed to read <%s>\n",
			path_.c_str() );

		return false;
	}
	return true;
}


/**
 *	This method returns a watcher suitable for this option.
 */
#if ENABLE_WATCHERS
template< typename TYPE >
WatcherPtr ConfigOption< TYPE >::pWatcher() /* override */ 
{
	Self * pNull = NULL;
	return makeWatcher( pNull->value_ );
}

#endif // ENABLE_WATCHERS


/**
 *	This class holds configuration options for the MySQL database connection's
 *	treatment of unicode strings.
 */
class UnicodeStringConfigBlock : public ConfigBlock
{
public:
	UnicodeStringConfigBlock( ConfigBlock & parent );

	ConfigOption< BW::string > 	characterSet;
	ConfigOption< BW::string > 	collation;
};


/**
 *	This class holds configuration options for the MySQL database, read from
 *	the "db/mysql" section in bw.xml.
 */
class MySQLConfigBlock : public ConfigBlock
{
public:
	MySQLConfigBlock( ConfigBlock & parent );

	ConfigOption< BW::string > 		host;
	ConfigOption< uint > 			port;
	ConfigOption< BW::string > 		username;
	ConfigOption< BW::string >		password;
	ConfigOption< BW::string >		databaseName;
	ConfigOption< uint > 			numConnections;
	ConfigOption< bool > 			secureAuth;
	ConfigOption< bool > 			syncTablesToDefs;
	ConfigOption< uint >			maxSpaceDataSize;

	UnicodeStringConfigBlock 		unicodeString;

protected:
	
	bool postInit() /* override */;
};


/**
 *	This class holds configuration options for the XML database, read from the
 *	"db/xml" section in bw.xml.
 */
class XMLConfigBlock : public ConfigBlock
{
public:
	XMLConfigBlock( ConfigBlock & parent );

	ConfigOption< float > 			archivePeriod;
	ConfigOption< uint >			numArchives;
	ConfigOption< float >			savePeriod;
};


/**
 *	This class holds the configuration relating to secondary database
 *	consolidation.
 */
class ConsolidationConfigBlock : public ConfigBlock
{
public:
	ConsolidationConfigBlock( ConfigBlock & parent );

	ConfigOption< BW::string >		directory;
};


/**
 *	This class holds the configuration relating to secondary databases.
 */
class SecondaryDatabasesConfigBlock : public ConfigBlock
{
public:
	SecondaryDatabasesConfigBlock( ConfigBlock & parent );

	ConfigOption< bool >			enable;
	ConfigOption< float >			maxCommitPeriod;
	uint maxCommitPeriodInTicks() const;

	ConfigOption< BW::string >		directory;

	ConsolidationConfigBlock		consolidation;
};


/**
 *	This class holds the configuration for the persistence layer, read from the
 *	"db" section in bw.xml.
 */
class DBConfigBlock : public ConfigBlock
{
public:
	DBConfigBlock( ConfigBlock & parent );

	bool isGood() const;

	ConfigOption< BW::string >		type;

	MySQLConfigBlock 				mysql;
	XMLConfigBlock 					xml;
	SecondaryDatabasesConfigBlock	secondaryDB;
};


/**
 *	This class is used to detect obsolete secondary database configuration in
 *	BaseApp's configuration section.
 */
class BaseAppConfigBlock : public ConfigBlock
{
public:
	BaseAppConfigBlock( ConfigBlock & parent );
};


/**
 *	This class is used to detect obsolete secondary database configuration in
 *	DBApp's configuration section.
 */
class DBAppConfigBlock : public ConfigBlock
{
public:
	DBAppConfigBlock( ConfigBlock & parent );
};


/**
 *	This class holds the configuration data held at the top-level. 
 *
 *	There is actually no such configuration, this is used to define some
 *	obsolete names to check for and fail if they exist, e.g. "dbMgr".
 */
class TopLevelConfigBlock : public ConfigBlock
{
public:
	TopLevelConfigBlock();

	bool isGood() const { return isGood_; }

	ConfigOption< uint >			updateHertz;
	uint secondsToTicks( float seconds, uint lowerBound ) const;

	DBConfigBlock 					db;

private:
	BaseAppConfigBlock obsoleteBaseAppConfigBlock_;
	DBAppConfigBlock obsoleteDBAppConfigBlock_;

	bool isGood_;
};

} // end namespace DBConfig 


BW_END_NAMESPACE


#endif // DB_CONFIG_HPP
