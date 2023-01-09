#ifndef HOSTNAMES_MLDB_VALIDATOR_HPP
#define HOSTNAMES_MLDB_VALIDATOR_HPP

#include "../hostnames_validator.hpp"
#include "../text_file_handler.hpp"
#include "../types.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE


/**
 * This class handles the mapping between IP addresses and hostnames
 */
class HostnamesValidatorMLDB : public TextFileHandler, public HostnamesValidator
{
public:
	HostnamesValidatorMLDB();

	~HostnamesValidatorMLDB();

	bool init( const char *logLocation );

	virtual bool handleLine( const char *line );

	virtual void flush();

	bool writeHostnamesToDB() const;

protected:
	bool writeHostEntryToDB( MessageLogger::IPAddress addr,
		const char * pHostname ) const;

private:
	BW::string getTempPath( const char *rootLogPath ) const;

	BW::string tempDirPath_;

};

BW_END_NAMESPACE

#endif // HOSTNAMES_MLDB_VALIDATOR_HPP
