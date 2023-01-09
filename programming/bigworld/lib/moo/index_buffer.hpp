#ifndef INDEX_BUFFER_HPP
#define INDEX_BUFFER_HPP

#include "render_context.hpp"
#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

class IndicesReference;

/**
 *	This is class is used to hold one reference to an index buffer
 *	entry. Its size is determinated by the index buffer's format
 */
class IndexReference
{
	int32 offset_;
	IndicesReference& reference_;
public:
	IndexReference( IndicesReference& reference, int32 offset )
		: reference_( reference ), offset_( offset )
	{}
	operator uint32() const;
	void operator=( uint32 value );
};

/**
 *	A reference to a block of memory representing an index buffer.
 *	These objects are generally returned by locking mechanics.
 *	The index array is not managed here, it is just used as a source.
 */
class IndicesReference
{
protected:
	void* indices_;
	uint32 size_;
	D3DFORMAT format_;
public:
	IndicesReference() : size_( 0 ), indices_( 0 ), format_( D3DFMT_INDEX16 )
	{}
	virtual ~IndicesReference(){}
	virtual void assign( void * buffer, uint32 numOfIndices, D3DFORMAT format )
	{
		size_ = numOfIndices;
		indices_ = buffer;
		format_ = format;
	}
	bool valid() const
	{
		return !!indices_;
	}
	void fill( const void* buffer, uint32 numOfIndices )
	{
		memcpy( indices_, buffer, numOfIndices * entrySize() );
	}
	void pull( void* buffer, uint32 numOfIndices ) const
	{
		memcpy( buffer, indices_, numOfIndices * entrySize() );
	}
	void copy( const IndicesReference& src, int count, int start = 0, int srcStart = 0, uint32 vertexBase = 0 )
	{
		for( int i = 0; i < count; i++ )
			set( start + i, (uint32)src[ srcStart + i ] + vertexBase );
	}
	D3DFORMAT format() const
	{
		return format_;
	}
	int entrySize() const
	{
		return bestEntrySize( format_ );
	}
	void* indices()
	{
		return indices_;
	}
	const void* indices() const
	{
		return indices_;
	}
	uint32 size() const
	{
		return size_;
	}
	IndexReference operator[]( int32 index )
	{
		return IndexReference( *this, index );
	}
	uint32 operator[]( int32 index ) const
	{
		return get( index );
	}
	void set( int32 index, uint32 value )
	{
		if( format_ == D3DFMT_INDEX16 )
			( (uint16*)indices_ )[ index ] = (uint16)value;
		else
			( (uint32*)indices_ )[ index ] = value;

	}
	uint32 get( int32 index ) const
	{
		if( format_ == D3DFMT_INDEX16 )
			return ( (uint16*)indices_ )[ index ];
		else
			return ( (uint32*)indices_ )[ index ];

	}
	static int bestEntrySize( D3DFORMAT format )
	{
		return format == D3DFMT_INDEX16 ? 2 : 4;
	}
	static D3DFORMAT bestFormat( uint32 vertexNum )
	{
		return vertexNum <= 65536 ? D3DFMT_INDEX16 : D3DFMT_INDEX32;
	}
};

inline IndexReference::operator uint32() const
{
	return reference_.get( offset_ );
}

inline void IndexReference::operator=( uint32 value )
{
	reference_.set( offset_, value );
}

/**
 *	A class to hold the actual index data which can be updated and used
 *	to upload into a D3D resource via the IndexBuffer class below.
 */
class IndicesHolder : public IndicesReference
{
	IndicesHolder( const IndicesHolder& );
	void operator= ( const IndicesHolder& );
public:
	IndicesHolder( D3DFORMAT format = D3DFMT_INDEX16, uint32 entryNum = 0 )
	{
		format_ = format;
		size_ = entryNum;
		if (size() > 0)
		{
			indices_ = new unsigned char[ size() * entrySize() ];
		}
		else
		{
			indices_ = NULL;
		}
	}
	~IndicesHolder()
	{
		bw_safe_delete_array((unsigned char * &)indices_);
	}
	void setSize( uint32 entryNum, D3DFORMAT format )
	{
		bw_safe_delete_array((unsigned char * &)indices_);
		format_ = format;
		if( entryNum != 0 )
		{
			size_ = entryNum;
			indices_ = new unsigned char[ size() * entrySize() ];
		}
	}
	// Override of (virtual) IndicesReference::assign
	void assign( void * buffer, uint32 entryNum, D3DFORMAT format )
	{
		// Hand off to const-buffer-safe overload
		this->assign( const_cast< const void * >( buffer ), entryNum, format );
	}
	// Const-buffer-safe overload of assign, since we take a copy of the
	// input buffer, unlike IndicesReference
	void assign( const void * buffer, uint32 entryNum, D3DFORMAT format )
	{// exception unsafe
		bw_safe_delete_array((unsigned char * &)indices_);
		size_ = entryNum;
		format_ = format;
		indices_ = new unsigned char[ size() * entrySize() ];
		memcpy( indices_, buffer, size() * entrySize() );
	}
};

