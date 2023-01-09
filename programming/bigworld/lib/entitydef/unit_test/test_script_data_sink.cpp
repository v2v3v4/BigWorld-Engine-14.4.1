#include "pch.hpp"

#include "test_harness.hpp"

#if defined( MF_SERVER )
#include "unittest_mailbox.hpp"
#include "entitydef/mailbox_base.hpp"
#endif // defined( MF_SERVER )

#include "entitydef/data_type.hpp"
#include "entitydef/script_data_sink.hpp"

#if defined( SCRIPT_PYTHON )
#include "entitydef/data_instances/array_data_instance.hpp"
#include "entitydef/data_instances/class_data_instance.hpp"
#include "entitydef/data_instances/fixed_dict_data_instance.hpp"
#endif // defined( SCRIPT_PYTHON )

#include "entitydef/data_types/array_data_type.hpp"
#include "entitydef/data_types/class_data_type.hpp"
#include "entitydef/data_types/fixed_dict_data_type.hpp"
#include "entitydef/data_types/tuple_data_type.hpp"
#if defined( SCRIPT_PYTHON )
#include "entitydef/data_types/user_data_type.hpp"
#endif // defined( SCRIPT_PYTHON )

#if defined( SCRIPT_PYTHON )
#include "chunk/user_data_object.hpp"
#include "chunk/user_data_object_type.hpp"
#endif // defined( SCRIPT_PYTHON )

#include "cstdmf/memory_stream.hpp"
#include "cstdmf/unique_id.hpp"

#include "network/basictypes.hpp"

#include "resmgr/xml_section.hpp"

#include "script/script_object.hpp"

#if defined( SCRIPT_PYTHON )
#include "pyscript/script_math.hpp"
#endif // defined( SCRIPT_PYTHON )


