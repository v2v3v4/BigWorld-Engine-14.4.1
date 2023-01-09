BW_BEGIN_NAMESPACE

template<class T>
inline matrix3<T>::matrix3( const T& a11, const T& a12, const T& a13, 
							const T& a21, const T& a22, const T& a23,
							const T& a31, const T& a32, const T& a33 ) :
	v11( a11 ), v12( a12 ), v13( a13 ),
	v21( a21 ), v22( a22 ), v23( a23 ),
	v31( a31 ), v32( a32 ), v33( a33 )
{
}

template<class T>
inline matrix3<T>::matrix3( const vector3<T>& a1, const vector3<T>& a2, const vector3<T>& a3 ) :
	v11( a1.x ), v12( a1.y ), v13( a1.z ),
	v21( a2.x ), v22( a2.y ), v23( a2.z ),
	v31( a3.x ), v32( a3.y ), v33( a3.z )
{
}

template<class T>
inline matrix3<T>::matrix3( const vector2<T>& a1, const vector2<T>& a2 ) :
	v11( a1.x ), v12( a1.y ), v13( 0 ),
	v21( a2.x ), v22( a2.y ), v23( 0 ),
	v31( 0 ), v32( 0 ), v33( 1 )
{
}

template<class T>
matrix3<T>::~matrix3()
{
}

template<class T>
inline matrix3<T> matrix3<T>::rotation( const T& aAngle )
{
	return matrix3<T>( cos( aAngle ), -sin( aAngle ), 0, sin( aAngle), cos( aAngle ), 0, 0, 0, 1 );
}

template<class T>
inline matrix3<T> matrix3<T>::translation( const T& aTranslateX, const T& aTranslateY )
{
	return matrix3<T>( 1, 0, aTranslateX, 0, 1, aTranslateY, 0, 0, 1 );
}

template<class T>
inline matrix3<T> matrix3<T>::scaling( const T& aScaleX, const T& aScaleY )
{
	return matrix3<T>( aScaleX, 0, 0, 0, aScaleY, 0, 0, 0, 1 );
}

template<class T>
inline void matrix3<T>::identity()
{
	v11 = 1;
	v12 = 0;
	v13 = 0;
	v21 = 0;
	v22 = 1;
	v23 = 0;
	v31 = 0;
	v32 = 0;
	v33 = 1;
}

template<class T>
inline T matrix3<T>::determinant() const
{
	return v11 * v22 * v33 - v11 * v23 * v32 - v12 * v21 * v33 + v12 * v23 * v31 + v13 * v21 * v32 - v13 * v22 * v31;
}

template<class T>
inline matrix3<T> matrix3<T>::inverse() const
{
	T det = determinant();
	
	if( det == 0 )
		; // REMOVED Error log for maya exporter

	return matrix3<T>( v22 * v33 - v32 * v23, v32 * v13 - v12 * v33, v12 * v23 - v22 * v13,
					   v31 * v23 - v21 * v33, v11 * v33 - v31 * v13, v21 * v13 - v11 * v23,
					   v21 * v32 - v31 * v22, v31 * v12 - v11 * v32, v11 * v22 - v21 * v12
					 ) / det;
}

template<class T>
inline matrix3<T> matrix3<T>::transpose() const
{
	return matrix3<T>( v11, v21, v31, v12, v22, v32, v13, v23, v33 );
}
	
template<class T>
inline bool matrix3<T>::operator==( const matrix3<T>& aMatrix ) const
{
	const matrix3<T>& m = aMatrix;
	return v11 == m.v11 && v12 == m.v12 && v13 == m.v13 &&
		   v21 == m.v21 && v22 == m.v22 && v23 == m.v23 &&
		   v31 == m.v31 && v32 == m.v32 && v33 == m.v33;
}

template<class T>
inline bool matrix3<T>::operator!=( const matrix3<T>& aMatrix ) const
{
	const matrix3<T>& m = aMatrix;
	return v11 != m.v11 || v12 != m.v12 || v13 != m.v13 ||
		   v21 != m.v21 || v22 != m.v22 || v23 != m.v23 ||
		   v31 != m.v31 || v32 != m.v32 || v33 != m.v33;
}
	
template<class T>
inline matrix3<T> matrix3<T>::operator*( const matrix3<T>& aMatrix ) const
{
	const matrix3<T>& m = aMatrix;
	return matrix3<T>( v11 * m.v11 + v12 * m.v21 + v13 * m.v31, v11 * m.v12 + v12 * m.v22 + v13 * m.v32, v11 * m.v13 + v12 * m.v23 + v13 * m.v33, 
					   v21 * m.v11 + v22 * m.v21 + v23 * m.v31, v21 * m.v12 + v22 * m.v22 + v23 * m.v32, v21 * m.v13 + v22 * m.v23 + v23 * m.v33, 
					   v31 * m.v11 + v32 * m.v21 + v33 * m.v31, v31 * m.v12 + v32 * m.v22 + v33 * m.v32, v31 * m.v13 + v32 * m.v23 + v33 * m.v33 );
}

template<class T>
inline matrix3<T> matrix3<T>::operator*( const T& aScalar ) const
{
	return matrix3<T>( v11 * aScalar, v12 * aScalar, v13 * aScalar, 
					   v21 * aScalar, v22 * aScalar, v23 * aScalar,
					   v31 * aScalar, v32 * aScalar, v33 * aScalar );
}

template<class T>
inline matrix3<T> matrix3<T>::operator/( const T& aScalar ) const
{
	T invert = 1 / aScalar;
	return matrix3<T>( v11 * invert, v12 * invert, v13 * invert, 
					   v21 * invert, v22 * invert, v23 * invert,
					   v31 * invert, v32 * invert, v33 * invert );
}

template<class T>
inline vector2<T> matrix3<T>::operator*( const vector2<T>& aVector ) const
{
	return vector2<T>( v11 * aVector.x + v12 * aVector.y + v13,
					   v21 * aVector.x + v22 * aVector.y + v23 );
}

template<class T>
inline vector3<T> matrix3<T>::operator*( const vector3<T>& aVector ) const
{
	return vector3<T>( v11 * aVector.x + v12 * aVector.y + v13 * aVector.z,
					   v21 * aVector.x + v22 * aVector.y + v23 * aVector.z,
					   v31 * aVector.x + v32 * aVector.y + v33 * aVector.z );
}

//~ template<class T>
//~ inline matrix3<T>::operator String() const
//~ {
	//~ return String( "[ " ) + (String)v11 + ", " + (String)v12 + ", " + (String)v13 + " ]\n" +
		   //~ String( "[ " ) + (String)v21 + ", " + (String)v22 + ", " + (String)v23 + " ]\n" +
		   //~ String( "[ " ) + (String)v31 + ", " + (String)v32 + ", " + (String)v33 + " ]";
//~ }

template<class T>
inline typename matrix3<T>::Row matrix3<T>::operator[]( uint32 aIndex )
{
	if( aIndex == 0 )
		return Row( v11, v12, v13 );	
	
	if( aIndex == 1 )
		return Row( v21, v22, v23 );

	if( aIndex == 2 )
		return Row( v31, v32, v33 );
	
	; // REMOVED error log for maya exporter

	// have to return something - Error::FATAL should halt program	
	return Row( v31, v32, v33 );
}

template<class T>
inline matrix3<T> operator*( const T& aScalar, const matrix3<T>& aMatrix )
{
	return aMatrix * aScalar;
}

BW_END_NAMESPACE

