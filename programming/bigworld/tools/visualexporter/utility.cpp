
#pragma warning( disable : 4530 )

#include "utility.hpp"
#include "expsets.hpp"

BW_BEGIN_NAMESPACE

BW::string toLower( const BW::string &s )
{
	BW::string newString = s;

	for( uint32 i = 0; i < newString.length(); i++ )
	{
		if( newString[ i ] >= 'A' && newString[ i ] <= 'Z' )
		{
			newString[ i ] = newString[ i ] + 'a' - 'A';
		}
	}
	return newString;
}

BW::string trimWhitespaces(const BW::string& s)
{
	size_t f = s.find_first_not_of( ' ' );
	size_t l = s.find_last_not_of( ' ' );
	return s.substr( f, l - f + 1 );
}

BW::string unifySlashes(const BW::string& s)
{
	BW::string ret = s;
	size_t n;
	while ((n = ret.find_first_of( "\\" )) != BW::string::npos)
	{
		ret[n] = '/';
	}
	return ret;
}

Matrix3 rearrangeMatrix( const Matrix3& m )
{

	Matrix3 mm;

	mm.SetColumn( 0, m.GetColumn(0) );
	mm.SetColumn( 1, m.GetColumn(2) );
	mm.SetColumn( 2, m.GetColumn(1) );

	Point3 r2 = mm.GetRow( 2 );
	mm.SetRow( 2, mm.GetRow( 1 ) );
	mm.SetRow( 1, r2 );

	if (ExportSettings::instance().useLegacyOrientation())
	{
		mm.SetRow( 2, -mm.GetRow(2) );
		mm.SetRow( 0, -mm.GetRow(0) );
		mm.SetColumn( 2, -mm.GetColumn(2) );
		mm.SetColumn( 0, -mm.GetColumn(0) );
	}

	return mm;
}

bool isMirrored( const Matrix3 &m )
{
	return ( DotProd( CrossProd( m.GetRow( 0 ), m.GetRow( 1 ) ), m.GetRow( 2 ) ) < 0.0 );
}

Matrix3 normaliseMatrix( const Matrix3 &m )
{
	Matrix3 m2;
	Point3 p = m.GetRow( 0 );
	p = normalisePoint( p );
	m2.SetRow( 0, p );
	p = m.GetRow( 1 );
	p = normalisePoint( p );
	m2.SetRow( 1, p );
	p = m.GetRow( 2 );
	p = normalisePoint( p );
	m2.SetRow( 2, p );
	m2.SetRow( 3, m.GetRow( 3 ) );

	return m2;
}

Point3 normalisePoint( const Point3 &p )
{
	static const Point3 zero( 0.0, 0.0, 0.0 );
	if(p == zero)
		return zero;
	return Normalize( p );
}

bool trailingLeadingWhitespaces( const BW::string& s )
{
	bool ret = false;
	if (s.length())
		ret = (s[0] == ' ') || (s[s.length()-1] == ' ');
	return ret;
}
/*
double snapValue( double v, double snapSize )
{
	if ( snapSize > 0.f )
	{
		double halfSnap = snapSize / 2.f;

		if (v > 0.f)
		{
			v += halfSnap;

			v -= ( fmod( v, snapSize ) );
		}
		else
		{
			v -= halfSnap;

			v += ( fmod( -v, snapSize ) ); 
		}
	}

	return v;
}

Point3 snapPoint3( const Point3& pt, double snapSize )
{
	Point3 p = pt;

	p.x = snapValue( p.x, snapSize );
	p.y = snapValue( p.y, snapSize );
	p.z = snapValue( p.z, snapSize );

	return p;
}
*/

float snapValue( float v, float snapSize )
{
	if ( snapSize > 0.f )
	{
		float halfSnap = snapSize / 2.f;

		if (v > 0.f)
		{
			v += halfSnap;

			v -= ( fmodf( v, snapSize ) );
		}
		else
		{
			v -= halfSnap;

			v += ( fmodf( -v, snapSize ) ); 
		}
	}

	return v;
}

Point3 snapPoint3( const Point3& pt, float snapSize )
{
	Point3 p = pt;

	p.x = snapValue( p.x, snapSize );
	p.y = snapValue( p.y, snapSize );
	p.z = snapValue( p.z, snapSize );

	return p;
}

BW_END_NAMESPACE

