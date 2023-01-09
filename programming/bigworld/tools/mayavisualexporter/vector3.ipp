BW_BEGIN_NAMESPACE

template<class T>
inline vector3<T>::vector3( const T& aX, const T& aY, const T& aZ ) :
	x( aX ), y( aY ), z( aZ )
{
}

template<class T>
inline vector3<T>::vector3( const vector2<T>& aVector2, const T& aZ = 0 ) :
	x( aVector2.x ), y( aVector2.y ), z( aZ )
{
}

template<class T>
vector3<T>::~vector3()
{
}

template<class T>
inline vector3<T> vector3<T>::X()
{
	return vector3<T>( 1, 0, 0 );
}

template<class T>
inline vector3<T> vector3<T>::Y()
{
	return vector3<T>( 0, 1, 0 );
}

template<class T>
inline vector3<T> vector3<T>::Z()
{
	return vector3<T>( 0, 0, 1 );
}

template<class T>
inline vector3<T>& vector3<T>::normalise()
{
	T invmag = 1 / magnitude();
	
	x = x  * invmag;
	y = y * invmag;
	z = z * invmag;
	
	return *this;
}

template<class T>
inline T vector3<T>::magnitude() const
{
	return (T)sqrt( x * x + y * y + z * z );
}

template<class T>
inline T vector3<T>::magnitudeSquared() const
{
	return x * x + y * y + z * z;
}

template<class T>
inline vector3<T> vector3<T>::cross( const vector3<T>& aVector3 ) const
{
	return vector3<T>( y * aVector3.z - z * aVector3.y,
					   z * aVector3.x - x * aVector3.z,
					   x * aVector3.y - y * aVector3.x );
}

template<class T>
inline T vector3<T>::dot( const vector3<T>& aVector3 ) const
{
	return x * aVector3.x + y * aVector3.y + z * aVector3.z;
}

template<class T>
inline bool vector3<T>::operator==( const vector3<T>& aVector3 ) const
{
	return x == aVector3.x && y == aVector3.y && z == aVector3.z;
}

template<class T>
inline bool vector3<T>::operator!=( const vector3<T>& aVector3 ) const
{
	return x != aVector3.x || y != aVector3.y || z != aVector3.z;
}

template<class T>
inline vector3<T> vector3<T>::operator-()
{
	return vector3<T>( -x, -y, -z );
}
	
template<class T>
inline vector3<T> vector3<T>::operator*( const T& aT ) const
{
	return vector3<T>( x * aT, y * aT, z * aT );
}

template<class T>
inline vector3<T> vector3<T>::operator/( const T& aT ) const
{
	return vector3<T>( x / aT, y / aT, z / aT );
}

template<class T>
inline vector3<T> vector3<T>::operator+( const vector3<T>& aVector3 ) const
{
	return vector3<T>( x + aVector3.x, y + aVector3.y, z + aVector3.z );
}

template<class T>
inline vector3<T> vector3<T>::operator-( const vector3<T>& aVector3 ) const
{
	return vector3<T>( x - aVector3.x, y - aVector3.y, z - aVector3.z );
}

template<class T>
inline vector3<T>& vector3<T>::operator*=( const T& aT )
{
	x *= aT;
	y *= aT;
	z *= aT;
	
	return *this;
}

template<class T>
inline vector3<T>& vector3<T>::operator/=( const T& aT )
{
	x /= aT;
	y /= aT;
	z /= aT;
	
	return *this;
}

template<class T>
inline vector3<T>& vector3<T>::operator+=( const vector3<T>& aVector3 )
{
	x += aVector3.x;
	y += aVector3.y;
	z += aVector3.z;
	
	return *this;
}

template<class T>
inline vector3<T>& vector3<T>::operator-=( const vector3<T>& aVector3 )
{
	x -= aVector3.x;
	y -= aVector3.y;
	z -= aVector3.z;
	
	return *this;	
}
	
template<class T>
inline T& vector3<T>::operator[]( uint32 aIndex )
{
	if( aIndex == 0 )
		return x;
	if( aIndex == 1 )
		return y;
	if( aIndex == 2 )	
		return z;
		
	; // REMOVED Error log for maya exporter

	// have to return something - Error::FATAL should stop execution.	
	return z;
}

template<class T>
inline vector3<T>::operator vector2<T>() const
{
	return Vector2<T>( x, y );
}

//~ template<class T>
//~ inline vector3<T>::operator String() const
//~ {
	//~ return String( "( " ) + String( x ) + ", " + String( y ) + ", " + String( z ) + String( " )" );
//~ }

template<class T>
inline vector3<T> operator*( const T& aT, const vector3<T>& aVector )
{
	return aVector * aT;
}

BW_END_NAMESPACE

