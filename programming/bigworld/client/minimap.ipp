// minimap.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


INLINE const BW::string& Minimap::maskName() const
{
	static BW::string noName("");
	if( mask_ )
		return mask_->resourceID();

	return noName;
}


INLINE const BW::string& Minimap::simpleEntryMap() const
{
	static BW::string noName("");
	if( simpleEntryMap_ )
		return simpleEntryMap_->resourceID();

	return noName;
}


INLINE Vector3 Minimap::viewpointPosition() const
{
	Matrix viewpoint;
	if (viewpoint_)
	{
		viewpoint_->matrix( viewpoint );
	}
	else
	{
		viewpoint = Moo::rc().invView();
	}
	return viewpoint.applyToOrigin();
}



// minimap.ipp