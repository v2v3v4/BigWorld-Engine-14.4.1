// simple_gui_component.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


INLINE ConstSimpleGUIComponentPtr SimpleGUIComponent::nearestRelativeParent(int depth) const
{
	BW_GUARD;
	if (!parent_.exists())
	{
		return NULL;
	}
	
	return parent_->nearestRelativeParent(depth+1);
}

INLINE void SimpleGUIComponent::nearestRelativeDimensions( float &width, float& height ) const
{
	BW_GUARD;
	// See if we have a relative parent, if we don't use the screen dimensions.
	ConstSimpleGUIComponentPtr nrp = nearestRelativeParent();
	if (nrp)
	{
		// If size mode is PIXELS, then take a short cut.
		if (nrp->widthMode_ == SIZE_MODE_PIXEL && nrp->heightMode_ == SIZE_MODE_PIXEL)
		{
			width = nrp->width_;
			height = nrp->height_;
		}
		else
		{
			// We need to get the width in pixels of the nrp's nrp :)
			float nrpRelativeParentWidth, nrpRelativeParentHeight;			
			nrp->nearestRelativeDimensions( nrpRelativeParentWidth, nrpRelativeParentHeight );
			
			// Get the width in pixels of the nrp.
			width = nrp->widthInPixels( nrpRelativeParentWidth );
			height = nrp->heightInPixels( nrpRelativeParentHeight );
		}
	}
	else
	{
		width = SimpleGUI::instance().screenWidth();
		height = SimpleGUI::instance().screenHeight();
	}
}

INLINE SimpleGUIComponent::eRotation SimpleGUIComponent::angle() const
{
	return cachedAngle_;
}


INLINE void SimpleGUIComponent::angle( eRotation r )
{
	BW_GUARD;
	cachedAngle_ = r;
	calculateAutoMC();
}


INLINE void SimpleGUIComponent::calculateAutoMC()
{
	BW_GUARD;
	switch( cachedAngle_ )
	{
	case ROT_0:
		blueprint_[0].uv_[0] = 0.f;		blueprint_[0].uv_[1] = 0.f;
		blueprint_[1].uv_[0] = 0.f;		blueprint_[1].uv_[1] = 1.f;
		blueprint_[2].uv_[0] = 1.f;		blueprint_[2].uv_[1] = 1.f;
		blueprint_[3].uv_[0] = 1.f;		blueprint_[3].uv_[1] = 0.f;
		break;
	case ROT_90:
		blueprint_[0].uv_[0] = 0.f;		blueprint_[0].uv_[1] = 1.f;
		blueprint_[1].uv_[0] = 1.f;		blueprint_[1].uv_[1] = 1.f;
		blueprint_[2].uv_[0] = 1.f;		blueprint_[2].uv_[1] = 0.f;
		blueprint_[3].uv_[0] = 0.f;		blueprint_[3].uv_[1] = 0.f;
		break;
	case ROT_180:
		blueprint_[0].uv_[0] = 1.f;		blueprint_[0].uv_[1] = 1.f;
		blueprint_[1].uv_[0] = 1.f;		blueprint_[1].uv_[1] = 0.f;
		blueprint_[2].uv_[0] = 0.f;		blueprint_[2].uv_[1] = 0.f;
		blueprint_[3].uv_[0] = 0.f;		blueprint_[3].uv_[1] = 1.f;
		break;
	case ROT_270:
		blueprint_[0].uv_[0] = 1.f;		blueprint_[0].uv_[1] = 0.f;
		blueprint_[1].uv_[0] = 0.f;		blueprint_[1].uv_[1] = 0.f;
		blueprint_[2].uv_[0] = 0.f;		blueprint_[2].uv_[1] = 1.f;
		blueprint_[3].uv_[0] = 1.f;		blueprint_[3].uv_[1] = 1.f;
		break;
	}
	
	for ( int i=0; i<4; i++ )
	{
		if ( flip_ & FLIP_X )
			blueprint_[i].uv_[0] = 1.f - blueprint_[i].uv_[0];
		if ( flip_ & FLIP_Y )
			blueprint_[i].uv_[1] = 1.f - blueprint_[i].uv_[1];
	}	
}


INLINE void SimpleGUIComponent::flip( int f )
{
	BW_GUARD;
	this->flip_ = f;
	this->calculateAutoMC();
}


