#include "pch.hpp"

#include "dynamic_vertex_buffer.hpp"
#include "render_context.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "dynamic_vertex_buffer.ipp"
#endif

namespace Moo
{

// This static list holds all the instances created. It is used when it is
// needed to have access to all vertex buffers, such as when finalising the
// application to delete all instances.
BW::list< DynamicVertexBufferBase* > DynamicVertexBufferBase::dynamicVBS_;

// This static will be set to true after the fini method is called.
bool DynamicVertexBufferBase::s_finalised = false;


/**
 *	This method releases all resources held by all dynamic vertex buffers.
 */
void DynamicVertexBufferBase::releaseAll( )
{
	BW_GUARD;
	BW::list< DynamicVertexBufferBase* >::iterator it = dynamicVBS_.begin();

	while( it != dynamicVBS_.end() )
	{
		(*it)->release();
		it++;
	}
}

/**
 *	This method resets the lock on all dynamic vertex buffers.
 */
void DynamicVertexBufferBase::resetLockAll( )
{
	BW_GUARD;
	BW::list< DynamicVertexBufferBase* >::iterator it = dynamicVBS_.begin();

	while( it != dynamicVBS_.end() )
	{
		(*it)->resetLock();
		it++;
	}
}


/**
 *	This method deletes all created vertex buffers.
 */
/*static*/ void DynamicVertexBufferBase::fini( )
{
	BW_GUARD;
	s_finalised = true;

	// Each time a vertex buffer gets deleted, it removes itself from the list,
	// so just looping until the list is empty.
	while (!dynamicVBS_.empty())
	{
		delete dynamicVBS_.front();
	}
}


void DynamicVertexBufferBase::deleteUnmanagedObjects( )
{
	release();
}

DynamicVertexBufferBase::DynamicVertexBufferBase()
: locked_( false ),
  lockBase_( 0 ),
  maxElements_( 2000 ),
  lockIndex_( 0 ),
  reset_( true )
{
	BW_GUARD;
	dynamicVBS_.push_back( this );
}

DynamicVertexBufferBase::~DynamicVertexBufferBase()
{
	BW_GUARD;
	release();
	BW::list< DynamicVertexBufferBase* >::iterator it = std::find( dynamicVBS_.begin(), dynamicVBS_.end(), this );
	if( it != dynamicVBS_.end() )
		dynamicVBS_.erase( it );
}



HRESULT DynamicVertexBufferBase::create( )
{
	BW_GUARD;
	// release the buffer just in case
	release();
	HRESULT hr = E_FAIL;

	// is the device ready?
	if( Moo::rc().device() != NULL )
	{
		Moo::VertexBuffer buffer;

		DWORD usage = D3DUSAGE_DYNAMIC;
		if (softwareBuffer_)
			usage |= D3DUSAGE_SOFTWAREPROCESSING;
		if (readOnly_)
			usage |= D3DUSAGE_WRITEONLY;

		D3DPOOL pool;

		if (softwareBuffer_)
			pool = D3DPOOL_SYSTEMMEM;
		else
			pool = D3DPOOL_DEFAULT;

		// Try to create the vertexbuffer with maxElements_ number of elements
		if( SUCCEEDED( hr = buffer.create( maxElements_ * vertexSize_, usage, 0, pool, "vertex buffer/dynamic" ) ) )
		{
			vertexBuffer_ = buffer;
			lockBase_ = 0;
			locked_ = false;
		}
	}
	return hr;
}

BYTE* DynamicVertexBufferBase::lock( uint32 nLockElements )
{
	BW_GUARD_PROFILER( DynamicVertexBufferBase_lock );

	IF_NOT_MF_ASSERT_DEV( nLockElements != 0 )
	{
		return NULL;
	}

	IF_NOT_MF_ASSERT_DEV( locked_ == false )
	{
		return NULL;
	}

	BYTE* lockedVertices = NULL;
	// Only try to lock if the device is ready.
	if( Moo::rc().device() != NULL )
	{
		bool lockBuffer = true;

		// Can we fit the number of elements into the vertexbuffer?
		if( maxElements_ < nLockElements )
		{
			// If not release the buffer so it can be recreated with the right size.
			release();
			maxElements_ = nLockElements;
		}

		// Do we have a buffer?
		if( !vertexBuffer_.valid() )
		{
			// Create a new buffer
			if( FAILED( create( ) ) )
			{
				// Something went wrong, do not try to lock the buffer.
				lockBuffer = false;
			}
		}

		// Should we attempt to lock?
		if( lockBuffer )
		{
			DWORD lockFlags = 0;

			// Can we fit our vertices in the sofar unused portion of the vertexbuffer?

			if (!lockFromStart_)
			{
				if( ( lockBase_ + nLockElements ) <= maxElements_ )
				{
					// Lock from the current position
					lockFlags = D3DLOCK_NOOVERWRITE;
				}
				else
				{
					// Lock from the beginning.
					lockFlags = D3DLOCK_DISCARD;
				}
			}
			else if (lockModeDiscard_)
			{
				lockFlags = D3DLOCK_DISCARD;
			}

			BYTE* vertices = NULL;

			// Try to lock the buffer, if we succeed update the return value to use the locked vertices
			if( SUCCEEDED( vertexBuffer_.lock( 0, nLockElements * vertexSize_, (void**)&vertices, lockFlags ) ) )
			{				
				lockIndex_ = 0;
				lockBase_ += nLockElements;
				lockedVertices = vertices;
				locked_ = true;
			}
		}
	}
	return lockedVertices;
}

BYTE* DynamicVertexBufferBase::lock2( uint32 nLockElements )
{
	BW_GUARD;

	IF_NOT_MF_ASSERT_DEV( nLockElements != 0 )
	{
		return NULL;
	}

	IF_NOT_MF_ASSERT_DEV( locked_ == false )
	{
		return NULL;
	}

	BYTE* lockedVertices = NULL;

	// Only try to lock if the device is ready.
	if( Moo::rc().device() != NULL )
	{
		bool lockBuffer = true;

		// Can we fit the number of elements into the vertexbuffer?
		if( reset_ )
		{
			lockBase_ = 0;
		}

		if( maxElements_ < nLockElements + lockBase_  )
		{
			int newSize = nLockElements + lockBase_;

			// If not release the buffer so it can be recreated with the right size.
			release();
			maxElements_ = newSize;
		}

		// Do we have a buffer?
		if( !vertexBuffer_.valid() )
		{
			// Create a new buffer
			if( FAILED( create( ) ) )
			{
				// Something went wrong, do not try to lock the buffer.
				lockBuffer = false;
			}
		}

		// Should we attempt to lock?
		if( lockBuffer )
		{
			DWORD lockFlags = 0;

			// Can we fit our vertices in the sofar unused portion of the vertexbuffer?
			if( (!reset_) && (( lockBase_ + nLockElements ) <= maxElements_) )
			{
				// Lock from the current position
				lockFlags = D3DLOCK_NOOVERWRITE;
			}
			else
			{
				// Lock from the beginning.
				lockFlags = D3DLOCK_DISCARD;
				lockBase_ = 0;
				reset_ = false;
			}

			BYTE* vertices = NULL;

			// Try to lock the buffer, if we succeed update the return value to use the locked vertices
			if( SUCCEEDED( vertexBuffer_.lock( lockBase_ * vertexSize_, nLockElements * vertexSize_, (void**)&vertices, lockFlags ) ) )
			{
				lockIndex_ = lockBase_;
				lockBase_ += nLockElements;
				lockedVertices = vertices;
				locked_ = true;
			}

		}
	}
	return lockedVertices;
}

std::ostream& operator<<(std::ostream& o, const DynamicVertexBufferBase& t)
{
	o << "DynamicVertexBufferBase\n";
	return o;
}

/**
 * This class represents a container for holding a collection of un-owned 
 * instance pointers that are to be indexed by a sparse distribution of integer
 * based keys. The idea is to use a collection of smaller IndexType (smaller than a pointer) to
 * indexing into a densely distributed instance container to save memory while
 * still allowing fast, constant-time lookup that doesn't require searching.
 */
template <typename IndexType, class T>
class SparseInstanceIndexer
{
public:
	SparseInstanceIndexer( T * defaultInstance = NULL)
	{
		// There should always be a index 0 instance.
		instances_.push_back(defaultInstance);
	}

