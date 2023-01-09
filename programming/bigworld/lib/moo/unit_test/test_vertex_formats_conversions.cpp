#include "pch.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "moo/vertex_declaration.hpp"
#include "moo/vertex_format.hpp"
#include "moo/vertex_format_cache.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/vertex_format_conversions.hpp"


BW_USE_NAMESPACE
using namespace Moo;

typedef VertexElement::SemanticType SemType;
typedef VertexElement::StorageType StorType;

//------------------------------------------------------------------------------
// Helper functions for allowing the vertex format xmls to be read

const char BIGWORLD_RES_RESOURCE_PATH[] = "../../../res/bigworld/";

void addResourcePathIfNotAdded( const BW::StringRef & path )
{
	BW::MultiFileSystem * pmfs = BW::BWResource::instance().fileSystem();
	if (pmfs)
	{
		BW::string abspath = pmfs->getAbsolutePath( path );
		BW::StringRef abspathref(abspath);
		int numPaths = BW::BWResource::getPathNum();
		for ( int i = 0; i < numPaths; ++i)
		{
			if (abspathref.equals_lower(BW::BWResource::getPath(i)))
			{
				return;
			}
		}
	}

	// resource path not found, add it
	BW::BWResource::addPath( path );
}

/// helper func for constructing normalised vectors
Vector3 normalisedVec3(float x, float y, float z)
{
	Vector3 n(x, y, z);
	n.normalise();
	return n;
}

/// helper struct for storing VF lookup results
struct VertexFormatTuple
{
	const VertexFormat * sourceFormat_;
	const VertexFormat * targetFormat_;
	const VertexFormat * expectedTargetFormat_;

	VertexFormatTuple()
	: sourceFormat_(NULL)
	, targetFormat_(NULL)
	, expectedTargetFormat_(NULL)
	{
	}

	bool isValid() const
	{
		return (sourceFormat_ && targetFormat_);
	}

	bool isTargetFound() const
	{
		// check target exists
		// if expected has been assigned, check same as target
		return (targetFormat_ && 
			(!expectedTargetFormat_ || targetFormat_ == expectedTargetFormat_));
	}

	operator bool() const
	{
		return isValid() && isTargetFound();
	}
};

/// helper function for querying vertex formats 
template <class SourceVertexType, class TargetVertexType>
VertexFormatTuple getFormats()
{
	addResourcePathIfNotAdded( BIGWORLD_RES_RESOURCE_PATH );

	const VertexFormat * sourceFormat = VertexFormatCache::get<SourceVertexType>();
	MF_ASSERT( sourceFormat );

	// we want to get a null pointer back if target could not be found
	const bool fallbackToSource = false;

	const BW::StringRef targetName = VertexDeclaration::getTargetDevice();

	const VertexFormat * targetFormat = 
		VertexFormatCache::getTarget<SourceVertexType>( targetName, fallbackToSource );

	const VertexFormat * expectedTargetFormat = VertexFormatCache::get<TargetVertexType>();

	MF_ASSERT( targetFormat );
	MF_ASSERT( expectedTargetFormat );
	MF_ASSERT( *targetFormat == *expectedTargetFormat );

	VertexFormatTuple vfTup;
	vfTup.sourceFormat_ = sourceFormat;
	vfTup.targetFormat_ = targetFormat;
	vfTup.expectedTargetFormat_ = expectedTargetFormat;
	return vfTup;
}

/// helper function for verifying size match
template <class SourceVertexType, class TargetVertexType>
bool checkSizeMatch( const VertexFormatTuple & vfTup, uint32 streamIndex=0 )
{
	if (!vfTup)
	{
		return false;
	}

	uint32 srcSizeExpected = sizeof( SourceVertexType );
	uint32 dstSizeExpected = sizeof( TargetVertexType );

	uint32 srcSize = vfTup.sourceFormat_->streamStride( streamIndex );
	uint32 dstSize = vfTup.targetFormat_->streamStride( streamIndex );

	if (srcSize != srcSizeExpected)
	{
		return false;
	}
	if (dstSize != dstSizeExpected)
	{
		return false;
	}

	return true;
}

// since we cant set a breakpoint in a macro,
// do this so that we can set a breakpoint for failures.
template <typename T>
bool vfc_test_isEqual( T a, T b )
{
	if (a != b)
	{
		return false;	// set breakpoint here.
	}
	return true;
}

#define VERTEX_MEMBER_COMPARE( memberName )			\
if (!vfc_test_isEqual( a.memberName, b.memberName ))	\
{													\
	return false;									\
}													\


//------------------------------------------------------------------------------
// VertexXYZNUVPC
/*
struct VertexXYZNUVPC
{
Vector3				pos_;
GPUNormalVector3	normal_;
GPUTexcoordVector2	uv_;
}
*/

