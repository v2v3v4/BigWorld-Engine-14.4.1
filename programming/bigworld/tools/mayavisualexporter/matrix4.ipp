BW_BEGIN_NAMESPACE

template<class T>
inline matrix4<T>::matrix4( const T& a11, const T& a12, const T& a13, const T& a14, 
							const T& a21, const T& a22, const T& a23, const T& a24,
							const T& a31, const T& a32, const T& a33, const T& a34,
							const T& a41, const T& a42, const T& a43, const T& a44 ) :
	v11( a11 ), v12( a12 ), v13( a13 ), v14( a14 ),
	v21( a21 ), v22( a22 ), v23( a23 ), v24( a24 ),
	v31( a31 ), v32( a32 ), v33( a33 ), v34( a34 ),
	v41( a41 ), v42( a42 ), v43( a43 ), v44( a44 )
{
}

template<class T>
inline matrix4<T>::matrix4( const vector3<T>& a1, const vector3<T>& a2, const vector3<T>& a3 ) :
	v11( a1.x ), v12( a1.y ), v13( a1.z ), v14( 0 ),
	v21( a2.x ), v22( a2.y ), v23( a2.z ), v24( 0 ),
	v31( a3.x ), v32( a3.y ), v33( a3.z ), v34( 0 ),
	v41( 0 ), v42( 0 ), v43( 0 ), v44( 1 )
{
}

template<class T>
matrix4<T>::~matrix4()
{
}

template<class T>
inline matrix4<T> matrix4<T>::rotationX( const T& aAngle )
{
	return matrix4<T>( 1, 0, 0, 0, 0, cos( aAngle ), sin( aAngle ), 0, 0, -sin( aAngle), cos( aAngle ), 0, 0, 0, 0, 1 );

//	return matrix4<T>( cos( aAngle ), -sin( aAngle ), 0, sin( aAngle), cos( aAngle ), 0, 0, 0, 1 );
}

template<class T>
inline matrix4<T> matrix4<T>::rotationY( const T& aAngle )
{
	return matrix4<T>( cos( aAngle ), 0, -sin( aAngle ), 0, 0, 1, 0, 0, sin( aAngle), 0, cos( aAngle ), 0, 0, 0, 0, 1 );
}

template<class T>
inline matrix4<T> matrix4<T>::rotationZ( const T& aAngle )
{
	return matrix4<T>( cos( aAngle ), -sin( aAngle ), 0, 0, sin( aAngle), cos( aAngle ), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 );
}

template<class T>
inline matrix4<T> matrix4<T>::translation( const T& aTranslateX, const T& aTranslateY, const T& aTranslateZ )
{
	return matrix4<T>( 1, 0, 0, aTranslateX, 0, 1, 0, aTranslateY, 0, 0, 1, aTranslateZ, 0, 0, 0, 1 );
}

template<class T>
inline matrix4<T> matrix4<T>::translation( const vector3<T>& aVector )
{
	return matrix4<T>( 1, 0, 0, aVector.x, 0, 1, 0, aVector.y, 0, 0, 1, aVector.z, 0, 0, 0, 1 );
}

template<class T>
inline matrix4<T> matrix4<T>::scaling( const T& aScaleX, const T& aScaleY, const T& aScaleZ )
{
	return matrix4<T>( aScaleX, 0, 0, 0, 0, aScaleY, 0, 0, 0, 0, aScaleZ, 0, 0, 0, 0, 1 );
}

