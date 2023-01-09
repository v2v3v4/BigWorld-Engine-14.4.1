BW_BEGIN_NAMESPACE

template<class T>
inline vector2<T>::vector2( const T& aX, const T& aY ) :
	x( aX ), y( aY )
{
}

template<class T>
vector2<T>::~vector2()
{
}

template<class T>
inline vector2<T> vector2<T>::X()
{
	return vector2<T>( 1, 0 );
}

template<class T>
inline vector2<T> vector2<T>::Y()
{
	return vector2<T>( 0, 1 );
}

template<class T>
inline vector2<T>& vector2<T>::normalise()
{
	T invmag = 1 / magnitude();
	
	x = x  * invmag;
	y = y * invmag;
	
	return *this;
}

template<class T>
inline T vector2<T>::magnitude() const
{
	return sqrt( x * x + y * y );
}

template<class T>
inline T vector2<T>::magnitudeSquared() const
{
	return x * x + y * y;
}

template<class T>
inline T vector2<T>::dot( const vector2<T>& aVector2 ) const
{
	return x * aVector2.x + y * aVector2.y;
}

template<class T>
inline bool vector2<T>::operator==( const vector2<T>& aVector2 ) const
{
	return x == aVector2.x && y == aVector2.y;
}

template<class T>
inline bool vector2<T>::operator!=( const vector2<T>& aVector2 ) const
{
	return x != aVector2.x || y != aVector2.y;
}

template<class T>
inline vector2<T> vector2<T>::operator-()
{
	return vector2<T>( -x, -y );
}
	
template<class T>
inline vector2<T> vector2<T>::operator*( const T& aT ) const
{
	return vector2<T>( x * aT, y * aT );
}

template<class T>
inline vector2<T> vector2<T>::operator/( const T& aT ) const
{
	return vector2<T>( x / aT, y / aT );
}

template<class T>
inline vector2<T> vector2<T>::operator+( const vector2<T>& aVector2 ) const
{
	return vector2<T>( x + aVector2.x, y + aVector2.y );
}

template<class T>
inline vector2<T> vector2<T>::operator-( const vector2<T>& aVector2 ) const
{
	return vector2<T>( x - aVector2.x, y - aVector2.y );
}

template<class T>
inline vector2<T>& vector2<T>::operator*=( const T& aT )
{
	x *= aT;
	y *= aT;
	
	return *this;
}

template<class T>
inline vector2<T>& vector2<T>::operator/=( const T& aT )
{
	x /= aT;
	y /= aT;
	
	return *this;
}

template<class T>
inline vector2<T>& vector2<T>::operator+=( const vector2<T>& aVector2 )
{
	x += aVector2.x;
	y += aVector2.y;
	
	return *this;
}

template<class T>
inline vector2<T>& vector2<T>::operator-=( const vector2<T>& aVector2 )
{
	x -= aVector2.x;
	y -= aVector2.y;
	
	return *this;	
}
	
template<class T>
inline T& vector2<T>::operator[]( uint32 aIndex )
{
	if( aIndex == 0 )
		return x;
	if( aIndex == 1 )	
		return y;

	; // REMOVED Error log fro maya exporter
	
	// have to return something - Error::FATAL should stop execution.
	return y;
}

//template<class T>
//inline vector2<T>::operator String() const
//{
//	return String( "( " ) + String( x ) + ", " + String( y ) + String( " )" );
//}

template<class T>
inline vector2<T> operator*( const T& aT, const vector2<T>& aVector )
{
	return aVector * aT;
}

BW_END_NAMESPACE

