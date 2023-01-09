#ifndef POOL_ALLOCATOR_HPP
#define POOL_ALLOCATOR_HPP

#include "concurrency.hpp"
#include "watcher.hpp"

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

/**
 *  A PoolAllocator is an object that never truly frees instances once
 *  allocated.  Therefore it should be used for small, frequently
 *  constructed/destroyed objects that will maintain a roughly constant active
 *  population over time.
 *
 *  Mercury::Packet is a good example of an object that fits this description.
 */
template <class MUTEX = DummyMutex>
class PoolAllocator
{
	struct FreeList
	{
		FreeList * next;
	};
public:
	/**
	 *  Initialise a new empty Pool.
	 */
	PoolAllocator( size_t size, const char * watcherPath = NULL ) :
		pHead_( NULL ),
		numInPoolUsed_( 0 ),
		numInPoolTotal_( 0 ),
		numAllocatesEver_( 0 ),
		size_( size )
	{
		MF_ASSERT( size >= sizeof (FreeList) );
#if ENABLE_WATCHERS
		if (watcherPath)
		{
			Watcher::rootWatcher().addChild( watcherPath,
					this->pWatcher(), this );
		}
#endif
}

	/**
	 *  This method frees all memory used in this pool.  This is just for
	 *  completeness, as typically this should only be called on exit.
	 */
	~PoolAllocator()
	{
		while (pHead_)
		{
			FreeList * pNext = pHead_->next;
			delete [] (char*)pHead_;
			pHead_ = pNext;
			--numInPoolTotal_;
		}
	}


	/**
	 *  This method returns a pointer to an available PooledObject instance,
	 *  allocating memory for a new one if necessary.
	 */
	void * allocate( size_t size )
	{
		MF_ASSERT( size == size_ );

		void * ret;

		mutex_.grab();
		{
			// Grab an instance from the pool if there's one available.
			if (pHead_)
			{
				ret = (void*)pHead_;
				pHead_ = pHead_->next;
			}

			// Otherwise just allocate new memory and return that instead.
			else
			{
				ret = (void*)new char[ size ];
				++numInPoolTotal_;
			}

			++numInPoolUsed_;
			++numAllocatesEver_;
		}
		mutex_.give();

		return ret;
	}

	/**
	 *  This method returns a deleted instance of PooledObject to the pool.
	 */
	void deallocate( void * pInstance )
	{
		mutex_.grab();
		{
			FreeList * pNewHead = (FreeList *)pInstance;
			pNewHead->next = pHead_;
			pHead_ = pNewHead;
		}
		--numInPoolUsed_;
		mutex_.give();
	}

	/**
	 *	This method returns the total number of instances in the pool being used
	 *	currently.
	 */
	uint	numInPoolUsed() const { return numInPoolUsed_; }

	/**
	 *	This method returns the total number of instances in the pool not being
	 *	used currently.
	 */
	uint	numInPoolUnused() const { return numInPoolTotal_ - numInPoolUsed_; }

	/**
	 *	This method returns the total number of instances in the pool, allocated
	 *	or not.
	 */
	uint	numInPoolTotal() const { return numInPoolTotal_; }

	/**
	 *	This method returns the total number of calls to allocate ever.
	 */
	uint	numAllocatesEver() const { return numAllocatesEver_; }

	/**
	 *	This method returns the expected allocation size.
	 */
	size_t	size() const { return size_; }

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher()
	{
		DirectoryWatcherPtr pWatcher = new DirectoryWatcher();

		PoolAllocator< MUTEX > * pNull = NULL;

		pWatcher->addChild( "numInPoolUsed",
				makeWatcher( pNull->numInPoolUsed_ ) );
		pWatcher->addChild( "numInPoolUnused",
				makeWatcher( *pNull, &PoolAllocator< MUTEX >::numInPoolUnused ) );

		pWatcher->addChild( "numInPoolTotal",
				makeWatcher( pNull->numInPoolTotal_ ) );
		pWatcher->addChild( "numAllocatesEver",
				makeWatcher( pNull->numAllocatesEver_ ) );

		pWatcher->addChild( "size", makeWatcher( pNull->size_ ) );

		return pWatcher;
	}
#endif

private:
	/// The linked-list of memory chunks in the pool.
	FreeList * pHead_;

	/// The total number of instances in the pool being used currently.
	uint	numInPoolUsed_;

	/// The total number of instances in the pool, allocated or not.
	uint	numInPoolTotal_;

	/// The total number of calls to allocate ever.
	uint	numAllocatesEver_;

	/// The expected allocation size.
	size_t	size_;

	/// A lock to guarantee thread-safety.
	MUTEX mutex_;
};

BW_END_NAMESPACE

#endif // POOL_ALLOCATOR_HPP
