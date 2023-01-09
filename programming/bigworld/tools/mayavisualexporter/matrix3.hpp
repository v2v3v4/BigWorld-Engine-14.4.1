#ifndef _INCLUDE_MATH_MATRIX3_HPP
#define _INCLUDE_MATH_MATRIX3_HPP

#include "vector2.hpp"
#include "vector3.hpp"

BW_BEGIN_NAMESPACE

//---------------------------------------------------------
// matrix3 implements a 2D matrix, with templated
// coordinates, to transform 2D coordinates. It is extended
// to a 3x3 matrix to allow translation as a matrix
// transform. When multiplying a vector2<T>, the z
// coordinate is assumed as 1.
//---------------------------------------------------------
template<class T>
class matrix3
{
public:
	//---------------------------------------------------------
	// Row provides the matrix3 class with access to its
	// elements through normal [] operations.
	//---------------------------------------------------------
	struct Row
	{
		inline Row( T& v1, T& v2, T& v3 ) : v1( v1 ), v2( v2 ), v3( v3 ) {}
		
		T& v1;
		T& v2;
		T& v3;
		
		inline T& operator[]( uint32 aIndex )
		{
			if( aIndex == 0 ) return v1;
			if( aIndex == 1 ) return v2;
			if( aIndex == 2 ) return v3;
			; // REMOVED Error log for maya exporter
			// have to return something - Error::FATAL halts execution
			return v3;
		}
	};
	
	// aNM has N as the row, and M as the column
	inline matrix3( const T& a11 = 1, const T& a12 = 0, const T& a13 = 0, 
					const T& a21 = 0, const T& a22 = 1, const T& a23 = 0,
					const T& a31 = 0, const T& a32 = 0, const T& a33 = 1 );
	// aN has N as the row
	inline matrix3( const vector3<T>& a1, const vector3<T>& a2, const vector3<T>& a3 );
	// aN has N as the row, all else zero except v33 which is 1
	inline matrix3( const vector2<T>& a1, const vector2<T>& a2 );
	// NOT virtual, so don't subclass matrix3
	~matrix3();

	// static special matrix3 constructors (2D only)

	static inline matrix3<T> rotation( const T& aAngle );
	static inline matrix3<T> translation( const T& aTranslateX, const T& aTranslateY );
	static inline matrix3<T> scaling( const T& aScaleX, const T& aScaleY );
	

	// column 1
	T v11;
	T v21;
	T v31;
	// column 2
	T v12;
	T v22;
	T v32;
	// column 3
	T v13;
	T v23;
	T v33;
	
	// resets this matrix to the identity matrix
	inline void identity();
		
	inline T determinant() const;

	inline matrix3<T> inverse() const;
	inline matrix3<T> transpose() const;
	
	inline bool operator==( const matrix3<T>& aMatrix ) const;
	inline bool operator!=( const matrix3<T>& aMatrix ) const;
	
	inline matrix3<T> operator*( const matrix3<T>& aMatrix ) const;
	inline matrix3<T> operator*( const T& aScalar ) const;
	inline matrix3<T> operator/( const T& aScalar ) const;
	inline vector2<T> operator*( const vector2<T>& aVector ) const;
	inline vector3<T> operator*( const vector3<T>& aVector ) const;
	
	// returns a Row struct containing references to elements in Row aIndex
	inline Row operator[]( uint32 aIndex );
};

template<class T>
inline matrix3<T> operator*( const T& aScalar, const matrix3<T>& aMatrix );

BW_END_NAMESPACE

#include "matrix3.ipp"

#endif