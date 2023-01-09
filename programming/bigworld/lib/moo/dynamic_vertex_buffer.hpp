#ifndef DYNAMIC_VERTEX_BUFFER_HPP
#define DYNAMIC_VERTEX_BUFFER_HPP

#include <iostream>
#include "cstdmf/bw_list.hpp"

#include "cstdmf/stdmf.hpp"
#include "moo_dx.hpp"
#include "vertex_buffer.hpp"
#include "com_object_wrap.hpp"
#include "device_callback.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This class implements the common elements of all dynamic vertex buffers.
 */
class DynamicVertexBufferBase : public DeviceCallback
{
public:
	virtual ~DynamicVertexBufferBase();

	HRESULT						unlock( );
	HRESULT						create( );

	Moo::VertexBuffer			vertexBuffer( );
	HRESULT						set( UINT streamNumber = 0, UINT offsetInBytes = 0, UINT stride = 0 );

	static void					releaseAll( );
	static void					resetLockAll( );
	static void					fini();

protected:

	DynamicVertexBufferBase();
	uint32						lockIndex_;
	int							vertexSize_;
	bool						lockModeDiscard_;
	bool						lockFromStart_;
	bool						softwareBuffer_;
	bool						readOnly_;
	BYTE*						lock( uint32 nLockElements );
	BYTE*						lock2( uint32 nLockElements );

	void						release( );
	void						resetLock( );

	void						deleteUnmanagedObjects( );

private:

	Moo::VertexBuffer			vertexBuffer_;

	bool						reset_;
	bool						locked_;
	int							lockBase_;
	uint						maxElements_;

	DynamicVertexBufferBase(const DynamicVertexBufferBase&);
	DynamicVertexBufferBase& operator=(const DynamicVertexBufferBase&);

	friend std::ostream& operator<<(std::ostream&, const DynamicVertexBufferBase&);

	static BW::list< DynamicVertexBufferBase* > dynamicVBS_;
protected:
	static bool s_finalised;
};

/**
 * A base class for vertex buffers.
 *
 * This was created to remove the constant selection between 
 * software/hardware buffers
 * 
 */
class DynamicVertexBufferBase2 : public DynamicVertexBufferBase
{
public:
	static DynamicVertexBufferBase2& instance( int vertexSize );

	/**
	 *	This method locks, transfers and then unlocks a buffer,
	 *	returning the lock index.
	 *	@param pSrc source vertices.
	 *	@param count vertex count.
	 *  @return lock index
	 */
	bool lockAndLoad( const void * pSrc, uint32 count, uint32& base )
	{
		BW_GUARD;

		void* pVertex = lock2( count );
		if (pVertex)
		{
			memcpy(pVertex, pSrc, count * vertexSize_);
			unlock();
			base = lockIndex();
			return true;
		}
		else
			return false;
	}

	/**
	 *	This method returns a pointer to the start of a vertex buffer
	 *	big enough to contain nLockElements vertices.
	 *	@param nLockElements the number of vertices to allocate
	 *	@return pointer to the vertices that have been allocated.
	 */
	void * lock( uint32 nLockElements )
	{
		BW_GUARD;
		return reinterpret_cast<void*>(
			DynamicVertexBufferBase::lock( nLockElements ));
	}

	template <class VertexType>
	VertexType * lock( uint32 nLockElements )
	{
		MF_ASSERT( sizeof(VertexType) == vertexSize_ );
		return reinterpret_cast<VertexType*>( lock( nLockElements ));
	}

	/**
	 *	This method returns a pointer to a write only vertex buffer
	 *	big enough to contain nLockElements. These vertices will
	 *	not necessarily be at the start of the vertex buffer. To get
	 *	the index of the first locked vertex use the method lockIndex.
	 *	It implements the best practices locking for dynamic vertex
	 *	buffers.
	 *	@param nLockElements the number of vertices to allocate
	 *	@return pointer to the vertices that have been allocated.
	 */
	void * lock2( uint32 nLockElements )
	{
		BW_GUARD;
		return reinterpret_cast<void*>(
			DynamicVertexBufferBase::lock2( nLockElements ));
	}

	template <class VertexType>
	VertexType * lock2( uint32 nLockElements )
	{
		MF_ASSERT( sizeof(VertexType) == vertexSize_ );
		return reinterpret_cast<VertexType*>( lock2( nLockElements ));
	}


	/**
	 *	This method binds the vertex buffer to the specified
	 *	stream.
	 *	@param stream stream id.
	 *  @return result
	 */
	HRESULT set( uint32 stream = 0 )
	{
		BW_GUARD;
		return vertexBuffer().set(stream, 0, vertexSize_);
	}

	HRESULT unset( uint32 stream = 0 )
	{
		BW_GUARD;
		return Moo::rc().device()->SetStreamSource(stream, 0, 0, 0);
	}

	/**
	 *	This method returns the lock index for this buffer.
	 *  @return lock index.
	 */
	uint32 lockIndex() const
	{
		return lockIndex_;
	}
};

/**
 *	This class implements dynamic vertex buffers stored in video memory.
 *	It always tries to grow the vertex buffer to the size passed into lock or lock2.
 */
class DynamicVertexBuffer : public DynamicVertexBufferBase2
{
public:
	static DynamicVertexBuffer&	instance( int vertexSize );

	~DynamicVertexBuffer()
	{

	}
private:
	DynamicVertexBuffer( int vertexSize )
	{
		MF_ASSERT( vertexSize > 0 );
		vertexSize_ = vertexSize;
		softwareBuffer_ = false;
		lockModeDiscard_ = true;
		lockFromStart_ = true;
		readOnly_ = true;
	}
};

/**
 *	This class implements dynamic vertex buffers stored in system memory.
 *	It always tries to grow the vertex buffer to the size passed into lock or lock2.
 */
class DynamicSoftwareVertexBuffer : public DynamicVertexBufferBase2
{
public:
	static DynamicSoftwareVertexBuffer&	instance( int vertexSize );

	~DynamicSoftwareVertexBuffer()
	{

	}
private:
	DynamicSoftwareVertexBuffer( int vertexSize )
	{
		MF_ASSERT( vertexSize > 0 );
		vertexSize_ = vertexSize;
		softwareBuffer_ = true;
		lockModeDiscard_ = true;
		lockFromStart_ = true;
		readOnly_ = false;
	}
};

}

#ifdef CODE_INLINE
#include "dynamic_vertex_buffer.ipp"
#endif

BW_END_NAMESPACE

#endif
/*dynamic_vertex_buffer.hpp*/
