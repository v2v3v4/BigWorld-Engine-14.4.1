#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{

INLINE
void DynamicVertexBufferBase::release( )
{
	lockBase_ = 0;
	vertexBuffer_.release();
}

INLINE

void DynamicVertexBufferBase::resetLock( )
{
	reset_ = true;	
}

INLINE
HRESULT DynamicVertexBufferBase::unlock( )
{
	BW_GUARD;

//	MF_ASSERT( locked_ == true );
	locked_ = false;
	return vertexBuffer_.unlock( );
}

INLINE
Moo::VertexBuffer DynamicVertexBufferBase::vertexBuffer( )
{
//	MF_ASSERT( vertexBuffer_.pComObject() != NULL );
	return vertexBuffer_;
}

INLINE
HRESULT DynamicVertexBufferBase::set( UINT streamNumber, UINT offsetInBytes, UINT stride )
{
	return vertexBuffer_.set( streamNumber, offsetInBytes, stride );
}

}
/*dynamic_vertex_buffer.ipp*/