namespace BW
{

namespace {

/**
 *	Utility method to get a DataSectionPtr given some XML
 */
static DataSectionPtr getDataSectionForXML( const BW::string & dataSectionStr )
{
	BW::stringstream dataSectionStream;
	dataSectionStream << dataSectionStr;
	XMLSectionPtr xml = XMLSection::createFromStream( "", dataSectionStream );
	return xml;
}


/**
 *	Utility method to hide the difference between ScriptList and PyArrayDataInstance
 */
static bool checkArrayType( ScriptObject & array )
{
#if defined( SCRIPT_PYTHON )
	return PyArrayDataInstance::Check( array );
#else // defined( SCRIPT_PYTHON )
	return ScriptList::check( array );
#endif // defined( SCRIPT_PYTHON )
}


/**
 *	Utility method to hide the difference between ScriptList and PyArrayDataInstance
 */
static bool getArrayLength( ScriptObject & array, size_t & size )
{
#if defined( SCRIPT_PYTHON )
	if( !PyArrayDataInstance::Check( array ) )
	{
		return false;
	}
	PyArrayDataInstance * pPyArray =
		static_cast< PyArrayDataInstance * >( array.get() ); 

	size = pPyArray->pySeq_length();
	return true;
#else // defined( SCRIPT_PYTHON )
	ScriptList list = ScriptList::create( array );

	if (!list)
	{
		return false;
	}

	size = list.size();

	return true;
#endif // defined( SCRIPT_PYTHON )
}


/**
 *	Utility method to hide the difference between ScriptList and PyArrayDataInstance
 */
static ScriptObject getArrayItem( ScriptObject & array, size_t item )
{
#if defined( SCRIPT_PYTHON )
	if( !PyArrayDataInstance::Check( array ) )
	{
		return ScriptObject();
	}
	PyArrayDataInstance * pPyArray =
		static_cast< PyArrayDataInstance * >( array.get() ); 

	ScriptObject pyIndex = pPyArray->getPyIndex( static_cast<int>(item) );
	ScriptInt index = ScriptInt::create( pyIndex );
	if (!index.exists())
	{
		return ScriptObject();
	}

	return ScriptObject( pPyArray->pySeq_item( index.asLong() ) );
#else // defined( SCRIPT_PYTHON )
	ScriptList list = ScriptList::create( array );
	if (!list)
	{
		return ScriptObject();
	}
	return list.getItem( item );
#endif // defined( SCRIPT_PYTHON )
}


/**
 *	Utility method to hide the difference between ScriptDict and
 *	PyFixedDictDataInstance
 */
static bool checkDictionaryType( ScriptObject & dictionary )
{
#if defined( SCRIPT_PYTHON )
	return PyFixedDictDataInstance::Check( dictionary );
#else // defined( SCRIPT_PYTHON )
	return ScriptDict::check( dictionary );
#endif // defined( SCRIPT_PYTHON )
}


/**
 *	Utility method to hide the difference between ScriptDict and
 *	PyFixedDictDataInstance
 */
static ScriptObject getDictionaryField( ScriptObject & dictionary,
	const BW::string & field )
{
#if defined( SCRIPT_PYTHON )
	if( !PyFixedDictDataInstance::Check( dictionary ) )
	{
		return ScriptObject();
	}
	PyFixedDictDataInstance * pPyDict =
		static_cast< PyFixedDictDataInstance * >( dictionary.get() );

	int index = pPyDict->dataType().getFieldIndex( field );
	if (index == -1)
	{
		return ScriptObject();
	}

	return pPyDict->getFieldValue( index );
#else // defined( SCRIPT_PYTHON )
	ScriptDict dict = ScriptDict::create( dictionary );
	if (!dict.exists())
	{
		return ScriptObject();
	}
	return dict.getItem( field.c_str(), ScriptErrorPrint() );
#endif // defined( SCRIPT_PYTHON )
}


/**
 *	Utility method to hide the difference between ScriptObject and
 *	PyClassDataInstance
 */
static bool checkClassType( ScriptObject & object )
{
#if defined( SCRIPT_PYTHON )
	return PyClassDataInstance::Check( object );
#else // defined( SCRIPT_PYTHON )
	return ScriptObject::check( object );
#endif // defined( SCRIPT_PYTHON )
}


/**
 *	Utility method to hide the difference between ScriptDict and
 *	PyFixedDictDataInstance
 */
static ScriptObject getClassField( ScriptObject & object,
	const BW::string & field )
{
#if defined( SCRIPT_PYTHON )
	if( !PyClassDataInstance::Check( object ) )
	{
		return ScriptObject();
	}
	PyClassDataInstance * pPyInstance =
		static_cast< PyClassDataInstance * >( object.get() );

	int index = pPyInstance->dataType()->fieldIndexFromName( field.c_str() );
	if (index == -1)
	{
		return ScriptObject();
	}

	return pPyInstance->getFieldValue( index );
#else // defined( SCRIPT_PYTHON )
	if (!object.exists())
	{
		return ScriptObject();
	}
	return dict.getAttribute( field.c_str(), ScriptErrorPrint() );
#endif // defined( SCRIPT_PYTHON )
}


} // Anonymous namespace

TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_float )
{
	ScriptDataSink sink;
	CHECK( sink.write( 5.f ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptFloat::check( resultObject ) );
	ScriptFloat resultFloat( resultObject );
	CHECK( isEqual( (double)5.f, resultFloat.asDouble() ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_double )
{
	ScriptDataSink sink;
	CHECK( sink.write( 8. ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptFloat::check( resultObject ) );
	ScriptFloat resultFloat( resultObject );
	CHECK( isEqual( (double)8., resultFloat.asDouble() ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_uint8 )
{
	ScriptDataSink sink;
	CHECK( sink.write( (uint8)17 ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultObject ) );
	ScriptInt resultInt( resultObject );
	CHECK_EQUAL( 17, resultInt.asLong() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_uint16 )
{
	ScriptDataSink sink;
	CHECK( sink.write( (uint16)27 ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultObject ) );
	ScriptInt resultInt( resultObject );
	CHECK_EQUAL( 27, resultInt.asLong() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_uint32 )
{
	ScriptDataSink sink;
	CHECK( sink.write( (uint32)47 ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultObject ) );
	ScriptInt resultInt( resultObject );
	CHECK_EQUAL( 47, resultInt.asLong() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_uint64 )
{
	ScriptDataSink sink;
	CHECK( sink.write( (uint64)87 ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultObject ) ||
		ScriptLong::check( resultObject ) );
	long result;
	if (ScriptInt::check( resultObject ))
	{
		ScriptInt resultInt( resultObject );
		result = resultInt.asLong();
	}
	else
	{
		ScriptLong resultLong( resultObject );
		result = resultLong.asLong();
	}

	CHECK_EQUAL( 87, result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_int8 )
{
	ScriptDataSink sink;
	CHECK( sink.write( (int8)97 ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultObject ) );
	ScriptInt resultInt( resultObject );
	CHECK_EQUAL( 97, resultInt.asLong() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_int16 )
{
	ScriptDataSink sink;
	CHECK( sink.write( (int16)107 ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultObject ) );
	ScriptInt resultInt( resultObject );
	CHECK_EQUAL( 107, resultInt.asLong() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_int32 )
{
	ScriptDataSink sink;
	CHECK( sink.write( (int32)207 ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultObject ) );
	ScriptInt resultInt( resultObject );
	CHECK_EQUAL( 207, resultInt.asLong() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_int64 )
{
	ScriptDataSink sink;
	CHECK( sink.write( (int64)407 ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultObject ) ||
		ScriptLong::check( resultObject ) );
	long result;
	if (ScriptInt::check( resultObject ))
	{
		ScriptInt resultInt( resultObject );
		result = resultInt.asLong();
	}
	else
	{
		ScriptLong resultLong( resultObject );
		result = resultLong.asLong();
	}
	
	CHECK_EQUAL( 407, result );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_string )
{
	ScriptDataSink sink;
	CHECK( sink.write( BW::string( "Test string" ) ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptString::check( resultObject ) );
	ScriptString resultString( resultObject );
	BW::string resultStr;
	resultString.getString( resultStr );
	CHECK_EQUAL( BW::string( "Test string" ), resultStr );
}


#if defined( SCRIPT_PYTHON )
// Non-SCRIPT_PYTHON doesn't define this at all...
TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_wstring )
{
	// Characters taken from our Python unicode output test:
	// Chinese character: 0x8FD9, UTF-8 e8 bf 99
	// Japanese "HIRAGANA LETTER KO": 0x3053, UTF-8 e3 81 93
	// Russian "CYRILLIC CAPITAL LETTER E": 0x42D, UTF-8 d0 ad
	BW::wstring input( L"\x8fd9\x3053\x042d" );
	ScriptDataSink sink;
	CHECK( sink.write( input ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( PyUnicode_Check( resultObject.get() ) );
	BW::wstring resultStr;
	CHECK_EQUAL( 0, Script::setData( resultObject.get(), resultStr, "" ) );
	// No CHECK_EQUAL for BW::wstring
	CHECK_EQUAL( input.size(), resultStr.size() );
	CHECK_EQUAL( input[ 0 ], resultStr[ 0 ] );
	CHECK_EQUAL( input[ 1 ], resultStr[ 1 ] );
	CHECK_EQUAL( input[ 2 ], resultStr[ 2 ] );
}


// TODO: Non-SCRIPT_PYTHON treats Vectors as ScriptSequences
TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_Vector2 )
{
	ScriptDataSink sink;
	CHECK( sink.write( Vector2( 5.f, 12.f ) ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( PyVector< Vector2 >::Check( resultObject ) );
	PyVector< Vector2 > * pResult =
		static_cast< PyVector< Vector2 > * >( resultObject.get() );
	Vector2 result = pResult->getVector();
	CHECK( isEqual( 5.f, result.x ) );
	CHECK( isEqual( 12.f, result.y ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_Vector3 )
{
	ScriptDataSink sink;
	CHECK( sink.write( Vector3( 5.f, 12.f, 1200.f ) ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( PyVector< Vector3 >::Check( resultObject ) );
	PyVector< Vector3 > * pResult =
		static_cast< PyVector< Vector3 > * >( resultObject.get() );
	Vector3 result = pResult->getVector();
	CHECK( isEqual( 5.f, result.x ) );
	CHECK( isEqual( 12.f, result.y ) );
	CHECK( isEqual( 1200.f, result.z ) );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_Vector4 )
{
	ScriptDataSink sink;
	CHECK( sink.write( Vector4( 5.f, 12.f, 1200.f, -14.5f ) ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( PyVector< Vector4 >::Check( resultObject ) );
	PyVector< Vector4 > * pResult =
		static_cast< PyVector< Vector4 > * >( resultObject.get() );
	Vector4 result = pResult->getVector();
	CHECK( isEqual( 5.f, result.x ) );
	CHECK( isEqual( 12.f, result.y ) );
	CHECK( isEqual( 1200.f, result.z ) );
	CHECK( isEqual( -14.5f, result.w ) );
}
#endif // defined( SCRIPT_PYTHON )


#if defined( MF_SERVER )
TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_MailBox )
{
	Mercury::Address inputAddr( 0xc0180010, 0x4567 );
	EntityMailBoxRef input;
	input.init( 2405, inputAddr, EntityMailBoxRef::BASE_VIA_CELL, 7 );
	CHECK_EQUAL( 0xc0180010, input.addr.ip );
	CHECK_EQUAL( 0x4567, input.addr.port );
	CHECK_EQUAL( (EntityID)2405, input.id );
	CHECK_EQUAL( EntityMailBoxRef::BASE_VIA_CELL, input.component() );
	CHECK_EQUAL( (EntityTypeID)7, input.type() );

	ScriptDataSink sink;
	CHECK( sink.write( input ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( PyEntityMailBox::Check( resultObject ) );
	RETURN_ON_FAIL_CHECK( UnittestMailBox::Check( resultObject ) );
	UnittestMailBox * pMailBox =
		static_cast< UnittestMailBox * >( resultObject.get() );

	CHECK_EQUAL( 0xc0180010, pMailBox->address().ip );
	CHECK_EQUAL( 0x4567, pMailBox->address().port );
	CHECK_EQUAL( (EntityID)2405, pMailBox->id() );
	CHECK_EQUAL( EntityMailBoxRef::BASE_VIA_CELL, pMailBox->component() );
	CHECK_EQUAL( (EntityTypeID)7, pMailBox->typeID() );
}
#endif // defined( MF_SERVER )


#if !defined( EDITOR_ENABLED )
TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_UDO_REF )
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
#endif // defined( SCRIPT_PYTHON )


	ScriptDataSink sink;
	CHECK( sink.write( input ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );

#if defined( SCRIPT_PYTHON )
	// SCRIPT_PYTHON creates UserDataObject types, see
	// chunk/user_data_object.hpp
	RETURN_ON_FAIL_CHECK( UserDataObject::Check( resultObject.get() ) );
	UserDataObject * pResultUDO = static_cast< UserDataObject * >(
		resultObject.get() );
	// UniqueID doesn't have operator<< so we can't test the whole object.
	CHECK_EQUAL( 0x40U, pResultUDO->guid().getA() );
	CHECK_EQUAL( 0x80U, pResultUDO->guid().getB() );
	CHECK_EQUAL( 0x400U, pResultUDO->guid().getC() );
	CHECK_EQUAL( 0x1200U, pResultUDO->guid().getD() );
#else // defined( SCRIPT_PYTHON )
	// Otherwise, we get a ScriptBlob containing the stringified GUID.
	RETURN_ON_FAIL_CHECK( ScriptBlob::check( resultObject ) );
	ScriptBlob resultBlob( resultObject );
	BW::string resultGUID;
	resultBlob.getString( resultGUID );
	CHECK( UniqueID::isUniqueID( resultGUID ) );
	CHECK_EQUAL( input.toString(), resultStr );
#endif // defined( SCRIPT_PYTHON )
}
#endif // !defined( EDITOR_ENABLED )


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_None )
{
	ScriptDataSink sink;
	CHECK( sink.writeNone( /* isNone */ true ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	CHECK( resultObject.isNone() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_notNone )
{
	ScriptDataSink sink;
	CHECK( sink.writeNone( /* isNone */ false ) );
	CHECK( sink.write( BW::string( "Test string" ) ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptString::check( resultObject ) );
	ScriptString resultString( resultObject );
	BW::string resultStr;
	resultString.getString( resultStr );
	CHECK_EQUAL( BW::string( "Test string" ), resultStr );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_blob )
{
	ScriptDataSink sink;
	CHECK( sink.writeBlob( BW::string( "Test blob" ) ) );
	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptBlob::check( resultObject ) );
	ScriptBlob resultBlob( resultObject );
	BW::string resultStr;
	resultBlob.getString( resultStr );
	CHECK_EQUAL( BW::string( "Test blob" ), resultStr );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_ScriptList )
{
	ScriptDataSink sink;
	// NULL type will cause us to always create a ScriptList
	CHECK( sink.beginArray( NULL, (size_t)5 ) );
	CHECK( sink.enterItem( 0 ) );
	CHECK( sink.write( (int8) 5 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 1 ) );
	CHECK( sink.write( (int8) 8 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 2 ) );
	CHECK( sink.write( (int8) 12 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 3 ) );
	CHECK( sink.write( (int8) 127 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 4 ) );
	CHECK( sink.write( (int8) -127 ) );
	CHECK( sink.leaveItem() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptList::check( resultObject ) );
	ScriptList resultList( resultObject );
	CHECK_EQUAL( 5, resultList.size() );

	CHECK( resultList.getItem ( 0 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultList.getItem ( 0 ) ) );
	CHECK_EQUAL( 5, ScriptInt( resultList.getItem ( 0 ) ).asLong() );

	CHECK( resultList.getItem ( 1 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultList.getItem ( 1 ) ) );
	CHECK_EQUAL( 8, ScriptInt( resultList.getItem ( 1 ) ).asLong() );

	CHECK( resultList.getItem ( 2 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultList.getItem ( 2 ) ) );
	CHECK_EQUAL( 12, ScriptInt( resultList.getItem ( 2 ) ).asLong() );

	CHECK( resultList.getItem ( 3 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultList.getItem ( 3 ) ) );
	CHECK_EQUAL( 127, ScriptInt( resultList.getItem ( 3 ) ).asLong() );

	CHECK( resultList.getItem ( 4 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultList.getItem ( 4 ) ) );
	CHECK_EQUAL( -127, ScriptInt( resultList.getItem ( 4 ) ).asLong() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_Array_of_int8 )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> ARRAY <size> 5 </size> <of> INT8 </of> </type>" );
	CHECK( pType );

	ArrayDataType * pArrayType = dynamic_cast< ArrayDataType * >( pType.get() );
	CHECK( pArrayType );

	ScriptDataSink sink;
	CHECK( sink.beginArray( pArrayType, (size_t)5 ) );
	CHECK( sink.enterItem( 0 ) );
	CHECK( sink.write( (int8) 5 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 1 ) );
	CHECK( sink.write( (int8) 8 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 2 ) );
	CHECK( sink.write( (int8) 12 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 3 ) );
	CHECK( sink.write( (int8) 127 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 4 ) );
	CHECK( sink.write( (int8) -127 ) );
	CHECK( sink.leaveItem() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptSequence::check( resultObject ) );
	RETURN_ON_FAIL_CHECK( checkArrayType( resultObject ) );
	size_t resultSize;
	CHECK( getArrayLength( resultObject, resultSize ) );
	CHECK_EQUAL( (size_t)5, resultSize );

	CHECK( getArrayItem( resultObject, 0 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( getArrayItem( resultObject, 0 ) ) );
	CHECK_EQUAL( 5, ScriptInt( getArrayItem( resultObject, 0 ) ).asLong() );

	CHECK( getArrayItem( resultObject, 1 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( getArrayItem( resultObject, 1 ) ) );
	CHECK_EQUAL( 8, ScriptInt( getArrayItem( resultObject, 1 ) ).asLong() );

	CHECK( getArrayItem( resultObject, 2 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( getArrayItem( resultObject, 2 ) ) );
	CHECK_EQUAL( 12, ScriptInt( getArrayItem( resultObject, 2 ) ).asLong() );

	CHECK( getArrayItem( resultObject, 3 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( getArrayItem( resultObject, 3 ) ) );
	CHECK_EQUAL( 127, ScriptInt( getArrayItem( resultObject, 3 ) ).asLong() );

	CHECK( getArrayItem( resultObject, 4 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( getArrayItem( resultObject, 4 ) ) );
	CHECK_EQUAL( -127, ScriptInt( getArrayItem( resultObject, 4 ) ).asLong() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_ScriptTuple )
{
	ScriptDataSink sink;
	// NULL type will cause us to always create a ScriptTuple
	CHECK( sink.beginTuple( NULL, (size_t)5 ) );
	CHECK( sink.enterItem( 0 ) );
	CHECK( sink.write( (int8) 5 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 1 ) );
	CHECK( sink.write( (int8) 8 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 2 ) );
	CHECK( sink.write( (int8) 12 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 3 ) );
	CHECK( sink.write( (int8) 127 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 4 ) );
	CHECK( sink.write( (int8) -127 ) );
	CHECK( sink.leaveItem() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptTuple::check( resultObject ) );
	ScriptTuple resultTuple( resultObject );
	CHECK_EQUAL( 5, resultTuple.size() );

	CHECK( resultTuple.getItem( 0 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultTuple.getItem( 0 ) ) );
	CHECK_EQUAL( 5, ScriptInt( resultTuple.getItem( 0 ) ).asLong() );

	CHECK( resultTuple.getItem( 1 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultTuple.getItem( 1 ) ) );
	CHECK_EQUAL( 8, ScriptInt( resultTuple.getItem( 1 ) ).asLong() );

	CHECK( resultTuple.getItem( 2 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultTuple.getItem( 2 ) ) );
	CHECK_EQUAL( 12, ScriptInt( resultTuple.getItem( 2 ) ).asLong() );

	CHECK( resultTuple.getItem( 3 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultTuple.getItem( 3 ) ) );
	CHECK_EQUAL( 127, ScriptInt( resultTuple.getItem( 3 ) ).asLong() );

	CHECK( resultTuple.getItem( 4 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultTuple.getItem( 4 ) ) );
	CHECK_EQUAL( -127, ScriptInt( resultTuple.getItem( 4 ) ).asLong() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_Tuple_of_int8 )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> TUPLE <size> 5 </size> <of> INT8 </of> </type>" );
	CHECK( pType );

	TupleDataType * pTupleType = dynamic_cast< TupleDataType * >( pType.get() );
	CHECK( pTupleType );

	ScriptDataSink sink;
	CHECK( sink.beginTuple( pTupleType, (size_t)5 ) );
	CHECK( sink.enterItem( 0 ) );
	CHECK( sink.write( (int8) 5 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 1 ) );
	CHECK( sink.write( (int8) 8 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 2 ) );
	CHECK( sink.write( (int8) 12 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 3 ) );
	CHECK( sink.write( (int8) 127 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 4 ) );
	CHECK( sink.write( (int8) -127 ) );
	CHECK( sink.leaveItem() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptTuple::check( resultObject ) );
	ScriptTuple resultTuple( resultObject );
	CHECK_EQUAL( 5, resultTuple.size() );

	CHECK( resultTuple.getItem( 0 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultTuple.getItem( 0 ) ) );
	CHECK_EQUAL( 5, ScriptInt( resultTuple.getItem( 0 ) ).asLong() );

	CHECK( resultTuple.getItem( 1 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultTuple.getItem( 1 ) ) );
	CHECK_EQUAL( 8, ScriptInt( resultTuple.getItem( 1 ) ).asLong() );

	CHECK( resultTuple.getItem( 2 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultTuple.getItem( 2 ) ) );
	CHECK_EQUAL( 12, ScriptInt( resultTuple.getItem( 2 ) ).asLong() );

	CHECK( resultTuple.getItem( 3 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultTuple.getItem( 3 ) ) );
	CHECK_EQUAL( 127, ScriptInt( resultTuple.getItem( 3 ) ).asLong() );

	CHECK( resultTuple.getItem( 4 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( resultTuple.getItem( 4 ) ) );
	CHECK_EQUAL( -127, ScriptInt( resultTuple.getItem( 4 ) ).asLong() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_Array_of_Tuple_of_int8 )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> ARRAY <of> TUPLE <of> INT8 </of> </of> </type>" );
	CHECK( pType );

	ArrayDataType * pArrayType = dynamic_cast< ArrayDataType * >( pType.get() );
	CHECK( pArrayType );
	TupleDataType * pElemType =
		dynamic_cast< TupleDataType * >( &pArrayType->getElemType() );
	CHECK( pElemType );

	ScriptDataSink sink;
	CHECK( sink.beginArray( pArrayType, (size_t)2 ) );

	CHECK( sink.enterItem( 0 ) );
	CHECK( sink.beginTuple( pElemType, (size_t) 2 ) );
	CHECK( sink.enterItem( 0 ) );
	CHECK( sink.write( (int8) 40 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 1 ) );
	CHECK( sink.write( (int8) 80 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.leaveItem() );

	CHECK( sink.enterItem( 1 ) );
	CHECK( sink.beginTuple( pElemType, (size_t) 1 ) );
	CHECK( sink.enterItem( 0 ) );
	CHECK( sink.write( (int8) 120 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.leaveItem() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptSequence::check( resultObject ) );
	RETURN_ON_FAIL_CHECK( checkArrayType( resultObject ) );
	size_t resultSize;
	CHECK( getArrayLength( resultObject, resultSize ) );
	CHECK_EQUAL( (size_t)2, resultSize );

	ScriptObject firstObject = getArrayItem( resultObject, 0 );
	CHECK( firstObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptTuple::check( firstObject ) );
	ScriptTuple firstTuple( firstObject );
	CHECK_EQUAL( 2, firstTuple.size() );

	CHECK( firstTuple.getItem( 0 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( firstTuple.getItem( 0 ) ) );
	CHECK_EQUAL( 40, ScriptInt( firstTuple.getItem( 0 ) ).asLong() );

	CHECK( firstTuple.getItem( 1 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( firstTuple.getItem( 1 ) ) );
	CHECK_EQUAL( 80, ScriptInt( firstTuple.getItem( 1 ) ).asLong() );

	ScriptObject secondObject = getArrayItem( resultObject, 1 );
	CHECK( secondObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptTuple::check( secondObject ) );
	ScriptTuple secondTuple( secondObject );
	CHECK_EQUAL( 1, secondTuple.size() );

	CHECK( secondTuple.getItem( 0 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( secondTuple.getItem( 0 ) ) );
	CHECK_EQUAL( 120, ScriptInt( secondTuple.getItem( 0 ) ).asLong() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_Tuple_of_Array_of_int8 )
{
	DataTypePtr pType = DataType::buildDataType(
		"<type> TUPLE <of> ARRAY <of> INT8 </of> </of> </type>" );
	CHECK( pType );

	TupleDataType * pTupleType = dynamic_cast< TupleDataType * >( pType.get() );
	CHECK( pTupleType );
	ArrayDataType * pElemType =
		dynamic_cast< ArrayDataType * >( &pTupleType->getElemType() );
	CHECK( pElemType );

	ScriptDataSink sink;
	CHECK( sink.beginTuple( pTupleType, (size_t)2 ) );

	CHECK( sink.enterItem( 0 ) );
	CHECK( sink.beginArray( pElemType, (size_t) 2 ) );
	CHECK( sink.enterItem( 0 ) );
	CHECK( sink.write( (int8) 45 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.enterItem( 1 ) );
	CHECK( sink.write( (int8) 85 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.leaveItem() );

	CHECK( sink.enterItem( 1 ) );
	CHECK( sink.beginArray( pElemType, (size_t) 1 ) );
	CHECK( sink.enterItem( 0 ) );
	CHECK( sink.write( (int8) 125 ) );
	CHECK( sink.leaveItem() );
	CHECK( sink.leaveItem() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptTuple::check( resultObject ) );
	ScriptTuple resultTuple( resultObject );
	CHECK_EQUAL( 2, resultTuple.size() );

	ScriptObject firstObject = resultTuple.getItem( 0 );
	RETURN_ON_FAIL_CHECK( ScriptSequence::check( firstObject ) );
	RETURN_ON_FAIL_CHECK( checkArrayType( firstObject ) );
	size_t resultSize;
	CHECK( getArrayLength( firstObject, resultSize ) );
	CHECK_EQUAL( (size_t)2, resultSize );

	CHECK( getArrayItem( firstObject, 0 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( getArrayItem( firstObject, 0 ) ) );
	CHECK_EQUAL( 45, ScriptInt( getArrayItem( firstObject, 0 ) ).asLong() );

	CHECK( getArrayItem( firstObject, 1 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( getArrayItem( firstObject, 1 ) ) );
	CHECK_EQUAL( 85, ScriptInt( getArrayItem( firstObject, 1 ) ).asLong() );

	ScriptObject secondObject = resultTuple.getItem( 1 );
	RETURN_ON_FAIL_CHECK( ScriptSequence::check( secondObject ) );
	RETURN_ON_FAIL_CHECK( ScriptSequence::check( secondObject ) );
	RETURN_ON_FAIL_CHECK( checkArrayType( secondObject ) );
	CHECK( getArrayLength( secondObject, resultSize ) );
	CHECK_EQUAL( (size_t)1, resultSize );

	CHECK( getArrayItem( secondObject, 0 ).exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( getArrayItem( secondObject, 0 ) ) );
	CHECK_EQUAL( 125, ScriptInt( getArrayItem( secondObject, 0 ) ).asLong() );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_ScriptObject )
{
	ScriptDataSink sink;

	// NULL type will cause us to always create a ScriptObject
	// But this is not implemented right now
	// See ScriptDataSink::beginClass
	CHECK( !sink.beginClass( NULL ) );
#if 0
	CHECK( sink.enterField( "int8_5" ) );
	CHECK( sink.write( (int8) 5 ) );
	CHECK( sink.leaveField() );
	CHECK( sink.enterField( "str_TestString" ) );
	CHECK( sink.write( BW::string( "TestString" ) ) );
	CHECK( sink.leaveField() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( checkClassType( resultObject ) );

	ScriptObject firstObject = getClassField( resultObject, "int8_5" );
	CHECK( firstObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( firstObject ) );
	ScriptInt firstInt( firstObject );
	CHECK( firstInt.exists() );
	CHECK_EQUAL( 5, firstInt.asLong() );

	ScriptObject secondObject = getClassField( resultObject,
		"str_TestString" );
	CHECK( secondObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptString::check( secondObject ) );
	ScriptString secondString( secondObject );
	BW::string secondStr;
	CHECK( secondString.exists() );
	secondString.getString( secondStr );
	CHECK_EQUAL( BW::string( "TestString" ), secondStr );
#endif
}


#if defined( SCRIPT_PYTHON )
TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_Class )
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

	ScriptDataSink sink;

	CHECK( sink.beginClass( pClassType ) );
	CHECK( sink.enterField( "int8_5" ) );
	CHECK( sink.write( (int8) 5 ) );
	CHECK( sink.leaveField() );
	CHECK( sink.enterField( "str_TestString" ) );
	CHECK( sink.write( BW::string( "TestString" ) ) );
	CHECK( sink.leaveField() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( checkClassType( resultObject ) );

	ScriptObject firstObject = getClassField( resultObject, "int8_5" );
	CHECK( firstObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( firstObject ) );
	ScriptInt firstInt( firstObject );
	CHECK( firstInt.exists() );
	CHECK_EQUAL( 5, firstInt.asLong() );

	ScriptObject secondObject = getClassField( resultObject,
		"str_TestString" );
	CHECK( secondObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptString::check( secondObject ) );
	ScriptString secondString( secondObject );
	BW::string secondStr;
	CHECK( secondString.exists() );
	secondString.getString( secondStr );
	CHECK_EQUAL( BW::string( "TestString" ), secondStr );
}
#endif // defined( SCRIPT_PYTHON )


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_ScriptDict )
{
	ScriptDataSink sink;

	// NULL type will cause us to always create a ScriptDict
	CHECK( sink.beginDictionary( NULL ) );
	CHECK( sink.enterField( "int8_5" ) );
	CHECK( sink.write( (int8) 5 ) );
	CHECK( sink.leaveField() );
	CHECK( sink.enterField( "str_TestString" ) );
	CHECK( sink.write( BW::string( "TestString" ) ) );
	CHECK( sink.leaveField() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptDict::check( resultObject ) );
	ScriptDict resultDict( resultObject );

	ScriptObject firstObject = resultDict.getItem( "int8_5",
		ScriptErrorPrint() );
	CHECK( firstObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( firstObject ) );
	ScriptInt firstInt( firstObject );
	CHECK( firstInt.exists() );
	CHECK_EQUAL( 5, firstInt.asLong() );

	ScriptObject secondObject = resultDict.getItem( "str_TestString",
		ScriptErrorPrint() );
	CHECK( secondObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptString::check( secondObject ) );
	ScriptString secondString( secondObject );
	BW::string secondStr;
	CHECK( secondString.exists() );
	secondString.getString( secondStr );
	CHECK_EQUAL( BW::string( "TestString" ), secondStr );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_FixedDict )
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

	ScriptDataSink sink;

	CHECK( sink.beginDictionary( pFixedDictType ) );
	CHECK( sink.enterField( "int8_5" ) );
	CHECK( sink.write( (int8) 5 ) );
	CHECK( sink.leaveField() );
	CHECK( sink.enterField( "str_TestString" ) );
	CHECK( sink.write( BW::string( "TestString" ) ) );
	CHECK( sink.leaveField() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( checkDictionaryType( resultObject ) );

	ScriptObject firstObject = getDictionaryField( resultObject, "int8_5" );
	CHECK( firstObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptInt::check( firstObject ) );
	ScriptInt firstInt( firstObject );
	CHECK( firstInt.exists() );
	CHECK_EQUAL( 5, firstInt.asLong() );

	ScriptObject secondObject = getDictionaryField( resultObject,
		"str_TestString" );
	CHECK( secondObject.exists() );
	RETURN_ON_FAIL_CHECK( ScriptString::check( secondObject ) );
	ScriptString secondString( secondObject );
	BW::string secondStr;
	CHECK( secondString.exists() );
	secondString.getString( secondStr );
	CHECK_EQUAL( BW::string( "TestString" ), secondStr );
}

#if defined( SCRIPT_PYTHON )
TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_PyFixedDict_as_dict )
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

	ScriptDataSink sink;

	// Not implemented: I haven't seen a good use case for it yet...
	// See ScriptDataSink::beginDictionary and ScriptDataSink::leaveField
	CHECK( !sink.beginDictionary( pFixedDictType ) );
#if 0 // We don't currently support writing custom FixedDict types by field.
	CHECK( sink.enterField( "int8_5" ) );
	CHECK( sink.write( (int8) 5 ) );
	CHECK( sink.leaveField() );
	CHECK( sink.enterField( "str_TestString" ) );
	CHECK( sink.write( BW::string( "TestString" ) ) );
	CHECK( sink.leaveField() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( wrapIntStrType.isObjectOfType( resultObject ) );
	BW::string strVal;
	uint8 intVal;
	CHECK( resultObject.getAttribute( "intVal", intVal, ScriptErrorPrint() ) );
	CHECK_EQUAL( (uint8)5, intVal );
	CHECK( resultObject.getAttribute( "strVal", strVal, ScriptErrorPrint() ) );
	CHECK_EQUAL( BW::string( "TestString" ), strVal );
#endif // 0
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_PyFixedDict_as_stream_default )
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

	MemoryOStream stream;

	stream << (uint8)5;
	stream << BW::string( "TestString" );

	ScriptDataSink sink;
	CHECK( sink.writeCustomType( *pFixedDictType, stream,
			/* isPersistentOnly */ false ) );
	CHECK_EQUAL( 0, stream.remainingLength() );
	CHECK( !stream.error() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( wrapIntStrType.isObjectOfType( resultObject ) );
	BW::string strVal;
	uint8 intVal;
	CHECK( resultObject.getAttribute( "intVal", intVal, ScriptErrorPrint() ) );
	CHECK_EQUAL( (uint8)5, intVal );
	CHECK( resultObject.getAttribute( "strVal", strVal, ScriptErrorPrint() ) );
	CHECK_EQUAL( BW::string( "TestString" ), strVal );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_PyFixedDict_as_stream_custom )
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

	// See FixedDictWrappers.WrapperWithStreaming
	BW::string inputBlob( "5||TestString" );

	MemoryOStream stream;

	stream << inputBlob;

	ScriptDataSink sink;
	CHECK( sink.writeCustomType( *pFixedDictType, stream,
			/* isPersistentOnly */ false ) );
	CHECK_EQUAL( 0, stream.remainingLength() );
	CHECK( !stream.error() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( wrapIntStrType.isObjectOfType( resultObject ) );
	BW::string strVal;
	uint8 intVal = 0;
	CHECK( resultObject.getAttribute( "intVal", intVal, ScriptErrorPrint() ) );
	CHECK_EQUAL( (unsigned)5, (unsigned)intVal );
	CHECK( resultObject.getAttribute( "strVal", strVal, ScriptErrorPrint() ) );
	CHECK_EQUAL( BW::string( "TestString" ), strVal );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_PyFixedDict_as_section )
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

	// Normal BW DataSection version of int8 then string.
	DataSectionPtr pInputSection = getDataSectionForXML(
		"<root> "
			" <int8_5> 5 </int8_5> "
			" <str_TestString> TestString </str_TestString> "
		"</root>" );

	ScriptDataSink sink;
	CHECK( sink.writeCustomType( *pFixedDictType, *pInputSection ) );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( wrapIntStrType.isObjectOfType( resultObject ) );
	BW::string strVal;
	uint8 intVal = 0;
	CHECK( resultObject.getAttribute( "intVal", intVal, ScriptErrorPrint() ) );
	CHECK_EQUAL( (uint8)5, intVal );
	CHECK( resultObject.getAttribute( "strVal", strVal, ScriptErrorPrint() ) );
	CHECK_EQUAL( BW::string( "TestString" ), strVal );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_PyUserType_as_stream )
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

	// See UserTypes.IntStrType
	BW::string inputBlob( "5||TestString" );

	MemoryOStream stream;

	stream << inputBlob;

	ScriptDataSink sink;
	CHECK( sink.writeCustomType( *pUserType, stream,
			/* isPersistentOnly */ false ) );
	CHECK_EQUAL( 0, stream.remainingLength() );
	CHECK( !stream.error() );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( intStrType.isObjectOfType( resultObject ) );
	BW::string strVal;
	uint8 intVal = 0;
	CHECK( resultObject.getAttribute( "intVal", intVal, ScriptErrorPrint() ) );
	CHECK_EQUAL( (unsigned)5, (unsigned)intVal );
	CHECK( resultObject.getAttribute( "strVal", strVal, ScriptErrorPrint() ) );
	CHECK_EQUAL( BW::string( "TestString" ), strVal );
}


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_PyUserType_as_section )
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

	// See UserTypes.IntStrType
	DataSectionPtr pInputSection = getDataSectionForXML(
		"<root> "
			" <intVal> 5 </intVal> "
			" <strVal> TestString </strVal> "
		"</root>" );


	ScriptDataSink sink;
	CHECK( sink.writeCustomType( *pUserType, *pInputSection ) );

	ScriptObject resultObject = sink.finalise();
	RETURN_ON_FAIL_CHECK( resultObject.exists() );
	RETURN_ON_FAIL_CHECK( intStrType.isObjectOfType( resultObject ) );
	BW::string strVal;
	uint8 intVal;
	CHECK( resultObject.getAttribute( "intVal", intVal, ScriptErrorPrint() ) );
	CHECK_EQUAL( (uint8)5, intVal );
	CHECK( resultObject.getAttribute( "strVal", strVal, ScriptErrorPrint() ) );
	CHECK_EQUAL( BW::string( "TestString" ), strVal );
}
#endif // defined( SCRIPT_PYTHON )


TEST_F( EntityDefUnitTestHarness, Script_ScriptDataSink_object_passthrough )
{
	ScriptString inputObject = ScriptString::create( "Test string" );
	ScriptDataSink sink;
	CHECK( sink.write( inputObject ) );
	ScriptObject resultObject = sink.finalise();
	CHECK_EQUAL( inputObject, resultObject );
}


} // namespace BW

// test_script_data_sink.cpp
