#ifndef _INCLUDE_MATH_VECTOR3_HPP
#define _INCLUDE_MATH_VECTOR3_HPP

#include "vector2.hpp"

BW_BEGIN_NAMESPACE

//---------------------------------------------------------
// vector3 implements a 3D vector, with templated coordinates,
// allowing for int32, real32, etc.
//---------------------------------------------------------
template<class T>
class vector3
{
public:
	inline vector3( const T& aX = 0, const T& aY = 0, const T& aZ = 0 );
	inline vector3( const vector2<T>& aVector2, const T& aZ = 0 );
	
	// NOT virtual, so don't subclass Float
	~vector3();

	T x;
	T y;
	T z;

	// basis vectors
	static inline vector3<T> X();
	static inline vector3<T> Y();
	static inline vector3<T> Z();

	inline vector3<T>& normalise();
	inline T magnitude() const;
	inline T magnitudeSquared() const;
	inline vector3<T> cross( const vector3<T>& aVector3 ) const;
	inline T dot( const vector3<T>& aVector3 ) const;

	inline bool operator==( const vector3<T>& aVector3 ) const;
	inline bool operator!=( const vector3<T>& aVector3 ) const;

	inline vector3<T> operator-();
	
	inline vector3<T> operator*( const T& aT ) const;
	inline vector3<T> operator/( const T& aT ) const;
	inline vector3<T> operator+( const vector3<T>& aVector3 ) const;
	inline vector3<T> operator-( const vector3<T>& aVector3 ) const;

	inline vector3<T>& operator*=( const T& aT );
	inline vector3<T>& operator/=( const T& aT );
	inline vector3<T>& operator+=( const vector3<T>& aVector3 );
	inline vector3<T>& operator-=( const vector3<T>& aVector3 );
	
	inline T& operator[]( uint32 aIndex );
	
	// discards z coordinate
	inline operator vector2<T>() const;

};

template<class T>
inline vector3<T> operator*( const T& aT, const vector3<T>& aVector );

BW_END_NAMESPACE

#include "vector3.ipp"

#endif