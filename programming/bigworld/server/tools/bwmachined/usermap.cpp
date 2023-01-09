#include "usermap.hpp"
#include "common_machine_guard.hpp"
#include <syslog.h>


BW_BEGIN_NAMESPACE

/**
 * Constructor.
 */
UserMap::UserMap()
{
	// Set up the failed reply
	notfound_.uid_ = notfound_.UID_NOT_FOUND;
	notfound_.outgoing( true );
}


/**
 * This method finds user configurations for all known users on the
 * current host.
 */
void UserMap::queryUserConfs()
{
	struct passwd *pEnt;

	while ((pEnt = getpwent()) != NULL)
	{
		UserMessage um;
		um.outgoing( true );
		um.init( *pEnt );

		// Initially are only interested in users with a valid ~/.bwmachined.conf
		if (this->getEnv( um, /*userAlreadyKnown*/ true ))
		{
			this->add( um );
		}
	}
	endpwent();
}


/**
 * Adds the user message to the cache of users.
 */
void UserMap::add( const UserMessage & um )
{
	map_.insert( Map::value_type( um.uid_, um ) );
}


/**
 * This method adds a new user to the known users from a password file entry.
 */
UserMessage* UserMap::add( struct passwd *ent )
{
	UserMessage newguy;
	newguy.init( *ent );
	newguy.outgoing( true );

	this->getEnv( newguy );
	this->add( newguy );

	return this->fetch( newguy.uid_ );
}


/**
 * This method retrieves UserMessage for the specified user id.
 */
UserMessage* UserMap::fetch( uint16 uid )
{
	Map::iterator it;
	if ((it = map_.find( uid )) != map_.end())
		return &it->second;
	else
		return NULL;
}

namespace Util
{

/**
 * Helper function for determining whether a line from .bwmachined.conf is
 * empty or not.
 */
bool isEmpty( const char *buf )
{
	while (*buf)
	{
		if (!isspace( *buf ))
			return false;
		++buf;
	}

	return true;
}

} // namespace Util


/**
 * This method attempts to find the specified user's server environment
 * configuration as defined in .bwmachined.conf.
 */
bool UserMap::getEnv( UserMessage & um, bool userAlreadyKnown )
{
	char buf[ 1024 ], mfroot[ 256 ], bwrespath[ 1024 ];
	const char *filename = um.getConfFilename();
	bool hasFoundEnv = false;

	// If this uid doesn't exist on this system, fail now
	if (!userAlreadyKnown && (getpwuid( um.uid_ ) == NULL))
	{
		syslog( LOG_ERR, "Uid %d doesn't exist on this system!", um.uid_ );
		return false;
	}

	// first look in the user's home directory (under linux)
	FILE * file;

	if ((file = fopen( filename, "r" )) != NULL)
	{
		while (fgets( buf, sizeof(buf)-1, file ) != NULL)
		{
			if (buf[0] != '#' && buf[0] != 0)
			{
				if (sscanf( buf, "%[^;];%s", mfroot, bwrespath ) == 2)
				{
					um.mfroot_ = mfroot;
					um.bwrespath_ = bwrespath;
					hasFoundEnv = true;
					break;
				}
				else if (!Util::isEmpty( buf ))
				{
					syslog( LOG_ERR, "%s has invalid line '%s'\n",
						filename, buf );
				}
			}
		}
	}

	if (file != NULL)
	{
		fclose( file );
	}

	if (hasFoundEnv)
	{
		return true;
	}

	// Now consult the global file in /etc/ (or C:program files/bigworld/).
	// Don't warn on missing /etc/bigworld.conf since this isn't strictly
	// required.
	if ((file = fopen( machinedConfFile, "r" )) == NULL)
	{
		return false;
	}

	while (fgets( buf, sizeof(buf)-1, file ) != NULL)
	{
		if (buf[0] == '#' || buf[0] == 0)
		{
			continue;
		}

		if (buf[0] == '[')
		{
			// Reached the tags section. Break out and fail.
			break;
		}

		int file_uid;

		if (sscanf( buf, "%d;%[^;];%s", &file_uid, mfroot, bwrespath ) == 3 &&
			file_uid == um.uid_)
		{
			um.mfroot_ = mfroot;
			um.bwrespath_ = bwrespath;
			hasFoundEnv = true;
			break;
		}
	}

	fclose( file );

	return hasFoundEnv;
}


/**
 * This method updates all user configurations.
 */
void UserMap::flush()
{
	syslog( LOG_INFO, "Starting user map flush\n" );
	map_.clear();
	this->queryUserConfs();
	syslog( LOG_INFO, "Finished user map flush\n" );
}

BW_END_NAMESPACE

// usermap.cpp
