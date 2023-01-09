#ifndef INDEXBUFFERWRAPPER_HPP
#define INDEXBUFFERWRAPPER_HPP


#include "moo/render_context.hpp"
#include "moo/moo_dx.hpp"
#include "moo/index_buffer.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	Wraps D3D index buffers with a convenient container interface.
 */
class IndexBufferWrapper : public SafeReferenceCount
{
public:
	typedef unsigned short * iterator;
	typedef const unsigned short * const_iterator;

	/**
	 *	Constructor.
	 */
	IndexBufferWrapper() :
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
	bool reset(int bufferSize)
	{
		BW_GUARD;
		bool result = true;
		if (bufferSize == 0)
		{
			this->ibuffer_.release();
		}
		else 
		{
			if (this->ibuffer_.valid())
			{
				this->reset(0);
			}
			MF_ASSERT_DEV(this->bufferSize_ == 0);
			int size = max(1, bufferSize);
			HRESULT hr = this->ibuffer_.create(
				bufferSize, D3DFMT_INDEX16,
				0, D3DPOOL_MANAGED,
				"index buffer/wrapper");	

			if (FAILED(hr))
			{
				this->ibuffer_.release();
				result = false;
			}
		}
		if( result )
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
	bool copy(const unsigned short *source)
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV(this->ibuffer_.valid())
		{
			return false;
		}
		IF_NOT_MF_ASSERT_DEV(this->data_ == NULL && this->cdata_ == NULL)
		{
			return false;
		}

		bool result = false;
        IndicesReference ir = this->ibuffer_.lock();
        if (ir.valid())
        {
			ir.fill( source, this->bufferSize_ );
			this->ibuffer_.unlock();
			result = true;
		}
		return result;
	}

	/**
	 *	Copies data from source untill buffer is full.
	 *
	 *	@param source	pointer to begining of data to be copied
	 *	
	 *	@return			true on success, false on error.
	 */
	bool copy(const int *source)
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV(this->ibuffer_.valid())
		{
			return false;
		}
		IF_NOT_MF_ASSERT_DEV(this->data_ == NULL && this->cdata_ == NULL)
		{
			return false;
		}

		bool result = false;
        IndicesReference ir = this->ibuffer_.lock();
        if (ir.valid())
        {
			for (unsigned int i=0; i<this->bufferSize_; ++i)
			{
				ir[i] = *source;
				++source;
			}
			this->ibuffer_.unlock();
			result = true;
		}
		return result;
	}

	bool dump(unsigned short *dst)
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV(this->ibuffer_.valid())
		{
			return false;
		}
		IF_NOT_MF_ASSERT_DEV(this->data_ == NULL && this->cdata_ == NULL)
		{
			return false;
		}

		bool result = false;
        IndicesReference ir = this->ibuffer_.lock(D3DLOCK_READONLY);
		if (ir.valid())
		{
			memcpy(dst, ir.indices(), this->bufferSize_ * sizeof(unsigned short));
			this->ibuffer_.unlock();
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
		IF_NOT_MF_ASSERT_DEV(this->ibuffer_.valid())
		{
			return false;
		}
		IF_NOT_MF_ASSERT_DEV(this->data_ == NULL && this->cdata_ == NULL)
		{
			return false;
		}

		bool result = false;
        IndicesReference ir = this->ibuffer_.lock();
		if (ir.valid())
		{
			this->data_ = (unsigned short*)ir.indices();
			result = true;
		}
		return result;
	}

	/**
	 *	Locks buffer for writting (const version).
	 */
	bool lock() const
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV(this->ibuffer_.valid())
		{
			return false;
		}
		IF_NOT_MF_ASSERT_DEV(this->data_ == NULL && this->cdata_ == NULL)
		{
			return false;
		}

		bool result = true;
        IndicesReference ir = this->ibuffer_.lock();
		if (ir.valid())
		{
			this->cdata_ = (unsigned short*)ir.indices();
			result = false;
		}
		return result;
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

        this->ibuffer_.unlock();
		this->data_ = NULL;
	}

	/**
	 *	Unlocks buffer (const version).
	 */
	void unlock() const
	{
		BW_GUARD;
		MF_ASSERT_DEV(this->cdata_ != NULL);

        this->ibuffer_.unlock();

		this->cdata_ = NULL;
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
		return this->ibuffer_.valid();
	}

	/**
	 *	Activates this buffer (sets it as a current stream source).
	 */
	void activate() const
	{
		if (this->isValid())
		{
			this->ibuffer_.set();
		}
	}

	/**
	 *	Deactivates this buffer.
	 */
	static void deactivate()
	{
		IndexBuffer().set();
	}

private:
	unsigned int              bufferSize_;
	mutable IndexBuffer       ibuffer_;
	unsigned short *          data_;
	mutable unsigned short *  cdata_;

	// disallow copy
	IndexBufferWrapper(const IndexBufferWrapper &);
	const IndexBufferWrapper & operator = (const IndexBufferWrapper &);
};

} // namespace Moo

BW_END_NAMESPACE

#endif // INDEXBUFFERWRAPPER_HPP

