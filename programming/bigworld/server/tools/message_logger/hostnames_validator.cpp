#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>

#include "cstdmf/debug.hpp"
#include "cstdmf/bw_util.hpp"

#include "hostnames_validator.hpp"
#include "constants.hpp"
#include "hostnames.hpp"


BW_BEGIN_NAMESPACE


static const int HOSTNAMES_VALIDATOR_MAX_LOOKUP_RETRIES = 10;


/**
 * Constructor.
 */
HostnamesValidator::HostnamesValidator() :
	lookupRetries_( 0 )
{ }


/**
 * Destructor
 */
HostnamesValidator::~HostnamesValidator()
{
	this->clear();
}


/**
 * This method releases allocated resources for the stored validation list and
 * clears the list.
 */
void HostnamesValidator::clear()
{
	HostnameStatusMap::iterator it = validationList_.begin();

	if (it != validationList_.end())
	{
		delete it->second;
		++it;
	}

	validationList_.clear();
}


/**
 * This method adds an address/hostname pair with validation status to the
 * stored validation list.
 */
void HostnamesValidator::addValidateHostname( MessageLogger::IPAddress addr,
	const BW::string & hostname, HostnameStatus status )
{
	HostnameStatusMapValue *pNewValue =
		new HostnameStatusMapValue( hostname, status );
	validationList_[ addr ] = pNewValue;
}


/**
 * This method validates a hostname/address pair using gethostbyaddr and writes
 * the validated pair to db.
 *
 * If the host is not found then the hostname pair will not be written, and a
 * warning log will indicate that the hostname was rem.  If any other error
 * occurs then simply add the original pair, as the host may
 * still be valid.
 *
 * If a hostname is found it will be written to db (regardless of what the
 * old value was, ie. whether it changed or not)
 *
 * @returns HostnamesValidatorProcessStatus
 */
HostnamesValidatorProcessStatus HostnamesValidator::validateNextHostname()
{
	HostnameStatusMap::iterator it = validationList_.begin();

	while ((it != validationList_.end()) &&
		(it->second->getStatus() != BW_HOSTNAME_STATUS_NOT_VALIDATED))
	{
		++it;
	}

	if (it == validationList_.end())
	{
		return BW_VALIDATE_HOSTNAMES_FINISHED;
	}

	MessageLogger::IPAddress addr = it->first;
	const char *ipAddress = inet_ntoa( (in_addr&)addr );
	HostnameStatusMapValue *pHostnameStatus = it->second;

	if (pHostnameStatus == NULL)
	{
		ERROR_MSG( "HostnamesValidator::validateNextHostname: Could not find "
			"hostname validation status for IP (%s).\n", ipAddress );
		return BW_VALIDATE_HOSTNAMES_FAILED;
	}

	BW::string oldHostname = pHostnameStatus->getHostname();
	BW::string newHostname;

	HostnamesValidateResult result = this->validateOneHostname(
			addr, oldHostname, newHostname );

	// validation should be retried
	if (result == BW_VALIDATE_RESULT_TRY_AGAIN)
	{
		if (lookupRetries_ < HOSTNAMES_VALIDATOR_MAX_LOOKUP_RETRIES)
		{
			// This will attempt the same address again in the next call
			++lookupRetries_;
			return BW_VALIDATE_HOSTNAMES_CONTINUE;
		}
		else
		{
			ERROR_MSG( "HostnamesValidator::validateNextHostname: Reached the "
				"retry limit for ip: %s\n", ipAddress );

			lookupRetries_ = 0;
			return BW_VALIDATE_HOSTNAMES_FAILED;
		}
	}

	// reset in case of reuse
	lookupRetries_ = 0;

	// validation failed
	if(result == BW_VALIDATE_RESULT_FAILED)
	{
		ERROR_MSG( "HostnamesValidator::validateNextHostname: Failed to "
				"validate ip: %s\n", ipAddress );

		return BW_VALIDATE_HOSTNAMES_FAILED;
	}

	// validation succeeded
	it->second->setStatus( BW_HOSTNAME_STATUS_VALIDATED );

	if(result == BW_VALIDATE_RESULT_CHANGED)
	{
		DEBUG_MSG( "HostnamesValidator::validateNextHostname: "
			"New hostname found for %s. Original: %s. New: %s. Action: "
			"Replacing hostname.\n",
			ipAddress, oldHostname.c_str(), newHostname.c_str() );
		it->second->setHostname( newHostname );

		this->hostnameChanged( addr, newHostname );
	}

	return BW_VALIDATE_HOSTNAMES_CONTINUE;
}



/*
 * Validate one host name and return the validation result.
 */
HostnamesValidateResult HostnamesValidator::validateOneHostname
(MessageLogger::IPAddress addr, const string & oldName, string & newName)
{
	HostnamesValidateResult result = BW_VALIDATE_RESULT_NOT_CHANGED;
	BW::string ipAddrStr = inet_ntoa( (in_addr&)addr );

	DEBUG_MSG( "HostnamesValidator::validateOneHostname: validating %s with "
			"hostname %s\n", ipAddrStr.c_str(), oldName.c_str() );

	struct hostent *pHostnameEntry =
		gethostbyaddr( &addr, sizeof( addr ), AF_INET );

	if (h_errno == TRY_AGAIN)
	{
		INFO_MSG( "HostnamesValidator::validateOneHostname: Got retry "
				"for the ip %s with name: %s\n",
				ipAddrStr.c_str(), oldName.c_str() );

		return BW_VALIDATE_RESULT_TRY_AGAIN;
	}

	if (pHostnameEntry == NULL)
	{
		if (h_errno == HOST_NOT_FOUND)
		{
			// Only need to warn/alter if the hostname was not already set to
			// the IP address
			if (ipAddrStr != oldName)
			{
				INFO_MSG( "HostnamesValidator::validateOneHostname: Hostname "
					"for %s not found. Old hostname: %s. Setting Hostname to "
					"its ip address.\n", ipAddrStr.c_str(), oldName.c_str() );

				newName = ipAddrStr;

				result = BW_VALIDATE_RESULT_CHANGED;
			}
			else
			{
				result = BW_VALIDATE_RESULT_NOT_CHANGED;
			}
		}
		else if (h_errno)
		{
			const char *reason = NULL;

			switch (h_errno)
			{
				case NO_DATA:
					reason = "NO_DATA"; break;
				case NO_RECOVERY:
					reason = "NO_RECOVERY"; break;
				default:
					reason = "Unknown reason";
			}

			ERROR_MSG( "HostnamesValidator::validateOneHostname: Hostname "
				"lookup failed. Error: %s.\n", reason );

			result = BW_VALIDATE_RESULT_FAILED;
		}
	}
	else
	{
		char *firstdot = strstr( pHostnameEntry->h_name, "." );
		if (firstdot != NULL)
		{
			*firstdot = '\0';
		}

		newName = pHostnameEntry->h_name;

		// Only need to warn if the hostname has changed
		if (newName != oldName)
		{
			result = BW_VALIDATE_RESULT_CHANGED;
		}
		else
		{
			result = BW_VALIDATE_RESULT_NOT_CHANGED;
		}
	}

	return result;
}


BW_END_NAMESPACE

// hostnames_validator.cpp
