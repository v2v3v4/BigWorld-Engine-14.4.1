#ifndef _INCLUDE_MATH_MATRIX4_HPP
#define _INCLUDE_MATH_MATRIX4_HPP

#include "vector3.hpp"

BW_BEGIN_NAMESPACE

//---------------------------------------------------------
// matrix4 implements a 3D matrix, with templated
// coordinates, to transform 3D coordinates. It is extended
// to a 4x4 matrix to allow translation as a matrix
// transform. When multiplying a vector3<T>, the w
// coordinate is assumed as 1.
//---------------------------------------------------------
template<class T>
class matrix4
{
public:
	//---------------------------------------------------------
	// Row provides the matrix4 class with access to its
	// elements through normal [] operations.
	//---------------------------------------------------------
	struct Row
	{
		inline Row( T& v1, T& v2, T& v3, T& v4 ) : v1( v1 ), v2( v2 ), v3( v3 ), v4( v4 ) {}
		
		T& v1;
		T& v2;
		T& v3;
		T& v4;
		
		inline T& operator[]( uint32 aIndex )
		{
			if( aIndex == 0 ) return v1;
			if( aIndex == 1 ) return v2;
			if( aIndex == 2 ) return v3;
			if( aIndex == 3 ) return v4;
				; // REMOVED for maya exporter: Error log.
			// have to return something - Error::FATAL halts execution
			return v4;
		}
	};

	// aNM has N as the row, and M as the column
	inline matrix4( const T& a11 = 1, const T& a12 = 0, const T& a13 = 0, const T& a14 = 0, 
					const T& a21 = 0, const T& a22 = 1, const T& a23 = 0, const T& a24 = 0,
					const T& a31 = 0, const T& a32 = 0, const T& a33 = 1, const T& a34 = 0,
					const T& a41 = 0, const T& a42 = 0, const T& a43 = 0, const T& a44 = 1 );
	// aN has N as the row, all else zero except v44 which is 1
	inline matrix4( const vector3<T>& a1, const vector3<T>& a2, const vector3<T>& a3 );
	// NOT virtual, so don't subclass matrix4
	~matrix4();

	// static special matrix4 constructors

	static inline matrix4<T> rotationX( const T& aAngle );
	static inline matrix4<T> rotationY( const T& aAngle );
	static inline matrix4<T> rotationZ( const T& aAngle );
	static inline matrix4<T> translation( const T& aTranslateX, const T& aTranslateY, const T& aTranslateZ );
	static inline matrix4<T> translation( const vector3<T>& aVector );
	static inline matrix4<T> scaling( const T& aScaleX, const T& aScaleY, const T& aScaleZ );	
	static inline matrix4<T> zero();	

	// column 1
	T v11;
	T v21;
	T v31;
	T v41;
	// column 2
	T v12;
	T v22;
	T v32;
	T v42;
	// column 3
	T v13;
	T v23;
	T v33;
	T v43;
	// column 4
	T v14;
	T v24;
	T v34;
	T v44;
	
	// resets this matrix to the identity matrix
	inline void identity();
		
	inline T determinant() const;

	inline matrix4<T> inverse() const;
	inline matrix4<T> transpose() const;
	
	inline vector3<T> translation() const;

	inline bool operator==( const matrix4<T>& aMatrix ) const;
	inline bool operator!=( const matrix4<T>& aMatrix ) const;
	
	inline matrix4<T> operator*( const matrix4<T>& aMatrix ) const;
	inline matrix4<T> operator*( const T& aScalar ) const;
	inline matrix4<T> operator/( const T& aScalar ) const;
	inline vector3<T> operator*( const vector3<T>& aVector ) const;
	inline matrix4<T> operator+( const matrix4<T>& aMatrix ) const;
	inline matrix4<T> operator-( const matrix4<T>& aMatrix ) const;
	
	// returns a Row struct containing references to elements in Row aIndex
	inline Row operator[]( uint32 aIndex );
};

BW_END_NAMESPACE

#include "matrix4.ipp"

#endif