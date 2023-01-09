// frame_gui_component.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE

INLINE const BW::string & FrameGUIComponent::edgeTextureName() const
{
	BW_GUARD;
	return edges_[0]->textureName();
}

INLINE void FrameGUIComponent::edgeTextureName( const BW::string & name )
{
	BW_GUARD;
	for (int i = 0; i < 4; i++)
	{
		edges_[i]->textureName( name );
	}
}

INLINE const BW::string & FrameGUIComponent::cornerTextureName() const
{
	BW_GUARD;
	return corners_[0]->textureName();
}

INLINE void FrameGUIComponent::cornerTextureName( const BW::string & name )
{
	BW_GUARD;
	for (int i = 0; i < 4; i++)
	{
		corners_[i]->textureName( name );
	}
}

BW_END_NAMESPACE

// frame_gui_component.ipp