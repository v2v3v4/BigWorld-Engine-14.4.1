#include "pch.hpp"

#include <memory>
#include "moo/visual.hpp"
#include "physics2/bsp.hpp"
#include "resmgr/xml_section.hpp"
#include "moo/render_context.hpp"

#include "moo/vertex_element.hpp"
#include "moo/vertex_element_value.hpp"
#include "moo/vertex_element_special_cases.hpp"
#include "moo/vertex_format.hpp"
#include "moo/vertex_format_cache.hpp"

BW_USE_NAMESPACE
using namespace Moo;

typedef VertexElement::SemanticType SemanticType;
typedef VertexElement::StorageType StorageType;

/**
 * Helper class for checking the details of vertex element semantic and stream info in a format.
 * Contains predicate testing functions that accept (arguments + expected results) to various query functions.
 * Aggregate test functions are provided, so with an over-specified set of arguments, the helper can
 * test the outputs of multiple API functions in a single call from the test case.
 */
class VertexFormatTestHelper
{
public:

	/// check expected element results, searched by format index
	static bool checkFindElementByFormatIndex( const VertexFormat& format, uint32 formatIndex, SemanticType::Value expectedSemanticType, uint32 expectedStreamIndex,
												uint32 expectedSemanticIndex, size_t expectedOffset, StorageType::Value expectedStorageType, bool checkExpected=true )
	{
		SemanticType::Value resultSemanticType;
		uint32 resultStreamIndex;	
		uint32 resultSemanticIndex;	
		size_t resultOffset;
		StorageType::Value resultStorageType;

		bool foundElement = format.findElement( formatIndex, &resultSemanticType, 
												&resultStreamIndex, &resultSemanticIndex,
												&resultOffset, &resultStorageType );

		// check results match expected
		if (foundElement == false)
		{
			return false;
		}
		if (checkExpected)
		{
			if (expectedSemanticType != resultSemanticType)
			{
				return false;
			}
			if (expectedStorageType != resultStorageType)
			{
				return false;
			}
			if (expectedStreamIndex != resultStreamIndex)
			{
				return false;
			}
			if (expectedSemanticIndex != resultSemanticIndex)
			{
				return false;
			}
			if (expectedOffset != resultOffset)
			{
				return false;
			}
		}
		return true;
	}

	/// check expected element results, searched by stream index
	static bool checkFindElementByStreamIndex( VertexFormat& format, 
		uint32 streamIndex, uint32 streamElementIndex, 
		uint32 expectedSemanticIndex,
	SemanticType::Value expectedSemanticType, size_t expectedOffset, StorageType::Value expectedStorageType, bool checkExpected=true)
	{

		SemanticType::Value resultSemanticType;
		uint32 resultSemanticIndex;	
		size_t resultOffset;
		StorageType::Value resultStorageType;

		bool foundElement = format.findElement(	streamIndex, streamElementIndex, 
												&resultSemanticType, &resultSemanticIndex,
												&resultOffset, &resultStorageType); 

		// check results match expected
		if (foundElement == false)
		{
			return false;
		}
		if (checkExpected)
		{
			if (expectedSemanticType != resultSemanticType)
			{
				return false;
			}
			if (expectedStorageType != resultStorageType)
			{
				return false;
			}
			if (expectedSemanticIndex != resultSemanticIndex)
			{
				return false;
			}
			if (expectedOffset != resultOffset)
			{
				return false;
			}
		}
		return true;
	}

	/// check expected element format find results, 
	/// searched by semantic type on format
	static bool checkFindFormatSemanticElement( 
		VertexFormat& format, SemanticType::Value semanticType, uint32 sematicIndex, 
		uint32 expectedStreamIndex, size_t expectedOffset, 
		StorageType::Value expectedStorageType, bool checkExpected = true )
	{
		uint32 resultStreamIndex;
		size_t resultOffset;
		StorageType::Value resultStorageType;

		bool foundElement = format.findElement( semanticType, sematicIndex, 
			&resultStreamIndex, &resultOffset, &resultStorageType );
		
		// check results match expected
		if (foundElement == false)
		{
			return false;
		}
		if (checkExpected)
		{
			if (expectedStreamIndex != resultStreamIndex)
			{
				return false;
			}
			if (expectedOffset != resultOffset)
			{
				return false;
			}
			if (expectedStorageType != resultStorageType)
			{
				return false;
			}
		}

		return true;
	}

	/// check expected element stream find results, searched by semantic type on stream
	static bool checkFindStreamSemanticElement( 
		VertexFormat& format, uint32 streamIndex, SemanticType::Value semanticType, 
		uint32 semanticIndex, size_t expectedOffset, 
		StorageType::Value expectedStorageType, bool checkExpected = true )
	{
		size_t resultOffset;
		StorageType::Value resultStorageType;

		bool foundElement = format.findElement( 
			streamIndex, semanticType, semanticIndex, &resultOffset, 
			&resultStorageType );
		
		// check results match expected
		if (foundElement == false)
		{
			return false;
		}
		if (checkExpected)
		{
			if (expectedOffset != resultOffset)
			{
				return false;
			}
			if (expectedStorageType != resultStorageType)
			{
				return false;
			}
		}

		return true;
	}

	/// check that all relevant "find*(...)" functions return the expected information about a certain stream semantic/index
	static bool checkElement(	
		VertexFormat& format,				// the format object
		uint32 formatElementIndex,			// the format-scoped "overall" index of the element
		uint32 formatSemanticIndex,			// the format-scoped "overall" semantic index of the element
		uint32 streamIndex,					// the stream's index
		uint32 streamElementIndex,			// the stream-local index of the element
		uint32 semanticIndex,				// the stream-local semantic index of the element
		SemanticType::Value semanticType,	// semantic type enum
		size_t offset,						// expected offset from start of element's owning stream
		StorageType::Value storageType,		// the expected storage type
		bool checkExpected=true				// if false, will ignore any checks on expected/output params
											// and simply return true/false if found/not found
				)
	{
		bool result;

		// run through all find functions 
		result = checkFindElementByStreamIndex( 
			format, streamIndex, streamElementIndex, semanticIndex, 
			semanticType, offset, storageType, checkExpected );
		if (!result) { return false; }

		result = checkFindElementByFormatIndex( 
			format, formatElementIndex, semanticType, streamIndex, 
			semanticIndex, offset, storageType, checkExpected );
		if (!result) { return false; }

		result = checkFindFormatSemanticElement( 
			format, semanticType, formatSemanticIndex, streamIndex, 
			offset, storageType, checkExpected );
		if (!result) { return false; }

		result = checkFindStreamSemanticElement( 
			format, streamIndex, semanticType, semanticIndex, offset, 
			storageType, checkExpected );
		if (!result) { return false; }

		return true;
	}

	/// check totals and other metrics of the stream against expected metrics
	static bool checkFormatMetrics( VertexFormat& format, 
		uint32 expectedStreamCount, uint32 expectedElements )
	{
		if (format.streamCount() != expectedStreamCount)
		{
			return false;
		}

		if (format.countElements() != expectedElements)
		{
			return false;
		}

		return true;
	}

	/// check stream based information against expected values
	static bool checkStreamMetrics(	VertexFormat& format, 
		uint32 streamIndex, uint32 expectedElements, uint32 expectedStride )
	{
		if (format.countElements(streamIndex) != expectedElements)
		{
			return false;
		}

		if (format.streamStride(streamIndex) != expectedStride)
		{
			return false;
		}

		return true;
	}

};

TEST( Moo_VertexElement_SemanticInfo )
{
	// Check that the sizes that we get back are as expected.

	typedef VertexElement::SemanticType Sem;

	struct SemanticInfo
	{
		Sem::Value val_;
		BW::string name_;

		SemanticInfo(Sem::Value val, const BW::string & name )
			: val_(val), name_(name)
		{}
	};

	SemanticInfo semanticData[] = 
	{
		SemanticInfo( Sem::POSITION, "POSITION" ),
		SemanticInfo( Sem::BLENDWEIGHT, "BLENDWEIGHT" ),
		SemanticInfo( Sem::BLENDINDICES, "BLENDINDICES" ),
		SemanticInfo( Sem::NORMAL, "NORMAL" ),
		SemanticInfo( Sem::PSIZE, "PSIZE" ),
		SemanticInfo( Sem::TEXCOORD, "TEXCOORD" ),
		SemanticInfo( Sem::TANGENT, "TANGENT" ),
		SemanticInfo( Sem::BINORMAL, "BINORMAL" ),
		SemanticInfo( Sem::TESSFACTOR, "TESSFACTOR" ),
		SemanticInfo( Sem::POSITIONT, "POSITIONT" ),
		SemanticInfo( Sem::COLOR, "COLOR" ),
		SemanticInfo( Sem::FOG, "FOG" ),
		SemanticInfo( Sem::DEPTH, "DEPTH" ),
		SemanticInfo( Sem::SAMPLE, "SAMPLE" )
	};

	// make sure that unit test array size matches internal one, 
	// otherwise the entries in this unit test array may need to be updated
	CHECK_EQUAL( ARRAYSIZE( semanticData ), VertexElement::SemanticEntryCount() );

	for (size_t i = 0; i < ARRAYSIZE(semanticData); ++i)
	{
		const SemanticInfo& tup = semanticData[i];
		CHECK_EQUAL( VertexElement::SemanticFromString( tup.name_ ), tup.val_ );
		CHECK( VertexElement::StringFromSemantic( tup.val_ ).equals( tup.name_ ));
	}
};

