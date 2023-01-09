// visual.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif



namespace Moo
{

INLINE
NodePtr Visual::rootNode( ) const
{
	return rootNode_;
}

INLINE
AnimationPtr Visual::animation( uint32 i ) const
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( animations_.size() > i )
	{
		return NULL;
	}
	return animations_[ i ];
}

INLINE
uint32 Visual::nAnimations( ) const
{
	return static_cast<uint32>( animations_.size() );
}

INLINE
void Visual::addAnimation( AnimationPtr animation )
{
	MF_ASSERT_DEV( animation );
	if( animation )
		animations_.push_back( animation );
}

INLINE
const BoundingBox& Visual::boundingBox() const
{
	return bb_;
}

INLINE
void Visual::boundingBox( const BoundingBox& bb )
{
	bb_ = bb;
}

INLINE
uint32 Visual::nPortals( void ) const
{
	return static_cast<uint32>( portals_.size() );
}

INLINE
const PortalData& Visual::portal( uint32 i ) const
{
	return portals_[ i ];
}

}

// visual.ipp