#include "pch.hpp"

#pragma warning( disable : 4530 )

#include "utility.hpp"
#include "expsets.hpp"

#include <maya/MCommonSystemUtils.h>

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
	while ((n = ret.find_first_of( "\\" )) != BW::string::npos )
	{
		ret[n] = '/';
	}
	return ret;
}

// neilr: Replace "%MACRO_NAME%" macros with strings from the environment.
bool replaceEnvironmentMacros(const BW::string& in, BW::string* pOut)
{
	assert(pOut != NULL);

	BW::string macro;
	BW::string out;
	bool isSuccess = true;		// Start successful, only mark as a failure if we can't substitute a macro.
	bool readingMacro = false;
	for (BW::string::size_type idx = 0; idx < in.length(); ++idx)
	{
		BW::string::value_type value = in[idx];

		// Capture a macro value.
		if (value == '%')
		{
			readingMacro = !readingMacro;

			// If we've left a macro, look it up and append it to the output.
			if (!readingMacro)
			{
				// Ignoring status, it does not return if there is a missing env var.
				MString value = MCommonSystemUtils::getEnv(MString(macro.c_str()));
				
				// Only append if it's not empty. If it's empty (i.e. does not exist in maya.env), report as error.
				if (value.length() > 0)
				{
					out += value.asChar();
				}
				else
				{
					isSuccess = false;
					break;
				}
			}
		}
		// If we're reading a macro, 
		else if (readingMacro)
		{
			macro += value;
		}
		else
		{
			out += value;
		}
	}

	// If successful, output the new decoded string, otherwise keep the original text (useful for debugging the asset).
	if(isSuccess)
	{
		*pOut = out;
	}
	else
	{
		*pOut = in;
	}
	return isSuccess;
}

matrix4<float> rearrangeMatrix( const matrix4<float>& m )
{
	matrix4<float> mm = m;
	matrix4<float> mn = mm;
	
	return mn;
}

bool isMirrored( const matrix3<float> &m )
{
	vector3<float> row0( m.v11, m.v12, m.v13 );
	vector3<float> row1( m.v21, m.v22, m.v23 );
	vector3<float> row2( m.v31, m.v32, m.v33 );
	
	return row0.cross( row1 ).dot( row2 ) < 0;
}

matrix4<float> normaliseMatrix( const matrix4<float> &m )
{
	vector3<float> row0( m.v11, m.v12, m.v13 );
	vector3<float> row1( m.v21, m.v22, m.v23 );
	vector3<float> row2( m.v31, m.v32, m.v33 );

	row0.normalise();
	row1.normalise();
	row2.normalise();
	
	matrix4<float> m2( row0.x, row0.y, row0.z, m.v14, row1.x, row1.y, row1.z, m.v24, row2.x, row2.y, row2.z, m.v23, m.v41, m.v42, m.v43, m.v44 );

	return m2;
}

bool trailingLeadingWhitespaces( const BW::string& s )
{
	bool ret = false;
	if (s.length())
		ret = (s[0] == ' ') || (s[s.length()-1] == ' ');
	return ret;
}

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


/**
 * This helper function converts a maya point to a bigworld point by changing 
 * the coordinate system and scale.
 */
Vector3 convertPointToBigWorldUnits( const Vector3 &v  )
{
	float unitScale = ExportSettings::instance().unitScale();

	Vector3 scale;

	if (ExportSettings::instance().useLegacyOrientation())
	{
		scale.set( -unitScale, unitScale, unitScale );
	}
	else
	{
		scale.set( unitScale, unitScale, -unitScale );
	}

	return scale * v;
}

/**
 * This helper function converts a maya vector to a bigworld vector by changing 
 * the coordinate system.
 */
Vector3 convertVectorToBigWorldUnits( const Vector3 &v  )
{
	Vector3 scale;

	if (ExportSettings::instance().useLegacyOrientation())
	{
		scale.set( -1.f, 1.f, 1.f );
	}
	else
	{
		scale.set( 1.f, 1.f, -1.f );
	}

	return scale * v;
}

/**
 *	This helper function converts a Maya vector of angle offsets (which
 *	are in radians) to those that can be used in the Bigworld coordinate
 *	system.
 */
Vector3 convertAngleOffsetToBigWorldUnits( const Vector3 &v  )
{
	Vector3 scale;

	if (ExportSettings::instance().useLegacyOrientation())
	{
		scale.set( 1.f, -1.f, -1.f );
	}
	else
	{
		scale.set( -1.f, -1.f, 1.f );
	}

	return scale * v;
}

BW_END_NAMESPACE

