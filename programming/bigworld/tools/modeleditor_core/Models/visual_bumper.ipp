// visual_bumper.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE

// The different vertex formats have different extra information so we 
// need to copy that information
template<class VertexType, class BumpedVertexType> 
INLINE
void copyVertsExtra( const VertexType& v, BumpedVertexType& bv ) {}


template<>
INLINE
void copyVertsExtra( const Moo::VertexXYZNUVIIIWW& v, Moo::VertexXYZNUVIIIWWTB& bv ) 
{
	bv.index_ = v.index_;
	bv.index2_ = v.index2_;
	bv.index3_ = v.index3_;
	bv.weight_ = v.weight_;
	bv.weight2_ = v.weight2_;
}


// For some of the vertex formats, the normal is packed, and therefore needs to be unpacked
template<class NormalFormat>
INLINE
Vector3 unpackTheNormal( NormalFormat normal ) 
{
	return normal;
}


template<>
INLINE
Vector3 unpackTheNormal( uint32 normal ) 
{
	return Moo::unpackNormal( normal );
}

BW_END_NAMESPACE
// visual_bumper.ipp
