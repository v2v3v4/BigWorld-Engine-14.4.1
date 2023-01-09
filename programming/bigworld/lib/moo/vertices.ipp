#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{

INLINE
const BW::string& Vertices::resourceID( ) const
{
	return resourceID_;
}

INLINE
void Vertices::resourceID( const BW::string& resourceID )
{
	resourceID_ = resourceID;
}

INLINE
uint32 Vertices::nVertices( ) const
{
	return nVertices_;
}

INLINE
const BW::string& Vertices::sourceFormat( ) const
{
	return sourceFormat_;
}

INLINE
Moo::VertexBuffer Vertices::vertexBuffer( ) const
{
	return vertexBuffer_;
}

}

/*vertices.ipp*/