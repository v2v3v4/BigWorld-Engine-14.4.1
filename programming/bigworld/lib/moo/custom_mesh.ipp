
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Detail
{
/**
 *	This method converts nVerts into nPrims given our prim type.
 */

inline uint32 nPrimitives( D3DPRIMITIVETYPE primitiveType, uint32 meshSize )
{
	BW_GUARD;
	switch ( primitiveType )
	{
	case D3DPT_POINTLIST:
		return meshSize;
		break;
	case D3DPT_LINELIST:
		return meshSize / 2;
		break;
	case D3DPT_LINESTRIP:
		return meshSize - 1;
		break;
	case D3DPT_TRIANGLESTRIP:
		return meshSize - 2;
		break;
	case D3DPT_TRIANGLELIST:
		return meshSize / 3;
		break;
	case D3DPT_TRIANGLEFAN:
		return meshSize - 2;
		break;
			
	case D3DPT_FORCE_DWORD:
		MF_ASSERT( false );		
		break;
	}

	return 0;
}

} // Detail


// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
template <class T>
CustomMesh<T>::CustomMesh( D3DPRIMITIVETYPE pType )
:primitiveType_( pType )
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
template <class T>
CustomMesh<T>::~CustomMesh()
{
	BW_GUARD;
}


/**
 *	This method draws the custom mesh.
 */
template <class T>
HRESULT CustomMesh<T>::draw()
{
	BW_GUARD;
	uint32 numPrimitives = Detail::nPrimitives( primitiveType_,
		(uint32)this->size() );
	if (numPrimitives)
	{
		Moo::rc().setPixelShader( NULL );
		Moo::rc().setVertexShader( NULL );
		Moo::rc().setFVF( T::fvf() );

		uint32 lockIndex = 0;
		//DynamicVertexBuffer
		int vertexSize = sizeof(T);
		Moo::DynamicVertexBufferBase2& vb =
			Moo::DynamicVertexBufferBase2::instance( vertexSize );
		if ( vb.lockAndLoad( &this->front(), (uint32)this->size(), 
			lockIndex ) && SUCCEEDED(vb.set( 0 )) )
		{
			HRESULT res = Moo::rc().drawPrimitive( primitiveType_, lockIndex, numPrimitives );
			//Reset stream
			vb.unset( 0 );		
			return res;
		}
	}
	return S_OK;
}


/**
 *	This method draws the custom mesh as an effect,
 *	meaning no vertex shader is set up.
 */
template <class T>
HRESULT CustomMesh<T>::drawEffect(Moo::VertexDeclaration* decl)
{
	BW_GUARD;
	uint32 numPrimitives = Detail::nPrimitives( primitiveType_,
		(uint32)this->size() );
	if (numPrimitives)
	{
		if (decl)
		{
			Moo::rc().setVertexDeclaration(decl->declaration());
		}
		else
		{
			Moo::rc().setFVF( T::fvf() );
		}
		uint32 lockIndex = 0;
		//DynamicVertexBuffer
		int vertexSize = sizeof(T);
		Moo::DynamicVertexBufferBase2& vb =
			Moo::DynamicVertexBufferBase2::instance( vertexSize );
		if ( vb.lockAndLoad( &front(), (uint32)this->size(), 
			lockIndex ) && SUCCEEDED(vb.set( 0 )) )
		{
			HRESULT res = Moo::rc().drawPrimitive( primitiveType_, lockIndex, numPrimitives );
			//Reset stream
			vb.unset( 0 );
			return res;
		}
	}
	return S_OK;
}


/**
 *	This method draws a range of vertices from the custom mesh.
 */
template <class T>
HRESULT CustomMesh<T>::drawRange( uint32 from, uint32 to )
{
	BW_GUARD;
	uint32 numPrimitives = Detail::nPrimitives( primitiveType_, to - from );
	if (numPrimitives)
	{
		Moo::rc().setFVF( T::fvf() );
		Moo::rc().setVertexShader( NULL );

		//DynamicVertexBuffer
		int vertexSize = sizeof(T);
		Moo::DynamicVertexBufferBase2& vb =
			Moo::DynamicVertexBufferBase2::instance( vertexSize );
		uint32 lockIndex = 0;
		if ( vb.lockAndLoad( &this->front()+from, numPrimitives, 
			lockIndex ) && SUCCEEDED(vb.set( 0 )) )
		{
			HRESULT res = Moo::rc().drawPrimitive( primitiveType_, lockIndex, numPrimitives );
			vb.unset( 0 );
			return res;
		}
	}
	return S_OK;
}


template < typename VertexType >
VertexType * StaticMesh::lock( D3DPRIMITIVETYPE primitiveType,
	uint32 numVertices )
{
	primitiveType_ = primitiveType;
	fvf_ = VertexType::fvf();
	numVertices_ = numVertices;
	stride_ = sizeof( VertexType );
	void * buffer = 0;
	vertexBuffer_.create< VertexType >( numVertices, 0, D3DPOOL_MANAGED );
	vertexBuffer_.lock( 0, 0, &buffer, 0 );
	return static_cast< VertexType * >( buffer );
}

// custom_mesh.ipp
