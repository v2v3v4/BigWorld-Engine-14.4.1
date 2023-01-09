#include "pch.hpp"

#include "test_harness.hpp"

#include "entitydef/single_type_data_sinks.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/unique_id.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "network/basictypes.hpp"

namespace BW
{
TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_float )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	FloatDataSink sink;

	DataSink * pSink = &sink;

	CHECK( pSink->write( floatInput ) );
	CHECK_CLOSE( floatInput, sink.getValue(),
		std::numeric_limits<float>::epsilon() );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_double )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	DoubleDataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK( pSink->write( doubleInput ) );
	CHECK_CLOSE( doubleInput, sink.getValue(),
		std::numeric_limits<double>::epsilon() );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_uint8 )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	UInt8DataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK( pSink->write( uint8Input ) );
	CHECK_EQUAL( uint8Input, sink.getValue() );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_uint16 )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	UInt16DataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK( pSink->write( uint16Input ) );
	CHECK_EQUAL( uint16Input, sink.getValue() );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_uint32 )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	UInt32DataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK( pSink->write( uint32Input ) );
	CHECK_EQUAL( uint32Input, sink.getValue() );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_uint64 )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	UInt64DataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK( pSink->write( uint64Input ) );
	CHECK_EQUAL( uint64Input, sink.getValue() );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_int8 )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	Int8DataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK( pSink->write( int8Input ) );
	CHECK_EQUAL( int8Input, sink.getValue() );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_int16 )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	Int16DataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK( pSink->write( int16Input ) );
	CHECK_EQUAL( int16Input, sink.getValue() );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_int32 )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	Int32DataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK( pSink->write( int32Input ) );
	CHECK_EQUAL( int32Input, sink.getValue() );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_int64 )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	Int64DataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK( pSink->write( int64Input ) );
	CHECK_EQUAL( int64Input, sink.getValue() );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_string )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	StringDataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK( pSink->write( stringInput ) );
	CHECK_EQUAL( stringInput, sink.getValue() );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_wstring )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	WStringDataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK( pSink->write( wstringInput ) );
	CHECK( wstringInput == sink.getValue());
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_Vector2 )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	Vector2DataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK( pSink->write( vector2Input ) );
	CHECK_EQUAL( vector2Input, sink.getValue() );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_Vector3 )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	Vector3DataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK( pSink->write( vector3Input ) );
	CHECK_EQUAL( vector3Input, sink.getValue() );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_Vector4 )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED
	
	Vector4DataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK( pSink->write( vector4Input ) );
	CHECK_EQUAL( vector4Input, sink.getValue() );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}

#if defined MF_SERVER
TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_EntityMailBoxRef )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	EntityMailBoxRefDataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
	CHECK( pSink->write( entityMailBoxRefInput ) );
	CHECK_EQUAL( entityMailBoxRefInput.id, sink.getValue().id );
	CHECK_EQUAL( entityMailBoxRefInput.addr, sink.getValue().addr );
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}
#endif // defined MF_SERVER

TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_UniqueID )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	UniqueIDDataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK( pSink->write( uniqueIDInput ) );
	CHECK( uniqueIDInput == sink.getValue() );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_blob )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	BlobDataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK_EQUAL( false, pSink->write( uint8Input ) );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK( pSink->writeBlob( stringInput ) );
	CHECK_EQUAL( stringInput, sink.getValue() );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSinkT_bool )
{
	float floatInput( 0.1f );
	double doubleInput( 0.1 );
	uint8 uint8Input( 5 );
	uint16 uint16Input( 530 );
	uint32 uint32Input( 75000 );
	uint64 uint64Input( 1234567768 );
	int8 int8Input( -5 );
	int16 int16Input( -700 );
	int32 int32Input( -450000 );
	int64 int64Input( -987453353 );
	BW::string stringInput( "Test String" );
	BW::wstring wstringInput( L"Test wstring" );
	Vector2 vector2Input( 15, 17 );
	Vector3 vector3Input( 15, 17, -9 );
	Vector4 vector4Input( 15, 17, -9, 5000 );
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefInput;
	entityMailBoxRefInput.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDInput( 0x16, 0x400, 0x695, 0x4 );
#endif // !defined EDITOR_ENABLED

	BoolDataSink sink;

	DataSink * pSink = &sink;

	CHECK_EQUAL( false, pSink->write( floatInput ) );
	CHECK_EQUAL( false, pSink->write( doubleInput ) );
	CHECK( pSink->write( uint8Input ) );
	CHECK_EQUAL( uint8Input != 0, sink.getValue() );
	CHECK_EQUAL( false, pSink->write( uint16Input ) );
	CHECK_EQUAL( false, pSink->write( uint32Input ) );
	CHECK_EQUAL( false, pSink->write( uint64Input ) );
	CHECK_EQUAL( false, pSink->write( int8Input ) );
	CHECK_EQUAL( false, pSink->write( int16Input ) );
	CHECK_EQUAL( false, pSink->write( int32Input ) );
	CHECK_EQUAL( false, pSink->write( int64Input ) );
	CHECK_EQUAL( false, pSink->write( stringInput ) );
	CHECK_EQUAL( false, pSink->write( wstringInput ) );
	CHECK_EQUAL( false, pSink->write( vector2Input ) );
	CHECK_EQUAL( false, pSink->write( vector3Input ) );
	CHECK_EQUAL( false, pSink->write( vector4Input ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSink->write( entityMailBoxRefInput ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->write( uniqueIDInput ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSink->writeBlob( stringInput ) );
}


} // namespace BW

// test_single_data_sink.cpp