TEST( Moo_VertexElement_StorageInfo )
{
	// Check that the sizes that we get back are as expected.

	typedef VertexElement::StorageType Stor;

	struct StorageInfo
	{
		Stor::Value val_;
		BW::string name_;
		size_t size_;
		size_t componentCount_;

		StorageInfo(Stor::Value val, const BW::string & name, 
			size_t componentCount, size_t size )
			: val_(val), name_(name), componentCount_(componentCount), size_(size)
		{}
	};

	StorageInfo storageData[] = 
	{
		StorageInfo( Stor::FLOAT1, "FLOAT1", 1, 4 ),
		StorageInfo( Stor::FLOAT2, "FLOAT2", 2, 8 ),
		StorageInfo( Stor::FLOAT3, "FLOAT3", 3, 12 ),
		StorageInfo( Stor::FLOAT4, "FLOAT4", 4, 16 ),
		StorageInfo( Stor::HALF2, "FLOAT16_2", 2, 4 ),
		StorageInfo( Stor::HALF4, "FLOAT16_4", 4, 8 ),
		StorageInfo( Stor::COLOR, "COLOR", 4, 4 ),
		StorageInfo( Stor::UBYTE1, "UBYTE1", 1, 1 ),
		StorageInfo( Stor::UBYTE2, "UBYTE2", 2, 2 ),
		StorageInfo( Stor::UBYTE3, "UBYTE3", 3, 3 ),
		StorageInfo( Stor::UBYTE4, "UBYTE4", 4, 4 ),
		StorageInfo( Stor::UBYTE4_NORMAL_8_8_8, "UBYTE4_NORMAL_8_8_8", 3, 4 ),
		StorageInfo( Stor::SHORT1, "SHORT1", 1, 2 ),
		StorageInfo( Stor::SHORT2, "SHORT2", 2, 4 ),
		StorageInfo( Stor::SHORT3, "SHORT3", 3, 6 ),
		StorageInfo( Stor::SHORT4, "SHORT4", 4, 8 ),
		StorageInfo( Stor::UBYTE4N, "UBYTE4N", 4, 4 ),
		StorageInfo( Stor::SHORT2N, "SHORT2N", 2, 4 ),
		StorageInfo( Stor::SHORT4N, "SHORT4N", 4, 8 ),
		StorageInfo( Stor::USHORT2N, "USHORT2N", 2, 4 ),
		StorageInfo( Stor::USHORT4N, "USHORT4N", 4, 8 ),
		StorageInfo( Stor::UDEC3, "UDEC3", 3, 4 ),
		StorageInfo( Stor::DEC3N, "DEC3N", 3, 4 ),

		StorageInfo( Stor::SC_DIV3_III, "SC_DIV3_III", 3, 3 ),
		StorageInfo( Stor::SC_REVERSE_III, "SC_REVERSE_III", 3, 3 ),
		StorageInfo( Stor::SC_REVERSE_PADDED_III_, "SC_REVERSE_PADDED_III_", 3, 4 ),
		StorageInfo( Stor::SC_REVERSE_PADDED__WW_, "SC_REVERSE_PADDED__WW_", 2, 4 ),
		StorageInfo( Stor::SC_REVERSE_PADDED_WW_, "SC_REVERSE_PADDED_WW_", 2, 3 )
	};
	
	// make sure that unit test array size matches internal one, 
	// otherwise the entries in this unit test array may need to be updated
	CHECK_EQUAL( ARRAYSIZE( storageData ), VertexElement::StorageEntryCount() );

	for (size_t i = 0; i < ARRAYSIZE(storageData); ++i)
	{
		const StorageInfo& tup = storageData[i];
		CHECK_EQUAL( VertexElement::StorageFromString( tup.name_ ), tup.val_ );
		CHECK( VertexElement::StringFromStorage( tup.val_ ).equals( tup.name_ ));
		CHECK_EQUAL( VertexElement::typeSize( tup.val_ ), tup.size_ );
		CHECK_EQUAL( VertexElement::componentCount( tup.val_ ), tup.componentCount_ );
	}
};

TEST( Moo_VertexElementValue_check )
{
	uint32 myPackedUnsignedData1 = 0xDEADBEEF;

	VertexElement::Ubyte4 a;
	a.copyBytesFrom(myPackedUnsignedData1);

	// compile-time indexing
	CHECK_EQUAL(0xEF, a.get<0>());
	CHECK_EQUAL(0xBE, a.get<1>());
	CHECK_EQUAL(0xAD, a.get<2>());
	CHECK_EQUAL(0xDE, a.get<3>());

	// This should fail to compile, by design:
	//  VertexElement::Ubyte3 val;
	//	uint val_w = val.get<3>();	

	//	compile time const ref
	const VertexElement::Ubyte4& a_cref = a;
	const uint8& a_cref_0_cref = a_cref.get<0>();	
	CHECK_EQUAL(0xEF, a_cref_0_cref);
	
	// run-time indexing
	CHECK_EQUAL(0xEF, a[0]);
	CHECK_EQUAL(0xBE, a[1]);
	CHECK_EQUAL(0xAD, a[2]);
	CHECK_EQUAL(0xDE, a[3]);
	
	// named access
	CHECK_EQUAL(0xEF, a.x);
	CHECK_EQUAL(0xBE, a.y);
	CHECK_EQUAL(0xAD, a.z);
	CHECK_EQUAL(0xDE, a.w);

	// test different runtime indexing access methods
	uint8 a_0_val = a[0];				// value copy
	uint8& a_0_ref = a[0];			// ref
	const uint8& a_0_cref = a[0];	// const ref

	CHECK_EQUAL(0xEF, a_0_val);
	CHECK_EQUAL(0xEF, a_0_ref);
	CHECK_EQUAL(0xEF, a_0_cref);


	// static type info check
	CHECK_EQUAL(VertexElement::Ubyte4::component_count, 4);
	CHECK_EQUAL(VertexElement::Ubyte4::component_size, sizeof(uint8));
	CHECK_EQUAL(VertexElement::Ubyte4::size_bytes, sizeof(uint8) * 4);

	// iteration example
	uint8 a_vals[] = {0xEF, 0xBE, 0xAD, 0xDE};
	for (uint i = 0; i < VertexElement::Ubyte4::component_count; ++i)
	{
		CHECK_EQUAL(a_vals[i], a[i]);
	}

};

TEST( Moo_VertexElementValue_colorComponents_D3D_check )
{
	// check type sizes match
	CHECK_EQUAL(sizeof(D3DCOLOR), sizeof(VertexElement::Ubyte4));
	CHECK_EQUAL(VertexElement::Ubyte4::size_bytes, sizeof(VertexElement::Ubyte4));

	// orange, almost-transparent
	uint8 r = 0xff;	// 1.0
	uint8 g = 0x7f;	// 0.5
	uint8 b = 0x00;	// 0.0
	uint8 a = 0x33;	// 0.2

	D3DCOLOR color_d3d = D3DCOLOR_RGBA(r, g, b, a);

	// populate via names
	VertexElement::Ubyte4 color_ve;
	color_ve.r = r;
	color_ve.g = g;
	color_ve.b = b;
	color_ve.a = a;

	// byte copy version
	VertexElement::Ubyte4 color_ve_bytecopy;
	color_ve_bytecopy.copyBytesFrom(color_d3d);

	// check that memory representation matches
	CHECK_EQUAL(memcmp( &color_ve, &color_d3d, VertexElement::Ubyte4::size_bytes ), 0);
	CHECK_EQUAL(memcmp( &color_ve, &color_ve_bytecopy, VertexElement::Ubyte4::size_bytes ), 0);

};

TEST( Moo_VertexElementValue_floatComponents_Vector_check )
{
	// check type sizes match
	CHECK_EQUAL(sizeof(Vector4), sizeof(VertexElement::Float4));
	CHECK_EQUAL(VertexElement::Float4::size_bytes, sizeof(VertexElement::Float4));

	float x = 1.0f;
	float y = 0.5f;
	float z = -0.2f;
	float w = 0.0f;

	Vector4 v4(x, y, z, w);

	// populate via names
	VertexElement::Float4 f4;
	f4.x = x;
	f4.y = y;
	f4.z = z;
	f4.w = w;

	// check that component names match 
	CHECK_EQUAL(f4.x, v4.x);
	CHECK_EQUAL(f4.y, v4.y);
	CHECK_EQUAL(f4.z, v4.z);
	CHECK_EQUAL(f4.w, v4.w);
	
	// byte copy version
	VertexElement::Float4 f4_bytecopy;
	f4_bytecopy.copyBytesFrom(v4);

	// check that memory representation matches
	CHECK_EQUAL(memcmp( &f4, &v4, VertexElement::Float4::size_bytes ), 0);
	CHECK_EQUAL(memcmp( &f4, &f4_bytecopy, VertexElement::Float4::size_bytes ), 0);

	// check expected ordering relationship of names vs array access
	CHECK_EQUAL(f4.x, f4[0]);
	CHECK_EQUAL(f4.y, f4[1]);
	CHECK_EQUAL(f4.z, f4[2]);
	CHECK_EQUAL(f4.w, f4[3]);

};



TEST( Moo_VertexElementValue_copyConvertMembers_check )
{
	uint32 myPackedUnsignedData1 = 0xDEADBEEF;

	VertexElement::Ubyte3 a_u3;
	VertexElement::Ubyte4 a_u4;
	a_u4.copyBytesFrom( myPackedUnsignedData1 );

	// check same component type conversion
	VertexElement::copyComponents( &a_u3, &a_u4, 0 );

	CHECK_EQUAL(a_u3.x, a_u4.x);
	CHECK_EQUAL(a_u3.y, a_u4.y);
	CHECK_EQUAL(a_u3.z, a_u4.z);

	// check different component type, via explicit conversion
	VertexElement::Float2 b_f2;
	VertexElement::copyComponents( &b_f2, &a_u3, 0, 
		&VertexConversions::normalizedConvert<float, uint8> );

	float expected_b_float_x = 0xEF/255.0f;
	float expected_b_float_y = 0xBE/255.0f;
	float expected_b_float_z = 0xAD/255.0f;

	CHECK_CLOSE(b_f2.x, expected_b_float_x, FLT_EPSILON);
	CHECK_CLOSE(b_f2.y, expected_b_float_y, FLT_EPSILON);

	// alternative way of calling, via explicit conversion function object
	VertexElement::Float3 b_f3;
	VertexConversions::NormalizedComponentConverter::convert< 
		VertexElement::Float3, VertexElement::Ubyte3>( &b_f3, &a_u3, 0 );
	CHECK_CLOSE(b_f3.z, expected_b_float_z, FLT_EPSILON);

	// expansion (component count from n to m where m > n) 
	// make sure it creates zeros for remaining components
	VertexElement::Float4 b_f4;
	VertexConversions::NormalizedComponentConverter::convert< 
		VertexElement::Float4, VertexElement::Ubyte3>( &b_f4, &a_u3, 0 );
	CHECK_CLOSE(b_f4.w, 0.0f, FLT_EPSILON);

};

