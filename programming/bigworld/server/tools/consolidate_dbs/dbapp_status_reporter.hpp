#ifndef CONSOLIDATE_DBS__DBAPP_STATUS_REPORTER_HPP
#define CONSOLIDATE_DBS__DBAPP_STATUS_REPORTER_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class DBAppStatusReporter
{
public:
	virtual void onStatus( const BW::string & status ) = 0;
};

BW_END_NAMESPACE

#endif // CONSOLIDATE_DBS__DBAPP_STATUS_REPORTER_HPP

