#ifndef HOSTNAMES_VALIDATOR_HPP
#define HOSTNAMES_VALIDATOR_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"

#include "types.hpp"
#include "hostnames.hpp"


BW_BEGIN_NAMESPACE


enum HostnamesValidatorProcessStatus
{
	BW_VALIDATE_HOSTNAMES_FAILED,
	BW_VALIDATE_HOSTNAMES_CONTINUE,
	BW_VALIDATE_HOSTNAMES_FINISHED
};


enum HostnameStatus
{
	BW_HOSTNAME_STATUS_NOT_VALIDATED = 0,
	BW_HOSTNAME_STATUS_VALIDATED = 1
};


enum HostnamesValidateResult
{
	BW_VALIDATE_RESULT_TRY_AGAIN,
	BW_VALIDATE_RESULT_CHANGED,
	BW_VALIDATE_RESULT_NOT_CHANGED,
	BW_VALIDATE_RESULT_FAILED
};


class HostnameStatusMapValue
{
public:
	HostnameStatusMapValue( BW::string hostname, HostnameStatus status )
	{
		hostname_ = hostname;
		status_ = status;
	}

	BW::string getHostname() { return hostname_; }
	HostnameStatus getStatus() { return status_; }

	void setHostname( BW::string hostname ) { hostname_ = hostname; }
	void setStatus( HostnameStatus status ) { status_ = status; }


private:
	BW::string hostname_;
	HostnameStatus status_;
};


typedef BW::map< MessageLogger::IPAddress, HostnameStatusMapValue* >
		HostnameStatusMap;

/**
 * This class handles the mapping between IP addresses and hostnames
 */
class HostnamesValidator
{
public:
	HostnamesValidator();
	virtual ~HostnamesValidator();

	void addValidateHostname( MessageLogger::IPAddress addr,
		const BW::string & hostname, HostnameStatus status );

	virtual HostnamesValidatorProcessStatus validateNextHostname();

	HostnamesValidateResult validateOneHostname(
			MessageLogger::IPAddress addr,
			const BW::string & oldName,
			BW::string & newName);

	virtual bool hostnameChanged( MessageLogger::IPAddress addr,
			const BW::string & newHostname ){ return true; };

protected:
	bool inProgress() { return !validationList_.empty(); };
	void clear();
	HostnameStatusMap validationList_;


private:
	int lookupRetries_;
};


//=====================================
// HostnameCopier
//=====================================
class HostnameCopier : public HostnameVisitor
{
public:
	HostnameCopier( HostnamesValidator *pHostnamesValidator )
	{
		pHostnamesValidator_ = pHostnamesValidator;
	}

	bool onHost( MessageLogger::IPAddress ipAddress,
		const BW::string & hostname )
	{
		pHostnamesValidator_->addValidateHostname( ipAddress, hostname,
			BW_HOSTNAME_STATUS_NOT_VALIDATED );
		return true;
	}
private:
	HostnameCopier() {}

	HostnamesValidator *pHostnamesValidator_;
};

BW_END_NAMESPACE

#endif // HOSTNAMES_VALIDATOR_HPP