TEST( Moo_VertexAccessors_BasicFormat )
{
	// FORMAT1:
	VertexFormat format;

	// F.STREAM0: [POSITION/FLOAT3, NORMAL/FLOAT3]
	uint32 stream = format.addStream();
	format.addElement( stream, SemanticType::POSITION, StorageType::FLOAT3 );
	format.addElement( stream, SemanticType::NORMAL, StorageType::FLOAT3 );

	{
		// semantic count
		CHECK_EQUAL(1, format.countElements( SemanticType::POSITION ));
		CHECK_EQUAL(1, format.countElements( SemanticType::NORMAL ));

		// format and stream info
		CHECK( VertexFormatTestHelper::checkFormatMetrics( format, 1, 2 ) );	
		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 0, 2, sizeof(Vector3) * 2 ) );

		// element info
		CHECK( VertexFormatTestHelper::checkElement( format, 0, 0, 0, 0, 0, SemanticType::POSITION, 0, StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 1, 0, 0, 1, 0, SemanticType::NORMAL, sizeof(Vector3), StorageType::FLOAT3 ) );
	}


	// F.STREAM1: [COLOR/UBYTE4, TEXCOORD/FLOAT2]
	stream = format.addStream();
	format.addElement( stream, SemanticType::COLOR, StorageType::UBYTE4 );
	format.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	
	{
		// semantic count
		CHECK_EQUAL(1, format.countElements( SemanticType::COLOR ));
		CHECK_EQUAL(1, format.countElements( SemanticType::TEXCOORD ));
		
		// format and stream info
		CHECK( VertexFormatTestHelper::checkFormatMetrics( format, 2, 4 ) );	
		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 1, 2, sizeof(Vector2) + 4) );

		// element info
		CHECK( VertexFormatTestHelper::checkElement( format, 2, 0, 1, 0, 0, SemanticType::COLOR, 0, StorageType::UBYTE4) );
		CHECK( VertexFormatTestHelper::checkElement( format, 3, 0, 1, 1, 0, SemanticType::TEXCOORD, 4, StorageType::FLOAT2) );

	}

	// F.STREAM2: [TEXCOORD/FLOAT2, TEXCOORD/FLOAT4]
	stream = format.addStream();
	format.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT4 );

	{
		// semantic count
		CHECK_EQUAL(3, format.countElements( SemanticType::TEXCOORD ));
		
		// format and stream info
		CHECK( VertexFormatTestHelper::checkFormatMetrics( format, 3, 6 ) );	
		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 2, 2, sizeof(float) * 6 ) );

		// element info
		CHECK( VertexFormatTestHelper::checkElement( format, 4, 1, 2, 0, 1, SemanticType::TEXCOORD, 0, StorageType::FLOAT2 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 5, 2, 2, 1, 2, SemanticType::TEXCOORD, sizeof(float) * 2, StorageType::FLOAT4 ) );
		
		// semantic searches on the 3 TEXCOORD elements 
		CHECK( VertexFormatTestHelper::checkFindFormatSemanticElement( format, SemanticType::TEXCOORD, 0, 1, 4, StorageType::FLOAT2 ) ); 
		CHECK( VertexFormatTestHelper::checkFindFormatSemanticElement( format, SemanticType::TEXCOORD, 1, 2, 0, StorageType::FLOAT2 ) ); 
		CHECK( VertexFormatTestHelper::checkFindFormatSemanticElement( format, SemanticType::TEXCOORD, 2, 2, sizeof(Vector2), StorageType::FLOAT4 ) ); 

	}

	{
		// make sure other elements still exist
		CHECK( VertexFormatTestHelper::checkElement( format, 0, 0, 0, 0, 0, SemanticType::POSITION, 0, StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 1, 0, 0, 1, 0, SemanticType::NORMAL, sizeof(Vector3), StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 2, 0, 1, 0, 0, SemanticType::COLOR, 0, StorageType::UBYTE4 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 3, 0, 1, 1, 0, SemanticType::TEXCOORD, 4, StorageType::FLOAT2 ) );
	}

	{
		// Try looking for things that don't exist.

		// Note the checkExpected flag is false (default true). Hence, only the lookup values matter, 
		// and the expected args are ignored/unchecked, so the function simply returns if AN element was found,
		// not if the found element matches expectations.
		CHECK_EQUAL(false, VertexFormatTestHelper::checkFindElementByFormatIndex( format, 6, SemanticType::POSITION, 0, 0, 0, StorageType::UNKNOWN, false ) );
		CHECK_EQUAL(false, VertexFormatTestHelper::checkFindFormatSemanticElement( format, SemanticType::TANGENT, 0, 0, 0, StorageType::UNKNOWN, false ) );
		CHECK_EQUAL(false, VertexFormatTestHelper::checkFindStreamSemanticElement( format, 1, SemanticType::POSITION, 0, 0, StorageType::UNKNOWN, false ) );
		
		//CHECK_ASSERT(VertexFormatTestHelper::checkFindElementByStreamIndex( format, 3, 0, 0, SemanticType::POSITION, 0, StorageType::FLOAT3, false ) );
	}

	{
		// Test sets of semantics contained in format/streams
		typedef VertexElement::SemanticType SemType;

		// generate some test semantics arrays
		SemType::Value sems_xyz[] = { SemType::POSITION };
		SemType::Value sems_xyzn[] = { SemType::POSITION, SemType::NORMAL };
		SemType::Value sems_xyznuvc[] = { SemType::POSITION, 
			SemType::NORMAL, SemType::TEXCOORD, SemType::COLOR };
		SemType::Value sems_uv[] = { SemType::TEXCOORD };

		// Stream description for semantics in format tested on:
		// F.STREAM0: [POSITION NORMAL]
		// F.STREAM1: [COLOR, TEXCOORD]
		// F.STREAM2: [TEXCOORD, TEXCOORD]

		// Sanity check, position only, stream and sem 0
		CHECK_EQUAL(true, format.containsSemantics( sems_xyz, 
			ARRAYSIZE(sems_xyz), 0, 0 ));

		// Check that incorrect stream index does not match
		CHECK_EQUAL(false, format.containsSemantics( sems_xyz, 
			ARRAYSIZE(sems_xyz), 0, 1 ));
		
		// Test zero sized array input
		CHECK_EQUAL(false, format.containsSemantics( sems_xyz, 0, 0, 1 ));

		// Test out of bounds semantic index
		CHECK_EQUAL(false, format.containsSemantics( sems_xyz, 
			ARRAYSIZE(sems_xyz), 1, 1 ));

		// Test out of bounds stream index
		CHECK_EQUAL(false, format.containsSemantics( sems_xyz, 0, 0, 2 ));
		
		// Test out of bounds semantic and stream index
		CHECK_EQUAL(false, format.containsSemantics( sems_xyz, 0, 1, 2 ));

		// Check that multiple semantic types are matched for same stream
		CHECK_EQUAL(true, format.containsSemantics( sems_xyzn, 
			ARRAYSIZE(sems_xyzn), VertexFormat::DEFAULT, 0 ));
		
		// Check that multiple semantic types are matched with both stream 
		//and semantic in "any" mode.
		CHECK_EQUAL(true, format.containsSemantics( sems_xyzn, 
			ARRAYSIZE(sems_xyzn), VertexFormat::DEFAULT, VertexFormat::DEFAULT ));

		// Check that multiple semantics that span multiple streams 
		// are all found in "any" mode
		CHECK_EQUAL(true, format.containsSemantics( sems_xyznuvc, 
			ARRAYSIZE(sems_xyznuvc), VertexFormat::DEFAULT, VertexFormat::DEFAULT ));

		// Check that multiple semantics that span multiple streams are 
		// not matched when provided a specific stream
		CHECK_EQUAL(false, format.containsSemantics( sems_xyznuvc, 
			ARRAYSIZE(sems_xyznuvc), VertexFormat::DEFAULT, 0 ));
		CHECK_EQUAL(false, format.containsSemantics( sems_xyznuvc, 
			ARRAYSIZE(sems_xyznuvc), VertexFormat::DEFAULT, 1 ));
		CHECK_EQUAL(false, format.containsSemantics( sems_xyznuvc, 
			ARRAYSIZE(sems_xyznuvc), VertexFormat::DEFAULT, 2 ));
		
		// Check that a single semantic with multiple instances in one stream 
		// are matched in "any" mode
		CHECK_EQUAL(true, format.containsSemantics( sems_uv, ARRAYSIZE(sems_uv), 
			VertexFormat::DEFAULT, VertexFormat::DEFAULT ));
		
		// Check that a single semantic with multiple instances in one stream 
		// are matched in "any" semantic index mode, but specific stream.
		CHECK_EQUAL(true, format.containsSemantics( sems_uv, ARRAYSIZE(sems_uv), 
			VertexFormat::DEFAULT, 2 ));

		// Check that a single semantic with multiple instances in one stream 
		// is matched given specific semantic and stream index
		CHECK_EQUAL(true, format.containsSemantics( sems_uv, 
			ARRAYSIZE(sems_uv), 2, 2 ));

		// Check that a single semantic with multiple instances in one stream 
		// is not matched given specific semantic and specific stream
		CHECK_EQUAL(true, format.containsSemantics( sems_uv, 
			ARRAYSIZE(sems_uv), 2, VertexFormat::DEFAULT ));

		// Check that a semantic is matched over multiple streams 
		// given "any" stream index and with specific semantic index
		CHECK_EQUAL(true, format.containsSemantics( sems_uv, 
			ARRAYSIZE(sems_uv), 0, VertexFormat::DEFAULT ));
		CHECK_EQUAL(true, format.containsSemantics( sems_uv, 
			ARRAYSIZE(sems_uv), 1, VertexFormat::DEFAULT ));
		CHECK_EQUAL(true, format.containsSemantics( sems_uv, 
			ARRAYSIZE(sems_uv), 2, VertexFormat::DEFAULT ));
		
		// Out of bounds semantic index, many stream matches
		CHECK_EQUAL(false, format.containsSemantics( sems_uv, 
			ARRAYSIZE(sems_uv), 3, VertexFormat::DEFAULT ));

		// Check that a single semantic with multiple instance in one stream 
		// is not matched in "any" semantic index mode while given non-matching 
		// stream index
		CHECK_EQUAL(false, format.containsSemantics( sems_uv, 
			ARRAYSIZE(sems_uv), VertexFormat::DEFAULT, 0 ));
	}
}

TEST( Moo_VertexAccessors_EmptyFormat )
{
	// test an empty/unpopulated format
	VertexFormat format;
	VertexFormatTestHelper::checkFormatMetrics( format, 0, 0 );
}

