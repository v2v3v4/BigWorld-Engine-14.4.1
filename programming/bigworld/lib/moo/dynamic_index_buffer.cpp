#include "pch.hpp"

#include "dynamic_index_buffer.hpp"

#include "render_context.hpp"
#include "cstdmf/profiler.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{


//-----------------------------------------------------------------------------
// Section: DynamicIndexBufferInterface
//-----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param usage		D3D usage flags.
 *	@param format		D3D surface format.
 */
DynamicIndexBufferBase::DynamicIndexBufferBase( DWORD usage, D3DFORMAT format )
:	locked_( false ),
	lockBase_( 0 ),
	lockIndex_( 0 ),
	maxIndices_( 2000 ),
	usage_( usage ),
	reset_( true ),
	format_( format )
{}

/**
 *	Destructor.
 */
DynamicIndexBufferBase::~DynamicIndexBufferBase()
{}

/**
 *	Release the index buffers D3D resources.
 */
void DynamicIndexBufferBase::release()
{
	indexBuffer_.release();
}

/**
 *	Release the index buffers D3D resources.
 */
void DynamicIndexBufferBase::deleteUnmanagedObjects()
{
	release();
}

/**
 *	Flag a reset/recreation of the index buffer.
 */
void DynamicIndexBufferBase::resetLock()
{
	reset_ = true;
}

/**
 *	Lock the index buffer for the required size / format. This method will
 *	not grow the existing size but mearly re-create it.
 *
 *	@param nLockIndices			Number of indices to lock.
 *
 *	@return A reference to the locked portion of memory representing the buffer.
 */
IndicesReference DynamicIndexBufferBase::lock( uint32 nLockIndices )
{
	BW_GUARD;
	IndicesReference result;

	IF_NOT_MF_ASSERT_DEV( nLockIndices != 0 )
	{
		return result;
	}

	IF_NOT_MF_ASSERT_DEV( locked_ == false )
	{
		return result;
	}

	// Only try to lock if the device is ready.
	if( Moo::rc().device() != NULL )
	{
		bool lockBuffer = true;

		// Can we fit the number of elements into the indexbuffer?
		if( maxIndices_ < nLockIndices )
		{
			// If not release the buffer so it can be recreated with the right size.
			release();
			maxIndices_ = nLockIndices;
		}

		// Do we have a buffer?
		if( !indexBuffer_.valid() )
		{
			// Create a new buffer
			if( FAILED( indexBuffer_.create( maxIndices_, format_, usage_, D3DPOOL_DEFAULT ) ) )
			{
				// Something went wrong, do not try to lock the buffer.
				lockBuffer = false;
			}
		}

		// Should we attempt to lock?
		if( lockBuffer )
		{
			DWORD lockFlags = 0;

			// Can we fit our vertices in the so far unused portion of the indexbuffer?
			if( ( lockBase_ + nLockIndices ) <= maxIndices_ )
			{
				// Lock from the current position
				lockFlags = D3DLOCK_NOOVERWRITE;
			}
			else
			{
				// Lock from the beginning.
				lockFlags = D3DLOCK_DISCARD;
				lockBase_ = 0;
			}

			// Try to lock the buffer, if we succeed update the return value to use the locked vertices
			result = indexBuffer_.lock( lockBase_, nLockIndices, lockFlags );
			if( result.size() )
			{
				lockIndex_ = lockBase_;
				lockBase_ += nLockIndices;
				locked_ = true;
			}
		}
	}
	return result;

}

/**
 *	Lock the index buffer for the required size / format. This method WILL
 *	grow the existing size of the buffer unlike lock(..).
 *
 *	@param nLockIndices			Number of indices to lock.
 *
 *	@return A reference to the locked portion of memory representing the buffer.
 */
IndicesReference DynamicIndexBufferBase::lock2( uint32 nLockIndices )
{
	BW_GUARD_PROFILER( DynamicIndexBufferBase_lock2 );

	IndicesReference result;

	IF_NOT_MF_ASSERT_DEV( nLockIndices != 0 )
	{
		return result;
	}

	IF_NOT_MF_ASSERT_DEV( locked_ == false )
	{
		return result;
	}

	// Only try to lock if the device is ready.
	if( Moo::rc().device() != NULL )
	{
		bool lockBuffer = true;

		// Can we fit the number of elements into the indexbuffer?
		if( reset_ )
		{
			lockBase_ = 0;
		}

		if( maxIndices_ < nLockIndices + lockBase_ )
		{
			int newSize = nLockIndices + lockBase_;

			// If not release the buffer so it can be recreated with the right size.
			release();
			maxIndices_ = newSize;
			lockBase_ = 0;
		}

		// Do we have a buffer?
		if( !indexBuffer_.valid() )
		{
			BW_GUARD_PROFILER( DynamicIndexBufferBase_lock2Create );
			// Create a new buffer
			if( FAILED( indexBuffer_.create( maxIndices_, format_, usage_, D3DPOOL_DEFAULT ) ) )
			{
				// Something went wrong, do not try to lock the buffer.
				lockBuffer = false;
			}
		}

		// Should we attempt to lock?
		if( lockBuffer )
		{
			DWORD lockFlags = 0;

			// Can we fit our vertices in the sofar unused portion of the indexbuffer?
			if( (!reset_) && (( lockBase_ + nLockIndices ) <= maxIndices_) )
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

			// Try to lock the buffer, if we succeed update the return value to use the locked vertices
			result = indexBuffer_.lock( lockBase_, nLockIndices, lockFlags );
			if( result.size() )
			{
				lockIndex_ = lockBase_;
				lockBase_ += nLockIndices;
				locked_ = true;
			}

		}
	}
	return result;
}

/**
 *	Unlock a previously locked index buffer.
 *
 *	@return The result of the lock.
 */
HRESULT DynamicIndexBufferBase::unlock()
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( locked_ == true )
	{
		return D3D_OK; // not locked
	}

	locked_ = false;
	return indexBuffer_.unlock();
}


//-----------------------------------------------------------------------------
// Section: DynamicIndexBufferInterface
//-----------------------------------------------------------------------------

/** 
 *	This method returns the appropriate index buffer depending on the format
 *	input parameter.
 *
 *	@param format	Desired index buffer format.
 *	@return		A dynamic index buffer that corresponds to the desired format.
 */
DynamicIndexBufferBase & DynamicIndexBufferInterface::get( D3DFORMAT format )
{
	BW_GUARD;
	if (format == D3DFMT_INDEX16)
	{
		if (Moo::rc().mixedVertexProcessing())
		{
			return swBuffer16_;
		}
		return hwBuffer16_;
	}
	if (Moo::rc().mixedVertexProcessing())
	{
		return swBuffer32_;
	}
	return hwBuffer32_;
}


/*
 *	This method resets locks on all the index buffers.
 */
void DynamicIndexBufferInterface::resetLocks()
{
	swBuffer16_.resetLock();
	hwBuffer16_.resetLock();
	swBuffer32_.resetLock();
	hwBuffer32_.resetLock();
}


} // namespace Moo

BW_END_NAMESPACE

/*dynamic_index_buffer.cpp*/
