#ifndef VERTEXBUFFERWRAPPER_HPP
#define VERTEXBUFFERWRAPPER_HPP


#include "moo_dx.hpp"
#include "vertex_buffer.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	Wraps D3D vertex buffers with a convenient container interface.
 */
template< typename VertexType >
class VertexBufferWrapper : public SafeReferenceCount
{
public:
	typedef VertexType * iterator;
	typedef const VertexType * const_iterator;

	/**
	 *	Constructor.
	 */
	VertexBufferWrapper() : 
		bufferSize_(0),
		data_(NULL),
		cdata_(NULL)
	{}

	/**
	 *	Resets buffer to given size.
	 *
	 *	@param	bufferSize	new size of buffer.
	 *	
	 *	@return				true on success, false on error.
	 */
	bool reset(int bufferSize = 0)
	{
		BW_GUARD;
		bool result = true;
		if (bufferSize == 0)
		{
			if (this->vbuffer_.valid())
			{
				MF_ASSERT_DEV(this->bufferSize_ != 0);
				this->vbuffer_.release();
			}
		}
		else 
		{
			if (this->vbuffer_.valid())
			{
				this->reset(0);
			}
			MF_ASSERT_DEV(this->bufferSize_ == 0);
			int size = max(1, bufferSize);
			HRESULT hr = vbuffer_.create( size * sizeof(VertexType), 
				0, VertexType::fvf(), 
				D3DPOOL_MANAGED, "vertex buffer/wrapper" );

			if (FAILED(hr))
			{
				this->vbuffer_.release();
				bufferSize = 0;
				result = false;
			}
		}
		this->bufferSize_ = bufferSize;
		return result;
	}

	/**
	 *	Copies data from source untill buffer is full.
	 *
	 *	@param source	pointer to begining of data to be copied
	 *	
	 *	@return			true on success, false on error.
	 */
	bool copy(const VertexType *source)
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV(this->vbuffer_.valid())
		{
			return false;
		}

		IF_NOT_MF_ASSERT_DEV(this->data_ == NULL)
		{
			return false;
		}

		bool result = false;
		SimpleVertexLock vl( this->vbuffer_ );
        if (vl)
		{
	        memcpy(vl, source, this->bufferSize_ * sizeof(VertexType));
			result = true;
		}
		return result;
	}

	bool dump(VertexType *dst)
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV(this->vbuffer_.valid())
		{
			return false;
		}

		IF_NOT_MF_ASSERT_DEV(this->data_ == NULL)
		{
			return false;
		}

		bool result = false;
		SimpleVertexLock vl( this->vbuffer_, 0, 0, D3DLOCK_READONLY );
        if (vl)
        {
			memcpy(dst, vl, this->bufferSize_ * sizeof(VertexType));
			result = true;
		}
		return result;
	}

	/**
	 *	Locks buffer for writting.
	 */
	bool lock()
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV(this->vbuffer_.valid())
		{
			return false;
		}

		IF_NOT_MF_ASSERT_DEV(this->data_ == NULL)
		{
			return false;
		}

		bool result = true;
        HRESULT hr = this->vbuffer_.lock(0, 0, reinterpret_cast< void ** >(&this->data_), 0);
		if (FAILED(hr))
		{
			this->data_ = NULL;
			result = false;
		}
		return result;
	}

	/**
	 *	Locks buffer for writting (const version).
	 */
	bool lock() const
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV(this->vbuffer_.valid())
		{
			return false;
		}

		IF_NOT_MF_ASSERT_DEV(this->cdata_ == NULL)
		{
			return false;
		}

		bool result = true;
		VertexType *& cdata = const_cast<VertexType *&>(this->cdata_);
        HRESULT hr = this->vbuffer_.lock(0, 0, reinterpret_cast< void ** >(&cdata), 0);
		if (FAILED(hr))
		{
			cdata = NULL;
			result = false;
		}
		return result;
	}

	/**
	 *	Returns single element in buffer
	 *
	 *	@param	index	index of element to be retrieved.
	 *
	 *	@return			reference to element requested.
	 */
	VertexType & operator [] (uint index)
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV(this->data_ != NULL)
		{
			MF_EXIT( "no vertex types to return" );
		}

		IF_NOT_MF_ASSERT_DEV(index <= this->bufferSize_)
		{
			MF_EXIT( "index out of range" );
		}
        return this->data_[index];
	}

	/**
	 *	Returns single element in buffer (const version)
	 *
	 *	@param	index	index of element to be retrieved.
	 *
	 *	@return			const reference to element requested.
	 */
	const VertexType & operator [] (uint index) const
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV(this->cdata_ != NULL)
		{
			MF_EXIT( "no vertex types to return" );
		}

		IF_NOT_MF_ASSERT_DEV(index <= this->bufferSize_)
		{
			MF_EXIT( "index out of range" );
		}
        return this->cdata_[index];
	}

	/**
	 *	Returns an iterator to first element in buffer.
	 */
	iterator begin()
	{
		return this->data_;
	}

	/**
	 *	Returns an iterator to first element in buffer (const version).
	 */
	const_iterator begin() const
	{
		return this->cdata_;
	}

	/**
	 *	Returns an iterator to one past last element in buffer.
	 */
	iterator end()
	{
		return this->data_ + this->bufferSize_;
	}

	/**
	 *	Returns an iterator to one past last element in buffer (const version).
	 */
	const_iterator end() const
	{
		return this->cdata_ + this->bufferSize_;
	}

	/**
	 *	Unlocks buffer.
	 */
	void unlock()
	{
		BW_GUARD;
		MF_ASSERT_DEV(this->data_ != NULL);

        this->vbuffer_.unlock();
		this->data_ = NULL;
	}

	/**
	 *	Unlocks buffer (const version).
	 */
	void unlock() const
	{
		BW_GUARD;
		MF_ASSERT_DEV(this->cdata_ != NULL);

        this->vbuffer_.unlock();

		VertexType *& cdata = const_cast<VertexType *&>(this->cdata_);
		cdata = NULL;
	}

	/**
	 *	Returns true if buffer is currently locked.
	 */
	bool isLocked() const
	{
		return this->cdata_ != NULL;
	}

	/**
	 *	Returns current size of buffer.
	 */
	int size() const
	{
		return this->bufferSize_;
	}

	/**
	 *	Queries buffer validity.
	 *	
	 *	@return		true if buffer is valid, false otherwise.
	 */
	bool isValid() const
	{
		return this->vbuffer_.valid() && this->bufferSize_ > 0;
	}

	/**
	 *	Activates this buffer (sets it as a current stream source).
	 */
	void activate( uint32 stream = 0 ) const
	{
		if (this->isValid())
		{
			this->vbuffer_.set( stream, 0, sizeof(VertexType));
		}
	}

	/**
	 *	Deactivates this buffer.
	 */
	static void deactivate( uint32 stream = 0 )
	{
		VertexBuffer().set( stream, 0, 0 );
	}

private:
	unsigned int       bufferSize_;
	mutable Moo::VertexBuffer  vbuffer_;
	VertexType *       data_;
	const VertexType * cdata_;

	// disallow copy
	VertexBufferWrapper(const VertexBufferWrapper &);
	const VertexBufferWrapper & operator = (const VertexBufferWrapper &);
};

} // namespace Moo

BW_END_NAMESPACE

#endif // VERTEXBUFFERWRAPPER_HPP