TEST( Moo_VertexAccessors_SparseStreams1 )
{
	// test a format with stream "gaps" outside
	VertexFormat format;
	format.addStream();
	uint32 stream = format.addStream();
	format.addStream();

	// F.STREAM0: []
	// F.STREAM1: [POSITION/FLOAT3, NORMAL/FLOAT3, TEXCOORD/FLOAT2, TEXCOORD/FLOAT2]
	// F.STREAM2: []
	format.addElement( stream, SemanticType::POSITION, StorageType::FLOAT3 );
	format.addElement( stream, SemanticType::NORMAL, StorageType::FLOAT3 );
	format.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT2 );

	{
		// check format
		CHECK( VertexFormatTestHelper::checkFormatMetrics( format, 3, 4 ) );

		// check streams
		uint32 stream2_stride = sizeof(Vector3) * 2 + sizeof(Vector2) * 2;

		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 0, 0, 0 ) );
		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 1, 4, stream2_stride ) );
		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 2, 0, 0 ) );

		// check elements
		CHECK( VertexFormatTestHelper::checkElement( format, 0, 0, 1, 0, 0, SemanticType::POSITION, 0, StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 1, 0, 1, 1, 0, SemanticType::NORMAL, sizeof(Vector3), StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 2, 0, 1, 2, 0, SemanticType::TEXCOORD, sizeof(Vector3)*2, StorageType::FLOAT2 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 3, 1, 1, 3, 1, SemanticType::TEXCOORD, sizeof(Vector3)*2 + sizeof(Vector2), StorageType::FLOAT2 ) );
	}
}

TEST( Moo_VertexAccessors_SparseStreams2 )
{
	// test a format with stream "gaps" inside
	VertexFormat format;
	format.addStream();
	format.addStream();
	format.addStream();

	// F.STREAM0: [POSITION/FLOAT3, NORMAL/FLOAT3, TEXCOORD/FLOAT2, TEXCOORD/FLOAT2]
	// F.STREAM1: []
	// F.STREAM2: [TEXCOORD/FLOAT2, TEXCOORD/FLOAT2, POSITION/FLOAT3, NORMAL/FLOAT3]
	format.addElement( 0, SemanticType::POSITION, StorageType::FLOAT3 );
	format.addElement( 0, SemanticType::NORMAL, StorageType::FLOAT3 );
	format.addElement( 0, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addElement( 0, SemanticType::TEXCOORD, StorageType::FLOAT2 );

	format.addElement( 2, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addElement( 2, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addElement( 2, SemanticType::POSITION, StorageType::FLOAT3 );
	format.addElement( 2, SemanticType::NORMAL, StorageType::FLOAT3 );
	{
		// check format
		CHECK( VertexFormatTestHelper::checkFormatMetrics( format, 3, 8 ) );

		// check streams
		uint32 edgeStreamsStride = sizeof(Vector3) * 2 + sizeof(Vector2) * 2;

		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 0, 4, edgeStreamsStride ) );
		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 1, 0, 0 ) );
		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 2, 4, edgeStreamsStride ) );

		// check elements
		CHECK( VertexFormatTestHelper::checkElement( format, 0, 0, 0, 0, 0, SemanticType::POSITION, 0, StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 1, 0, 0, 1, 0, SemanticType::NORMAL, sizeof(Vector3), StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 2, 0, 0, 2, 0, SemanticType::TEXCOORD, sizeof(Vector3)*2, StorageType::FLOAT2 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 3, 1, 0, 3, 1, SemanticType::TEXCOORD, sizeof(Vector3)*2 + sizeof(Vector2), StorageType::FLOAT2 ) );
		
		CHECK( VertexFormatTestHelper::checkElement( format, 4, 2, 2, 0, 2, SemanticType::TEXCOORD, 0, StorageType::FLOAT2 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 5, 3, 2, 1, 3, SemanticType::TEXCOORD, sizeof(Vector2), StorageType::FLOAT2 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 6, 1, 2, 2, 1, SemanticType::POSITION, sizeof(Vector2)*2, StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 7, 1, 2, 3, 1, SemanticType::NORMAL, sizeof(Vector2)*2 + sizeof(Vector3), StorageType::FLOAT3 ) );
	}
}

TEST( Moo_VertexAccessors_SparseStreams3 )
{
	// test a format with stream "gaps" inside and outside
	VertexFormat format;

	for (size_t i = 0; i < 16; ++i)
	{
		format.addStream();
	}
	
	// F.STREAM1: [POSITION/FLOAT3, TEXCOORD/FLOAT2, NORMAL/FLOAT3, TEXCOORD/FLOAT2, POSITION/FLOAT3, COLOR/UBYTE4]
	// ... 
	// F.STREAM3: [TEXCOORD/FLOAT2, TEXCOORD/FLOAT2, NORMAL/FLOAT3, POSITION/FLOAT3]
	// F.STREAM4: [TEXCOORD/FLOAT2, TANGENT/FLOAT3, BINORMAL/FLOAT3]
	// ... 
	// F.STREAM12: [NORMAL/FLOAT3, TEXCOORD/FLOAT2, COLOR/USHORT4N, POSITION/FLOAT3]
	// ... 

	format.addElement( 1, SemanticType::POSITION, StorageType::FLOAT3 );
	format.addElement( 1, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addElement( 1, SemanticType::NORMAL, StorageType::FLOAT3 );
	format.addElement( 1, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addElement( 1, SemanticType::POSITION, StorageType::FLOAT3 );
	format.addElement( 1, SemanticType::COLOR, StorageType::UBYTE4 );

	format.addElement( 3, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addElement( 3, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addElement( 3, SemanticType::NORMAL, StorageType::FLOAT3 );
	format.addElement( 3, SemanticType::POSITION, StorageType::FLOAT3 );
	
	format.addElement( 4, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addElement( 4, SemanticType::TANGENT, StorageType::FLOAT3 );
	format.addElement( 4, SemanticType::BINORMAL, StorageType::FLOAT3 );

	format.addElement( 12, SemanticType::NORMAL, StorageType::FLOAT3 );
	format.addElement( 12, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addElement( 12, SemanticType::COLOR, StorageType::USHORT4N );
	format.addElement( 12, SemanticType::POSITION, StorageType::FLOAT3 );
	
	{
		const size_t vec2Size = sizeof(Vector2);
		const size_t vec3Size = sizeof(Vector3);
		const size_t ushort4NSize = sizeof(unsigned short) * 4;
		const size_t Ubyte4Size = 4;

		// check format
		CHECK( VertexFormatTestHelper::checkFormatMetrics( format, 16, 17 ) );

		// check streams
		uint32 stream1_stride = vec3Size * 3 + vec2Size * 2 + Ubyte4Size;
		uint32 stream3_stride = vec3Size * 2 + vec2Size * 2;
		uint32 stream4_stride = vec2Size + vec3Size * 2;
		uint32 stream12_stride = vec3Size * 2 + vec2Size + ushort4NSize;
		
		uint32 blankStreamIndexes[] = {0, 2, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15};

		for (uint32 i = 0; i < sizeof(blankStreamIndexes)/sizeof(uint32); ++i)
		{
			uint32 currentBlankStreamIndex = blankStreamIndexes[i];
			CHECK( VertexFormatTestHelper::checkStreamMetrics( format, currentBlankStreamIndex, 0, 0 ) );
		}

		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 1, 6, stream1_stride ) );
		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 3, 4, stream3_stride ) );
		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 4, 3, stream4_stride ) );
		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 12, 4, stream12_stride ) );


		// check elements
		CHECK( VertexFormatTestHelper::checkElement( format, 0, 0, 1, 0, 0, SemanticType::POSITION, 0, StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 1, 0, 1, 1, 0, SemanticType::TEXCOORD, vec3Size, StorageType::FLOAT2 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 2, 0, 1, 2, 0, SemanticType::NORMAL, vec2Size + vec3Size, StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 3, 1, 1, 3, 1, SemanticType::TEXCOORD, vec2Size + vec3Size * 2, StorageType::FLOAT2 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 4, 1, 1, 4, 1, SemanticType::POSITION, vec2Size * 2 + vec3Size *2, StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 5, 0, 1, 5, 0, SemanticType::COLOR, vec2Size * 2 + vec3Size * 3, StorageType::UBYTE4 ) );

		CHECK( VertexFormatTestHelper::checkElement( format, 6, 2, 3, 0, 2, SemanticType::TEXCOORD, 0, StorageType::FLOAT2 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 7, 3, 3, 1, 3, SemanticType::TEXCOORD, vec2Size, StorageType::FLOAT2 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 8, 1, 3, 2, 1, SemanticType::NORMAL, vec2Size * 2, StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 9, 2, 3, 3, 2, SemanticType::POSITION, vec2Size * 2 + vec3Size, StorageType::FLOAT3 ) );

		CHECK( VertexFormatTestHelper::checkElement( format, 10, 4, 4, 0, 4, SemanticType::TEXCOORD, 0, StorageType::FLOAT2 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 11, 0, 4, 1, 0, SemanticType::TANGENT, vec2Size, StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 12, 0, 4, 2, 0, SemanticType::BINORMAL, vec2Size + vec3Size, StorageType::FLOAT3 ) );

		CHECK( VertexFormatTestHelper::checkElement( format, 13, 2, 12, 0, 2, SemanticType::NORMAL, 0, StorageType::FLOAT3 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 14, 5, 12, 1, 5, SemanticType::TEXCOORD, vec3Size, StorageType::FLOAT2 ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 15, 1, 12, 2, 1, SemanticType::COLOR, vec3Size + vec2Size, StorageType::USHORT4N ) );
		CHECK( VertexFormatTestHelper::checkElement( format, 16, 3, 12, 3, 3, SemanticType::POSITION, vec3Size + vec2Size + ushort4NSize, StorageType::FLOAT3 ) );
	}
}


TEST( Moo_VertexAccessors_BasicAccess_InterfaceBoundaries )
{
	// F.STREAM0: [POSITION/FLOAT3, NORMAL/FLOAT3]
	VertexFormat format;
	format.addStream();
	format.addElement( 0, SemanticType::POSITION, StorageType::FLOAT3 );
	format.addElement( 0, SemanticType::NORMAL, StorageType::FLOAT3 );

	const uint32 vertStride = sizeof(Vector3) * 2;
	const uint32 numVerts = 50;
	const uint32 bufferSize = numVerts * 2;

	// we don't fill this buffer, since these tests never actually access the data inside.
	Vector3 buffer[100];

	{
		// test raw accessors, when offset is larger than buffsize
		RawElementAccessor<Vector3> posRawIter = 
			format.createRawElementAccessor<Vector3>( &buffer[0], sizeof(Vector3) * 100, 0, sizeof(Vector3) * 101 );
		CHECK_EQUAL(false, posRawIter.isValid());
	}

	{
		// test incorrect semantic proxy accessors fail
		ProxyElementAccessor<SemanticType::COLOR> colorFalseProxyStream =
			format.createProxyElementAccessor<SemanticType::COLOR>( &buffer[0], sizeof(Vector3) * 100, 0, 0 );
		CHECK_EQUAL(false, colorFalseProxyStream.isValid());
	}

	{
		// test incorrect proxy accessors fail for existing semantic with out of bounds semantic index
		ProxyElementAccessor<SemanticType::POSITION> positionFalseProxyStream =
			format.createProxyElementAccessor<SemanticType::POSITION>( &buffer[0], sizeof(Vector3) * 100, 0, 1 );
		CHECK_EQUAL(false, positionFalseProxyStream.isValid());
	}

}

