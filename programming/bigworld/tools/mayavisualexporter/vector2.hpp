#ifndef _INCLUDE_MATH_VECTOR2_HPP
#define _INCLUDE_MATH_VECTOR2_HPP

BW_BEGIN_NAMESPACE

//---------------------------------------------------------
// vector2 implements a 2D vector, with templated coordinates,
// allowing for int32, real32, etc.
//---------------------------------------------------------
template<class T>
class vector2
{
public:
	inline vector2( const T& aX = 0, const T& aY = 0 );
	
	// NOT virtual, so don't subclass Float
	~vector2();

	T x;
	T y;

	// basis vectors
	static inline vector2<T> X();
	static inline vector2<T> Y();

	inline vector2<T>& normalise();
	inline T magnitude() const;
	inline T magnitudeSquared() const;
	inline T dot( const vector2<T>& aVector2 ) const;

	inline bool operator==( const vector2<T>& aVector2 ) const;
	inline bool operator!=( const vector2<T>& aVector2 ) const;

	inline vector2<T> operator-();
	
	inline vector2<T> operator*( const T& aT ) const;
	inline vector2<T> operator/( const T& aT ) const;
	inline vector2<T> operator+( const vector2<T>& aVector2 ) const;
	inline vector2<T> operator-( const vector2<T>& aVector2 ) const;

	inline vector2<T>& operator*=( const T& aT );
	inline vector2<T>& operator/=( const T& aT );
	inline vector2<T>& operator+=( const vector2<T>& aVector2 );
	inline vector2<T>& operator-=( const vector2<T>& aVector2 );
	
	inline T& operator[]( uint32 aIndex );

};

template<class T>
inline vector2<T> operator*( const T& aT, const vector2<T>& aVector );

BW_END_NAMESPACE

#include "vector2.ipp"

#endif