bool equals_VertexXYZNUVPC( const VertexXYZNUVPC & a, const VertexXYZNUVPC & b )
{
	VERTEX_MEMBER_COMPARE( pos_ )
	VERTEX_MEMBER_COMPARE( normal_ )
	VERTEX_MEMBER_COMPARE( uv_ )

	return true;
}


// VertexXYZNUV -> VertexXYZNUVPC 
TEST( Moo_VertexFormats_conversions_VertexXYZNUV_VertexXYZNUVPC )
{
	typedef VertexXYZNUV SourceVertexType;
	typedef VertexXYZNUVPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = normalisedVec3(-1, 2, 3);
	vin1.uv_ = Vector2(1.239f, -3.73456f);

	// expected output
	TargetVertexType vout1 = vin1;

	// check target vertex format is the expected one
	VertexFormatTuple vfTup = getFormats<SourceVertexType, TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVPC( vout1, vconv1) );
	CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}

//------------------------------------------------------------------------------
// VertexXYZNUV2PC
/*
struct VertexXYZNUV2PC
{
Vector3				pos_;
GPUNormalVector3	normal_;
GPUTexcoordVector2	uv_;
GPUTexcoordVector2	uv2_;
}
*/

bool equals_VertexXYZNUV2PC( const VertexXYZNUV2PC & a, const VertexXYZNUV2PC & b )
{
	VERTEX_MEMBER_COMPARE( pos_ )
	VERTEX_MEMBER_COMPARE( normal_ )
	VERTEX_MEMBER_COMPARE( uv_ )
	VERTEX_MEMBER_COMPARE( uv2_ )

	return true;
}


// VertexXYZNUV2 -> VertexXYZNUV2PC 
TEST( Moo_VertexFormats_conversions_VertexXYZNUV2_VertexXYZNUV2PC )
{
	typedef VertexXYZNUV2 SourceVertexType;
	typedef VertexXYZNUV2PC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = normalisedVec3(-1, 2, 3);
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.uv2_ = Vector2(1.239f, -3.73456f);

	// expected output
	TargetVertexType vout1 = vin1;

	// check target vertex format is the expected one
	VertexFormatTuple vfTup = getFormats<SourceVertexType, TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUV2PC( vout1, vconv1) );
	CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );



}


//------------------------------------------------------------------------------
// VertexXYZNUVIPC
/*
struct VertexXYZNUVIPC
{
Vector3				pos_;
GPUNormalVector3	normal_;
GPUTexcoordVector2	uv_;
float				index_;
}
*/

bool equals_VertexXYZNUVIPC( const VertexXYZNUVIPC & a, const VertexXYZNUVIPC & b )
{
	VERTEX_MEMBER_COMPARE( pos_ )
	VERTEX_MEMBER_COMPARE( normal_ )
	VERTEX_MEMBER_COMPARE( uv_ )
	VERTEX_MEMBER_COMPARE( index_ )

	return true;
}


// VertexXYZNUVI -> VertexXYZNUVIPC 
TEST( Moo_VertexFormats_conversions_VertexXYZNUVI_VertexXYZNUVIPC )
{
	typedef VertexXYZNUVI SourceVertexType;
	typedef VertexXYZNUVIPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = normalisedVec3(-1, 2, 3);
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index_ = 5.0f;

	// expected output
	TargetVertexType vout1 = vin1;

	// check target vertex format is the expected one
	VertexFormatTuple vfTup = getFormats<SourceVertexType, TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVIPC( vout1, vconv1) );
	CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}


//------------------------------------------------------------------------------
// VertexXYZNUVTBPC
/*
struct VertexXYZNUVTBPC
{
Vector3				pos_;
GPUNormalVector3	normal_;
GPUTexcoordVector2	uv_;
GPUNormalVector3	tangent_;
GPUNormalVector3	binormal_;
}
*/

bool equals_VertexXYZNUVTBPC( const VertexXYZNUVTBPC & a, const VertexXYZNUVTBPC & b )
{
	VERTEX_MEMBER_COMPARE( pos_ )
	VERTEX_MEMBER_COMPARE( normal_ )
	VERTEX_MEMBER_COMPARE( uv_ )
	VERTEX_MEMBER_COMPARE( tangent_ )
	VERTEX_MEMBER_COMPARE( binormal_ )

	return true;
}


