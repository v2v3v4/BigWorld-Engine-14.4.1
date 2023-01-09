#ifndef TOOLS__CONSOLIDATE_DBS__COMMAND_LINE_PARSER_HPP
#define TOOLS__CONSOLIDATE_DBS__COMMAND_LINE_PARSER_HPP

#include "db_storage_mysql/connection_info.hpp"

#include <memory>
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class CommandLineParser
{
public:
	CommandLineParser();
	~CommandLineParser();

	bool init( int argc, char * const argv[] );

	bool isVerbose() const 
		{ return isVerbose_; }

	bool shouldClear() const
		{ return shouldClear_; }

	bool shouldList() const
		{ return shouldList_; }

	bool shouldIgnoreSqliteErrors() const
		{ return shouldIgnoreSqliteErrors_; }

	const BW::string & resPath() const
		{ return resPath_; }

	bool hadNonOptionArgs() const 
		{ return hadNonOptionArgs_; }

	const DBConfig::ConnectionInfo & primaryDatabaseInfo() const 
		{ return primaryDatabaseInfo_; }

	typedef BW::vector< BW::string > SecondaryDatabases;
	const SecondaryDatabases & secondaryDatabases() const
		{ return secondaryDatabases_; }

	void printUsage() const;

private:
	bool parsePrimaryDatabaseInfo( const BW::string & commandLineSpec );
	bool readSecondaryDatabaseList( const BW::string & path );

// Member data

	bool isVerbose_;
	bool shouldClear_;
	bool shouldList_;
	bool shouldIgnoreSqliteErrors_;
	bool hadNonOptionArgs_;

	BW::string resPath_;

	DBConfig::ConnectionInfo primaryDatabaseInfo_;
	SecondaryDatabases secondaryDatabases_;
};

BW_END_NAMESPACE

#endif // TOOLS__CONSOLIDATE_DBS__COMMAND_LINE_PARSER_HPP