TEST( Moo_VertexAccessors_BasicAccess1 )
{
	// test basic vertex access on basic single stream format

	// F.STREAM0: [POSITION/FLOAT3, NORMAL/FLOAT3]
	VertexFormat format;
	format.addStream();
	format.addElement( 0, SemanticType::POSITION, StorageType::FLOAT3 );
	format.addElement( 0, SemanticType::NORMAL, StorageType::FLOAT3 );
	
	const uint32 vertStride = sizeof(Vector3) * 2;
	const uint32 numVerts = 50;
	const uint32 bufferSize = numVerts * 2;

	// populate our data buffer with verifiable values
	Vector3 buffer[100];
	for (uint32 index = 0; index < numVerts; ++index)
	{
		uint32 bufferIndex = index * 2;
		Vector3& pos = buffer[bufferIndex];
		pos.x = float(100 + index);
		pos.y = float(200 + index);
		pos.z = float(300 + index);

		Vector3& norm = buffer[bufferIndex + 1];
		norm.x = float(1000 + index);
		norm.y = float(2000 + index);
		norm.z = float(3000 + index);
	}

	{
		// test raw accessors
		RawElementAccessor<Vector3> posRawIter = 
			format.createRawElementAccessor<Vector3>( &buffer[0], sizeof(Vector3) * 100, 0, 0 );
		CHECK_EQUAL(true, posRawIter.isValid());
		CHECK_EQUAL(50, posRawIter.size());
		for (uint32 index = 0; index < posRawIter.size(); ++index)
		{
			CHECK_EQUAL(float(index + 100), posRawIter[index].x);
			CHECK_EQUAL(float(index + 200), posRawIter[index].y);
			CHECK_EQUAL(float(index + 300), posRawIter[index].z);
		}

		RawElementAccessor<Vector3> normRawIter = 
			format.createRawElementAccessor<Vector3>( &buffer[0], sizeof(Vector3) * 100, 0, sizeof(Vector3) );
		CHECK_EQUAL(true, normRawIter.isValid());
		CHECK_EQUAL(50, normRawIter.size());
		for (uint32 index = 0; index < normRawIter.size(); ++index)
		{
			CHECK_EQUAL(float(index + 1000), normRawIter[index].x);
			CHECK_EQUAL(float(index + 2000), normRawIter[index].y);
			CHECK_EQUAL(float(index + 3000), normRawIter[index].z);
		}
	}
	
	{
		// test proxy accessors
		ProxyElementAccessor<SemanticType::POSITION> posProxyStream = 
			format.createProxyElementAccessor<SemanticType::POSITION>( &buffer[0], sizeof(Vector3) * 100, 0, 0 );
		CHECK_EQUAL(true, posProxyStream.isValid());
		CHECK_EQUAL(50, posProxyStream.size());
		for (uint32 index = 0; index < posProxyStream.size(); ++index)
		{
			const Vector3& pos = posProxyStream[index];
			CHECK_EQUAL(float(index + 100), pos.x);
			CHECK_EQUAL(float(index + 200), pos.y);
			CHECK_EQUAL(float(index + 300), pos.z);
		}

		ProxyElementAccessor<SemanticType::NORMAL> normProxyStream = 
			format.createProxyElementAccessor<SemanticType::NORMAL>( &buffer[0], sizeof(Vector3) * 100, 0, 0 );
		CHECK_EQUAL(true, normProxyStream.isValid());
		CHECK_EQUAL(50, normProxyStream.size());
		for (uint32 index = 0; index < normProxyStream.size(); ++index)
		{
			const Vector3& norm = normProxyStream[index];
			CHECK_EQUAL(float(index + 1000), norm.x);
			CHECK_EQUAL(float(index + 2000), norm.y);
			CHECK_EQUAL(float(index + 3000), norm.z);
		}
	}


}

TEST( Moo_VertexAccessors_BasicAccess2 )
{
	// test basic vertex access on multiple stream arrangement from multiple data buffers
	// with multiple semantic types within a stream and multiple semantic types across streams

	// F.STREAM0: [POSITION/FLOAT3, COLOR/UBYTE4, TEXCOORD/FLOAT2, TEXCOORD/FLOAT2]
	// F.STREAM1: [POSITION/FLOAT3, TEXCOORD/FLOAT2,]
	VertexFormat format;
	format.addStream();
	format.addElement( 0, SemanticType::POSITION, StorageType::FLOAT3 );
	format.addElement( 0, SemanticType::COLOR, StorageType::UBYTE4 );
	format.addElement( 0, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addElement( 0, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	format.addStream();
	format.addElement( 1, SemanticType::POSITION, StorageType::FLOAT3 );
	format.addElement( 1, SemanticType::TEXCOORD, StorageType::FLOAT2 );

	const uint32 numVerts = 50;
	const uint32 vertStrideStream1 = sizeof(Vector3) + sizeof(uint32) + sizeof(Vector2)*2;
	const uint32 vertStrideStream2 = sizeof(Vector3) + sizeof(Vector2);
	const uint32 buffer1Size = numVerts * vertStrideStream1;
	const uint32 buffer2Size = numVerts * vertStrideStream2;

	// data buffer for first stream
	unsigned char buffer1[vertStrideStream1 * numVerts];
	for (uint32 index = 0; index < numVerts; ++index)
	{
		uint32 bufferIndex = index * vertStrideStream1;
		Vector3& pos = (Vector3&)buffer1[bufferIndex];
		pos.x = float(100 + index);
		pos.y = float(200 + index);
		pos.z = float(300 + index);
		
		uint32& color = (uint32&)buffer1[bufferIndex + sizeof(Vector3)];
		color = (index*251) << 4;

		Vector2& uv0 = (Vector2&)buffer1[bufferIndex + sizeof(Vector3) + sizeof(uint32)];
		uv0.x = float(1000 + index);
		uv0.y = float(2000 + index);

		Vector2& uv1 = (Vector2&)buffer1[bufferIndex + sizeof(Vector3) + sizeof(uint32) + sizeof(Vector2)];
		uv1.x = float(10000 + index);
		uv1.y = float(20000 + index);
	}

	// data buffer for second stream
	unsigned char buffer2[numVerts * vertStrideStream2];
	for (uint32 index = 0; index < numVerts; ++index)
	{
		uint32 bufferIndex = index * vertStrideStream2;
		Vector3& pos = (Vector3&)buffer2[bufferIndex];
		pos.x = float(400 + index);
		pos.y = float(500 + index);
		pos.z = float(600 + index);

		Vector2& uv0 = (Vector2&)buffer2[bufferIndex + sizeof(Vector3)];
		uv0.x = float(3000 + index);
		uv0.y = float(4000 + index);
	}

	{
		// check format matches calculations
		CHECK( VertexFormatTestHelper::checkFormatMetrics( format, 2, 6 ) );
		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 0, 4, vertStrideStream1 ) );
		CHECK( VertexFormatTestHelper::checkStreamMetrics( format, 1, 2, vertStrideStream2 ) );
	}

	{
		// test raw accessors
		// position
		RawElementAccessor<Vector3> posRawIter = 
			format.createRawElementAccessor<Vector3>( &buffer1[0], buffer1Size, 0, 0 );
		CHECK_EQUAL(true, posRawIter.isValid());
		CHECK_EQUAL(numVerts, posRawIter.size());
		for (uint32 index = 0; index < posRawIter.size(); ++index)
		{
			CHECK_EQUAL(float(index + 100), posRawIter[index].x);
			CHECK_EQUAL(float(index + 200), posRawIter[index].y);
			CHECK_EQUAL(float(index + 300), posRawIter[index].z);
		}

		// color
		RawElementAccessor<uint32> colorRawIter = 
			format.createRawElementAccessor<uint32>( &buffer1[0], buffer1Size, 0, sizeof(Vector3) );
		CHECK_EQUAL(true, colorRawIter.isValid());
		CHECK_EQUAL(numVerts, colorRawIter.size());
		for (uint32 index = 0; index < colorRawIter.size(); ++index)
		{
			CHECK_EQUAL((index*251) << 4, colorRawIter[index]);
		}

		// tex0
		RawElementAccessor<Vector2> tex1RawIter = 
			format.createRawElementAccessor<Vector2>( &buffer1[0], buffer1Size, 0, sizeof(Vector3) + sizeof(uint32) );
		CHECK_EQUAL(true, tex1RawIter.isValid());
		CHECK_EQUAL(numVerts, tex1RawIter.size());
		for (uint32 index = 0; index < tex1RawIter.size(); ++index)
		{
			CHECK_EQUAL(float(index + 1000), tex1RawIter[index].x);
			CHECK_EQUAL(float(index + 2000), tex1RawIter[index].y);
		}

		// tex1
		RawElementAccessor<Vector2> tex2RawIter = 
			format.createRawElementAccessor<Vector2>( &buffer1[0], buffer1Size, 0, sizeof(Vector3) + sizeof(uint32) + sizeof(Vector2) );
		CHECK_EQUAL(true, tex2RawIter.isValid());
		CHECK_EQUAL(numVerts, tex2RawIter.size());
		for (uint32 index = 0; index < tex2RawIter.size(); ++index)
		{
			CHECK_EQUAL(float(index + 10000), tex2RawIter[index].x);
			CHECK_EQUAL(float(index + 20000), tex2RawIter[index].y);
		}
	}

	{
		// test proxy accessor, const variant
		// tex0
		const ProxyElementAccessor<SemanticType::TEXCOORD> tex0ProxyStream = 
			format.createProxyElementAccessor<SemanticType::TEXCOORD>( &buffer1[0], buffer1Size, 0, 0 );
		CHECK_EQUAL(true, tex0ProxyStream.isValid());
		CHECK_EQUAL(numVerts, tex0ProxyStream.size());
		for (uint32 index = 0; index < tex0ProxyStream.size(); ++index)
		{
			const Vector2& uv = tex0ProxyStream[index];
			CHECK_EQUAL(float(index + 1000), uv.x);
			CHECK_EQUAL(float(index + 2000), uv.y);
		}

		// test access on semantic N in same stream
		// tex1
		ProxyElementAccessor<SemanticType::TEXCOORD> tex1ProxyStream = 
			format.createProxyElementAccessor<SemanticType::TEXCOORD>( &buffer1[0], buffer1Size, 0, 1 );
		CHECK_EQUAL(true, tex1ProxyStream.isValid());
		CHECK_EQUAL(numVerts, tex1ProxyStream.size());
		for (uint32 index = 0; index < tex1ProxyStream.size(); ++index)
		{
			const Vector2& uv = tex1ProxyStream[index];
			CHECK_EQUAL(float(index + 10000), uv.x);
			CHECK_EQUAL(float(index + 20000), uv.y);
		}

		// test access on semantic N in next stream via second buffer
		// tex2
		ProxyElementAccessor<SemanticType::TEXCOORD> tex2ProxyStream = 
			format.createProxyElementAccessor<SemanticType::TEXCOORD>( &buffer2[0], buffer2Size, 1, 2 );
		CHECK_EQUAL(true, tex2ProxyStream.isValid());
		CHECK_EQUAL(numVerts, tex2ProxyStream.size());
		for (uint32 index = 0; index < tex2ProxyStream.size(); ++index)
		{
			const Vector2& uv = tex2ProxyStream[index];
			CHECK_EQUAL(float(index + 3000), uv.x);
			CHECK_EQUAL(float(index + 4000), uv.y);
		}

	}
}