// VertexXYZNUVTB -> VertexXYZNUVTBPC 
TEST( Moo_VertexFormats_conversions_VertexXYZNUVTB_VertexXYZNUVTBPC )
{
	typedef VertexXYZNUVTB SourceVertexType;
	typedef VertexXYZNUVTBPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = 207689245;
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.tangent_ = 126786234;
	vin1.binormal_ = 195877892;

	// expected output
	TargetVertexType vout1 = vin1;
	TargetVertexType vout2;
	vout2.pos_ = vin1.pos_;
	vout2.normal_ = GPUNormalVector3( unpackNormal( vin1.normal_ ) );
	vout2.tangent_ = GPUNormalVector3( unpackNormal( vin1.tangent_ ) );
	vout2.binormal_ = GPUNormalVector3( unpackNormal( vin1.binormal_ ) );
	vout2.uv_ = GPUTexcoordVector2( vin1.uv_ );
	CHECK( equals_VertexXYZNUVTBPC( vout1, vout2 ) );

	// check target vertex format is the expected one
	VertexFormatTuple vfTup = getFormats<SourceVertexType, TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVTBPC( vout1, vconv1) );
	CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}


// VertexXYZNUV -> VertexXYZNUVTBPC 
// tangent_.setZero();
// binormal_.setZero();
TEST( Moo_VertexFormats_conversions_VertexXYZNUV_VertexXYZNUVTBPC  )
{
	typedef VertexXYZNUV SourceVertexType;
	typedef VertexXYZNUVTBPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = normalisedVec3(-1, 2, 3);
	vin1.uv_ = Vector2(1.239f, -3.73456f);

	// expected output
	TargetVertexType vout1 = vin1;

	// get vertex formats
	VertexFormatTuple vfTup;
	vfTup.sourceFormat_ = VertexFormatCache::get<SourceVertexType>();
	vfTup.targetFormat_ = VertexFormatCache::get<TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVTBPC( vout1, vconv1) );
	CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}


//------------------------------------------------------------------------------
// VertexXYZNUV2TBPC
/*
struct VertexXYZNUV2TBPC
{
Vector3				pos_;
GPUNormalVector3	normal_;
GPUTexcoordVector2	uv_;
GPUTexcoordVector2	uv2_;
GPUNormalVector3	tangent_;
GPUNormalVector3	binormal_;
}
*/

bool equals_VertexXYZNUV2TBPC( const VertexXYZNUV2TBPC & a, const VertexXYZNUV2TBPC & b )
{
	VERTEX_MEMBER_COMPARE( pos_ )
	VERTEX_MEMBER_COMPARE( normal_ )
	VERTEX_MEMBER_COMPARE( uv_ )
	VERTEX_MEMBER_COMPARE( uv2_ )
	VERTEX_MEMBER_COMPARE( tangent_ )
	VERTEX_MEMBER_COMPARE( binormal_ )

	return true;
}


// VertexXYZNUV2TB -> VertexXYZNUV2TBPC 
TEST( Moo_VertexFormats_conversions_VertexXYZNUV2TB_VertexXYZNUV2TBPC )
{
	typedef VertexXYZNUV2TB SourceVertexType;
	typedef VertexXYZNUV2TBPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = 207689245;
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.uv2_ = Vector2(1.239f, -3.73456f);	// same on purpose for comparison
	vin1.tangent_ = 126786234;
	vin1.binormal_ = 195877892;

	// expected output
	TargetVertexType vout1 = vin1;

	// check target vertex format is the expected one
	VertexFormatTuple vfTup = getFormats<SourceVertexType, TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUV2TBPC( vout1, vconv1) );
	CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );

	// check that both uv semantics got converted the same
	CHECK( vout1.uv_ == vout1.uv2_ );
	Vector2 vout1_uv = vout1.uv_;
	CHECK_CLOSE( vout1_uv.x, vin1.uv2_.x, 0.001f );
	CHECK_CLOSE( vout1_uv.y, vin1.uv2_.y, 0.001f );
}



// VertexXYZNUV2 -> VertexXYZNUV2TBPC
// tangent_.setZero();
// binormal_.setZero();
TEST( Moo_VertexFormats_conversions_VertexXYZNUV2_VertexXYZNUVTBPC  )
{
	typedef VertexXYZNUV2 SourceVertexType;
	typedef VertexXYZNUV2TBPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = Vector3(1.8903476f, -0.34768f, -0.194f);
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.uv2_ = Vector2(2.39f, -3.3456f);

	// expected output
	TargetVertexType vout1 = vin1;

	// get vertex formats
	VertexFormatTuple vfTup;
	vfTup.sourceFormat_ = VertexFormatCache::get<SourceVertexType>();
	vfTup.targetFormat_ = VertexFormatCache::get<TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUV2TBPC( vout1, vconv1) );
	CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}



