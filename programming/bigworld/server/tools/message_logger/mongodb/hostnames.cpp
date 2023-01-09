#include "hostnames.hpp"

#include "constants.hpp"

#include "../hostnames_validator.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bgtask_manager.hpp"

#include "tasks/update_id_string_pair_task.hpp"
#include "tasks/write_id_string_pair_task.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param conn		Established MongoDB connection
 */
HostnamesMongoDB::HostnamesMongoDB( TaskManager & mongoDBTaskMgr,
		mongo::DBClientConnection & conn, const BW::string & collName ) :
	mongoDBTaskMgr_( mongoDBTaskMgr ),
	conn_( conn ),
	namespace_( collName )
{}


/**
 * Write a host name record to database
 */
bool HostnamesMongoDB::writeHostnameToDB( const MessageLogger::IPAddress & addr,
		const BW::string & hostname )
{
	BW::string addressString = inet_ntoa( (in_addr&)addr );

	// Queue up a task to insert the category map to database
	mongoDBTaskMgr_.addBackgroundTask( 
		new MongoDB::WriteIDStringPairTask< MessageLogger::IPAddress >
			( namespace_, MongoDB::HOST_NAMES_KEY_IP, addr,
				MongoDB::HOST_NAMES_KEY_HOSTNAME, hostname ) );

	// Success status is not known until the task is completed by the
	// background TaskManager thread.
	return true;
}


/**
 * Init host names from database. Throw exception upon error.
 * Prerequisite: The conn has been initialized and connected to MongoDB server
 */
bool HostnamesMongoDB::init()
{
	try
	{
		std::auto_ptr<mongo::DBClientCursor> cursor = conn_.query(
				namespace_.c_str(), mongo::BSONObj() );

		if (!cursor.get())
		{
			ERROR_MSG( "HostnamesMongoDB::init: Couldn't get cursor.\n" );
			return false;
		}

		while (cursor->more())
		{
			mongo::BSONObj rec = cursor->next();
			MessageLogger::IPAddress ip = rec.getIntField(
				MongoDB::HOST_NAMES_KEY_IP );
			BW::string hostname = rec.getStringField(
				MongoDB::HOST_NAMES_KEY_HOSTNAME );

			this->addHostnameToMap( ip, hostname );
		}
	}
	catch (mongo::DBException & e)
	{
		ERROR_MSG( "HostnamesMongoDB::init: "
				"Couldn't read hostnames from db: %s\n", e.what() );
		return false;
	}

	return true;
}

/**
 * Validate next hostname
 */
HostnamesValidatorProcessStatus HostnamesMongoDB::validateNextHostname()
{
	if (!HostnamesValidator::inProgress())
	{
		HostnameCopier hostnameCopier( this );
		this->visitAllWith( hostnameCopier );
	}

	HostnamesValidatorProcessStatus result =
			HostnamesValidator::validateNextHostname();

	if (result != BW_VALIDATE_HOSTNAMES_CONTINUE)
	{
		HostnamesValidator::clear();
	}

	return result;
}


/**
 * This will be called during host name validation proces
 */
bool HostnamesMongoDB::hostnameChanged( MessageLogger::IPAddress addr,
			const BW::string & newHostname )
{
	BW::string addressString = inet_ntoa( (in_addr&)addr );

	DEBUG_MSG( "HostnamesMongoDB::hostnameChanged: "
			"address: %s, new hostname:%s\n",
			addressString.c_str(), newHostname.c_str());

	this->updateHostname( addr, newHostname );

	// Queue up a task to insert the category map to database
	mongoDBTaskMgr_.addBackgroundTask(
		new MongoDB::UpdateIDStringPairTask< MessageLogger::IPAddress >
			( namespace_, MongoDB::HOST_NAMES_KEY_IP, addr,
				MongoDB::HOST_NAMES_KEY_HOSTNAME, newHostname ) );

	// Success status is not known until the task is completed by the
	// background TaskManager thread.
	return true;
}


BW_END_NAMESPACE
