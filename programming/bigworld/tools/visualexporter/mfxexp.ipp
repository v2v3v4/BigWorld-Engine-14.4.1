
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE


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
	return p * (float)GetMasterScale( UNITS_CENTIMETERS ) / 100;
}

INLINE
Matrix3 MFXExport::applyUnitScale( const Matrix3& m )
{
	Matrix3 m2 = m;
	m2.SetRow( 3, m.GetRow( 3 ) * (float)GetMasterScale( UNITS_CENTIMETERS ) / 100 );
	return m2;
}

BW_END_NAMESPACE

/*mfxexp.ipp*/