//------------------------------------------------------------------------------
// VertexXYZNUVITBPC
/*
struct VertexXYZNUVITBPC
{
Vector3				pos_;
GPUNormalVector3	normal_;
GPUTexcoordVector2	uv_;
float				index_;
GPUNormalVector3	tangent_;
GPUNormalVector3	binormal_;
}
*/

bool equals_VertexXYZNUVITBPC( const VertexXYZNUVITBPC & a, const VertexXYZNUVITBPC & b )
{
	VERTEX_MEMBER_COMPARE( pos_ )
	VERTEX_MEMBER_COMPARE( normal_ )
	VERTEX_MEMBER_COMPARE( uv_ )
	VERTEX_MEMBER_COMPARE( index_ )
	VERTEX_MEMBER_COMPARE( tangent_ )
	VERTEX_MEMBER_COMPARE( binormal_ )

	return true;
}


// VertexXYZNUVITB -> VertexXYZNUVITBPC
TEST( Moo_VertexFormats_conversions_VertexXYZNUVITB_VertexXYZNUVITBPC )
{
	typedef VertexXYZNUVITB SourceVertexType;
	typedef VertexXYZNUVITBPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = 207689245;
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index_ = 98.0f;
	vin1.tangent_ = 126786234;
	vin1.binormal_ = 195877892;

	// expected output
	TargetVertexType vout1 = vin1;

	// check target vertex format is the expected one
	VertexFormatTuple vfTup = getFormats<SourceVertexType, TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVITBPC( vout1, vconv1) );
	CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}


//------------------------------------------------------------------------------
// VertexXYZNUVIIIWW
/*
struct VertexXYZNUVIIIWW
{
Vector3		pos_;
uint32		normal_;
Vector2		uv_;
uint8		index_;
uint8		index2_;
uint8		index3_;
uint8		weight_;
uint8		weight2_;
}
*/

bool equals_VertexXYZNUVIIIWW( const VertexXYZNUVIIIWW & a, const VertexXYZNUVIIIWW & b )
{
	VERTEX_MEMBER_COMPARE( pos_ )
	VERTEX_MEMBER_COMPARE( normal_ )
	VERTEX_MEMBER_COMPARE( uv_ )
	VERTEX_MEMBER_COMPARE( index_ )
	VERTEX_MEMBER_COMPARE( index2_ )
	VERTEX_MEMBER_COMPARE( index3_ )
	VERTEX_MEMBER_COMPARE( weight_ )
	VERTEX_MEMBER_COMPARE( weight2_ )

	return true;
}


// VertexXYZNUVI -> VertexXYZNUVIIIWW
// weight_ = 255;
// weight2_ = 0;
// index_ = (uint8)in.index_;
// index2_ = index_;
// index3_ = index_;
TEST( Moo_VertexFormats_conversions_VertexXYZNUVI_VertexXYZNUVIIIWW )
{
	typedef VertexXYZNUVI SourceVertexType;
	typedef VertexXYZNUVIIIWW TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = Vector3(1.8903476f, -0.34768f, -0.194f);
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index_ = 3.0f;

	// expected output
	TargetVertexType vout1;
	vout1.operator=(vin1);

	// get vertex formats
	VertexFormatTuple vfTup;
	vfTup.sourceFormat_ = VertexFormatCache::get<SourceVertexType>();
	vfTup.targetFormat_ = VertexFormatCache::get<TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVIIIWW( vout1, vconv1) );
	CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}


// VertexXYZNUVIIIWW_v2 -> VertexXYZNUVIIIWW
// index_ = in.index_ * 3;
// index2_ = in.index2_ * 3;
// index3_ = in.index3_ * 3;
TEST( Moo_VertexFormats_conversions_VertexXYZNUVIIIWW_v2_VertexXYZNUVIIIWW )
{
	typedef VertexXYZNUVIIIWW_v2 SourceVertexType;
	typedef VertexXYZNUVIIIWW TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = 207689245;
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index3_ = 5;
	vin1.index2_ = 18;
	vin1.index_ = 98;
	vin1.weight2_ = 3;
	vin1.weight_ = 9;

	// expected output
	TargetVertexType vout1;
	vout1.operator=(vin1);

	// get vertex formats
	VertexFormatTuple vfTup;
	vfTup.sourceFormat_ = VertexFormatCache::get<SourceVertexType>();
	vfTup.targetFormat_ = VertexFormatCache::get<TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVIIIWW( vout1, vconv1) );
	CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}


//------------------------------------------------------------------------------
// VertexXYZNUV2IIIWW
/*
struct VertexXYZNUV2IIIWW
{
Vector3		pos_;
uint32		normal_;
Vector2		uv_;
Vector2		uv2_;
uint8		index_;
uint8		index2_;
uint8		index3_;
uint8		weight_;
uint8		weight2_;
}
*/

