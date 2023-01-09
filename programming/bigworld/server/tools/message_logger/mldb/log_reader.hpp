#ifndef LOG_READER_MLDB_HPP
#define LOG_READER_MLDB_HPP

#include "log_common.hpp"
#include "../hostnames.hpp"
#include "../user_log_reader.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

typedef BW::map< uint16, BW::string > UsernamesMap;

class LogReaderMLDB : public LogCommonMLDB
{
public:
	//LogReaderMLDB();
	virtual ~LogReaderMLDB();

	bool init( const char *root );
	bool onUserLogInit( uint16 uid, const BW::string &username );

	/* Main Public Interface */
	const char *getLogDirectory() const;
	bool getCategoryNames( CategoriesVisitor & visitor ) const;
	bool getComponentNames( LogComponentsVisitor &visitor ) const;
	bool getHostnames( HostnameVisitor &visitor ) const;
	FormatStringList getFormatStrings() const;
	const UsernamesMap & getUsernames() const;

	UserLogReaderPtr getUserLog( uint16 uid );

	bool getCategoryIDByName( const BW::string & categoryName,
		MessageLogger::CategoryID & categoryID ) const;

	uint32 getAddressFromHost( const char *hostname ) const;

	const LogStringInterpolator *getHandlerForLogEntry( const LogEntry &entry );

	bool refreshFileMaps();

private:
	UsernamesMap usernames_;

	// Both LogStorageMLDB and LogReaderMLDB need a UserLog list, however they both
	// handle the descruction of these lists differently so are kept in their
	// respective subclasses.
	UserLogs userLogs_;
};

BW_END_NAMESPACE

#endif // LOG_READER_MLDB_HPP