template<class T>
inline matrix4<T> matrix4<T>::zero()
{
	return matrix4<T>( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
}

template<class T>
inline void matrix4<T>::identity()
{
	v11 = 1;
	v12 = 0;
	v13 = 0;
	v14 = 0;
	v21 = 0;
	v22 = 1;
	v23 = 0;
	v24 = 0;
	v31 = 0;
	v32 = 0;
	v33 = 1;
	v34 = 0;
	v41 = 0;
	v42 = 0;
	v43 = 0;
	v44 = 1;
}

// helper function to return determinant of a 3x3 matrix
template<class T>
inline T determinant3x3( const T& a11, const T& a12, const T& a13, 
						 const T& a21, const T& a22, const T& a23,
						 const T& a31, const T& a32, const T& a33 )
{
	return a11 * a22 * a33 - a11 * a23 * a32 - a12 * a21 * a33 + a12 * a23 * a31 + a13 * a21 * a32 - a13 * a22 * a31;
}

template<class T>
inline T matrix4<T>::determinant() const
{
	return v11 * determinant3x3( v22, v23, v24, v32, v33, v34, v42, v43, v44 ) -
		   v12 * determinant3x3( v21, v23, v24, v31, v33, v34, v41, v43, v44 ) +
		   v13 * determinant3x3( v21, v22, v24, v31, v32, v34, v41, v42, v44 ) -
		   v14 * determinant3x3( v21, v22, v23, v31, v32, v33, v41, v42, v43 );
}

template<class T>
inline matrix4<T> matrix4<T>::inverse() const
{
	T det = determinant();
	
	if( det == 0 )
		0; // REMOVED Error log for maya exporter

	return matrix4<T>( +determinant3x3( v22, v32, v42, v23, v33, v43, v24, v34, v44 ),
					   -determinant3x3( v12, v32, v42, v13, v33, v43, v14, v34, v44 ),
					   +determinant3x3( v12, v22, v42, v13, v23, v43, v14, v24, v44 ),
					   -determinant3x3( v12, v22, v32, v13, v23, v33, v14, v24, v34 ),
	
					   -determinant3x3( v21, v31, v41, v23, v33, v43, v24, v34, v44 ),
					   +determinant3x3( v11, v31, v41, v13, v33, v43, v14, v34, v44 ),
					   -determinant3x3( v11, v21, v41, v13, v23, v43, v14, v24, v44 ),
					   +determinant3x3( v11, v21, v31, v13, v23, v33, v14, v24, v34 ),

					   +determinant3x3( v21, v31, v41, v22, v32, v42, v24, v34, v44 ),
					   -determinant3x3( v11, v31, v41, v12, v32, v42, v14, v34, v44 ),
					   +determinant3x3( v11, v21, v41, v12, v22, v42, v14, v24, v44 ),
					   -determinant3x3( v11, v21, v31, v12, v22, v32, v14, v24, v34 ),

					   -determinant3x3( v21, v31, v41, v22, v32, v42, v23, v33, v43 ),
					   +determinant3x3( v11, v31, v41, v12, v32, v42, v13, v33, v43 ),
					   -determinant3x3( v11, v21, v41, v12, v22, v42, v13, v23, v43 ),
					   +determinant3x3( v11, v21, v31, v12, v22, v32, v13, v23, v33 )
					 ) / det;
}

template<class T>
inline matrix4<T> matrix4<T>::transpose() const
{
	return matrix4<T>( v11, v21, v31, v41, v12, v22, v32, v42, v13, v23, v33, v43, v14, v24, v34, v44 );
}

template<class T>
inline vector3<T> matrix4<T>::translation() const
{
	return vector3<T>( v14, v24, v34 );
}

template<class T>
inline bool matrix4<T>::operator==( const matrix4<T>& aMatrix ) const
{
	const matrix4<T>& m = aMatrix;
	return v11 == m.v11 && v12 == m.v12 && v13 == m.v13 && v14 == m.v14 &&
		   v21 == m.v21 && v22 == m.v22 && v23 == m.v23 && v24 == m.v24 &&
		   v31 == m.v31 && v32 == m.v32 && v33 == m.v33 && v34 == m.v34 &&
		   v41 == m.v41 && v42 == m.v42 && v43 == m.v43 && v44 == m.v44;
}

template<class T>
inline bool matrix4<T>::operator!=( const matrix4<T>& aMatrix ) const
{
	const matrix4<T>& m = aMatrix;
	return v11 != m.v11 || v12 != m.v12 || v13 != m.v13 || v14 != m.v14 ||
		   v21 != m.v21 || v22 != m.v22 || v23 != m.v23 || v24 != m.v24 ||
		   v31 != m.v31 || v32 != m.v32 || v33 != m.v33 || v34 != m.v34 ||
		   v41 != m.v41 || v42 != m.v42 || v43 != m.v43 || v44 != m.v44;
}
	
template<class T>
inline matrix4<T> matrix4<T>::operator*( const matrix4<T>& aMatrix ) const
{
	const matrix4<T>& m = aMatrix;
	return matrix4<T>( v11 * m.v11 + v12 * m.v21 + v13 * m.v31 + v14 * m.v41, v11 * m.v12 + v12 * m.v22 + v13 * m.v32 + v14 * m.v42, v11 * m.v13 + v12 * m.v23 + v13 * m.v33 + v14 * m.v43, v11 * m.v14 + v12 * m.v24 + v13 * m.v34 + v14 * m.v44,
					   v21 * m.v11 + v22 * m.v21 + v23 * m.v31 + v24 * m.v41, v21 * m.v12 + v22 * m.v22 + v23 * m.v32 + v24 * m.v42, v21 * m.v13 + v22 * m.v23 + v23 * m.v33 + v24 * m.v43, v21 * m.v14 + v22 * m.v24 + v23 * m.v34 + v24 * m.v44,
					   v31 * m.v11 + v32 * m.v21 + v33 * m.v31 + v34 * m.v41, v31 * m.v12 + v32 * m.v22 + v33 * m.v32 + v34 * m.v42, v31 * m.v13 + v32 * m.v23 + v33 * m.v33 + v34 * m.v43, v31 * m.v14 + v32 * m.v24 + v33 * m.v34 + v34 * m.v44,
					   v41 * m.v11 + v42 * m.v21 + v43 * m.v31 + v44 * m.v41, v41 * m.v12 + v42 * m.v22 + v43 * m.v32 + v44 * m.v42, v41 * m.v13 + v42 * m.v23 + v43 * m.v33 + v44 * m.v43, v41 * m.v14 + v42 * m.v24 + v43 * m.v34 + v44 * m.v44 );
}

template<class T>
inline matrix4<T> matrix4<T>::operator*( const T& aScalar ) const
{
	return matrix4<T>( v11 * aScalar, v12 * aScalar, v13 * aScalar, v14 * aScalar,
					   v21 * aScalar, v22 * aScalar, v23 * aScalar, v24 * aScalar,
					   v31 * aScalar, v32 * aScalar, v33 * aScalar, v34 * aScalar,
					   v41 * aScalar, v42 * aScalar, v43 * aScalar, v44 * aScalar );
}

template<class T>
inline matrix4<T> matrix4<T>::operator/( const T& aScalar ) const
{
	T invert = 1 / aScalar;
	return matrix4<T>( v11 * invert, v12 * invert, v13 * invert, v14 * invert,
					   v21 * invert, v22 * invert, v23 * invert, v24 * invert,
					   v31 * invert, v32 * invert, v33 * invert, v34 * invert,
					   v41 * invert, v42 * invert, v43 * invert, v44 * invert );
}

template<class T>
inline vector3<T> matrix4<T>::operator*( const vector3<T>& aVector ) const
{
	return vector3<T>( v11 * aVector.x + v12 * aVector.y + v13 * aVector.z + v14,
					   v21 * aVector.x + v22 * aVector.y + v23 * aVector.z + v24,
					   v31 * aVector.x + v32 * aVector.y + v33 * aVector.z + v34 );
}

template<class T>
inline matrix4<T> matrix4<T>::operator+( const matrix4<T>& aMatrix ) const
{
	const matrix4<T>& m = aMatrix;
	return matrix4<T>( v11 + m.v11, v12 + m.v12, v13 + m.v13, v14 + m.v14,
					   v21 + m.v21, v22 + m.v22, v23 + m.v23, v24 + m.v24,
					   v31 + m.v31, v32 + m.v32, v33 + m.v33, v34 + m.v34,
					   v41 + m.v41, v42 + m.v42, v43 + m.v43, v44 + m.v44 );
}

template<class T>
inline matrix4<T> matrix4<T>::operator-( const matrix4<T>& aMatrix ) const
{
	const matrix4<T>& m = aMatrix;
	return matrix4<T>( v11 - m.v11, v12 - m.v12, v13 - m.v13, v14 - m.v14,
					   v21 - m.v21, v22 - m.v22, v23 - m.v23, v24 - m.v24,
					   v31 - m.v31, v32 - m.v32, v33 - m.v33, v34 - m.v34,
					   v41 - m.v41, v42 - m.v42, v43 - m.v43, v44 - m.v44 );
}

//~ template<class T>
//~ inline matrix4<T>::operator String() const
//~ {
	//~ return String( "[ " ) + (String)v11 + ", " + (String)v12 + ", " + (String)v13 + ", " + (String)v14 + " ]\n" +
		   //~ String( "[ " ) + (String)v21 + ", " + (String)v22 + ", " + (String)v23 + ", " + (String)v24 + " ]\n" +
		   //~ String( "[ " ) + (String)v31 + ", " + (String)v32 + ", " + (String)v33 + ", " + (String)v34 + " ]\n" +
		   //~ String( "[ " ) + (String)v41 + ", " + (String)v42 + ", " + (String)v43 + ", " + (String)v44 + " ]";
//~ }

template<class T>
inline typename matrix4<T>::Row matrix4<T>::operator[]( uint32 aIndex )
{
	if( aIndex == 0 )
		return Row( v11, v12, v13, v14 );	
	
	if( aIndex == 1 )
		return Row( v21, v22, v23, v24 );

	if( aIndex == 2 )
		return Row( v31, v32, v33, v34 );

	if( aIndex == 3 )
		return Row( v41, v42, v43, v44 );
	
	// REMOVED Error log - for Maya Exporter

	// have to return something - Error::FATAL should halt program	
	return Row( v41, v42, v43, v44 );
}

BW_END_NAMESPACE

