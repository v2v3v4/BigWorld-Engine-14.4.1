#include "pch.hpp"

#include "test_harness.hpp"

#include "entitydef/data_type.hpp"
#include "entitydef/script_data_sink.hpp"
#include "entitydef/script_data_source.hpp"

#include "resmgr/datasection.hpp"
#include "resmgr/xml_section.hpp"

#if defined( SCRIPT_PYTHON )
#include "entitydef/data_instances/array_data_instance.hpp"
#include "entitydef/data_instances/fixed_dict_data_instance.hpp"
#include "entitydef/data_types/fixed_dict_data_type.hpp"
#endif // defined( SCRIPT_PYTHON )

BW_BEGIN_NAMESPACE

//#define TEST_DATASECTION_HACK_MSG

namespace
{

/**
 *	Utility method to get a DataSectionPtr given some XML
 */
DataSectionPtr getDataSectionForXML( const BW::string & dataSectionStr )
{
	BW::stringstream dataStrStream;
	dataStrStream << dataSectionStr;
	XMLSectionPtr xml = XMLSection::createFromStream( "", dataStrStream );
	return xml;
}


/**
 *	This function determines if the two ScriptObjects are equal,
 *	allowing for BigWorld-specific type differences.
 *
 *	@param	left	The ScriptObject created by evaluating the
 *					pythonStr given to the object constructor
 *	@param	right	The ScriptObject created by DataType::createFromSection
 *	@param	result	Whether the two objects are equal by our measure
 *	@return	true unless a script call return an error.
 */
bool isEqual( ScriptObject left, ScriptObject right, bool & result )
{
	// Firstly, if they're both ScriptFloats, just use almostEqual
	if (ScriptFloat::check( left ) && ScriptFloat::check( right ))
	{
		ScriptFloat leftFloat( left );
		ScriptFloat rightFloat( right );

		result = almostEqual( leftFloat.asDouble(), rightFloat.asDouble() );
		return true;
	}

	// Otherwise, try the default ScriptObject::compareTo
	result = (left.compareTo( right, ScriptErrorRetain() ) == 0);

	if (Script::hasError())
	{
		Script::printError();
		return false;
	}

	if (result)
	{
		return true;
	}

	// If that didn't match, try instantiating an instance of right's class
	// using left as the argument.
	ScriptType leftType = ScriptType::getType( left );
	ScriptType rightType = ScriptType::getType( right );
	if (leftType != rightType)
	{
		MF_ASSERT( rightType.isCallable() );
		ScriptObject leftConverted = rightType.callFunction(
			ScriptArgs::create( left ), ScriptErrorPrint() );

		if (leftConverted.exists())
		{
			result =
				(leftConverted.compareTo( right, ScriptErrorRetain() ) == 0);
			if (result)
			{
				return true;
			}
		}
	}

#if defined( SCRIPT_PYTHON )
	// Now try manual conversions for some known instance types:
	// ScriptSequence against PyArrayDataInstance
	if (PyArrayDataInstancePtr::check( right ) &&
		ScriptSequence::check( left ))
	{
		PyArrayDataInstancePtr rightPyArray( right );
		ScriptSequence leftSeq( left );
		PyArrayDataInstancePtr leftPyArray(
			new PyArrayDataInstance( rightPyArray->dataType(), leftSeq.size() ),
			PyArrayDataInstancePtr::FROM_NEW_REFERENCE );
		leftPyArray->setInitialSequence( leftSeq );
		
		result = (leftPyArray.compareTo( right, ScriptErrorRetain() ) == 0);

		if (Script::hasError())
		{
			Script::printError();
			return false;
		}

		if (result)
		{
			return true;
		}
	}

	// ScriptMapping against PyFixedDictDataInstance
	if (PyFixedDictDataInstancePtr::check( right ) &&
		ScriptMapping::check( left ))
	{
		PyFixedDictDataInstancePtr rightPyFixedDict( right );
		ScriptMapping leftMap( left );

		// FixedDictDataType has some surprisingly non-const methods...
		FixedDictDataType * pDataType =
			const_cast< FixedDictDataType * >( &rightPyFixedDict->dataType() );

		PyFixedDictDataInstancePtr leftPyFixedDict(
			new PyFixedDictDataInstance( pDataType ),
			PyFixedDictDataInstancePtr::FROM_NEW_REFERENCE );

		ScriptObject leftInstance =
			pDataType->createInstanceFromMappingObj( leftMap );
		
		result = (leftInstance.compareTo( right, ScriptErrorRetain() ) == 0);

		if (Script::hasError())
		{
			Script::printError();
			return false;
		}

		if (result)
		{
			return true;
		}
	}
#endif // defined( SCRIPT_PYTHON )

	// If we got this far, they're just not equal.
	MF_ASSERT( result == false );
	return true;
}


/**
 *	This functor tests the DataSection format of an object.
 *
 *	It requires a DataType to test, the input Python object and the expected
 *	DataSection result.
 *
 *	It tests that DataType::createFromSection is able to convert the result
 *	into an object matching the input.
 *
 *	It tests that DataType::addToSection is able to convert the input into a
 *	DataSection matching the result, and that DataType::createFromSection is
 *	then able to convert that DataSection back into an object matching the
 *	input.
 */
class CheckDSConversion
{
public:
	/**
	 *	Constructor
	 *
	 *	@param	typeDataSectionStr	An XML string of a DataType
	 *	@param	pythonStr			An evalable string of Python producing the
	 * 								input object.
	 *	@param	dataSectionStr		An XML string of the DataSection format we
	 *								expect.
	 */
	CheckDSConversion( const BW::string & typeDataSectionStr,
		const BW::string & pythonStr, const BW::string & dataSectionStr ) :
			pDataType_( DataType::buildDataType( typeDataSectionStr ) ),
			inputObject_( Script::runString(
					pythonStr.c_str(), /* printResult */ false ),
				ScriptObject::FROM_NEW_REFERENCE ),
			pResultDS_( getDataSectionForXML( dataSectionStr ) )
	{
	}