/**
 *	The D3D index buffer implementation. Use the create / lock / set methods
 *	to realise the functionality. This is purely a helper for the creation of
 *	the D3D index buffer objects, meaning an instance of this object can
 *	represent any type of index buffer desired.
 *	A ComObjectWrap class is used to Administer the D3D object.
 */
class IndexBuffer
{
	ComObjectWrap<DX::IndexBuffer> indexBuffer_;
	D3DFORMAT format_;
	DWORD numOfIndices_;
public:
	IndexBuffer() : format_( D3DFMT_INDEX16 ), numOfIndices_( 0 )
	{}
	HRESULT create( int numOfIndices, D3DFORMAT format, DWORD usage, D3DPOOL pool, const char* allocator = "index buffer/unknown" )
	{
		BW_GUARD;
		HRESULT hr = E_FAIL;

		// Fail if we try to create a 32 bit index buffer on a device that 
 	    // does not support it 
 	    if (format == D3DFMT_INDEX32 &&  
 		   rc().maxVertexIndex() <= 0xffff) 
 		   return hr; 

		ComObjectWrap<DX::IndexBuffer> temp;

		pool = RenderContext::patchD3DPool(pool);

		// Try to create the indexbuffer with maxIndices_ number of indices
		if( SUCCEEDED( hr = Moo::rc().device()->CreateIndexBuffer( 
			numOfIndices * IndicesReference::bestEntrySize( format ),
			usage, format, pool, &temp, NULL ) ) )
		{
			format_ = format;
			numOfIndices_ = numOfIndices;
			indexBuffer_ = temp;
			indexBuffer_.addAlloc(BW::StringRef(allocator));
		}
		return hr;
	}
	HRESULT set() const
	{
		BW_GUARD;
		return Moo::rc().setIndices( indexBuffer_.pComObject() );
	}
	bool valid() const
	{
		return indexBuffer_.hasComObject();
	}
	void release()
	{
		indexBuffer_.pComObject( NULL );
		format_ = D3DFMT_INDEX16;
		numOfIndices_ = 0;
	}
	IndicesReference lock( DWORD flags = 0 )
	{
		BW_GUARD;
		IndicesReference result;

		if (valid())
		{
			BYTE* indices = NULL;

			// Try to lock the buffer, if we succeed update the return value to use the locked vertices
			if( SUCCEEDED( indexBuffer_.pComObject()->Lock( 0, 0, (void**)&indices, flags ) ) )
			{
				result.assign( indices, numOfIndices_, format_ );
			}
		}

		return result;
	}
	IndicesReference lock( UINT offset, UINT numOfIndices, DWORD flags )
	{
		BW_GUARD;
		IndicesReference result;

		if (valid())
		{
			BYTE* indices = NULL;

			// Try to lock the buffer, if we succeed update the return value to use the locked vertices
			if( SUCCEEDED( indexBuffer_.pComObject()->Lock(
				offset * IndicesReference::bestEntrySize( format_ ),
				numOfIndices * IndicesReference::bestEntrySize( format_ ), (void**)&indices, flags ) ) )
			{
				result.assign( indices, numOfIndices, format_ );
			}
		}

		return result;
	}
	D3DFORMAT format() const
	{
		return format_;
	}
	bool isCurrent() const
	{
		return ( indexBuffer_ && Moo::rc().getIndices() == indexBuffer_ );
	}
	HRESULT unlock()
	{
		if (valid())
		{
			return indexBuffer_.pComObject()->Unlock();
		}

		return E_FAIL;
	}

	void preload()
	{
		if (valid())
		{
			indexBuffer_->PreLoad();
		}
	}

	/**
	 * This method adds the index buffer to the preload list in
	 * the render context. This causes the buffer to have its 
	 * preload method called in the next few frames, so that it
	 * is uploaded to video memory. This is only useful for
	 * managed pool resources.
	 */
	void addToPreloadList()
	{
		BW_GUARD;

		if (valid())
		{
			rc().addPreloadResource( indexBuffer_.pComObject() );
		}
	}
};

} // namespace Moo

BW_END_NAMESPACE

#endif//INDEX_BUFFER_HPP
