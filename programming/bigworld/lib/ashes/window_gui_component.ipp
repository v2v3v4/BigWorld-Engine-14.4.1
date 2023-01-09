// window.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

INLINE ConstSimpleGUIComponentPtr WindowGUIComponent::nearestRelativeParent(int depth) const
{
	if (depth > 0)
	{
		return ConstSimpleGUIComponentPtr(this);
	}
	
	return SimpleGUIComponent::nearestRelativeParent(depth);
}


// window.ipp