bool equals_VertexXYZNUV2IIIWW( const VertexXYZNUV2IIIWW & a, const VertexXYZNUV2IIIWW & b, bool checkUV2 )
{
	VERTEX_MEMBER_COMPARE( pos_ )
	VERTEX_MEMBER_COMPARE( normal_ )
	VERTEX_MEMBER_COMPARE( uv_ )
	
	if (checkUV2)
	{
		VERTEX_MEMBER_COMPARE( uv2_ )
	}

	VERTEX_MEMBER_COMPARE( index_ )
	VERTEX_MEMBER_COMPARE( index2_ )
	VERTEX_MEMBER_COMPARE( index3_ )
	VERTEX_MEMBER_COMPARE( weight_ )
	VERTEX_MEMBER_COMPARE( weight2_ )

	return true;
}

// VertexXYZNUVI -> VertexXYZNUV2IIIWW
// weight_ = 255;
// weight2_ = 0;
// index_ = (uint8)in.index_;
// index2_ = index_;
// index3_ = index_;
TEST( Moo_VertexFormats_conversions_VertexXYZNUVI_VertexXYZNUV2IIIWW )
{
	typedef VertexXYZNUVI SourceVertexType;
	typedef VertexXYZNUV2IIIWW TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = Vector3(1.8903476f, -0.34768f, -0.194f);
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index_ = 3.0f;

	// expected output
	TargetVertexType vout1;
	vout1.operator=(vin1);

	// get vertex formats
	VertexFormatTuple vfTup;
	vfTup.sourceFormat_ = VertexFormatCache::get<SourceVertexType>();
	vfTup.targetFormat_ = VertexFormatCache::get<TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	// We do not compare uv2_, since it is uninitialized memory
	// Therefore, we skip the memcmp here also.
	CHECK( equals_VertexXYZNUV2IIIWW( vout1, vconv1, false ) );
	//CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}



//------------------------------------------------------------------------------
// VertexXYZNUVIIIWWTB
/*
struct VertexXYZNUVIIIWWTB
{
Vector3		pos_;
uint32		normal_;
Vector2		uv_;
uint8		index_;
uint8		index2_;
uint8		index3_;
uint8		weight_;
uint8		weight2_;
uint32		tangent_;
uint32		binormal_;
}
*/
bool equals_VertexXYZNUVIIIWWTB( const VertexXYZNUVIIIWWTB & a, const VertexXYZNUVIIIWWTB & b )
{
	VERTEX_MEMBER_COMPARE( pos_ )
	VERTEX_MEMBER_COMPARE( normal_ )
	VERTEX_MEMBER_COMPARE( uv_ )
	VERTEX_MEMBER_COMPARE( index_ )
	VERTEX_MEMBER_COMPARE( index2_ )
	VERTEX_MEMBER_COMPARE( index3_ )
	VERTEX_MEMBER_COMPARE( weight_ )
	VERTEX_MEMBER_COMPARE( weight2_ )
	VERTEX_MEMBER_COMPARE( tangent_ )
	VERTEX_MEMBER_COMPARE( binormal_ )

	return true;
}

// VertexXYZNUVITB -> VertexXYZNUVIIIWWTB
// weight_ = 255;
// weight2_ = 0;
// index_ = (uint8)in.index_;
// index2_ = index_;
// index3_ = index_;
TEST( Moo_VertexFormats_conversions_VertexXYZNUVITB_VertexXYZNUVIIIWWTB )
{
	typedef VertexXYZNUVITB SourceVertexType;
	typedef VertexXYZNUVIIIWWTB TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = 207689245;
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index_ = 3.0f;
	vin1.tangent_ = 126786234;
	vin1.binormal_ = 195877892;

	// expected output
	TargetVertexType vout1;
	vout1.operator=(vin1);

	// get vertex formats
	VertexFormatTuple vfTup;
	vfTup.sourceFormat_ = VertexFormatCache::get<SourceVertexType>();
	vfTup.targetFormat_ = VertexFormatCache::get<TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVIIIWWTB( vout1, vconv1) );
	CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}



// VertexXYZNUVIIIWWTB_v2 -> VertexXYZNUVIIIWWTB
// index_ = in.index_ * 3;
// index2_ = in.index2_ * 3;
// index3_ = in.index3_ * 3;
TEST( Moo_VertexFormats_conversions_VertexXYZNUVIIIWWTB_v2_VertexXYZNUVIIIWWTB )
{
	typedef VertexXYZNUVIIIWWTB_v2 SourceVertexType;
	typedef VertexXYZNUVIIIWWTB TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = 207689245;
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index3_ = 5;
	vin1.index2_ = 18;
	vin1.index_ = 98;
	vin1.weight2_ = 3;
	vin1.weight_ = 9;
	vin1.tangent_ = 126786234;
	vin1.binormal_ = 195877892;

	// expected output
	TargetVertexType vout1;
	vout1.operator=(vin1);

	// get vertex formats
	VertexFormatTuple vfTup;
	vfTup.sourceFormat_ = VertexFormatCache::get<SourceVertexType>();
	vfTup.targetFormat_ = VertexFormatCache::get<TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVIIIWWTB( vout1, vconv1) );
	CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}

