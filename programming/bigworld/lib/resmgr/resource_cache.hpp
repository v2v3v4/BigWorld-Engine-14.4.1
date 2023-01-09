#ifndef _RESOURCE_HPP__
#define _RESOURCE_HPP__

#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

/**********************************************
 * This class is used to hold some reference of resources
 * which should be cached in memory for efficiency
 * every resource could be unloaded without crash
 *********************************************/
class CachedResource
{
	void* key_;
public:
	CachedResource( void* key = NULL );
	virtual void init(){};
	virtual void fini(){};
	virtual ~CachedResource();
};

typedef CachedResource* CachedResourcePtr;

template<typename SP>
class SmartPointerCache : public CachedResource
{
	SP sp_;
public:
	SmartPointerCache( SP sp ) : CachedResource( sp.getObject() ), sp_( sp )
	{}
	virtual void fini()
	{
		delete this;
	}
};

class ResourceCache
{
	BW::map<void*,CachedResourcePtr> resources_;
	bool inited_;
public:
	ResourceCache() : inited_( false )
	{}
	~ResourceCache()
	{
		fini();
	}
	static ResourceCache& instance()
	{
		static ResourceCache ResourceCache;
		return ResourceCache;
	}
	void registerResource( void* key, CachedResourcePtr resource )
	{
		if( resources_.find( key ) == resources_.end() )
		{
			resources_[ key ] = resource;
			if( inited_ )
				resource->init();
		}
	}
	void unregisterResource( void* key )
	{
		BW::map<void*,CachedResourcePtr>::iterator iter = resources_.find( key );
		if( iter != resources_.end() )
		{
			if( inited_ )
				(*iter).second->fini();
			resources_.erase( iter );
		}
	}
	template<typename SP>
	void addResource( SP sp )
	{
		if( resources_.find( sp.getObject() ) == resources_.end() )
			new SmartPointerCache<SP>( sp );
	}
	void init()
	{
		if( !inited_ )
		{
			for( BW::map<void*,CachedResourcePtr>::iterator iter = resources_.begin();
				iter != resources_.end(); ++iter )
			{
				iter->second->init();
			}
			inited_ = true;
		}
	}
	void fini()
	{
		if( inited_ )
		{
			inited_ = false;// set inited to false so fini won't be called for a second time
			BW::map<void*,CachedResourcePtr> copy = resources_;
			for( BW::map<void*,CachedResourcePtr>::iterator iter = copy.begin();
				iter != copy.end(); ++iter )
			{
				iter->second->fini();
			}
			resources_.clear();
		}
	}
};

inline CachedResource::CachedResource( void* key )
	:key_( key )
{
	if( !key_ )
		key_ = this;
	ResourceCache::instance().registerResource( key_, this );
}

inline CachedResource::~CachedResource()
{
	ResourceCache::instance().unregisterResource( key_ );
}

BW_END_NAMESPACE

#endif//_RESOURCE_HPP__