TEST( Moo_VertexAccessors_MoveConversion )
{
	const uint32 numVerts = 50;
	const uint32 vertStride = (sizeof(Vector3) + sizeof(Vector2) + sizeof(uint32));
	unsigned char buffer[vertStride * 50];
	for (uint32 index = 0; index < numVerts; ++index)
	{
		uint32 bufferIndex = index * vertStride;
		Vector3& pos = (Vector3&)buffer[bufferIndex];
		pos.x = float(100 + index);
		pos.y = float(200 + index);
		pos.z = float(300 + index);

		Vector2& uv = (Vector2&)buffer[bufferIndex + sizeof(Vector3)];
		uv.x = float(1000 + index);
		uv.y = float(2000 + index);

		uint32& color = (uint32&)buffer[bufferIndex + sizeof(Vector3) + sizeof(Vector2)];
		color = (index*251) << 4;
	}

	// Verify these using format
	VertexFormat srcFormat;
	uint32 stream = srcFormat.addStream();
	srcFormat.addElement( stream, SemanticType::POSITION, StorageType::FLOAT3 );
	srcFormat.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	srcFormat.addElement( stream, SemanticType::COLOR, StorageType::UBYTE4 );

	// check stream stride
	CHECK( VertexFormatTestHelper::checkStreamMetrics( srcFormat, 0, 3, vertStride ));

	VertexFormat::BufferSet srcSet( srcFormat );
	srcSet.buffers_.push_back( VertexFormat::Buffer( &buffer[0], vertStride*numVerts ) );

	// set1: Destination formats
	Vector3 positionsSet[numVerts];
	Vector2 uvsSet[numVerts];
	uint32 colorsSet[numVerts];
	bw_zero_memory( &positionsSet[0], sizeof(positionsSet) );
	bw_zero_memory( &uvsSet[0], sizeof(uvsSet) );
	bw_zero_memory( &colorsSet[0], sizeof(colorsSet) );

	// conversion set 1: reordering
	VertexFormat dstFormat;
	stream = dstFormat.addStream();
	dstFormat.addElement(stream, SemanticType::TEXCOORD, StorageType::FLOAT2);
	stream = dstFormat.addStream();
	dstFormat.addElement(stream, SemanticType::COLOR, StorageType::UBYTE4);
	stream = dstFormat.addStream();
	dstFormat.addElement(stream, SemanticType::POSITION, StorageType::FLOAT3);

	VertexFormat::BufferSet dstSet(dstFormat);
	dstSet.buffers_.push_back( VertexFormat::Buffer( &uvsSet[0], numVerts * sizeof(Vector2) ) );
	dstSet.buffers_.push_back( VertexFormat::Buffer( &colorsSet[0], numVerts * sizeof(uint32) ) );
	dstSet.buffers_.push_back( VertexFormat::Buffer( &positionsSet[0], numVerts * sizeof(Vector3) ) );

	// Perform conversion
	bool result = VertexFormat::convert( dstSet, srcSet );
	CHECK_EQUAL(true, result);


	// check the output
	for (uint32 index = 0; index < numVerts; ++index)
	{
		uint32 bufferIndex = index;
		Vector3& pos = (Vector3&)positionsSet[bufferIndex];
		CHECK_EQUAL(float(100 + index), pos.x);
		CHECK_EQUAL(float(200 + index), pos.y);
		CHECK_EQUAL(float(300 + index), pos.z);

		Vector2& uv = (Vector2&)uvsSet[bufferIndex];
		CHECK_EQUAL(float(1000 + index), uv.x);
		CHECK_EQUAL(float(2000 + index), uv.y);

		uint32& color = (uint32&)colorsSet[bufferIndex];
		CHECK_EQUAL((index*251) << 4, color);
	}

}


TEST( Moo_VertexFormat_Conversion_VertexCount )
{
	const uint32 numVerts = 50;
	const uint32 vertStride = (sizeof(Vector3) + sizeof(Vector2) + sizeof(uint32));
	unsigned char buffer[vertStride * 50];
	for (uint32 index = 0; index < numVerts; ++index)
	{
		uint32 bufferIndex = index * vertStride;
		Vector3& pos = (Vector3&)buffer[bufferIndex];
		pos.x = float(100 + index);
		pos.y = float(200 + index);
		pos.z = float(300 + index);

		Vector2& uv = (Vector2&)buffer[bufferIndex + sizeof(Vector3)];
		uv.x = float(1000 + index);
		uv.y = float(2000 + index);

		uint32& color = (uint32&)buffer[bufferIndex + sizeof(Vector3) + sizeof(Vector2)];
		color = (index*251) << 4;
	}

	// Verify these using format
	VertexFormat srcFormat;
	uint32 stream = srcFormat.addStream();
	srcFormat.addElement( stream, SemanticType::POSITION, StorageType::FLOAT3 );
	srcFormat.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	srcFormat.addElement( stream, SemanticType::COLOR, StorageType::UBYTE4 );

	// check stream stride
	CHECK( VertexFormatTestHelper::checkStreamMetrics( srcFormat, 0, 3, vertStride ) );

	VertexFormat::BufferSet srcSet( srcFormat );
	srcSet.buffers_.push_back( VertexFormat::Buffer( &buffer[0], vertStride*numVerts ) );

	// -------------------------------------------------------------
	// test vertex count mismatch

	const uint32 numVertsTruncated = 20;
	Vector3 positionsSet[numVertsTruncated];
	Vector2 uvsSet[numVertsTruncated];
	uint32 colorsSet[numVertsTruncated];
	bw_zero_memory(&positionsSet[0], sizeof(positionsSet));
	bw_zero_memory(&uvsSet[0], sizeof(uvsSet));
	bw_zero_memory(&colorsSet[0], sizeof(colorsSet));

	// conversion set: reordering
	VertexFormat dstFormat;
	stream = dstFormat.addStream();
	dstFormat.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	stream = dstFormat.addStream();
	dstFormat.addElement( stream, SemanticType::COLOR, StorageType::UBYTE4 );
	stream = dstFormat.addStream();
	dstFormat.addElement( stream, SemanticType::POSITION, StorageType::FLOAT3 );

	VertexFormat::BufferSet dstSet(dstFormat);
	dstSet.buffers_.push_back(VertexFormat::Buffer(&uvsSet[0], numVertsTruncated*sizeof(Vector2)));
	dstSet.buffers_.push_back(VertexFormat::Buffer(&colorsSet[0], numVertsTruncated*sizeof(uint32)));
	dstSet.buffers_.push_back(VertexFormat::Buffer(&positionsSet[0], numVertsTruncated*sizeof(Vector3)));


	// test that the conversion fails due to vertex count mismatch
	bool result = VertexFormat::convert( dstSet, srcSet );
	CHECK_EQUAL(false, result);

}


