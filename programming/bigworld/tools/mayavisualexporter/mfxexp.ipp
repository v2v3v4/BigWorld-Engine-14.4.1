
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// INLINE void MFXExport::inlineFunction()
// {
// }

INLINE
TimeValue MFXExport::staticFrame( void )
{
	return settings_.staticFrame() * GetTicksPerFrame();
}

INLINE
Point3 MFXExport::applyUnitScale( const Point3& p )
{
	return p * settings_.unitScale();
}

INLINE
Matrix3 MFXExport::applyUnitScale( const Matrix3& m )
{
	Matrix3 m2 = m;
	m2.SetRow( 3, m.GetRow( 3 ) * settings_.unitScale() );
	return m2;
}

/*mfxexp.ipp*/
