#ifndef LOG_COMMON_MLDB_HPP
#define LOG_COMMON_MLDB_HPP

#include "categories.hpp"
#include "format_strings.hpp"
#include "hostnames.hpp"
#include "log_component_names.hpp"
#include "../unary_integer_file.hpp"
#include "../types.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class LogCommonMLDB
{
public:
	virtual ~LogCommonMLDB() {}

	BW::string getHostByAddr( MessageLogger::IPAddress ipAddress );
	const char * getComponentNameByAppTypeID(
		MessageLogger::AppTypeID appTypeID ) const;
	const char * getCategoryNameByID(
		MessageLogger::CategoryID categoryID ) const;

protected:
	bool initRootLogPath( const char *rootPath );
	bool initCommonFiles( const char *mode );
	bool initUserLogs( const char *mode );

	virtual bool onUserLogInit( uint16 uid, const BW::string &username ) = 0;

	BW::string rootLogPath_;

	UnaryIntegerFile version_;

	LogComponentNamesMLDB componentNames_;

	HostnamesMLDB hostnames_;

	FormatStringsMLDB formatStrings_;

	CategoriesMLDB categories_;
};

BW_END_NAMESPACE

#endif // LOG_COMMON_MLDB_HPP
