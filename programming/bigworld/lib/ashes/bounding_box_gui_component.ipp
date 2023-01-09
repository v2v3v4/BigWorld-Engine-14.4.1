
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE

/**
 *	This method sets the name of the texture used by this component.
 */
INLINE void
BoundingBoxGUIComponent::textureName( const BW::string& name )
{
	BW_GUARD;
	SimpleGUIComponent::textureName( name );

	corners_[0]->textureName( name );
	corners_[1]->textureName( name );
	corners_[2]->textureName( name );
	corners_[3]->textureName( name );
}


/**
 *	This method sets the width of this component.
 */
INLINE void
BoundingBoxGUIComponent::width( float w )
{
	BW_GUARD;
	SimpleGUIComponent::width( w );

	corners_[0]->width( w );
	corners_[1]->width( w );
	corners_[2]->width( w );
	corners_[3]->width( w );
}


/**
 *	This method sets the height of this component.
 */
INLINE void
BoundingBoxGUIComponent::height( float h )
{
	BW_GUARD;
	SimpleGUIComponent::height( h );

	corners_[0]->height( h );
	corners_[1]->height( h );
	corners_[2]->height( h );
	corners_[3]->height( h );
}


/**
 *	This method sets the colour of this component.
 */
INLINE void
BoundingBoxGUIComponent::colour( uint32 col )
{
	BW_GUARD;
	SimpleGUIComponent::colour( col );

	corners_[0]->colour( col );
	corners_[1]->colour( col );
	corners_[2]->colour( col );
	corners_[3]->colour( col );
}

BW_END_NAMESPACE

/*bounding_box_gui_component.ipp*/