//------------------------------------------------------------------------------
// VertexXYZNUVIIIWWPC
/*
struct VertexXYZNUVIIIWWPC
{
Vector3				pos_;
GPUNormalVector3	normal_;
GPUTexcoordVector2	uv_;
uint8		index3_;
uint8		index2_;
uint8		index_;
uint8		pad_;
uint8		pad2_;
uint8		weight2_;
uint8		weight_;
uint8		pad3_;
}
*/

bool equals_VertexXYZNUVIIIWWPC( const VertexXYZNUVIIIWWPC & a, const VertexXYZNUVIIIWWPC & b, bool comparePadding )
{
	VERTEX_MEMBER_COMPARE( pos_ )
	VERTEX_MEMBER_COMPARE( normal_ )
	VERTEX_MEMBER_COMPARE( uv_ )
	VERTEX_MEMBER_COMPARE( index3_ )
	VERTEX_MEMBER_COMPARE( index2_ )
	VERTEX_MEMBER_COMPARE( index_ )

	if (comparePadding)
	{
		VERTEX_MEMBER_COMPARE( pad_ )
		VERTEX_MEMBER_COMPARE( pad2_ )
		VERTEX_MEMBER_COMPARE( pad3_ )
	}

	VERTEX_MEMBER_COMPARE( weight2_ )
	VERTEX_MEMBER_COMPARE( weight_ )

	return true;
};

// I, W are declared in reverse order

// VertexXYZNUVIIIWW -> VertexXYZNUVIIIWWPC
// III_	(copy reverse order)
// _WW_	(copy reverse order)
TEST( Moo_VertexFormats_conversions_VertexXYZNUVIIIWW_VertexXYZNUVIIIWWPC )
{
	typedef VertexXYZNUVIIIWW SourceVertexType;
	typedef VertexXYZNUVIIIWWPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = 207689245;
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index3_ = 5;
	vin1.index2_ = 18;
	vin1.index_ = 98;
	vin1.weight2_ = 3;
	vin1.weight_ = 9;

	// expected output
	TargetVertexType vout1;
	vout1.operator=(vin1);

	// check target vertex format is the expected one
	VertexFormatTuple vfTup = getFormats<SourceVertexType, TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVIIIWWPC( vout1, vconv1, false ) );
	// uninitialized padding difference
	//CHECK( equals_VertexXYZNUVIIIWWPC( vout1, vconv1, true ) );	
	//CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}


// VertexXYZNUVI -> VertexXYZNUVIIIWWPC
// III_	(all same val from index1)
	// index_ = (uint8)in.index_;
	// index2_ = index_;
	// index3_ = index_;
// _WW_ (hardcoded, reverse order)
	// weight_ = 255;
	// weight2_ = 0;
TEST( Moo_VertexFormats_conversions_VertexXYZNUVI_VertexXYZNUVIIIWWPC )
{
	typedef VertexXYZNUVI SourceVertexType;
	typedef VertexXYZNUVIIIWWPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = Vector3(1.8903476f, -0.34768f, -0.194f);
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index_ = 3.0f;

	// expected output
	TargetVertexType vout1;
	vout1.operator=(vin1);

	// get vertex formats
	VertexFormatTuple vfTup;
	vfTup.sourceFormat_ = VertexFormatCache::get<SourceVertexType>();
	vfTup.targetFormat_ = VertexFormatCache::get<TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVIIIWWPC( vout1, vconv1, false ) );

	// uninitialized padding
	// CHECK( equals_VertexXYZNUVIIIWWPC( vout1, vconv1, true ) );
	// CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}


//------------------------------------------------------------------------------
// VertexXYZNUV2IIIWWPC
/*
struct VertexXYZNUV2IIIWWPC
{
Vector3		pos_;
GPUNormalVector3	normal_;
GPUTexcoordVector2	uv_;
GPUTexcoordVector2	uv2_;
uint8		index3_;
uint8		index2_;
uint8		index_;
uint8		weight2_;
uint8		weight_;
uint8		pad3_;
}
*/

