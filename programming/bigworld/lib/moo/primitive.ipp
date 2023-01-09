
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{

INLINE
uint32 Primitive::nPrimGroups( ) const
{
	return static_cast<uint32>( primGroups_.size() );
}

INLINE
uint32 Primitive::maxVertices( ) const
{
	return maxVertices_;
}

INLINE
void Primitive::resourceID( const BW::string& resourceID )
{
	resourceID_ = resourceID;
}

INLINE
const BW::string& Primitive::resourceID( ) const
{
	return resourceID_;
}
INLINE
const Vector3& Primitive::origin( uint32 i ) const
{
	//make sure the origins have been initialised
	MF_ASSERT_DEBUG( i<groupOrigins_.size() );
	return groupOrigins_[ i ];
}

INLINE
const PrimitiveGroup& Primitive::primitiveGroup( uint32 i ) const
{
#if !defined( _RELEASE )
	if ( ! (i < primGroups_.size() ) )
	{
		BW::string buf( "primitiveGroup access violation, resource " );
		buf += resourceID_;
		MF_EXIT( buf.c_str() );
	}	
#endif
	return primGroups_[ i ];
}

INLINE
D3DPRIMITIVETYPE Primitive::primType() const
{
	return primType_;
}

INLINE
const IndexBuffer&	Primitive::indexBuffer() const
{ 
	return indexBuffer_;
}

}

/*primitive.ipp*/
