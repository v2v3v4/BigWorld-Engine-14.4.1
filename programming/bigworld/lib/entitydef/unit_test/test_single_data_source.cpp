#include "pch.hpp"

#include "test_harness.hpp"

#include "entitydef/single_type_data_sources.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/unique_id.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "network/basictypes.hpp"

namespace BW
{

TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_float )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	float input( 0.1f );
	FloatDataSource source( input );

	DataSource * pSource = &source;

	CHECK( pSource->read( floatResult ) );
	CHECK_CLOSE( input, floatResult, std::numeric_limits<float>::epsilon() );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_double )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	double input( 0.1 );
	DoubleDataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK( pSource->read( doubleResult ) );
	CHECK_CLOSE( input, doubleResult, std::numeric_limits<double>::epsilon() );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_uint8 )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	uint8 input( 5 );
	UInt8DataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK( pSource->read( uint8Result ) );
	CHECK_EQUAL( input, uint8Result );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_uint16 )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	uint16 input( 530 );
	UInt16DataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK( pSource->read( uint16Result ) );
	CHECK_EQUAL( input, uint16Result );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_uint32 )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	uint32 input( 75000 );
	UInt32DataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK( pSource->read( uint32Result ) );
	CHECK_EQUAL( input, uint32Result );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_uint64 )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	uint64 input( 1234567768 );
	UInt64DataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK( pSource->read( uint64Result ) );
	CHECK_EQUAL( input, uint64Result );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_int8 )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	int8 input( -5 );
	Int8DataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK( pSource->read( int8Result ) );
	CHECK_EQUAL( input, int8Result );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_int16 )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	int16 input( -700 );
	Int16DataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK( pSource->read( int16Result ) );
	CHECK_EQUAL( input, int16Result );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_int32 )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	int32 input( -450000 );
	Int32DataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK( pSource->read( int32Result ) );
	CHECK_EQUAL( input, int32Result );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_int64 )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	int64 input( -987453353 );
	Int64DataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK( pSource->read( int64Result ) );
	CHECK_EQUAL( input, int64Result );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_string )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	BW::string input( "Test String" );
	StringDataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK( pSource->read( stringResult ) );
	CHECK_EQUAL( input, stringResult );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_wstring )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	BW::wstring input( L"Test wstring" );
	WStringDataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK( pSource->read( wstringResult ) );
	CHECK( input == wstringResult );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_Vector2 )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	Vector2 input( 15, 17 );
	Vector2DataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK( pSource->read( vector2Result ) );
	CHECK_EQUAL( input, vector2Result );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_Vector3 )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	Vector3 input( 15, 17, -9 );
	Vector3DataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK( pSource->read( vector3Result ) );
	CHECK_EQUAL( input, vector3Result );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_Vector4 )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED
	
	Vector4 input( 15, 17, -9, 5000 );
	Vector4DataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK( pSource->read( vector4Result ) );
	CHECK_EQUAL( input, vector4Result );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}

#if defined MF_SERVER
TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_EntityMailBoxRef )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
	EntityMailBoxRef entityMailBoxRefResult;
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	EntityMailBoxRef input;
	input.init( 400, Mercury::Address( 0x0a141e28, 5432 ),
		EntityMailBoxRef::CLIENT, 4 );
	EntityMailBoxRefDataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
	CHECK( pSource->read( entityMailBoxRefResult ) );
	CHECK_EQUAL( input.id, entityMailBoxRefResult.id );
	CHECK_EQUAL( input.addr, entityMailBoxRefResult.addr );
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}
#endif // defined MF_SERVER

TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_UniqueID )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	UniqueID input( 0x16, 0x400, 0x695, 0x4 );
	UniqueIDDataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK( pSource->read( uniqueIDResult ) );
	CHECK( input == uniqueIDResult );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_blob )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	BW::string input( "Test String" );
	BlobDataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK_EQUAL( false, pSource->read( uint8Result ) );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK( pSource->readBlob( stringResult ) );
	CHECK_EQUAL( input, stringResult );
}


TEST_F( EntityDefUnitTestHarness, SingleTypeDataSourceT_bool )
{
	float floatResult;
	double doubleResult;
	uint8 uint8Result;
	uint16 uint16Result;
	uint32 uint32Result;
	uint64 uint64Result;
	int8 int8Result;
	int16 int16Result;
	int32 int32Result;
	int64 int64Result;
	BW::string stringResult;
	BW::wstring wstringResult;
	Vector2 vector2Result;
	Vector3 vector3Result;
	Vector4 vector4Result;
#if defined MF_SERVER
	EntityMailBoxRef entityMailBoxRefResult;
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	UniqueID uniqueIDResult;
#endif // !defined EDITOR_ENABLED

	bool input( true );
	BoolDataSource source( input );

	DataSource * pSource = &source;

	CHECK_EQUAL( false, pSource->read( floatResult ) );
	CHECK_EQUAL( false, pSource->read( doubleResult ) );
	CHECK( pSource->read( uint8Result ) );
	CHECK_EQUAL( input ? 1 : 0, uint8Result );
	CHECK_EQUAL( false, pSource->read( uint16Result ) );
	CHECK_EQUAL( false, pSource->read( uint32Result ) );
	CHECK_EQUAL( false, pSource->read( uint64Result ) );
	CHECK_EQUAL( false, pSource->read( int8Result ) );
	CHECK_EQUAL( false, pSource->read( int16Result ) );
	CHECK_EQUAL( false, pSource->read( int32Result ) );
	CHECK_EQUAL( false, pSource->read( int64Result ) );
	CHECK_EQUAL( false, pSource->read( stringResult ) );
	CHECK_EQUAL( false, pSource->read( wstringResult ) );
	CHECK_EQUAL( false, pSource->read( vector2Result ) );
	CHECK_EQUAL( false, pSource->read( vector3Result ) );
	CHECK_EQUAL( false, pSource->read( vector4Result ) );
#if defined MF_SERVER
	CHECK_EQUAL( false, pSource->read( entityMailBoxRefResult ) );
#endif // defined MF_SERVER
#if !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->read( uniqueIDResult ) );
#endif // !defined EDITOR_ENABLED
	CHECK_EQUAL( false, pSource->readBlob( stringResult ) );
}


} // namespace BW

// test_single_data_source.cpp
