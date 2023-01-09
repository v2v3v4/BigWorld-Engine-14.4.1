#ifndef BW_STACK_ALLOCATOR_HPP
#define BW_STACK_ALLOCATOR_HPP

namespace BW
{

template< typename T, size_t CAPACITY >
class StackAllocator
	: public IAllocator
{
public:
	struct SourceBuffer
	{
		SourceBuffer()
			: memUsed_( 0 )
		{
		}

		void * allocate( size_t bytesNeeded )
		{
			if (memUsed_ + bytesNeeded > (CAPACITY + 1 ))
			{
				return NULL;
			}
			size_t pos = memUsed_;
			memUsed_ += bytesNeeded;
			lastAllocation_ = ( void * ) &buffer_[ pos ];
			return lastAllocation_;
		}

		bool deallocate( void * pMemory, size_t size )
		{
			if (pMemory > &buffer_[ CAPACITY ] ||
				pMemory < &buffer_[ 0 ])
			{
				return false;
			}
			if (pMemory == lastAllocation_)
			{
				memUsed_ -= size;
				lastAllocation_ = ( ( char * ) lastAllocation_ ) - size;
			}
			return true;
		}

		//Allocates one extra element for NULL terminator in string
		char        buffer_[ sizeof( T ) * ( CAPACITY + 1 ) ];
		size_t		memUsed_;
		void *		lastAllocation_;
	};

public:

	// For the straight up copy c-tor, we can share storage.
	StackAllocator(const StackAllocator< T, CAPACITY >& rhs)
		: sourceBuffer_(rhs.sourceBuffer_)
		, canAllocate_( rhs.canAllocate_ )
	{
	}


	template<typename U, size_t OTHERCAPACITY >
	StackAllocator( const StackAllocator< U, OTHERCAPACITY > & other )
		: sourceBuffer_( NULL )
		, canAllocate_( false )
	{
	}

	explicit StackAllocator( SourceBuffer * sourceBuffer )
		: sourceBuffer_( sourceBuffer )
		, canAllocate_( true )
	{
	}

	void * intAllocate( size_t size, const void * hint )
	{
		if (sourceBuffer_ == NULL)
		{
			return bw_new( size * sizeof( T ) );
		}
		void * obj = sourceBuffer_->allocate( size );
		if (obj != NULL)
		{
			return obj;
		}
		throw std::bad_alloc();
	}


	void intDeallocate( void * p, size_t size )
	{
		if (sourceBuffer_ == NULL)
		{
			bw_delete( reinterpret_cast< void * >( p ) );
			return;
		}
		if (sourceBuffer_->deallocate( p, size ))
		{
			return;
		}
		bw_delete( reinterpret_cast< void * >( p ) );
	}

	void setCanAllocate( bool canAllocate )
	{
		canAllocate_ = canAllocate;
	}

	bool canAllocate()
	{
		return canAllocate_;
	}

	size_t getMaxSize() const
	{
		return CAPACITY + 1;
	}

private:
	SourceBuffer * sourceBuffer_;
	bool		   canAllocate_;
};

} //BW_STACK_ALLOCATOR_HPP

#endif