	T * getInstance( IndexType index )
	{
		growIndexesTo( index + 1 );
		return instances_[indexes_[index]];
	}

	void growIndexesTo( size_t count )
	{
		MF_ASSERT( count < static_cast<size_t>(std::numeric_limits<IndexType>::max()) );
		size_t indexSize = indexes_.size();
		if (count > indexSize)
		{
			// Need to expand, set all new "sparse" values to zero.
			indexes_.resize( count );
			for (size_t i = indexSize; i < count; ++i )
			{
				indexes_[i] = 0;
			}
		}
	}

	void setInstance( IndexType index, T * instance )
	{
		// First see if we have the instance
		bool found = false;
		size_t instancePos;
		for (instancePos = 0; instancePos < instances_.size(); ++instancePos)
		{
			if (instances_[instancePos] == instance)
			{
				found = true;
			}
		}
		if (!found)
		{
			// We do not have the instance already, need to add it.
			instances_.push_back(instance);
			instancePos = instances_.size() - 1;
		}

		// Set the index to point to the instance
		growIndexesTo( index + 1 );
		indexes_[index] = static_cast<IndexType>(instancePos);
	}


protected:
	// TODO: replace _indexes with cstdmf's lookup table.
	BW::vector<IndexType> indexes_; 
	BW::vector<T *> instances_; 

private:
	SparseInstanceIndexer( const SparseInstanceIndexer& other );
	SparseInstanceIndexer& operator=( const SparseInstanceIndexer& other );
};

template <class BufferType>
struct BufferMap
{
public:
	typedef SparseInstanceIndexer< int, BufferType > InstanceMap;