TEST( Moo_VertexFormat_Conversion1 )
{
	const uint32 numVerts = 50;
	const uint32 vertStride = (sizeof(Vector3) + sizeof(Vector2) + sizeof(uint32));
	unsigned char buffer[vertStride * 50];
	for (uint32 index = 0; index < numVerts; ++index)
	{
		uint32 bufferIndex = index * vertStride;
		Vector3& pos = (Vector3&)buffer[bufferIndex];
		pos.x = float(100 + index);
		pos.y = float(200 + index);
		pos.z = float(300 + index);

		Vector2& uv = (Vector2&)buffer[bufferIndex + sizeof(Vector3)];
		uv.x = float(1000 + index);
		uv.y = float(2000 + index);

		uint32& color = (uint32&)buffer[bufferIndex + sizeof(Vector3) + sizeof(Vector2)];
		color = (index*251) << 4;
	}

	// Verify these using format
	VertexFormat srcFormat;
	uint32 stream = srcFormat.addStream();
	srcFormat.addElement( stream, SemanticType::POSITION, StorageType::FLOAT3 );
	srcFormat.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	srcFormat.addElement( stream, SemanticType::COLOR, StorageType::UBYTE4 );


	// check stream stride
	CHECK( VertexFormatTestHelper::checkStreamMetrics( srcFormat, 0, 3, vertStride ) );

	VertexFormat::BufferSet srcSet( srcFormat );
	srcSet.buffers_.push_back( VertexFormat::Buffer( &buffer[0], vertStride*numVerts ) );

	//----------------------------------------------------------
	// conversion: reordering/striping + type conversion/unpacking

	VertexFormat dstFormat;
	stream = dstFormat.addStream();
	dstFormat.addElement( stream, SemanticType::COLOR, StorageType::FLOAT4 );
	stream = dstFormat.addStream();
	dstFormat.addElement( stream, SemanticType::POSITION, StorageType::FLOAT4 );
	stream = dstFormat.addStream();
	dstFormat.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT1 );

	// Destination buffers
	Vector4 colorsSet[numVerts];
	Vector4 positionsSet[numVerts];
	float uvsSet[numVerts];

	bw_zero_memory( &colorsSet[0], sizeof(colorsSet) );
	bw_zero_memory( &positionsSet[0], sizeof(positionsSet) );
	bw_zero_memory( &uvsSet[0], sizeof(uvsSet) );

	VertexFormat::BufferSet dstSet(dstFormat);
	dstSet.buffers_.push_back( VertexFormat::Buffer(&colorsSet[0], numVerts*sizeof(Vector4)) );
	dstSet.buffers_.push_back( VertexFormat::Buffer(&positionsSet[0], numVerts*sizeof(Vector4)) );
	dstSet.buffers_.push_back( VertexFormat::Buffer(&uvsSet[0], numVerts*sizeof(float)) );

	// Perform conversion
	bool result = VertexFormat::convert( dstSet, srcSet );

	// We check that the defined conversion functions match our expectations 
	// in this unit test, otherwise our conversion value checks will be wrong.

	// COLOR [Ubyte4->Float4]
	VertexConversions::ConversionFunc color_Ubyte4_to_Float4_conv_func = 
		VertexConversions::fetchConvertFunction(
		VertexElement::SemanticType::COLOR, 
		VertexElement::StorageType::FLOAT4, 
		VertexElement::StorageType::UBYTE4 );
	VertexConversions::ConversionFunc color_Ubyte4_to_Float4_conv_func_expected = 
		&VertexConversions::NormalizedComponentConverter::convert<VertexElement::Float4,VertexElement::Ubyte4>;
	CHECK_EQUAL(color_Ubyte4_to_Float4_conv_func, color_Ubyte4_to_Float4_conv_func_expected);

	// POSITION [Float3->Float4]
	VertexConversions::ConversionFunc pos_Float3_to_Float4_conv_func = 
		VertexConversions::fetchConvertFunction(
		VertexElement::SemanticType::POSITION, 
		VertexElement::StorageType::FLOAT4, 
		VertexElement::StorageType::FLOAT3 );
	VertexConversions::ConversionFunc pos_Float3_to_Float4_conv_func_expected = 
		&VertexConversions::positionAddWConversion;
	CHECK_EQUAL(pos_Float3_to_Float4_conv_func, pos_Float3_to_Float4_conv_func_expected);

	// TEXCOORD [Float2->Float1]
	VertexConversions::ConversionFunc tex_Float2_to_Float1_conv_func = 
		VertexConversions::fetchConvertFunction(
		VertexElement::SemanticType::TEXCOORD, 
		VertexElement::StorageType::FLOAT1, 
		VertexElement::StorageType::FLOAT2 );
	VertexConversions::ConversionFunc tex_Float2_to_Float1_conv_func_expected = 
		&VertexConversions::AssignmentComponentConverter::convert<VertexElement::Float1,VertexElement::Float2>;
	CHECK_EQUAL(tex_Float2_to_Float1_conv_func, tex_Float2_to_Float1_conv_func_expected);

	// should have been able to perform this conversions
	CHECK_EQUAL(true, result);

	// Check results of conversion against expected values...
	for (uint32 bufferIndex = 0; bufferIndex < numVerts; ++bufferIndex)
	{
		// color
		VertexElement::Ubyte4 generated_color;
		uint32 original_shifted_color = (bufferIndex*251) << 4;
		generated_color.copyBytesFrom( original_shifted_color );

		// color (D3DCOLOR/Ubyte4 -> Float4)	// unpacking
		const Vector4& color = colorsSet[bufferIndex];
		CHECK_EQUAL(color.x, generated_color.x/255.0f);
		CHECK_EQUAL(color.y, generated_color.y/255.0f);
		CHECK_EQUAL(color.z, generated_color.z/255.0f);
		CHECK_EQUAL(color.w, generated_color.w/255.0f);

		// position (Float3->Float4)	// expansion
		const Vector4& position = positionsSet[bufferIndex];
		CHECK_EQUAL(position.x, float(100+bufferIndex));
		CHECK_EQUAL(position.y, float(200+bufferIndex));
		CHECK_EQUAL(position.z, float(300+bufferIndex));
		CHECK_EQUAL(position.w, 1.0f);

		// texcoord (Float2->Float1)	// truncation
		const float& texcoord = uvsSet[bufferIndex];
		CHECK_EQUAL(texcoord, float(1000+bufferIndex));
	}
}

// test vertex format equality
TEST( Moo_VertexFormat_Equality )
{
	VertexFormat srcFormat;
	uint32 stream = srcFormat.addStream();
	srcFormat.addElement( stream, SemanticType::POSITION, StorageType::FLOAT3 );
	srcFormat.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	srcFormat.addElement( stream, SemanticType::COLOR, StorageType::UBYTE4 );

	VertexFormat dstFormat;
	stream = dstFormat.addStream();
	dstFormat.addElement( stream, SemanticType::COLOR, StorageType::FLOAT4 );
	stream = dstFormat.addStream();
	dstFormat.addElement( stream, SemanticType::POSITION, StorageType::FLOAT4 );
	stream = dstFormat.addStream();
	dstFormat.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT1 );

	VertexFormat srcFormatCopy = srcFormat;
	VertexFormat dstFormatCopy = dstFormat;

	CHECK( !srcFormat.equals( dstFormat ) );
	CHECK( srcFormat == srcFormatCopy );
	CHECK( dstFormat.equals( dstFormatCopy ) );

	srcFormatCopy.addStream();
	CHECK( !srcFormatCopy.equals( srcFormat ) );

	dstFormatCopy.addElement( dstFormat.streamCount() - 1, 
		VertexElement::SemanticType::TEXCOORD,  
		VertexElement::StorageType::FLOAT2 );
	CHECK( !dstFormatCopy.equals( dstFormat ) );

}


