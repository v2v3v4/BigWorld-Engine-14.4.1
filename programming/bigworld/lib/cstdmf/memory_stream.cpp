#include "pch.hpp"
#include "memory_stream.hpp"

BW_BEGIN_NAMESPACE

#if defined( __APPLE__ ) || defined( __ANDROID__ )
#define MY_INLINE
#include "memory_stream.ipp"
#endif // __APPLE__ || __ANDROID__

/**
 * 	This is the constructor.
 */
MemoryOStream::MemoryOStream( int size )
{
	pBegin_ = new char[ size ];
	pCurr_ = pBegin_;
	pEnd_ = pBegin_ + size;
	shouldDelete_ = true;
	canRewind_ = true;
	pRead_ = pBegin_;
}


/**
 * 	This is the destructor. If the stream owns the data buffer, it
 * 	deletes it.
 *
 * 	@see shouldDelete
 */
MemoryOStream::~MemoryOStream()
{
	if (shouldDelete_)
	{
		bw_safe_delete_array(pBegin_);
	}
}


/**
 *	This method is called by 'reserve' method, when
 *	there's insufficient space in the current buffer.
 *
 *	@param nBytes	Number of bytes to reserve.
 *
 *	@return Pointer to the reserved data in the stream.
 */
void * MemoryOStream::grow( int nBytes )
{
	char * pOldCurr = pCurr_;
	pCurr_ += nBytes;

	if (canRewind_)
	{
		// Can rewind, must grow all
		int multiplier = (int)((pCurr_ - pBegin_)/(pEnd_ - pBegin_) + 1);
		int newSize = multiplier * (int)(pEnd_ - pBegin_);
		char * pNewData = new char[ newSize ];
		memcpy( pNewData, pBegin_, pOldCurr - pBegin_ );
		pCurr_ = pCurr_ - pBegin_ + pNewData;
		pOldCurr = pOldCurr - pBegin_ + pNewData;
		pRead_ = pRead_ - pBegin_ + pNewData;
		MF_ASSERT( shouldDelete_ );
		bw_safe_delete_array( pBegin_ );
		pBegin_ = pNewData;
		pEnd_ = pBegin_ + newSize;
	}
	else if (pCurr_ - (pRead_ - pBegin_) < pEnd_)
	{
		MF_ASSERT( pRead_ <= pCurr_ );
		// Don't need rewind, and enough space to move buffer
		memcpy( pBegin_, pRead_, pOldCurr - pRead_ );
		pCurr_ = pCurr_ - (pRead_ - pBegin_);
		pOldCurr = pOldCurr - (pRead_ - pBegin_);
		pRead_ = pBegin_;
	}
	else
	{
		MF_ASSERT( pRead_ <= pCurr_ );
		int multiplier = (int)((pCurr_ - pRead_)/(pEnd_ - pBegin_) + 1);
		int newSize = multiplier * (int)(pEnd_ - pBegin_);
		char * pNewData = new char[ newSize ];
		memcpy( pNewData, pRead_, pOldCurr - pRead_ );
		pCurr_ = pCurr_ - pRead_ + pNewData;
		pOldCurr = pOldCurr - pRead_ + pNewData;
		pRead_ = pNewData;
		MF_ASSERT( shouldDelete_ );
		bw_safe_delete_array( pBegin_ );
		pBegin_ = pNewData;
		pEnd_ = pBegin_ + newSize;
	}

	return pOldCurr;
}

BW_END_NAMESPACE


