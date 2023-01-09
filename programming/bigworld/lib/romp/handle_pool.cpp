#include "pch.hpp"
#include "handle_pool.hpp"


BW_BEGIN_NAMESPACE

HandlePool::HandlePool( uint16 numHandles ):
	numHandles_( numHandles )
{
	this->reset();
}


/**
 *	This method returns a handle given an arbitrary 32 bit id.  If
 *	accessed, the handles used_ is set to true.
 *
 *	@param	id		some arbitrary id (perhaps a pointer...?)
 *
 *	@return	Handle	a handle that can be used for occlusion querying.
 */
HandlePool::Handle HandlePool::handleFromId( size_t id )
{	
	BW_GUARD;

	HandleMap::iterator it = handleMap_.find( id );
	if (it == handleMap_.end())
	{
		//allocate a new id
		if( !unusedHandles_.empty() )
		{
			int newHandle = unusedHandles_.top();
			unusedHandles_.pop();
			Info info;
			info.handle_ = newHandle;
			info.used_ = true;
			handleMap_.insert( std::make_pair( id,info ) );
			return newHandle;
		}
		else
		{
			//No more handles available right now.
			return INVALID_HANDLE;
		}
	}
	else
	{
		//flag as used, and return the handle
		Info& info = it->second;
		info.used_ = true;
		return info.handle_;
	}

	// Can't actually get here
	return INVALID_HANDLE;
}


void HandlePool::beginFrame()
{
	BW_GUARD;
	HandleMap::iterator it = handleMap_.begin();
	HandleMap::iterator end = handleMap_.end();

	while ( it != end )
	{
		Info& info = it->second;
		info.used_ = false;
		it++;
	}
}


void HandlePool::endFrame()
{
	BW_GUARD;
	//find all unused ids - remove from map, and push onto the unused ids stack
	static BW::vector<size_t>	eraseMe;

	HandleMap::iterator it = handleMap_.begin();
	HandleMap::iterator end = handleMap_.end();

	while ( it != end )
	{
		Info& info = it->second;
		if ( !info.used_ )
		{
			unusedHandles_.push( info.handle_ );
			eraseMe.push_back( it->first );
		}
		it++;
	}

	BW::vector<size_t>::iterator eit = eraseMe.begin();
	BW::vector<size_t>::iterator een = eraseMe.end();
	while ( eit != een )
	{
		handleMap_.erase( *eit++ );
	}

	eraseMe.clear();
}


void HandlePool::reset()
{
	BW_GUARD;
	handleMap_.clear();

	while ( !unusedHandles_.empty() )
	{
		unusedHandles_.pop();
	}

	for ( int i=numHandles_-1; i>=0; i-- )
	{
		unusedHandles_.push( i );
	}
}

BW_END_NAMESPACE

// handle_pool.cpp
