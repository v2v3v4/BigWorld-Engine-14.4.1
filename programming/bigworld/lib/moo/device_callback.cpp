#include "pch.hpp"

#include "device_callback.hpp"

#include <algorithm>
#include "cstdmf/concurrency.hpp"
#include "cstdmf/debug.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "device_callback.ipp"
#endif

namespace Moo
{

struct DeviceCallbackStatics
{
	DeviceCallbackStatics()
		: callbacks_( NULL )
		, callbacksLock_( NULL )
	{
		REGISTER_SINGLETON( DeviceCallbackStatics )
	}

	static DeviceCallbackStatics & instance();

	DeviceCallback::CallbackList * callbacks_;
	SimpleMutex * callbacksLock_;
	DeviceCallback* s_curCallback;
};



DeviceCallbackStatics & DeviceCallbackStatics::instance()
{
	SINGLETON_MANAGER_WRAPPER( DeviceCallbackStatics )
	static DeviceCallbackStatics s_DeviceCallbackStatics;
	return s_DeviceCallbackStatics;
}

struct DeviceCallbackLockStruct {};
// we assume that there will not be thread contention for the
// creation of the first DeviceCallback and the deletion of the last.
static THREADLOCAL( bool ) s_locked;

bool & getLocked()
{
	REGISTER_SINGLETON_FUNC( DeviceCallbackLockStruct, &getLocked )
	SINGLETON_MANAGER_WRAPPER_FUNC( DeviceCallbackLockStruct, &getLocked )
	return s_locked;
}

/**
 *	Check to see the callback objects have all been destructed properly.
 */
void DeviceCallback::fini()
{
	BW_GUARD;
	// check leaking
	DeviceCallbackStatics & statics = DeviceCallbackStatics::instance();
	if (statics.callbacks_)
	{
		statics.callbacksLock_->grab();
		uint count = static_cast<uint>(statics.callbacks_->size());
		WARNING_MSG("%d DeviceCallback object(s) NOT DELETED\n", count);
		CallbackList::iterator it = statics.callbacks_->begin();
		for (; it!= statics.callbacks_->end(); it++)
		{
			WARNING_MSG("DeviceCallback: NOT DELETED : %p\n", (*it) );
		}
		statics.callbacksLock_->give();
	}
}


/**
 *	Constructor.
 */
DeviceCallback::DeviceCallback()
{
	BW_GUARD;

	DeviceCallbackStatics & statics = DeviceCallbackStatics::instance();

	if (!statics.callbacks_)	// assumed to be unthreaded
	{
    	statics.callbacks_ = new DeviceCallback::CallbackList;
		statics.callbacksLock_ = new SimpleMutex;
	}

	if ( !getLocked() )
	{
		statics.callbacksLock_->grab();
	}
	else
	{
		MF_ASSERT( this != statics.s_curCallback );
	}

	statics.callbacks_->push_back( this );

	if ( !getLocked() )
	{
		statics.callbacksLock_->give();
	}
}


/**
 *	Destructor.
 */
DeviceCallback::~DeviceCallback()
{
	BW_GUARD;

	DeviceCallbackStatics & statics = DeviceCallbackStatics::instance();

	if (statics.callbacks_)
    {
		if ( !getLocked() )
		{
			statics.callbacksLock_->grab();
		}
		else
		{
			MF_ASSERT( this != statics.s_curCallback );
		}

        CallbackList::iterator it = std::find( statics.callbacks_->begin(), statics.callbacks_->end(), this );
        if( it != statics.callbacks_->end() )
        {
            statics.callbacks_->erase( it );
        }

	    bool wasEmpty = statics.callbacks_->empty();

		if ( !getLocked() )
		{
			statics.callbacksLock_->give();
		}

		if (wasEmpty)	// assumed to be unthreaded
        {
	    	bw_safe_delete(statics.callbacks_);
			bw_safe_delete(statics.callbacksLock_);
        }
    }
}

void DeviceCallback::deleteUnmanagedObjects( )
{

}

void DeviceCallback::createUnmanagedObjects( )
{

}

void DeviceCallback::deleteManagedObjects( )
{

}

void DeviceCallback::createManagedObjects( )
{

}

void DeviceCallback::deleteAllUnmanaged( bool forceRelease )
{
	BW_GUARD;

	// if we're not in d3dex mode we have to release everything
	if (!Moo::rc().usingD3DDeviceEx())
	{
		forceRelease = true;
	}

	DeviceCallbackStatics & statics = DeviceCallbackStatics::instance();

	if ( statics.callbacks_ )
    {
		statics.callbacksLock_->grab();
		getLocked() = true;

        CallbackList::iterator it = statics.callbacks_->begin();
        CallbackList::iterator end = statics.callbacks_->end();

        while( it != end )
        {
			statics.s_curCallback = *it;
			if (forceRelease || (*it)->recreateForD3DExDevice())
			{
				(*it)->deleteUnmanagedObjects();
			}
            it++;
        }

		getLocked() = false;
		statics.callbacksLock_->give();
    }
}

void DeviceCallback::createAllUnmanaged( bool forceCreate /*= false*/)
{
	BW_GUARD;

	DeviceCallbackStatics & statics = DeviceCallbackStatics::instance();

	if ( statics.callbacks_ )
    {
		statics.callbacksLock_->grab();
		getLocked() = true;

        CallbackList::iterator it = statics.callbacks_->begin();
        CallbackList::iterator end = statics.callbacks_->end();

        while( it != end )
        {
			statics.s_curCallback = *it;
			if (forceCreate || (*it)->recreateForD3DExDevice())
			{
				(*it)->createUnmanagedObjects();
			}
            it++;
        }

		getLocked() = false;
		statics.callbacksLock_->give();
    }
}

void DeviceCallback::deleteAllManaged( )
{
	BW_GUARD;

	DeviceCallbackStatics & statics = DeviceCallbackStatics::instance();

	if ( statics.callbacks_ )
    {
		statics.callbacksLock_->grab();
		getLocked() = true;

        CallbackList::iterator it = statics.callbacks_->begin();
        CallbackList::iterator end = statics.callbacks_->end();

        while( it != end )
        {
			statics.s_curCallback = *it;
			(*it)->deleteManagedObjects();
            it++;
        }

		getLocked() = false;
		statics.callbacksLock_->give();
    }
}

void DeviceCallback::createAllManaged( )
{
	BW_GUARD;

	DeviceCallbackStatics & statics = DeviceCallbackStatics::instance();

	if ( statics.callbacks_ )
    {
		statics.callbacksLock_->grab();
		getLocked() = true;

        CallbackList::iterator it = statics.callbacks_->begin();
        CallbackList::iterator end = statics.callbacks_->end();

        while( it != end )
        {
			statics.s_curCallback = *it;
            (*it)->createManagedObjects();
            it++;
        }

		getLocked() = false;
		statics.callbacksLock_->give();
    }
}

GenericUnmanagedCallback::GenericUnmanagedCallback( Function* createFunction, Function* destructFunction  )
: createFunction_( createFunction ),
  destructFunction_( destructFunction )
{
}

GenericUnmanagedCallback::~GenericUnmanagedCallback( )
{
}

void GenericUnmanagedCallback::deleteUnmanagedObjects( )
{
	destructFunction_( );
}

void GenericUnmanagedCallback::createUnmanagedObjects( )
{
	createFunction_( );
}

} // namespace Moo

BW_END_NAMESPACE

// device_callback.cpp