	BufferMap()
	{
		REGISTER_SINGLETON( BufferMap< BufferType > )
	}

	static InstanceMap & instance()
	{
		SINGLETON_MANAGER_WRAPPER( BufferMap< BufferType > );

		static InstanceMap s_instances;
		return s_instances;
	}
};

/**
 *	This method returns a DynamicVertexBuffer instance.
 */
DynamicVertexBuffer &
DynamicVertexBuffer::instance( int vertexSize )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( !s_finalised )
	{
		MF_EXIT( "access after finalisation" );
	}

	// The static instance is created here, it registers itself in the base
	// class static list of vertex buffers, and gets deleted when the base
	// class fini method is called.
	typedef DynamicVertexBuffer InstanceType;

	BufferMap< InstanceType >::InstanceMap & map = BufferMap< InstanceType >::instance();
	InstanceType * instance = map.getInstance( vertexSize );
	if (instance == NULL)
	{
		instance = new InstanceType( vertexSize );
		map.setInstance( vertexSize, instance );
	}

	MF_ASSERT( instance != NULL );
	MF_ASSERT( vertexSize == instance->vertexSize_ );
	return *instance;
}


/**
 *	This method returns a DynamicSoftwareVertexBuffer instance.
 */
DynamicSoftwareVertexBuffer &
DynamicSoftwareVertexBuffer::instance( int vertexSize )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( !s_finalised )
	{
		MF_EXIT( "access after finalisation" );
	}

	// The static instance is created here, it registers itself in the base
	// class static list of vertex buffers, and gets deleted when the base
	// class fini method is called.
	typedef DynamicSoftwareVertexBuffer InstanceType;
	typedef SparseInstanceIndexer<int, InstanceType> InstanceMap;
	BufferMap< InstanceType >::InstanceMap & map = BufferMap< InstanceType >::instance();

	InstanceType * instance = map.getInstance( vertexSize );
	if (instance == NULL)
	{
		instance = new InstanceType( vertexSize );
		map.setInstance( vertexSize, instance );
	}

	MF_ASSERT( instance != NULL );
	MF_ASSERT( vertexSize == instance->vertexSize_ );
	return *instance;
}

DynamicVertexBufferBase2& 
DynamicVertexBufferBase2::instance( int vertexSize )
{
	IF_NOT_MF_ASSERT_DEV( !s_finalised )
	{
		MF_EXIT( "access after finalisation" );
	}
	if( Moo::rc().mixedVertexProcessing() )
	{
		return DynamicSoftwareVertexBuffer::instance( vertexSize );
	}
	else
	{
		return DynamicVertexBuffer::instance( vertexSize );
	}
}


} // namespace Moo

BW_END_NAMESPACE

// dynamic_vertex_buffer.cpp
