// mesh_gui_adaptor.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// mesh_gui_adaptor.ipp

INLINE const Matrix & MeshGUIAdaptor::getMatrix() const
{
	BW_GUARD;
	static Matrix m;
	if ( transform_ )
		transform_->matrix(m);
	return m;
}