TEST( Moo_VertexFormat_Conversion2 )
{
	const uint32 numVerts = 50;
	const float numVertsf = static_cast<float>(numVerts);
	const uint32 srcBuffer0VertStride = (sizeof(VertexElement::Float3)*2 + sizeof(VertexElement::Ubyte3) + sizeof(VertexElement::Float2) + sizeof(VertexElement::Float4));
	unsigned char srcBuffer0[srcBuffer0VertStride * 50];

	for (uint32 index = 0; index < numVerts; ++index)
	{
		uint32 bufferIndex = index * srcBuffer0VertStride;
		VertexElement::Float3& pos = (VertexElement::Float3&)srcBuffer0[bufferIndex];
		pos.x = float(100 + index);
		pos.y = float(200 + index);
		pos.z = float(300 + index);

		Vector3& normal = (Vector3&)srcBuffer0[bufferIndex + sizeof(VertexElement::Float3)];
		normal.x = sinf(1.0f * index/numVertsf) + 0.5f;
		normal.y = cosf(2.0f * index/numVertsf) * 0.5f;
		normal.z = sinf(0.5f * index/numVertsf) + 0.25f;
		normal.normalise();

		VertexElement::Ubyte3& blend = (VertexElement::Ubyte3&)srcBuffer0[bufferIndex + sizeof(VertexElement::Float3)*2];
		blend.x = index;
		blend.y = index * 2;
		blend.z = index * 3;

		VertexElement::Float2& uv = (VertexElement::Float2&)srcBuffer0[bufferIndex + sizeof(VertexElement::Float3)*2 + sizeof(VertexElement::Ubyte3)];
		uv.x = float(1000 + index);
		uv.y = float(2000 + index);

		VertexElement::Float4& color = (VertexElement::Float4&)srcBuffer0[bufferIndex + sizeof(VertexElement::Float3)*2 + sizeof(VertexElement::Ubyte3) + sizeof(VertexElement::Float2)];
		color.r = float(1.0f/(index+1.0f));
		color.g = float(0.9f/(index+2.0f));
		color.b = float(0.8f/(index+3.0f));
		color.a = float(1.0f/(index+4.0f));
	}

	// Verify these using format
	VertexFormat srcFormat;
	uint32 stream = srcFormat.addStream();
	srcFormat.addElement( stream, SemanticType::POSITION, StorageType::FLOAT3 );
	srcFormat.addElement( stream, SemanticType::NORMAL, StorageType::FLOAT3 );
	srcFormat.addElement( stream, SemanticType::BLENDINDICES, StorageType::UBYTE3 );
	srcFormat.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	srcFormat.addElement( stream, SemanticType::COLOR, StorageType::FLOAT4 );


	// check stream stride
	CHECK( VertexFormatTestHelper::checkStreamMetrics( srcFormat, 0, 5, srcBuffer0VertStride ));

	VertexFormat::BufferSet srcSet( srcFormat );
	srcSet.buffers_.push_back( VertexFormat::Buffer( &srcBuffer0[0], srcBuffer0VertStride*numVerts ) );

	//----------------------------------------------------------
	// conversion aspects: 
	//	- reordering semantics
	//	- splitting streams
	//	- omitting streams
	//	- semantic specific type conversions 
	//	- direct copying

	VertexFormat dstFormat;
	stream = dstFormat.addStream();
	dstFormat.addElement( stream, SemanticType::POSITION, StorageType::FLOAT4 );
	dstFormat.addElement( stream, SemanticType::TEXCOORD, StorageType::FLOAT2 );
	dstFormat.addElement( stream, SemanticType::NORMAL, StorageType::UBYTE4 );
	stream = dstFormat.addStream();
	dstFormat.addElement( stream, SemanticType::COLOR, StorageType::UBYTE3 );

	// Destination buffers
	const uint32 dstBuffer0VertStride = (sizeof(Vector4) + sizeof(Vector2) + sizeof(VertexElement::Ubyte4));
	const size_t expected_dstMainVertStride = VertexElement::typeSize( VertexElement::StorageType::FLOAT4 ) +
		VertexElement::typeSize( VertexElement::StorageType::FLOAT2 ) +
		VertexElement::typeSize( VertexElement::StorageType::UBYTE4 );

	const uint32 dstBuffer1VertStride = sizeof(VertexElement::Ubyte3);
	const size_t expected_dstColorVertStride = VertexElement::typeSize( VertexElement::StorageType::UBYTE3 );

	// check our buffer strides
	CHECK_EQUAL( dstBuffer0VertStride, expected_dstMainVertStride );
	CHECK_EQUAL( dstBuffer1VertStride, expected_dstColorVertStride );

	CHECK_EQUAL ( dstBuffer0VertStride, dstFormat.streamStride( 0 ) );
	CHECK_EQUAL ( dstBuffer1VertStride, dstFormat.streamStride( 1 ) );

	CHECK( VertexFormatTestHelper::checkStreamMetrics( dstFormat, 0, 3, dstBuffer0VertStride ) );
	CHECK( VertexFormatTestHelper::checkStreamMetrics( dstFormat, 1, 1, dstBuffer1VertStride ) );

	unsigned char dstBuffer0[dstBuffer0VertStride * numVerts];
	unsigned char dstBuffer1[dstBuffer1VertStride * numVerts];

	bw_zero_memory(&dstBuffer0[0], sizeof(dstBuffer0));
	bw_zero_memory(&dstBuffer1[0], sizeof(dstBuffer1));

	VertexFormat::BufferSet dstSet(dstFormat);
	dstSet.buffers_.push_back( VertexFormat::Buffer(&dstBuffer0[0], sizeof(dstBuffer0)) );
	dstSet.buffers_.push_back( VertexFormat::Buffer(&dstBuffer1[0], sizeof(dstBuffer1)) );

	// -----------------------------------------------------------------------
	// Perform conversion
	bool result = VertexFormat::convert( dstSet, srcSet );

	// We check that the defined conversion functions match our expectations 
	// in this unit test, otherwise our conversion value checks will be wrong.

	// POSITION [Float3->Float4], semantic specific, fill concatenated position.w component with 1.0f 
	VertexConversions::ConversionFunc pos_Float3_to_Float4_conv_func = 
		VertexConversions::fetchConvertFunction(
		VertexElement::SemanticType::POSITION, 
		VertexElement::StorageType::FLOAT4, 
		VertexElement::StorageType::FLOAT3 );
	VertexConversions::ConversionFunc pos_Float3_to_Float4_conv_func_expected = 
		&VertexConversions::positionAddWConversion;
	CHECK_EQUAL(pos_Float3_to_Float4_conv_func, pos_Float3_to_Float4_conv_func_expected);

	// TEXCOORD [Float2->Float2], direct copy
	VertexConversions::ConversionFunc tex_Float2_to_Float2_conv_func = 
		VertexConversions::fetchConvertFunction(
		VertexElement::SemanticType::TEXCOORD, 
		VertexElement::StorageType::FLOAT2, 
		VertexElement::StorageType::FLOAT2 );
	VertexConversions::ConversionFunc tex_Float2_to_Float2_conv_func_expected = &VertexConversions::reinterpretConvert;
	CHECK_EQUAL(tex_Float2_to_Float2_conv_func, tex_Float2_to_Float2_conv_func_expected);

	// NORMAL [Float3->Ubyte4], semantic specific, pack normal using moo_math pack
	VertexConversions::ConversionFunc norm_Float3_to_Ubyte4_conv_func = 
		VertexConversions::fetchConvertFunction(
		VertexElement::SemanticType::NORMAL, 
		VertexElement::StorageType::UBYTE4, 
		VertexElement::StorageType::FLOAT3 );
	VertexConversions::ConversionFunc norm_Float3_to_Ubyte4_conv_func_expected = 
		&VertexConversions::normalPackf3tob4;
	CHECK_EQUAL(norm_Float3_to_Ubyte4_conv_func, norm_Float3_to_Ubyte4_conv_func_expected);

	// COLOR [Float4->Ubyte3], non-semantic specific, packing + truncation, different stream
	VertexConversions::ConversionFunc col_Float4_to_Ubyte3_conv_func = 
		VertexConversions::fetchConvertFunction(
		VertexElement::SemanticType::COLOR, 
		VertexElement::StorageType::UBYTE3, 
		VertexElement::StorageType::FLOAT4 );
	VertexConversions::ConversionFunc col_Float4_to_Ubyte3_conv_func_expected = 
		&VertexConversions::NormalizedComponentConverter::convert<VertexElement::Ubyte3,VertexElement::Float4>;
	CHECK_EQUAL(col_Float4_to_Ubyte3_conv_func, col_Float4_to_Ubyte3_conv_func_expected);

	// should have been able to perform this conversions
	CHECK_EQUAL(true, result);

	// Create Vertex Accessors for destination buffer 0
	// dest buffer POSITION accessors
	const Moo::RawElementAccessor<VertexElement::Float4> positionRawAccessor = 
		dstFormat.createRawElementAccessor<VertexElement::Float4>( &dstBuffer0, sizeof(dstBuffer0), 0, 0 );
	const Moo::ProxyElementAccessor<VertexElement::SemanticType::POSITION> positionProxyAccessor = 
		dstFormat.createProxyElementAccessor<VertexElement::SemanticType::POSITION>( &dstBuffer0, sizeof(dstBuffer0), 0, 0 );
	
	// dest buffer TEXCOORD accessors
	const Moo::RawElementAccessor<VertexElement::Float2> texcoordRawAccessor = 
		dstFormat.createRawElementAccessor<VertexElement::Float2>( &dstBuffer0, sizeof(dstBuffer0), 0, sizeof(VertexElement::Float4) );
	const Moo::ProxyElementAccessor<VertexElement::SemanticType::TEXCOORD> texcoordProxyAccessor = 
		dstFormat.createProxyElementAccessor<VertexElement::SemanticType::TEXCOORD>( &dstBuffer0, sizeof(dstBuffer0), 0, 0 );

	// dest buffer NORMAL accessors
	const Moo::RawElementAccessor<VertexElement::Ubyte4> normalRawAccessor = 
		dstFormat.createRawElementAccessor<VertexElement::Ubyte4>( &dstBuffer0, sizeof(dstBuffer0), 0, sizeof(VertexElement::Float4) + sizeof(VertexElement::Float2) );
	const Moo::ProxyElementAccessor<VertexElement::SemanticType::NORMAL> normalProxyAccessor = 
		dstFormat.createProxyElementAccessor<VertexElement::SemanticType::NORMAL>( &dstBuffer0, sizeof(dstBuffer0), 0, 0 );

	// dest buffer COLOR accessors [2nd buffer]
	const Moo::RawElementAccessor<VertexElement::Ubyte3> colorRawAccessor = 
		dstFormat.createRawElementAccessor<VertexElement::Ubyte3>( &dstBuffer1, sizeof(dstBuffer1), 1, 0 );
	const Moo::ProxyElementAccessor<VertexElement::SemanticType::COLOR> colorProxyAccessor = 
		dstFormat.createProxyElementAccessor<VertexElement::SemanticType::COLOR>( &dstBuffer1, sizeof(dstBuffer1), 1, 0 );

	// Check results of conversion against expected values...
	for (uint32 bufferIndex = 0; bufferIndex < numVerts; ++bufferIndex)
	{

		// POSITION [Float3->Float4], semantic specific, fill concatenated position.w component with 1.0f 
		const VertexElement::Float4& position = positionRawAccessor[bufferIndex];
		VertexElement::Float4 position_prox = positionProxyAccessor[bufferIndex];
		CHECK(position == position_prox);
		CHECK_EQUAL(position.x, float(100+bufferIndex));
		CHECK_EQUAL(position.y, float(200+bufferIndex));
		CHECK_EQUAL(position.z, float(300+bufferIndex));
		CHECK_EQUAL(position.w, 1.0f);

		// TEXCOORD [Float2->Float2], direct copy
		const VertexElement::Float2& texcoord = texcoordRawAccessor[bufferIndex];
		VertexElement::Float2 texcoord_prox = texcoordProxyAccessor[bufferIndex];
		CHECK(texcoord == texcoord_prox);
		CHECK_EQUAL(texcoord.x, float(1000+bufferIndex));
		CHECK_EQUAL(texcoord.y, float(2000+bufferIndex));

		// NORMAL [Float3->Ubyte2], semantic specific, pack normal using moo_math pack
		Vector3 normal_orig;	
		normal_orig.x = sinf( bufferIndex/numVertsf ) + 0.5f;
		normal_orig.y = cosf( 2.0f * bufferIndex/numVertsf ) * 0.5f;
		normal_orig.z = sinf( 0.5f * bufferIndex/numVertsf ) + 0.25f;
		normal_orig.normalise();

			// create packed version
		VertexElement::Ubyte4 normal_expected;
		uint32 packed_normal = Moo::packNormal( normal_orig );
		normal_expected.copyBytesFrom(packed_normal);

			// check
		const VertexElement::Ubyte4& normal = normalRawAccessor[bufferIndex];
		VertexElement::Ubyte4 normal_prox = normalProxyAccessor[bufferIndex];
		CHECK(normal == normal_prox);
		CHECK(normal == normal_expected);
		
		// COLOR [Float4->Ubyte3], non-semantic specific, packing + truncation, different stream
		const VertexElement::Ubyte3& color = colorRawAccessor[bufferIndex];
		VertexElement::Ubyte3 color_prox = colorProxyAccessor[bufferIndex];
		CHECK(color == color_prox);

			// recreate initial value
		VertexElement::Float4 color_expected;
		color_expected.r = float(1.0f/(bufferIndex+1.0f));
		color_expected.g = float(0.9f/(bufferIndex+2.0f));
		color_expected.b = float(0.8f/(bufferIndex+3.0f));
		color_expected.a = float(1.0f/(bufferIndex+4.0f));

			// check against packed version
		CHECK_EQUAL(color.r, static_cast<uint8>(color_expected.r * 255.0f));
		CHECK_EQUAL(color.g, static_cast<uint8>(color_expected.g * 255.0f));
		CHECK_EQUAL(color.b, static_cast<uint8>(color_expected.b * 255.0f));
	}

}

TEST( Moo_VertexFormat_Validation1 )
{
	typedef VertexElement::SemanticType Sem;
	typedef VertexElement::StorageType Stor;

	VertexElement::Float2 a;
	a.x = 1.0f;
	a.y = -0.2f;

	VertexElement::Float2 b;
	b.x = 17.0f;
	b.y = -0.2f;

	VertexConversions::ValidationFunc float2_to_short2_validate = 
		Moo::VertexConversions::fetchValidationFunction(
			Sem::TEXCOORD, Stor::SHORT2, Stor::FLOAT2 );

	bool a_valid = float2_to_short2_validate( &a );
	bool b_valid = float2_to_short2_validate( &b );

	CHECK(a_valid);
	CHECK(!b_valid);

}

TEST( Moo_VertexFormat_Validation2 )
{
	typedef VertexElement::SemanticType Sem;
	typedef VertexElement::StorageType Stor;

	Vector3 a;
	a.x = 1.0f;
	a.y = -0.2f;
	a.z = 0.0f;
	a.normalise();


	VertexElement::Float3 b;
	b.x = 17.0f;
	b.y = -0.2f;
	b.z = 0.01f;

	VertexElement::Float3 c;
	c.x = 0.0f;
	c.y = 0.0f;
	c.z = 0.0f;


	VertexConversions::ValidationFunc norm_to_ubyte4_validate = 
		Moo::VertexConversions::fetchValidationFunction(
		Sem::BINORMAL, Stor::UBYTE4_NORMAL_8_8_8, Stor::FLOAT3 );

	bool a_valid = norm_to_ubyte4_validate( &a );
	bool b_valid = norm_to_ubyte4_validate( &b );
	bool c_valid = norm_to_ubyte4_validate( &c );

	CHECK(a_valid);
	CHECK(!b_valid);
	CHECK(!c_valid);

}