	TEST_SUB_FUNCTOR_OPERATOR
	{
		CHECK( !Script::hasError() );
		Script::printError();

		// Check all our fixed inputs
		RETURN_ON_FAIL_CHECK( pDataType_.exists() );
		RETURN_ON_FAIL_CHECK( pResultDS_.exists() );
		RETURN_ON_FAIL_CHECK( inputObject_.exists() );
#ifdef TEST_DATASECTION_HACK_MSG
		HACK_MSG( "inputObject_: %s\n",
			inputObject_.str( ScriptErrorPrint() ).c_str() );
		HACK_MSG( "pResultDS_: %s\n", pResultDS_->asString().c_str() );
#endif // TEST_DATASECTION_HACK_MSG

		// Check that the given XMLSection produces a matching object
		ScriptDataSink resultSink;
		CHECK(
			pDataType_->createFromSection( pResultDS_, resultSink ) );
		ScriptObject resultObject = resultSink.finalise();
		CHECK( !Script::hasError() );
		Script::printError();
		RETURN_ON_FAIL_CHECK( resultObject.exists() );
#ifdef TEST_DATASECTION_HACK_MSG
		HACK_MSG( "pResultDS_->ScriptObject: %s\n",
			resultObject.str( ScriptErrorPrint() ).c_str() );
#endif // TEST_DATASECTION_HACK_MSG

		CHECK( pDataType_->isSameType( inputObject_ ) );
		CHECK( pDataType_->isSameType( resultObject ) );

		bool result;
		CHECK( isEqual( inputObject_, resultObject, result ) );
		CHECK_EQUAL( true, result );
		CHECK( !Script::hasError() );

		// Check that the given object produces a matching XMLSection
		DataSectionPtr pOutputDS( new XMLSection( "value" ) );
		ScriptDataSource source( inputObject_ );
		CHECK( pDataType_->addToSection( source, pOutputDS ) );
#ifdef TEST_DATASECTION_HACK_MSG
		HACK_MSG( "inputObject_->DataSection: %s\n",
			pOutputDS->asString().c_str() );
#endif // TEST_DATASECTION_HACK_MSG

		CHECK( !pResultDS_->compare( pOutputDS ) );

		// Check that the generated XMLSection produces a matching
		// object
		ScriptDataSink testSink;
		CHECK( pDataType_->createFromSection( pOutputDS, testSink ) );
		ScriptObject testObject = testSink.finalise();
		CHECK( !Script::hasError() );
		Script::printError();
		RETURN_ON_FAIL_CHECK( testObject.exists() );

#ifdef TEST_DATASECTION_HACK_MSG
		HACK_MSG( "inputObject_->DataSection->ScriptObject: %s\n",
			testObject.str( ScriptErrorPrint() ).c_str() );
#endif // TEST_DATASECTION_HACK_MSG
		CHECK( isEqual( inputObject_, testObject, result ) );
		CHECK_EQUAL( true, result );

		CHECK( !Script::hasError() );
		Script::printError();
	}

private:
	DataTypePtr pDataType_;
	ScriptObject inputObject_;
	DataSectionPtr pResultDS_;
};

} // Anonymous namespace