INLINE int SimpleGUIComponent::flip() const
{
	return flip_;
}


INLINE void SimpleGUIComponent::mapping( Vector2* mc )
{
	BW_GUARD;
	for ( int i = 0; i < 4; i++ )
	{
		blueprint_[i].uv_[0] = mc[i].x;
		blueprint_[i].uv_[1] = mc[i].y;
	}
}


INLINE SimpleGUIComponent::eHAnchor SimpleGUIComponent::horizontalAnchor() const
{
	return horizontalAnchor_;
}


INLINE SimpleGUIComponent::eVAnchor SimpleGUIComponent::verticalAnchor() const
{
	return verticalAnchor_;
}


INLINE void SimpleGUIComponent::horizontalAnchor( SimpleGUIComponent::eHAnchor h )
{
	horizontalAnchor_ = h;
}

INLINE void SimpleGUIComponent::verticalAnchor( SimpleGUIComponent::eVAnchor v )
{
	verticalAnchor_ = v;
}

INLINE void SimpleGUIComponent::anchor( eHAnchor h, eVAnchor v )
{
	horizontalAnchor_ = h;
	verticalAnchor_ = v;
}

INLINE uint32 SimpleGUIComponent::colour() const
{
	return colour_;
}


INLINE void SimpleGUIComponent::colour( uint32 col )
{
	colour_ = col;
}

/*INLINE Material& SimpleGUIComponent::material()
{
	return material_;
}*/


INLINE const Vector3& SimpleGUIComponent::position() const
{
	return position_;
}

INLINE SimpleGUIComponent::ePositionMode SimpleGUIComponent::horizontalPositionMode() const
{
	return horizontalPositionMode_;
}


INLINE SimpleGUIComponent::ePositionMode SimpleGUIComponent::verticalPositionMode() const
{
	return verticalPositionMode_;
}


INLINE void SimpleGUIComponent::position( const Vector3& p )
{
	position_ = p;
}


INLINE float SimpleGUIComponent::width() const
{
	return width_;
}


INLINE void SimpleGUIComponent::width( float w )
{
	width_ = w;
}

INLINE SimpleGUIComponent::eSizeMode SimpleGUIComponent::widthMode() const
{
	return widthMode_;
}


INLINE void SimpleGUIComponent::widthInClipCoords( bool state )
{
	BW_GUARD;
	WARNING_MSG( "SimpleGUIComponent.widthRelative has been deprecated. Use widthMode instead.\n" );
	widthMode( state ? SIZE_MODE_LEGACY : SIZE_MODE_PIXEL );
}


INLINE bool SimpleGUIComponent::widthInClipCoords() const
{
	BW_GUARD;
	WARNING_MSG( "SimpleGUIComponent.widthRelative has been deprecated. Use widthMode instead.\n" );
	return widthMode() == SIZE_MODE_LEGACY;
}


INLINE float SimpleGUIComponent::height() const
{
	return height_;
}


INLINE void SimpleGUIComponent::height( float h )
{
	height_ = h;
}

INLINE SimpleGUIComponent::eSizeMode SimpleGUIComponent::heightMode() const
{
	return heightMode_;
}


INLINE void SimpleGUIComponent::heightInClipCoords( bool state )
{
	WARNING_MSG( "SimpleGUIComponent.heightRelative has been deprecated. Use heightMode instead.\n" );
	heightMode( state ? SIZE_MODE_LEGACY : SIZE_MODE_PIXEL );
}


INLINE bool SimpleGUIComponent::heightInClipCoords() const
{
	WARNING_MSG( "SimpleGUIComponent.heightRelative has been deprecated. Use heightMode instead.\n" );
	return heightMode() == SIZE_MODE_LEGACY;
}


INLINE Vector2 SimpleGUIComponent::size()
{
	return Vector2( this->width(), this->height() );
}


INLINE bool SimpleGUIComponent::visible() const
{
	return visible_;
}


INLINE void SimpleGUIComponent::visible( bool v )
{
	visible_ = v;
}


INLINE bool SimpleGUIComponent::visibleInAppFocusOnly() const
{
	return visibleInAppFocusOnly_;
}


