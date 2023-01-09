#ifndef LOG_ON_RECORDS_CACHE_HPP
#define LOG_ON_RECORDS_CACHE_HPP

#include "db_storage/entity_key.hpp"
#include "network/basictypes.hpp"
#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

class BackupHash;

class LogOnRecordsCache
{
public:
	void insert( const EntityKey & entityKey, const EntityMailBoxRef & mb );
	void erase( const EntityKey & entityKey );
	void remapMailboxes( const Mercury::Address & oldAddr,
			const BackupHash & destAddrs );
	bool lookUp( const EntityKey & entityKey, EntityMailBoxRef & mb ) const;

private:

	typedef BW::map< DatabaseID, EntityMailBoxRef > MailBoxMap;
	typedef BW::map< EntityTypeID, MailBoxMap > MailBoxMapCache;
	MailBoxMapCache cache_;
};

BW_END_NAMESPACE

#endif // LOG_ON_RECORDS_CACHE_HPP