TEST_F( EntityDefUnitTestHarness, Script_DataSection_basictypes )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> BLOB </Type>", "'\\0\\1\\2'",
			// Base64 encoding of 0x0 0x1 0x2
			"<value> AAEC </value>" ), "BLOB_1" );
	
	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> FLOAT32 </Type>", "56.75",
			"<value> 56.750000 </value>" ), "FLOAT32_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> FLOAT64 </Type>", "123456.75",
			"<value> 123456.75000000000000000 </value>" ), "FLOAT64_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> INT8 </Type>", "127",
			"<value> 127 </value>" ), "INT8_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> INT8 </Type>", "-128",
			"<value> -128 </value>" ), "INT8_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> INT16 </Type>", "32767",
			"<value> 32767 </value>" ), "INT16_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> INT16 </Type>", "-32768",
			"<value> -32768 </value>" ), "INT16_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> INT32 </Type>", "2147483647",
			"<value> 2147483647 </value>" ), "INT32_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> INT32 </Type>", "-2147483648",
			"<value> -2147483648 </value>" ), "INT32_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> INT64 </Type>", "9223372036854775807L",
			"<value> 9223372036854775807 </value>" ), "INT64_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> INT64 </Type>", "-9223372036854775808L",
			"<value> -9223372036854775808 </value>" ), "INT64_2" );

#if 0 && defined( MF_SERVER )
	// TODO: Create Mailbox from Python object string? Can we do that?
	// Or just a specifialised functor?
	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> MAILBOX </Type>", "None",
			"<value>  </value>" ), "MAILBOX_1" );
#endif // defined( MF_SERVER )

#if 0
	// TODO: These don't match because we can't round-trip pickles.
	// PythonDataType::isExpression sees anything that doesn't end in
	// '=' as an expression rather than a Pickle, so when we try
	// to read the pickle out of the DataSection, PythonDataType tries
	// to evaluate the Base64-encoded value instead.
	// Unless you're lucky and your pickle gets Base64-padding, like
	// PYTHON_4 does.
	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> PYTHON </Type>", "'A string'",
			"<value> gAJVCEEgc3RyaW5ncQEu </value>" ), "PYTHON_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> PYTHON </Type>", "543",
			"<value> gAJNHwIu </value>" ), "PYTHON_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> PYTHON </Type>", "4.2325",
			"<value> gAJHQBDuFHrhR64u </value>" ), "PYTHON_3" );
#endif

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> PYTHON </Type>", "[ 3, 'a', 4.5 ]",
			"<value> gAJdcQEoSwNVAWFHQBIAAAAAAABlLg== </value>" ), "PYTHON_4" );

#if 0
	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> PYTHON </Type>",
			"[ 3, 'a', 4.5, [ 'b', -0.01 ] ]",
			"<value> gAJdcQEoSwNVAWFHQBIAAAAAAABdcQIoVQFiR7+EeuFHrhR7ZWUu </value>" ),
		"PYTHON_5" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> PYTHON </Type>",
			"( 'hello', 3, -4.56 )",
			"<value> gAJVBWhlbGxvSwNHwBI9cKPXCj2HcQEu </value>" ),
		"PYTHON_6" );
