#include "pch.hpp"

#include "test_harness.hpp"

#if defined( MF_SERVER )
#include "unittest_mailbox.hpp"
#include "entitydef/mailbox_base.hpp"
#endif // defined( MF_SERVER )

#include "entitydef/data_type.hpp"
#include "entitydef/script_data_source.hpp"

#if defined( SCRIPT_PYTHON )
#include "entitydef/data_instances/array_data_instance.hpp"
#include "entitydef/data_instances/class_data_instance.hpp"
#include "entitydef/data_instances/fixed_dict_data_instance.hpp"
#endif // defined( SCRIPT_PYTHON )

#if defined( SCRIPT_PYTHON )
#include "entitydef/data_types/array_data_type.hpp"
#include "entitydef/data_types/class_data_type.hpp"
#include "entitydef/data_types/fixed_dict_data_type.hpp"
#include "entitydef/data_types/user_data_type.hpp"
#endif // defined( SCRIPT_PYTHON )

#if defined( SCRIPT_PYTHON )
#include "chunk/user_data_object.hpp"
#include "chunk/user_data_object_type.hpp"
#endif // defined( SCRIPT_PYTHON )

#include "cstdmf/memory_stream.hpp"
#include "cstdmf/unique_id.hpp"

#include "resmgr/xml_section.hpp"

