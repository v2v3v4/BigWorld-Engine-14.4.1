#ifndef HOSTNAMES_MLDB_HPP
#define HOSTNAMES_MLDB_HPP

#include "../hostnames.hpp"
#include "../text_file_handler.hpp"
#include "../types.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE


/**
 * This class handles the mapping between IP addresses and hostnames
 */
class HostnamesMLDB : public TextFileHandler, public Hostnames
{
public:
	bool init( const char *logLocation, const char *mode );

	virtual bool handleLine( const char *line );

	bool writeHostnameToDB( const MessageLogger::IPAddress & addr,
		const BW::string & hostname );

	static const char * getHostnamesFilename();

protected:
	virtual void flush();
};

BW_END_NAMESPACE

#endif // HOSTNAMES_MLDB_HPP
