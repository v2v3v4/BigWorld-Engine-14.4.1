#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"
#include "resmgr/datasection.hpp"
#include "max.h"

BW_BEGIN_NAMESPACE

Matrix3 normaliseMatrix( const Matrix3 &m );
bool isMirrored( const Matrix3 &m );
BW::string toLower( const BW::string &s );
BW::string trimWhitespaces( const BW::string &s );
BW::string unifySlashes( const BW::string &s );
Matrix3 rearrangeMatrix( const Matrix3& m );
bool trailingLeadingWhitespaces( const BW::string&s );

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

