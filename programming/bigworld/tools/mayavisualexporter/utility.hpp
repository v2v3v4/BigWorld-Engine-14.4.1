#include <string>
#include "cstdmf/bw_vector.hpp"
#include "resmgr/datasection.hpp"
//~ #include "max.h"
#include <maya/MDistance.h>

#include "matrix3.hpp"
#include "math/vector3.hpp"

BW_BEGIN_NAMESPACE

matrix4<float> normaliseMatrix( const matrix4<float> &m );
bool isMirrored( const matrix3<float> &m );
BW::string toLower( const BW::string &s );
BW::string trimWhitespaces( const BW::string &s );
BW::string unifySlashes( const BW::string &s );
bool replaceEnvironmentMacros( const BW::string& in, BW::string* pOut );
matrix4<float> rearrangeMatrix( const matrix4<float>& m );
bool trailingLeadingWhitespaces( const BW::string&s );
float snapValue( float v, float snapSize );
Point3 snapPoint3( const Point3& pt, float snapSize = 0.001f );

Vector3 convertVectorToBigWorldUnits( const Vector3 &v  );
Vector3 convertPointToBigWorldUnits( const Vector3 &v  );
Vector3 convertAngleOffsetToBigWorldUnits( const Vector3 &v  );

template<class T> 
class ConditionalDeleteOnDestruct
{
public:
	ConditionalDeleteOnDestruct( T* object, bool del = true)
	: object_( object ),
	  delete_( del )
	{
	}
	ConditionalDeleteOnDestruct()
	: object_( NULL ),
	  delete_( true )
	{
	}
	~ConditionalDeleteOnDestruct()
	{
		if (object_&&delete_)
		{
			delete object_;
		}
	}
	void operator = ( T* object )
	{
		object_ = object;
	}
	T& operator *(){ return *object_;}
	T* operator ->(){ return object_;};

	bool del() const { return delete_; };
	void del( bool del ){ delete_ = del; };

private:
	T* object_;
	bool delete_;
};

BW_END_NAMESPACE