bool equals_VertexXYZNUV2IIIWWPC( const VertexXYZNUV2IIIWWPC & a, const VertexXYZNUV2IIIWWPC & b, bool comparePadding, bool compareUV2 )
{
	VERTEX_MEMBER_COMPARE( pos_ )
	VERTEX_MEMBER_COMPARE( normal_ )
	VERTEX_MEMBER_COMPARE( uv_ )

	if (compareUV2)
	{
		VERTEX_MEMBER_COMPARE( uv2_ )
	}

	VERTEX_MEMBER_COMPARE( index3_ )
	VERTEX_MEMBER_COMPARE( index2_ )
	VERTEX_MEMBER_COMPARE( index_ )
	VERTEX_MEMBER_COMPARE( weight2_ )
	VERTEX_MEMBER_COMPARE( weight_ )

	if (comparePadding)
	{
		VERTEX_MEMBER_COMPARE( pad3_ )
	}

	return true;
}



// I, W declared in reverse order

// VertexXYZNUVIIIWW -> VertexXYZNUV2IIIWWPC
// III (copy reverse order)
// WW_ (copy reverse order)
// uv2_ skipped
TEST( Moo_VertexFormats_conversions_VertexXYZNUVIIIWW_VertexXYZNUV2IIIWWPC )
{
	typedef VertexXYZNUVIIIWW SourceVertexType;
	typedef VertexXYZNUV2IIIWWPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = 207689245;
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index3_ = 5;
	vin1.index2_ = 18;
	vin1.index_ = 98;
	vin1.weight2_ = 3;
	vin1.weight_ = 9;

	// expected output
	TargetVertexType vout1;
	vout1.operator=(vin1);

	// get vertex formats
	VertexFormatTuple vfTup;
	vfTup.sourceFormat_ = VertexFormatCache::get<SourceVertexType>();
	vfTup.targetFormat_ = VertexFormatCache::get<TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUV2IIIWWPC( vout1, vconv1, false, false ) );

	// uninitialized values
	//CHECK( equals_VertexXYZNUV2IIIWWPC( vout1, vconv1, true, true ) );
	//CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}



// VertexXYZNUV2IIIWW -> VertexXYZNUV2IIIWWPC
// III (copy reverse order)
// WW_ (copy reverse order)
TEST( Moo_VertexFormats_conversions_VertexXYZNUV2IIIWW_VertexXYZNUV2IIIWWPC )
{
	typedef VertexXYZNUV2IIIWW SourceVertexType;
	typedef VertexXYZNUV2IIIWWPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = 207689245;
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.uv2_ = Vector2(2.39f, -3.3456f);
	vin1.index3_ = 5;
	vin1.index2_ = 18;
	vin1.index_ = 98;
	vin1.weight2_ = 3;
	vin1.weight_ = 9;

	// expected output
	TargetVertexType vout1;
	vout1.operator=(vin1);

	// check target vertex format is the expected one
	VertexFormatTuple vfTup = getFormats<SourceVertexType, TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUV2IIIWWPC( vout1, vconv1, false, true ) );

	// uninitialized padding
	//CHECK( equals_VertexXYZNUV2IIIWWPC( vout1, vconv1, true, true ) );
	//CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}


// VertexXYZNUVI -> VertexXYZNUV2IIIWWPC
// III (all same value)
	// index_ = (uint8)in.index_;
	// index2_ = index_;
	// index3_ = index_;
// WW_ (hardcoded, reverse order)
	// weight_ = 255;
	// weight2_ = 0;
TEST( Moo_VertexFormats_conversions_VertexXYZNUVI_VertexXYZNUV2IIIWWPC )
{
	typedef VertexXYZNUVI SourceVertexType;
	typedef VertexXYZNUV2IIIWWPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = Vector3(1.8903476f, -0.34768f, -0.194f);
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index_ = 3.0f;

	// expected output
	TargetVertexType vout1;
	vout1.operator=(vin1);

	// get vertex formats
	VertexFormatTuple vfTup;
	vfTup.sourceFormat_ = VertexFormatCache::get<SourceVertexType>();
	vfTup.targetFormat_ = VertexFormatCache::get<TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUV2IIIWWPC( vout1, vconv1, false, false ) );

	// uninitialized values
	//CHECK( equals_VertexXYZNUV2IIIWWPC( vout1, vconv1, true, true ) );
	//CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}

//------------------------------------------------------------------------------
// VertexXYZNUVIIIWWTBPC

/*
struct VertexXYZNUVIIIWWTBPC
{
Vector3		pos_;
GPUNormalVector3	normal_;
GPUTexcoordVector2	uv_; 
uint8		index3_;
uint8		index2_;
uint8		index_;
uint8		pad_;
uint8		pad2_;
uint8		weight2_;
uint8		weight_;
uint8		pad3_;
GPUNormalVector3	tangent_;
GPUNormalVector3	binormal_;
}
*/

