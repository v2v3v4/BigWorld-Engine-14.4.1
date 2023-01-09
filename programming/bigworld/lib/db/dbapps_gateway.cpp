#include "dbapps_gateway.hpp"

#include "cstdmf/binary_stream.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This template specialises streaming for DBAppsGateway.
 *
 *	This needs to be done so we don't have to stream DBApp ID twice: once as
 *	the bucket, and once again for the data for the descriptor.
 */
template<>
struct DBAppsGateway::HashScheme::MapStreaming
{
	template< typename BUCKET_MAP >
	static void addToStream( BinaryOStream & os,
			const BUCKET_MAP & map )
	{
		os.writePackedInt( static_cast< int >( map.size() ) );
		for (typename BUCKET_MAP::const_iterator iter = map.begin();
				iter != map.end();
				++iter)
		{
			os << iter->first << iter->second.address();
		}
	}

	template< typename BUCKET_MAP >
	static bool readFromStream( BinaryIStream & is, BUCKET_MAP & map )
	{
		map.clear();

		int size = is.readPackedInt();

		while ((size-- > 0) && !is.error())
		{
			DBAppID id;
			Mercury::Address address;
			is >> id >> address;

			map.insert( BucketMap::value_type( id,
				DBAppGateway( id, address ) ) );
		}

		return !is.error();
	}
};


/** Out-streaming operator for DBAppsGateway. */
BinaryOStream & operator<<( BinaryOStream & os, const DBAppsGateway & gateway )
{
	return os << gateway.hashScheme_;
}


/** In-streaming operator for DBAppsGateway. */
BinaryIStream & operator>>( BinaryIStream & is, DBAppsGateway & gateway )
{
	return is >> gateway.hashScheme_;
}


/**
 *	Constructor.
 */
DBAppsGateway::DBAppsGateway() :
		hashScheme_()
{}


/**
 *	This method adds a DBApp to the collection.
 */
void DBAppsGateway::addDBApp( const DBAppGateway & descriptor )
{
	hashScheme_.insert( std::make_pair( descriptor.id(), descriptor ) );
}


/**
 *	This method removes a DBApp from the collection.
 *
 *	@param descriptor 
 */
bool DBAppsGateway::removeDBApp( DBAppID appID )
{
	return hashScheme_.erase( appID );
}


/**
 *	This method updates the gateway hash with an input stream. It can also take
 *	an optional visitor object to notify of changes.
 *
 *	@param data 	The input stream.
 *	@param pVisitor The visitor, or NULL if visiting is not required.
 *
 *	@return 		false if a stream error occurred, true otherwise.
 */
bool DBAppsGateway::updateFromStream( BinaryIStream & data,
		IUpdateVisitor * pVisitor /* = NULL */ )
{
	HashScheme oldHashScheme = hashScheme_;

	data >> hashScheme_;

	if (data.error())
	{
		return false;
	}

	if (!pVisitor)
	{
		return true;
	}

	HashScheme::const_iterator iter = hashScheme_.begin();
	while (iter != hashScheme_.end())
	{
		if (oldHashScheme.count( iter->first ) == 0)
		{
			pVisitor->onDBAppAdded( *this, iter->second, 
				/* isAlpha */ (iter == hashScheme_.begin()) );
		}

		++iter;
	}

	iter = oldHashScheme.begin();
	while (iter != oldHashScheme.end())
	{
		if (hashScheme_.count( iter->first ) == 0)
		{
			pVisitor->onDBAppRemoved( *this, iter->second, 
				/* wasAlpha */ (iter == oldHashScheme.begin()) );
		}

		++iter;
	}

	return true;
}


/**
 *	This method returns the DBApp descriptor for the given database ID.
 *
 *	@param dbID 	The database ID. If zero, then the alpha DBApp is returned.
 *	@return 		The DBApp to handle that database ID, or
 *					DBAppGateway::NONE if the DBApp map is empty.
 */
const DBAppGateway & DBAppsGateway::getDBApp( DatabaseID dbID /* = 0 */ ) const
{
	if (hashScheme_.empty())
	{
		return DBAppGateway::NONE;
	}

	if (dbID == 0)
	{
		return this->alpha();
	}

	return hashScheme_[ dbID ];
}


/**
 *	This method returns DBApp Alpha.
 */
const DBAppGateway & DBAppsGateway::alpha() const
{
	if (hashScheme_.empty())
	{
		return DBAppGateway::NONE;
	}

	 // TODO: Scalable DB: When we switch to use
	 // DBAppHashSchemes::StringBuckets, we will need to be configured to know
	 // what the alpha storage shard is, and which DBApp is handling it.
	return hashScheme_.smallest().second;
}


/**  *	This method returns a watcher for this class.
 */
WatcherPtr DBAppsGateway::pWatcher()
{
	DBAppsGateway * pNull = NULL;

	Watcher * pWatcher = new MapWatcher< HashScheme >(
		pNull->hashScheme_ );

	pWatcher->addChild( "*", DBAppGateway::pWatcher() );

	return pWatcher;
}

BW_END_NAMESPACE

// dbapps_gateway.cpp