namespace BW
{

TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_float )
{
	ScriptFloat inputObject = ScriptFloat::create( 5. );
	float result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK( isEqual( (float)5., result ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_double )
{
	ScriptFloat inputObject = ScriptFloat::create( 8. );
	double result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK( isEqual( (double)8., result ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_uint8 )
{
	ScriptInt inputObject = ScriptInt::create( 17 );
	uint8 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK_EQUAL( 17, result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_uint16 )
{
	ScriptInt inputObject = ScriptInt::create( 27 );
	uint16 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK_EQUAL( 27, result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_uint32 )
{
	ScriptInt inputObject = ScriptInt::create( 47 );
	uint32 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK_EQUAL( (uint32)47, result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_uint64 )
{
	ScriptInt inputObject = ScriptInt::create( 87 );
	uint64 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK_EQUAL( (uint64)87, result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_int8 )
{
	ScriptInt inputObject = ScriptInt::create( 97 );
	int8 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK_EQUAL( 97, result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_int16 )
{
	ScriptInt inputObject = ScriptInt::create( 107 );
	int16 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK_EQUAL( 107, result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_int32 )
{
	ScriptInt inputObject = ScriptInt::create( 207 );
	int32 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK_EQUAL( 207, result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_int64 )
{
	ScriptInt inputObject = ScriptInt::create( 407 );
	int64 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK_EQUAL( 407, result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_string )
{
	ScriptString inputObject = ScriptString::create( "Test string" );
	BW::string result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK_EQUAL( BW::string( "Test string" ), result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_wstring )
{
	// Characters taken from our Python unicode output test:
	// Chinese character: 0x8FD9, UTF-8 e8 bf 99
	// Japanese "HIRAGANA LETTER KO": 0x3053, UTF-8 e3 81 93
	// Russian "CYRILLIC CAPITAL LETTER E": 0x42D, UTF-8 d0 ad
	BW::wstring input( L"\x8fd9\x3053\x042d" );
	ScriptObject inputObject = ScriptObject::createFrom( input );
	BW::wstring result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	// No CHECK_EQUAL for BW::wstring
	CHECK_EQUAL( 3U, result.size() );
	CHECK_EQUAL( L'\x8fd9', result[0] );
	CHECK_EQUAL( L'\x3053', result[1] );
	CHECK_EQUAL( L'\x042d', result[2] );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_wstring_from_utf8 )
{
	// Characters taken from our Python unicode output test:
	// Chinese character: 0x8FD9, UTF-8 e8 bf 99
	// Japanese "HIRAGANA LETTER KO": 0x3053, UTF-8 e3 81 93
	// Russian "CYRILLIC CAPITAL LETTER E": 0x42D, UTF-8 d0 ad
	ScriptString inputObject = ScriptString::create( "\xe8\xbf\x99\xe3\x81\x93\xd0\xad" );
	BW::wstring result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	// No CHECK_EQUAL for BW::wstring
	CHECK_EQUAL( 3U, result.size() );
	CHECK_EQUAL( L'\x8fd9', result[0] );
	CHECK_EQUAL( L'\x3053', result[1] );
	CHECK_EQUAL( L'\x042d', result[2] );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Vector2 )
{
	Vector2 input( 5.f, 12.f );
	// Produces a PyVector< Vector2 > under SCRIPT_PYTHON
	ScriptObject inputObject = ScriptObject::createFrom( input );
	Vector2 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK( isEqual( input.x, result.x ) );
	CHECK( isEqual( input.y, result.y ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Vector2_from_tuple )
{
	ScriptTuple inputObject = ScriptTuple::create( 2 );
	inputObject.setItem( 0, ScriptFloat::create( 5.f ) );
	inputObject.setItem( 1, ScriptFloat::create( 12.f ) );
	Vector2 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK( isEqual( 5.f, result.x ) );
	CHECK( isEqual( 12.f, result.y ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Vector2_from_list )
{
	ScriptList inputObject = ScriptList::create( 2 );
	inputObject.setItem( 0, ScriptFloat::create( 5.f ) );
	inputObject.setItem( 1, ScriptFloat::create( 12.f ) );
	Vector2 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK( isEqual( 5.f, result.x ) );
	CHECK( isEqual( 12.f, result.y ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Vector3 )
{
	Vector3 input( 5.f, 12.f, 1200.f );
	// Produces a PyVector< Vector3 > under SCRIPT_PYTHON
	ScriptObject inputObject = ScriptObject::createFrom( input );
	Vector3 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK( isEqual( input.x, result.x ) );
	CHECK( isEqual( input.y, result.y ) );
	CHECK( isEqual( input.z, result.z ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Vector3_from_tuple )
{
	ScriptTuple inputObject = ScriptTuple::create( 3 );
	inputObject.setItem( 0, ScriptFloat::create( 5.f ) );
	inputObject.setItem( 1, ScriptFloat::create( 12.f ) );
	inputObject.setItem( 2, ScriptFloat::create( 1200.f ) );
	Vector3 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK( isEqual( 5.f, result.x ) );
	CHECK( isEqual( 12.f, result.y ) );
	CHECK( isEqual( 1200.f, result.z ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Vector3_from_list )
{
	ScriptList inputObject = ScriptList::create( 3 );
	inputObject.setItem( 0, ScriptFloat::create( 5.f ) );
	inputObject.setItem( 1, ScriptFloat::create( 12.f ) );
	inputObject.setItem( 2, ScriptFloat::create( 1200.f ) );
	Vector3 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK( isEqual( 5.f, result.x ) );
	CHECK( isEqual( 12.f, result.y ) );
	CHECK( isEqual( 1200.f, result.z ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Vector4 )
{
	Vector4 input( 5.f, 12.f, 1200.f, -14.5f );
	// Produces a PyVector< Vector4 > under SCRIPT_PYTHON
	ScriptObject inputObject = ScriptObject::createFrom( input );
	Vector4 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK( isEqual( input.x, result.x ) );
	CHECK( isEqual( input.y, result.y ) );
	CHECK( isEqual( input.z, result.z ) );
	CHECK( isEqual( input.w, result.w ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Vector4_from_tuple )
{
	ScriptTuple inputObject = ScriptTuple::create( 4 );
	inputObject.setItem( 0, ScriptFloat::create( 5.f ) );
	inputObject.setItem( 1, ScriptFloat::create( 12.f ) );
	inputObject.setItem( 2, ScriptFloat::create( 1200.f ) );
	inputObject.setItem( 3, ScriptFloat::create( -14.5f ) );
	Vector4 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK( isEqual( 5.f, result.x ) );
	CHECK( isEqual( 12.f, result.y ) );
	CHECK( isEqual( 1200.f, result.z ) );
	CHECK( isEqual( -14.5f, result.w ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Vector4_from_list )
{
	ScriptList inputObject = ScriptList::create( 4 );
	inputObject.setItem( 0, ScriptFloat::create( 5.f ) );
	inputObject.setItem( 1, ScriptFloat::create( 12.f ) );
	inputObject.setItem( 2, ScriptFloat::create( 1200.f ) );
	inputObject.setItem( 3, ScriptFloat::create( -14.5f ) );
	Vector4 result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK( isEqual( 5.f, result.x ) );
	CHECK( isEqual( 12.f, result.y ) );
	CHECK( isEqual( 1200.f, result.z ) );
	CHECK( isEqual( -14.5f, result.w ) );
}


#if defined( MF_SERVER )
TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_MailBox )
{
	ScriptObjectPtr< UnittestMailBox > pInputObject(
		new UnittestMailBox(), ScriptObject::FROM_NEW_REFERENCE );
	pInputObject->address( Mercury::Address( 0xc0180010, 0x4567 ) );
	pInputObject->id( 2405 );
	pInputObject->component( EntityMailBoxRef::BASE_VIA_CELL );
	pInputObject->typeID( 7 );
	CHECK_EQUAL( 0xc0180010, pInputObject->address().ip );
	CHECK_EQUAL( 0x4567, pInputObject->address().port );
	CHECK_EQUAL( (EntityID)2405, pInputObject->id() );
	CHECK_EQUAL( EntityMailBoxRef::BASE_VIA_CELL, pInputObject->component() );
	CHECK_EQUAL( (EntityTypeID)7, pInputObject->typeID() );

	EntityMailBoxRef result;
	ScriptDataSource source( pInputObject );
	CHECK( source.read( result ) );
	CHECK_EQUAL( 0xc0180010, result.addr.ip );
	CHECK_EQUAL( 0x4567, result.addr.port );
	CHECK_EQUAL( (EntityID)2405, result.id );
	CHECK_EQUAL( EntityMailBoxRef::BASE_VIA_CELL, result.component() );
	CHECK_EQUAL( (EntityTypeID)7, result.type() );
}
#endif // defined( MF_SERVER )


#if !defined( EDITOR_ENABLED )
TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_UDO_REF )
{
	UniqueID input( 0x40U, 0x80U, 0x400U, 0x1200U );
	CHECK_EQUAL( 0x40U, input.getA() );
	CHECK_EQUAL( 0x80U, input.getB() );
	CHECK_EQUAL( 0x400U, input.getC() );
	CHECK_EQUAL( 0x1200U, input.getD() );
#if defined( SCRIPT_PYTHON )
	// This should surely be part of UserDataObjectType::init()?
	ScriptModule BWModule =
		ScriptModule::import( "BigWorld", ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( BWModule.exists() );
	CHECK( BWModule.addObject( "UserDataObject",
		&UserDataObject::s_type_, ScriptErrorPrint() ) );
	// Should this be part of the fixture?
	CHECK( UserDataObjectType::init() );

	ScriptObject inputObject( UserDataObject::createRef( input ) );
#else // defined( SCRIPT_PYTHON )
	ScriptString inputObject =
		ScriptBlob::create( input.toString() );
#endif // defined( SCRIPT_PYTHON )

	UniqueID result;
	ScriptDataSource source( inputObject );
	CHECK( source.read( result ) );
	CHECK_EQUAL( 0x40U, result.getA() );
	CHECK_EQUAL( 0x80U, result.getB() );
	CHECK_EQUAL( 0x400U, result.getC() );
	CHECK_EQUAL( 0x1200U, result.getD() );
}
#endif // !defined( EDITOR_ENABLED )


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_None )
{
	bool isNone;
	ScriptDataSource source( ScriptObject::none() );
	CHECK( source.readNone( isNone ) );
	CHECK_EQUAL( true, isNone );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_notNone )
{
	ScriptString inputObject = ScriptString::create( "Test string" );
	bool isNone;
	BW::string result;
	ScriptDataSource source( inputObject );
	CHECK( source.readNone( isNone ) );
	CHECK_EQUAL( false, isNone );
	CHECK( source.read( result ) );
	CHECK_EQUAL( BW::string( "Test string" ), result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_blob )
{
	ScriptBlob inputObject = ScriptBlob::create( "Test blob" );
	BW::string result;
	ScriptDataSource source( inputObject );
	CHECK( source.readBlob( result ) );
	CHECK_EQUAL( BW::string( "Test blob" ), result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Array )
{
	ScriptList inputObject = ScriptList::create( 5 );
	inputObject.setItem( 0, ScriptInt::create( 5 ) );
	inputObject.setItem( 1, ScriptInt::create( 8 ) );
	inputObject.setItem( 2, ScriptInt::create( 12 ) );
	inputObject.setItem( 3, ScriptInt::create( 127 ) );
	inputObject.setItem( 4, ScriptInt::create( -127 ) );

	size_t count;
	int8 result;
	ScriptDataSource source( inputObject );
	CHECK( source.beginSequence( count ) );
	CHECK_EQUAL( (size_t)5, count );
	CHECK( source.enterItem( 0 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 5, result );
	CHECK( source.enterItem( 1 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 8, result );
	CHECK( source.enterItem( 2 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 12, result );
	CHECK( source.enterItem( 3 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 127, result );
	CHECK( source.enterItem( 4 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( -127, result );
}


#if defined( SCRIPT_PYTHON )
TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_PyArray_of_int8 )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> ARRAY <size> 5 </size> <of> INT8 </of> </type>" );
	CHECK( pType );

	ArrayDataType * pArrayType = dynamic_cast< ArrayDataType * >( pType.get() );
	CHECK( pArrayType );

	ScriptObjectPtr< PyArrayDataInstance > pArray(
		new PyArrayDataInstance( pArrayType, 5 ),
		ScriptObject::FROM_NEW_REFERENCE );
	CHECK( pArray.exists() );

	pArray->setInitialItem( 0, ScriptInt::create( 5 ) );
	pArray->setInitialItem( 1, ScriptInt::create( 8 ) );
	pArray->setInitialItem( 2, ScriptInt::create( 12 ) );
	pArray->setInitialItem( 3, ScriptInt::create( 127 ) );
	pArray->setInitialItem( 4, ScriptInt::create( -127 ) );

	size_t count;
	int8 result;
	ScriptDataSource source( pArray );
	CHECK( source.beginSequence( count ) );
	CHECK_EQUAL( (size_t)5, count );
	CHECK( source.enterItem( 0 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 5, result );
	CHECK( source.enterItem( 1 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 8, result );
	CHECK( source.enterItem( 2 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 12, result );
	CHECK( source.enterItem( 3 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 127, result );
	CHECK( source.enterItem( 4 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( -127, result );
}
#endif // defined( SCRIPT_PYTHON )


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Tuple )
{
	ScriptTuple inputObject = ScriptTuple::create( 5 );
	inputObject.setItem( 0, ScriptInt::create( 5 ) );
	inputObject.setItem( 1, ScriptInt::create( 8 ) );
	inputObject.setItem( 2, ScriptInt::create( 12 ) );
	inputObject.setItem( 3, ScriptInt::create( 127 ) );
	inputObject.setItem( 4, ScriptInt::create( -127 ) );

	size_t count;
	int8 result;
	ScriptDataSource source( inputObject );
	CHECK( source.beginSequence( count ) );
	CHECK_EQUAL( (size_t)5, count );
	CHECK( source.enterItem( 0 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 5, result );
	CHECK( source.enterItem( 1 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 8, result );
	CHECK( source.enterItem( 2 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 12, result );
	CHECK( source.enterItem( 3 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 127, result );
	CHECK( source.enterItem( 4 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( -127, result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Array_of_Tuple )
{
	ScriptList inputObject = ScriptList::create( 2 );

	ScriptTuple innerTuple = ScriptTuple::create( 2 );
	innerTuple.setItem( 0 , ScriptInt::create( 40 ) );
	innerTuple.setItem( 1 , ScriptInt::create( 80 ) );
	inputObject.setItem( 0, innerTuple );

	innerTuple = ScriptTuple::create( 1 );
	innerTuple.setItem( 0 , ScriptInt::create( 120 ) );
	inputObject.setItem( 1, innerTuple );


	size_t count;
	int8 result;
	ScriptDataSource source( inputObject );
	CHECK( source.beginSequence( count ) );
	CHECK_EQUAL( (size_t)2, count );
	CHECK( source.enterItem( 0 ) );

	CHECK( source.beginSequence( count ) );
	CHECK_EQUAL( (size_t)2, count );
	CHECK( source.enterItem( 0 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 40, result );
	CHECK( source.enterItem( 1 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 80, result );

	CHECK( source.leaveItem() );
	CHECK( source.enterItem( 1 ) );

	CHECK( source.beginSequence( count ) );
	CHECK_EQUAL( (size_t)1, count );
	CHECK( source.enterItem( 0 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 120, result );

	CHECK( source.leaveItem() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Tuple_of_Array )
{
	ScriptTuple inputObject = ScriptTuple::create( 2 );

	ScriptList innerList = ScriptList::create( 2 );
	innerList.setItem( 0 , ScriptInt::create( 45 ) );
	innerList.setItem( 1 , ScriptInt::create( 85 ) );
	inputObject.setItem( 0, innerList );

	innerList = ScriptList::create( 1 );
	innerList.setItem( 0 , ScriptInt::create( 125 ) );
	inputObject.setItem( 1, innerList );


	size_t count;
	int8 result;
	ScriptDataSource source( inputObject );
	CHECK( source.beginSequence( count ) );
	CHECK_EQUAL( (size_t)2, count );
	CHECK( source.enterItem( 0 ) );

	CHECK( source.beginSequence( count ) );
	CHECK_EQUAL( (size_t)2, count );
	CHECK( source.enterItem( 0 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 45, result );
	CHECK( source.enterItem( 1 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 85, result );

	CHECK( source.leaveItem() );
	CHECK( source.enterItem( 1 ) );

	CHECK( source.beginSequence( count ) );
	CHECK_EQUAL( (size_t)1, count );
	CHECK( source.enterItem( 0 ) );
	CHECK( source.read( result ) );
	CHECK( source.leaveItem() );
	CHECK_EQUAL( 125, result );

	CHECK( source.leaveItem() );
}


#if defined( SCRIPT_PYTHON )
TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_PyClass )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> CLASS <Properties> "
			" <int8_5> <Type> INT8 </Type> </int8_5> "
			" <str_TestString> <Type> STRING </Type> </str_TestString> "
		"</Properties> <AllowNone> false </AllowNone> </type>" );
	CHECK( pType );

	ClassDataType * pClassType = dynamic_cast< ClassDataType * >(
		pType.get() );
	CHECK( pClassType );

	PyClassDataInstancePtr pClass(
		new PyClassDataInstance( pClassType ),
		PyClassDataInstancePtr::FROM_NEW_REFERENCE );
	CHECK( pClass.exists() );

	int index;
	index = pClassType->fieldIndexFromName( "int8_5" );
	CHECK_EQUAL( 0, index );
	pClass->setFieldValue( index, ScriptObject::createFrom( 5 ) );
	index = pClassType->fieldIndexFromName( "str_TestString" );
	CHECK_EQUAL( 1, index );
	pClass->setFieldValue( index,
		ScriptObject::createFrom( BW::string( "TestString" ) ) );

	ScriptDataSource source( pClass );
	CHECK( source.beginClass() );

	int8 firstResult;
	CHECK( source.enterField( "int8_5" ) );
	CHECK( source.read( firstResult ) );
	CHECK( source.leaveField() );

	BW::string secondResult;
	CHECK( source.enterField( "str_TestString" ) );
	CHECK( source.read( secondResult ) );
	CHECK( source.leaveField() );

	CHECK_EQUAL( 5, firstResult );
	CHECK_EQUAL( BW::string( "TestString" ), secondResult );
}
#endif // defined( SCRIPT_PYTHON )


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_PyClass_from_mapping )
{
	// Tests a specific behaviour of the ClassDataType. If given a Mapping that
	// isn't a PyClassDataInstance, then it will read the fields of the mapping
	// rather than its attributes.
	ScriptDict inputObject = ScriptDict::create( 2 );
	RETURN_ON_FAIL_CHECK( inputObject.exists() );
	CHECK( inputObject.setItem( "int8_5", ScriptObject::createFrom( (int8)5 ), 
		ScriptErrorPrint() ) );
	CHECK( inputObject.setItem( "str_TestString",
			ScriptObject::createFrom( BW::string( "TestString" ) ),
		ScriptErrorPrint() ) );

	ScriptDataSource source( inputObject );
	CHECK( source.beginClass() );

	int8 firstResult;
	CHECK( source.enterField( "int8_5" ) );
	CHECK( source.read( firstResult ) );
	CHECK( source.leaveField() );

	BW::string secondResult;
	CHECK( source.enterField( "str_TestString" ) );
	CHECK( source.read( secondResult ) );
	CHECK( source.leaveField() );

	CHECK_EQUAL( 5, firstResult );
	CHECK_EQUAL( BW::string( "TestString" ), secondResult );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_Dict )
{
	ScriptDict inputObject = ScriptDict::create( 2 );
	CHECK( inputObject.setItem( "int8_5", ScriptObject::createFrom( (int8)5 ), 
		ScriptErrorPrint() ) );
	CHECK( inputObject.setItem( "str_TestString",
			ScriptObject::createFrom( BW::string( "TestString" ) ),
		ScriptErrorPrint() ) );

	ScriptDataSource source( inputObject );
	CHECK( source.beginDictionary( NULL ) );

	int8 firstResult;
	CHECK( source.enterField( "int8_5" ) );
	CHECK( source.read( firstResult ) );
	CHECK( source.leaveField() );

	BW::string secondResult;
	CHECK( source.enterField( "str_TestString" ) );
	CHECK( source.read( secondResult ) );
	CHECK( source.leaveField() );

	CHECK_EQUAL( 5, firstResult );
	CHECK_EQUAL( BW::string( "TestString" ), secondResult );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_FixedDict_from_dict )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> FIXED_DICT <Properties> "
		" <int8_5> <Type> INT8 </Type> </int8_5> "
		" <str_TestString> <Type> STRING </Type> </str_TestString> "
		"</Properties> <AllowNone> false </AllowNone> </type>" );
	CHECK( pType );

	FixedDictDataType * pFixedDictType = dynamic_cast< FixedDictDataType * >(
		pType.get() );
	CHECK( pFixedDictType );

	ScriptDict inputObject = ScriptDict::create( 2 );
	RETURN_ON_FAIL_CHECK( inputObject.exists() );
	CHECK( inputObject.setItem( "int8_5", ScriptObject::createFrom( (int8)5 ), 
		ScriptErrorPrint() ) );
	CHECK( inputObject.setItem( "str_TestString",
		ScriptObject::createFrom( BW::string( "TestString" ) ),
		ScriptErrorPrint() ) );

	ScriptDataSource source( inputObject );
	CHECK( source.beginDictionary( pFixedDictType ) );

	int8 firstResult;
	CHECK( source.enterField( "int8_5" ) );
	CHECK( source.read( firstResult ) );
	CHECK( source.leaveField() );

	BW::string secondResult;
	CHECK( source.enterField( "str_TestString" ) );
	CHECK( source.read( secondResult ) );
	CHECK( source.leaveField() );

	CHECK_EQUAL( 5, firstResult );
	CHECK_EQUAL( BW::string( "TestString" ), secondResult );
}


#if defined( SCRIPT_PYTHON )
TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_FixedDict_from_PyFixedDictDataInstance )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> FIXED_DICT <Properties> "
			" <int8_5> <Type> INT8 </Type> </int8_5> "
			" <str_TestString> <Type> STRING </Type> </str_TestString> "
		"</Properties> <AllowNone> false </AllowNone> </type>" );
	CHECK( pType );

	FixedDictDataType * pFixedDictType = dynamic_cast< FixedDictDataType * >(
		pType.get() );
	CHECK( pFixedDictType );

	PyFixedDictDataInstancePtr pFixedDict(
		new PyFixedDictDataInstance( pFixedDictType ),
		PyFixedDictDataInstancePtr::FROM_NEW_REFERENCE );
	CHECK( pFixedDict.exists() );

	int index;
	index = pFixedDictType->getFieldIndex( "int8_5" );
	CHECK_EQUAL( 0, index );
	pFixedDict->initFieldValue( index, ScriptObject::createFrom( 5 ) );
	index = pFixedDictType->getFieldIndex( "str_TestString" );
	CHECK_EQUAL( 1, index );
	pFixedDict->initFieldValue( index,
		ScriptObject::createFrom( BW::string( "TestString" ) ) );

	ScriptDataSource source( pFixedDict );
	CHECK( source.beginDictionary( pFixedDictType ) );

	int8 firstResult;
	CHECK( source.enterField( "int8_5" ) );
	CHECK( source.read( firstResult ) );
	CHECK( source.leaveField() );

	BW::string secondResult;
	CHECK( source.enterField( "str_TestString" ) );
	CHECK( source.read( secondResult ) );
	CHECK( source.leaveField() );

	CHECK_EQUAL( 5, firstResult );
	CHECK_EQUAL( BW::string( "TestString" ), secondResult );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_PyFixedDict_as_dict )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> FIXED_DICT "
		"<implementedBy> FixedDictWrappers.noStream </implementedBy> "
		"<Properties> "
			" <int8_5> <Type> INT8 </Type> </int8_5> "
			" <str_TestString> <Type> STRING </Type> </str_TestString> "
		"</Properties> <AllowNone> false </AllowNone> </type>" );
	CHECK( pType );

	FixedDictDataType * pFixedDictType = dynamic_cast< FixedDictDataType * >(
		pType.get() );
	CHECK( pFixedDictType );

	// This should probably be in a fixture...
	ScriptModule fixedDictWrappers = ScriptModule::import( "FixedDictWrappers",
		ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( fixedDictWrappers.exists() );
	ScriptType wrapIntStrType =
		ScriptType::create( fixedDictWrappers.getAttribute( "WrapIntStr",
			ScriptErrorPrint() ) );
	RETURN_ON_FAIL_CHECK( wrapIntStrType.exists() );

	ScriptDict inputObject = ScriptDict::create( 2 );
	RETURN_ON_FAIL_CHECK( inputObject.exists() );
	CHECK( inputObject.setItem( "int8_5", ScriptObject::createFrom( (int8)5 ), 
		ScriptErrorPrint() ) );
	CHECK( inputObject.setItem( "str_TestString",
			ScriptObject::createFrom( BW::string( "TestString" ) ),
		ScriptErrorPrint() ) );

	ScriptObject instance = wrapIntStrType.callFunction(
		ScriptArgs::create( inputObject ), ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( instance.exists() );

	ScriptDataSource source( instance );
	CHECK( source.beginDictionary( pFixedDictType ) );

	int8 firstResult;
	CHECK( source.enterField( "int8_5" ) );
	CHECK( source.read( firstResult ) );
	CHECK( source.leaveField() );

	BW::string secondResult;
	CHECK( source.enterField( "str_TestString" ) );
	CHECK( source.read( secondResult ) );
	CHECK( source.leaveField() );

	CHECK_EQUAL( 5, firstResult );
	CHECK_EQUAL( BW::string( "TestString" ), secondResult );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_PyFixedDict_as_stream_default )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> FIXED_DICT "
		"<implementedBy> FixedDictWrappers.noStream </implementedBy> "
		"<Properties> "
			" <int8_5> <Type> INT8 </Type> </int8_5> "
			" <str_TestString> <Type> STRING </Type> </str_TestString> "
		"</Properties> <AllowNone> false </AllowNone> </type>" );
	CHECK( pType );

	FixedDictDataType * pFixedDictType = dynamic_cast< FixedDictDataType * >(
		pType.get() );
	CHECK( pFixedDictType );

	// This should probably be in a fixture...
	ScriptModule fixedDictWrappers = ScriptModule::import( "FixedDictWrappers",
		ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( fixedDictWrappers.exists() );
	ScriptType wrapIntStrType =
		ScriptType::create( fixedDictWrappers.getAttribute( "WrapIntStr",
			ScriptErrorPrint() ) );
	RETURN_ON_FAIL_CHECK( wrapIntStrType.exists() );

	ScriptDict inputObject = ScriptDict::create( 2 );
	RETURN_ON_FAIL_CHECK( inputObject.exists() );
	CHECK( inputObject.setItem( "int8_5", ScriptObject::createFrom( (int8)5 ), 
		ScriptErrorPrint() ) );
	CHECK( inputObject.setItem( "str_TestString",
			ScriptObject::createFrom( BW::string( "TestString" ) ),
		ScriptErrorPrint() ) );

	ScriptObject instance = wrapIntStrType.callFunction(
		ScriptArgs::create( inputObject ), ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( instance.exists() );

	ScriptDataSource source( instance );

	MemoryOStream stream;
	CHECK( source.readCustomType( *pFixedDictType, stream, 
		/* isPersistentOnly */ false ) );

	uint8 intVal;
	BW::string strVal;

	stream >> intVal;
	stream >> strVal;

	CHECK_EQUAL( 0, stream.remainingLength() );
	CHECK( !stream.error() );

	CHECK_EQUAL( BW::string( "TestString" ), strVal );
	CHECK_EQUAL( (unsigned)5, (unsigned)intVal );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_PyFixedDict_as_stream_custom )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> FIXED_DICT "
		"<implementedBy> FixedDictWrappers.stream </implementedBy> "
		"<Properties> "
			" <int8_5> <Type> INT8 </Type> </int8_5> "
			" <str_TestString> <Type> STRING </Type> </str_TestString> "
		"</Properties> <AllowNone> false </AllowNone> </type>" );
	CHECK( pType );

	FixedDictDataType * pFixedDictType = dynamic_cast< FixedDictDataType * >(
		pType.get() );
	CHECK( pFixedDictType );

	// This should probably be in a fixture...
	ScriptModule fixedDictWrappers = ScriptModule::import( "FixedDictWrappers",
		ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( fixedDictWrappers.exists() );
	ScriptType wrapIntStrType =
		ScriptType::create( fixedDictWrappers.getAttribute( "WrapIntStr",
			ScriptErrorPrint() ) );
	RETURN_ON_FAIL_CHECK( wrapIntStrType.exists() );

	ScriptDict inputObject = ScriptDict::create( 2 );
	RETURN_ON_FAIL_CHECK( inputObject.exists() );
	CHECK( inputObject.setItem( "int8_5", ScriptObject::createFrom( (int8)5 ), 
		ScriptErrorPrint() ) );
	CHECK( inputObject.setItem( "str_TestString",
			ScriptObject::createFrom( BW::string( "TestString" ) ),
		ScriptErrorPrint() ) );

	ScriptObject instance = wrapIntStrType.callFunction(
		ScriptArgs::create( inputObject ), ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( instance.exists() );

	ScriptDataSource source( instance );

	MemoryOStream stream;
	CHECK( source.readCustomType( *pFixedDictType, stream, 
		/* isPersistentOnly */ false ) );

	BW::string result;
	stream >> result;
	CHECK_EQUAL( 0, stream.remainingLength() );
	CHECK( !stream.error() );

	// See FixedDictWrappers.WrapperWithStreaming
	CHECK_EQUAL( BW::string( "5||TestString" ), result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_PyFixedDict_as_section )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> FIXED_DICT "
		"<implementedBy> FixedDictWrappers.noStream </implementedBy> "
		"<Properties> "
			" <int8_5> <Type> INT8 </Type> </int8_5> "
			" <str_TestString> <Type> STRING </Type> </str_TestString> "
		"</Properties> <AllowNone> false </AllowNone> </type>" );
	CHECK( pType );

	FixedDictDataType * pFixedDictType = dynamic_cast< FixedDictDataType * >(
		pType.get() );
	CHECK( pFixedDictType );

	// This should probably be in a fixture...
	ScriptModule fixedDictWrappers = ScriptModule::import( "FixedDictWrappers",
		ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( fixedDictWrappers.exists() );
	ScriptType wrapIntStrType =
		ScriptType::create( fixedDictWrappers.getAttribute( "WrapIntStr",
			ScriptErrorPrint() ) );
	RETURN_ON_FAIL_CHECK( wrapIntStrType.exists() );

	ScriptDict inputObject = ScriptDict::create( 2 );
	RETURN_ON_FAIL_CHECK( inputObject.exists() );
	CHECK( inputObject.setItem( "int8_5", ScriptObject::createFrom( (int8)5 ), 
		ScriptErrorPrint() ) );
	CHECK( inputObject.setItem( "str_TestString",
			ScriptObject::createFrom( BW::string( "TestString" ) ),
		ScriptErrorPrint() ) );

	ScriptObject instance = wrapIntStrType.callFunction(
		ScriptArgs::create( inputObject ), ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( instance.exists() );

	ScriptDataSource source( instance );

	DataSectionPtr pSection( new XMLSection( BW::string( "TestObj" ) ) );
	CHECK( source.readCustomType( *pFixedDictType, *pSection ) );

	CHECK_EQUAL( 2, pSection->countChildren() );

	DataSectionPtr pIntSection = pSection->findChild( BW::string( "int8_5" ) );
	RETURN_ON_FAIL_CHECK( pIntSection );
	CHECK_EQUAL( (uint8)5, pIntSection->asUInt() );

	DataSectionPtr pStrSection = pSection->findChild( BW::string( "str_TestString" ) );
	RETURN_ON_FAIL_CHECK( pStrSection );
	CHECK_EQUAL( BW::string( "TestString" ), pStrSection->asString() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_PyUserType_as_stream )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> USER_TYPE "
		"<implementedBy> UserTypes.intStr </implementedBy> </type>" );
	CHECK( pType );

	UserDataType * pUserType = dynamic_cast< UserDataType * >(
		pType.get() );
	CHECK( pUserType );

	// This should probably be in a fixture...
	ScriptModule userTypes = ScriptModule::import( "UserTypes",
		ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( userTypes.exists() );
	ScriptType intStrType =
		ScriptType::create( userTypes.getAttribute( "IntStr",
			ScriptErrorPrint() ) );
	RETURN_ON_FAIL_CHECK( intStrType.exists() );

	ScriptObject instance = intStrType.callFunction(
		ScriptArgs::create( ScriptObject::createFrom( (int8)5 ),
			ScriptObject::createFrom( BW::string( "TestString" ) ) ),
		ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( instance.exists() );

	ScriptDataSource source( instance );

	MemoryOStream stream;
	CHECK( source.readCustomType( *pUserType, stream, 
		/* isPersistentOnly */ false ) );

	BW::string result;
	stream >> result;
	CHECK_EQUAL( 0, stream.remainingLength() );
	CHECK( !stream.error() );

	// See UserTypes.IntStrType
	CHECK_EQUAL( BW::string( "5||TestString" ), result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_PyUserType_as_section )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> USER_TYPE "
		"<implementedBy> UserTypes.intStr </implementedBy> </type>" );
	CHECK( pType );

	UserDataType * pUserType = dynamic_cast< UserDataType * >(
		pType.get() );
	CHECK( pUserType );

	// This should probably be in a fixture...
	ScriptModule userTypes = ScriptModule::import( "UserTypes",
		ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( userTypes.exists() );
	ScriptType intStrType =
		ScriptType::create( userTypes.getAttribute( "IntStr",
			ScriptErrorPrint() ) );
	RETURN_ON_FAIL_CHECK( intStrType.exists() );

	ScriptObject instance = intStrType.callFunction(
		ScriptArgs::create( ScriptObject::createFrom( (int8)5 ),
			ScriptObject::createFrom( BW::string( "TestString" ) ) ),
		ScriptErrorPrint() );
	RETURN_ON_FAIL_CHECK( instance.exists() );

	ScriptDataSource source( instance );

	DataSectionPtr pSection( new XMLSection( BW::string( "TestObj" ) ) );
	CHECK( source.readCustomType( *pUserType, *pSection ) );

	CHECK_EQUAL( 2, pSection->countChildren() );

	DataSectionPtr pIntSection = pSection->findChild( BW::string( "intVal" ) );
	RETURN_ON_FAIL_CHECK( pIntSection );
	CHECK_EQUAL( (uint8)5, pIntSection->asUInt() );

	DataSectionPtr pStrSection = pSection->findChild( BW::string( "strVal" ) );
	RETURN_ON_FAIL_CHECK( pStrSection );
	CHECK_EQUAL( BW::string( "TestString" ), pStrSection->asString() );
}
#endif // defined( SCRIPT_PYTHON )


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSource_object_passthrough )
{
	ScriptString inputObject = ScriptString::create( "Test string" );
	ScriptObject resultObject;
	ScriptDataSource source( inputObject );
	CHECK( source.read( resultObject ) );
	CHECK_EQUAL( inputObject, resultObject );
}


} // namespace BW

// test_script_data_source.cpp
