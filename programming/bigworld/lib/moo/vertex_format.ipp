// vertex_format.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{
	
// -----------------------------------------------------------------------------
// Section: RawElementAccessor
// -----------------------------------------------------------------------------
template < class Type >
INLINE RawElementAccessor<Type>::RawElementAccessor( void* buffer, size_t size, size_t offset, size_t stride )
	: buffer_( (uint8*)buffer )
	, size_( size )
	, offset_( offset )
	, stride_( stride )
{
}


template < class Type >
INLINE const Type& RawElementAccessor<Type>::operator[](uint32 index) const
{
	return (*(const_cast<SelfType*>(this)))[index];
}


template < class Type >
INLINE Type& RawElementAccessor<Type>::operator[](uint32 index)
{
	MF_ASSERT( index < size() );

	return *((Type*)(buffer_ + index * stride_ + offset_));
}


template < class Type >
INLINE size_t RawElementAccessor<Type>::size() const
{
	return size_ / stride_;
}


template < class Type >
INLINE bool RawElementAccessor<Type>::isValid() const
{
	return buffer_ != 0 && offset_ < stride_;
}


// -----------------------------------------------------------------------------
// Section: ProxyElementAccessor
// -----------------------------------------------------------------------------
template < VertexElement::SemanticType::Value Semantic >
INLINE ProxyElementAccessor<Semantic>::ProxyElementAccessor()
	: buffer_( NULL )
	, offset_( 0 )
	, stride_( 0 )
	, type_( VertexElement::StorageType::UNKNOWN )
{

}

template < VertexElement::SemanticType::Value Semantic >
INLINE ProxyElementAccessor<Semantic>::ProxyElementAccessor( void* buffer, 
	size_t size, size_t offset, size_t stride, 
	VertexElement::StorageType::Value type )
	: buffer_( (uint8*)buffer )
	, size_( size )
	, offset_( offset )
	, stride_( stride )
	, type_( type )
{
}

template < VertexElement::SemanticType::Value Semantic >
INLINE bool ProxyElementAccessor<Semantic>::isValid() const
{
	return buffer_ != 0 && size() != 0;
}


template < VertexElement::SemanticType::Value Semantic >
INLINE size_t ProxyElementAccessor<Semantic>::size() const
{
	return size_ / stride_;
}


template < VertexElement::SemanticType::Value Semantic >
INLINE const typename ProxyElementAccessor<Semantic>::ElementRefType 
	ProxyElementAccessor<Semantic>::operator[]( uint32 index ) const
{
	return (*(const_cast<SelfType*>(this)))[index];
}


template < VertexElement::SemanticType::Value Semantic >
INLINE typename ProxyElementAccessor<Semantic>::ElementRefType 
	ProxyElementAccessor<Semantic>::operator[]( uint32 index )
{
	MF_ASSERT( index < size() );

	return ElementRefType((void*)(buffer_ + index * stride_ + offset_), type_);
}


// -----------------------------------------------------------------------------
// Section: VertexFormat
// -----------------------------------------------------------------------------
template <VertexElement::SemanticType::Value Semantic>
INLINE ProxyElementAccessor<Semantic> VertexFormat::createProxyElementAccessor( void* buffer, 
	size_t bufferSize, uint32 streamIndex, uint32 semanticIndex ) const
{
	// Find the relevant element
	VertexElement::StorageType::Value storageType;
	size_t offset;
	if (!findElement( streamIndex, Semantic, semanticIndex, &offset, &storageType ))
	{
		return ProxyElementAccessor<Semantic>();
	}

	return ProxyElementAccessor<Semantic>( buffer, bufferSize, offset, streamStride( streamIndex ), storageType );
}


template <VertexElement::SemanticType::Value Semantic>
INLINE const ProxyElementAccessor<Semantic> VertexFormat::createProxyElementAccessor( 
	const void* buffer, size_t bufferSize, uint32 streamIndex, uint32 semanticIndex ) const
{
	return VertexFormat::createProxyElementAccessor<Semantic>( const_cast<void*>(buffer), bufferSize, streamIndex, semanticIndex );
}


template <class AccessType>
INLINE RawElementAccessor<AccessType> VertexFormat::createRawElementAccessor( 
	void* buffer, size_t bufferSize, uint32 streamIndex, size_t offset ) const
{
	return RawElementAccessor<AccessType>( buffer, bufferSize, offset, (size_t)streamStride( streamIndex ) );
}

} // namespace Moo

// vertex_format.ipp
