#include "log_on_records_cache.hpp"

#include "cstdmf/debug.hpp"
#include "server/backup_hash.hpp"

BW_BEGIN_NAMESPACE

bool LogOnRecordsCache::lookUp( const EntityKey & entityKey,
		EntityMailBoxRef & mb ) const
{
	MailBoxMapCache::const_iterator cacheIter = cache_.find( entityKey.typeID );

	if (cacheIter == cache_.end())
	{
		return false;
	}

	const MailBoxMap & mailboxes = cacheIter->second;
	MailBoxMap::const_iterator mbIter = mailboxes.find( entityKey.dbID );

	if (mbIter == mailboxes.end())
	{
		return false;
	}

	mb = mbIter->second;
	return true;
}


void LogOnRecordsCache::insert( const EntityKey & entityKey,
		const EntityMailBoxRef & mb )
{
	cache_[ entityKey.typeID ][ entityKey.dbID ] = mb;
}


void LogOnRecordsCache::erase( const EntityKey & entityKey )
{
	MailBoxMapCache::iterator cacheIter = cache_.find( entityKey.typeID );

	if (cacheIter == cache_.end() ||
		   	cacheIter->second.erase( entityKey.dbID ) != 1)
	{
		ERROR_MSG( "LogOnRecordsCache::erase: "
				"Could not find entity %ld of type %u\n",
			entityKey.dbID, entityKey.typeID );
	}
}


void LogOnRecordsCache::remapMailboxes( const Mercury::Address & oldAddr,
		const BackupHash & destAddrs )
{
	MailBoxMapCache::iterator cacheIter = cache_.begin();

	while (cacheIter != cache_.end())
	{
		MailBoxMap::iterator mbIter = cacheIter->second.begin();

		while (mbIter != cacheIter->second.end())
		{
			EntityMailBoxRef & mb = mbIter->second;

			if (mb.addr == oldAddr)
			{
				Mercury::Address newAddr =
						destAddrs.addressFor( mb.id );

				mb.addr.ip = newAddr.ip;
				mb.addr.port = newAddr.port;
			}

			++mbIter;
		}

		++cacheIter;
	}
}

BW_END_NAMESPACE