INLINE void SimpleGUIComponent::visibleInAppFocusOnly( bool state )
{
	visibleInAppFocusOnly_ = state;
}


INLINE bool SimpleGUIComponent::momentarilyInvisible() const
{
	return momentarilyInvisible_ || !texture_;
}


INLINE void SimpleGUIComponent::momentarilyInvisible( bool mi )
{
	momentarilyInvisible_ = mi;
}

INLINE const BW::string& SimpleGUIComponent::textureName() const
{
	BW_GUARD;
	static BW::string noName;
	if( texture_ )
		return texture_->resourceID();

	return noName;
}


INLINE void SimpleGUIComponent::materialFX( SimpleGUIComponent::eMaterialFX fx )
{
	if ( fx != materialFX_ )
	{
		materialFX_ = fx;
		buildMaterial();
	}
}


INLINE SimpleGUIComponent::eMaterialFX SimpleGUIComponent::materialFX() const
{
	return materialFX_;
}


INLINE void SimpleGUIComponent::filterType( SimpleGUIComponent::eFilterType ft )
{
	if ( ft != filterType_ )
	{
		filterType_ = ft;
	}
}


INLINE SimpleGUIComponent::eFilterType SimpleGUIComponent::filterType() const
{
	return filterType_;
}


INLINE bool	SimpleGUIComponent::tiled() const
{
	return tiled_;
}


INLINE void	SimpleGUIComponent::tiled( bool state )
{
	tiled_ = state;
}


INLINE int SimpleGUIComponent::tileWidth() const
{
	return int(tileWidth_);
}


INLINE void SimpleGUIComponent::tileWidth( int w )
{
	tileWidth_ = float(w);
}


INLINE int SimpleGUIComponent::tileHeight() const
{
	return int(tileHeight_);
}


INLINE void SimpleGUIComponent::tileHeight( int h )
{
	tileHeight_ = float(h);
}


/**
 *	This method returns whether this component has the input focus.
 *
 *	@return True if this component has the input focus.
 */
INLINE bool SimpleGUIComponent::focus() const
{
	return focus_;
}

/**
 *	This method returns whether this component has mouse input focus.
 *
 *	@return True if this component has mouse input focus.
 */
INLINE bool SimpleGUIComponent::mouseButtonFocus() const
{
	return mouseButtonFocus_;
}



/**
 *	This method returns whether this component has the mouve move focus.
 *
 *	@return True if this component has the move focus.
 */
INLINE bool SimpleGUIComponent::moveFocus() const
{
	return moveFocus_;
}


/**
 *	This method returns whether this component has the mouse cross focus.
 *
 *	@return True if this component has the cross focus.
 */
INLINE bool SimpleGUIComponent::crossFocus() const
{
	return crossFocus_;
}


/**
 *	This method returns whether this component has the drag focus.
 *
 *	@return True if this component has the drag focus.
 */
INLINE bool SimpleGUIComponent::dragFocus() const
{
	return dragFocus_;
}


/**
 *	This method returns whether this component has the drop focus.
 *
 *	@return True if this component has the drop focus.
 */
INLINE bool SimpleGUIComponent::dropFocus() const
{
	return dropFocus_;
}

INLINE uint32 SimpleGUIComponent::runTimeColour() const
{
	return runTimeColour_;
}


INLINE void SimpleGUIComponent::runTimeColour( uint32 col )
{
	runTimeColour_ = col;
}


INLINE const Matrix & SimpleGUIComponent::runTimeTransform() const
{
	return runTimeTransform_;
}


INLINE void SimpleGUIComponent::runTimeTransform( const Matrix& m )
{
	runTimeTransform_ = m;
}

INLINE void SimpleGUIComponent::pixelSnap( bool state )
{
	pixelSnap_ = state;
}


INLINE bool SimpleGUIComponent::pixelSnap() const
{
	return pixelSnap_;
}

/**
 *	This method returns whether this component is able to handle input events 
 *	(i.e. if it or any of its parents are invisible then it will not handle input 
 *	events).
 *
 *	@return True if this component is currently able to handle input events.
 */
INLINE bool SimpleGUIComponent::handlesInput() const
{
	if (!visible_)
	{
		return false;
	}
	 
	return parent_.exists() ? parent_->handlesInput() : true;
}

// simple_gui_component.ipp