#endif

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> STRING </Type>", "'hello!'",
			"<value> hello! </value>" ), "STRING_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> UINT8 </Type>", "255",
			"<value> 255 </value>" ), "UINT8_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> UINT16 </Type>", "65535",
			"<value> 65535 </value>" ), "UINT16_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> UINT32 </Type>", "4294967295",
			"<value> 4294967295 </value>" ), "UINT32_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> UINT64 </Type>",
			"18446744073709551615L",
			"<value> 18446744073709551615 </value>" ),
		"UINT64_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> UNICODE_STRING </Type>", "u'hello!'",
			"<value> hello! </value>" ), "UNICODE_STRING_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> UNICODE_STRING </Type>", "u'hello'",
			"<value> hello </value>" ), "UNICODE_STRING_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> VECTOR2 </Type>",
			"( -1.23456, 34567.89 )",
			"<value> -1.234560 34567.890625 </value>" ),
		"VECTOR2_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> VECTOR3 </Type>",
			"( 1.0, 2.0, 3.0 )",
			"<value> 1.000000 2.000000 3.000000 </value>" ),
		"VECTOR3_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> VECTOR4 </Type>",
			"( 3, 4, 5, 6 )",
			"<value> 3.000000 4.000000 5.000000 6.000000 </value>" ),
		"VECTOR4_1" );

	Script::runString( "import Math", /* printResult */ true );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> VECTOR2 </Type>",
			"Math.Vector2( -1.23456, 34567.9 )",
			"<value> -1.234560 34567.898438 </value>" ),
		"VECTOR2_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> VECTOR3 </Type>",
			"Math.Vector3( 1.0, 2.0, 3.0 )",
			"<value> 1.000000 2.000000 3.000000 </value>" ),
		"VECTOR3_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion( "<Type> VECTOR4 </Type>",
			"Math.Vector4( 3, 4, 5, 6 )",
			"<value> 3.000000 4.000000 5.000000 6.000000 </value>" ),
		"VECTOR4_2" );
}


TEST_F( EntityDefUnitTestHarness, Script_DataSection_ARRAY )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion(
			"<Type> ARRAY <of> UINT32 </of> </Type>",
			"[ 8, 16, 400 ]",
			"<value> "
				"<item> 8 </item> "
				"<item> 16 </item> "
				"<item> 400 </item> "
			"</value>" ),
		"ARRAY_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion(
			"<Type> ARRAY <of> ARRAY <of> UINT32 </of> </of> </Type>",
			"[ [ 8, 16, 400 ], [ 5, 7 ] ]",
			"<value> "
				"<item> "
					"<item> 8 </item> "
					"<item> 16 </item> "
					"<item> 400 </item> "
				"</item> "
				"<item> "
					"<item> 5 </item> "
					"<item> 7 </item> "
				"</item> "
			"</value>" ),
		"ARRAY_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion(
			"<Type> ARRAY <of> UINT32 </of> <size> 3 </size> </Type>",
			"[ 8, 16, 400 ]",
			"<value> "
				"<item> 8 </item> "
				"<item> 16 </item> "
				"<item> 400 </item> "
			"</value>" ),
		"ARRAY_3" );
}


TEST_F( EntityDefUnitTestHarness, Script_DataSection_TUPLE )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion(
			"<Type> TUPLE <of> UINT32 </of> </Type>",
			"( 8, 16, 400 )",
			"<value> "
				"<item> 8 </item> "
				"<item> 16 </item> "
				"<item> 400 </item> "
			"</value>" ),
		"TUPLE_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion(
			"<Type> TUPLE <of> TUPLE <of> UINT32 </of> </of> </Type>",
			"( ( 8, 16, 400 ), ( 5, 7 ) )",
			"<value> "
				"<item> "
					"<item> 8 </item> "
					"<item> 16 </item> "
					"<item> 400 </item> "
				"</item> "
				"<item> "
					"<item> 5 </item> "
					"<item> 7 </item> "
				"</item> "
			"</value>" ),
		"TUPLE_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion(
			"<Type> TUPLE <of> UINT32 </of> <size> 3 </size> </Type>",
			"( 8, 16, 400 )",
			"<value> "
				"<item> 8 </item> "
				"<item> 16 </item> "
				"<item> 400 </item> "
			"</value>" ),
		"TUPLE_3" );

}


TEST_F( EntityDefUnitTestHarness, Script_DataSection_FIXED_DICT )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> </intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> false </AllowNone> </Type>",
			"{ 'intVal': 5, 'strVal': 'string' }",
			"<value> "
				"<intVal> 5 </intVal> "
				"<strVal> string </strVal> "
			"</value>" ),
		"FIXED_DICT_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> </intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> true </AllowNone> </Type>",
			"{ 'intVal': 5, 'strVal': 'string' }",
			"<value> "
				"<intVal> 5 </intVal> "
				"<strVal> string </strVal> "
			"</value>" ),
		"FIXED_DICT_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckDSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> </intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> true </AllowNone> </Type>",
			"None",
			"<value> </value>" ),
		"FIXED_DICT_3" );
}

BW_END_NAMESPACE

// test_datasection.cpp