bool equals_VertexXYZNUVIIIWWTBPC( const VertexXYZNUVIIIWWTBPC & a, const VertexXYZNUVIIIWWTBPC & b, bool comparePadding )
{
	VERTEX_MEMBER_COMPARE( pos_ )
	VERTEX_MEMBER_COMPARE( normal_ )
	VERTEX_MEMBER_COMPARE( uv_ )
	VERTEX_MEMBER_COMPARE( index3_ )
	VERTEX_MEMBER_COMPARE( index2_ )
	VERTEX_MEMBER_COMPARE( index_ )
	VERTEX_MEMBER_COMPARE( weight2_ )
	VERTEX_MEMBER_COMPARE( weight_ )

	if (comparePadding)
	{
		VERTEX_MEMBER_COMPARE( pad_ )
		VERTEX_MEMBER_COMPARE( pad2_ )
		VERTEX_MEMBER_COMPARE( pad3_ )
	}

	VERTEX_MEMBER_COMPARE( tangent_ )
	VERTEX_MEMBER_COMPARE( binormal_ )

	return true;
}

// I, W declared in reverse order

// VertexXYZNUVIIIWWTB -> VertexXYZNUVIIIWWTBPC
// III_ (copy reversed)
// _WW_ (copy reversed)
TEST( Moo_VertexFormats_conversions_VertexXYZNUVIIIWWTB_VertexXYZNUVIIIWWTBPC )
{
	typedef VertexXYZNUVIIIWWTB SourceVertexType;
	typedef VertexXYZNUVIIIWWTBPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = 207689245;
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index3_ = 5;
	vin1.index2_ = 18;
	vin1.index_ = 98;
	vin1.weight2_ = 3;
	vin1.weight_ = 9;
	vin1.tangent_ = 126786234;
	vin1.binormal_ = 195877892;

	// expected output
	TargetVertexType vout1;
	vout1.operator=(vin1);

	// check target vertex format is the expected one
	VertexFormatTuple vfTup = getFormats<SourceVertexType, TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVIIIWWTBPC( vout1, vconv1, false ) );

	// uninitialized padding
	//CHECK( equals_VertexXYZNUVIIIWWTBPC( vout1, vconv1, true ) );
	//CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}


// VertexXYZNUVITB -> VertexXYZNUVIIIWWTBPC
// III_ (all same value)
	// index_ = (uint8)in.index_;
	// index2_ = index_;
	// index3_ = index_;
// _WW_ (hardcoded, reverse order)
	// weight_ = 255;
	// weight2_ = 0;
TEST( Moo_VertexFormats_conversions_VertexXYZNUVITB_VertexXYZNUVIIIWWTBPC )
{
	typedef VertexXYZNUVITB SourceVertexType;
	typedef VertexXYZNUVIIIWWTBPC TargetVertexType;

	// sanity check on 1 vertex
	// input buffer
	SourceVertexType vin1;

	vin1.pos_ = Vector3(4.8903476f, 0.34768f, -0.194f);
	vin1.normal_ = 207689245;
	vin1.uv_ = Vector2(1.239f, -3.73456f);
	vin1.index_ = 3.0f;
	vin1.tangent_ = 126786234;
	vin1.binormal_ = 195877892;

	// expected output
	TargetVertexType vout1;
	vout1.operator=(vin1);

	// get vertex formats
	VertexFormatTuple vfTup;
	vfTup.sourceFormat_ = VertexFormatCache::get<SourceVertexType>();
	vfTup.targetFormat_ = VertexFormatCache::get<TargetVertexType>();
	CHECK( vfTup );

	// check vertex sizes match
	bool sizeMatchSuccess = checkSizeMatch<SourceVertexType, TargetVertexType>( vfTup );
	CHECK( sizeMatchSuccess );

	// conversion target buffer
	TargetVertexType vconv1;

	// setup and perform conversion
	VertexFormat::ConversionContext conversion( vfTup.targetFormat_, vfTup.sourceFormat_ );
	CHECK( conversion.isValid() );
	bool converted = conversion.convertSingleStream( &vconv1, &vin1, 1 );
	CHECK( converted );

	// finally, check conversion result matches assignment operator
	CHECK( equals_VertexXYZNUVIIIWWTBPC( vout1, vconv1, false ) );

	// uninitialised padding
	//CHECK( equals_VertexXYZNUVIIIWWTBPC( vout1, vconv1, true ) );
	//CHECK( memcmp( &vout1, &vconv1, sizeof(vconv1) ) == 0 );
}

//----------------------------------------------------------------------------

#undef VERTEX_MEMBER_COMPARE

