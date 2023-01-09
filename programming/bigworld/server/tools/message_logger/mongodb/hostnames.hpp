#ifndef HOSTNAMES_MONGODB_HPP
#define HOSTNAMES_MONGODB_HPP

#include "../hostnames.hpp"
#include "../hostnames_validator.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bgtask_manager.hpp"

#include "mongo/client/dbclient.h"


BW_BEGIN_NAMESPACE


class HostnamesMongoDB : public Hostnames, public HostnamesValidator
{
public:
   	HostnamesMongoDB (TaskManager & mongoDBTaskMgr,
			mongo::DBClientConnection & conn, const BW::string & collName );
	
	bool writeHostnameToDB( const MessageLogger::IPAddress & addr,
			const BW::string & hostname );

	bool init();

	HostnamesValidatorProcessStatus validateNextHostname();

	bool hostnameChanged( MessageLogger::IPAddress addr,
			const BW::string & newHostname );

private:
	TaskManager & mongoDBTaskMgr_;
	mongo::DBClientConnection & conn_;
	BW::string namespace_;
};


BW_END_NAMESPACE


#endif // HOSTNAMES_MONGODB_HPP
