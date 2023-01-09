#include "pch.hpp"

#include "test_harness.hpp"

#include "entitydef/data_type.hpp"
#include "entitydef/script_data_sink.hpp"
#include "entitydef/script_data_source.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/memory_stream.hpp"

#include "resmgr/datasection.hpp"
#include "resmgr/xml_section.hpp"

#if defined( SCRIPT_PYTHON )
#include "entitydef/data_instances/array_data_instance.hpp"
#include "entitydef/data_instances/fixed_dict_data_instance.hpp"
#include "entitydef/data_types/fixed_dict_data_type.hpp"
#endif // defined( SCRIPT_PYTHON )

BW_BEGIN_NAMESPACE

//#define TEST_STREAM_HACK_MSG

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
	// using left as the argument, and then vice-versa.
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

		MF_ASSERT( leftType.isCallable() );
		ScriptObject rightConverted = leftType.callFunction(
			ScriptArgs::create( right ), ScriptErrorPrint() );

		if (rightConverted.exists())
		{
			result =
				(rightConverted.compareTo( left, ScriptErrorRetain() ) == 0);

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

		result = leftPyArray->equals_seq( right );

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
 *	This functor tests the Binary*Stream format of an object.
 *
 *	It requires a DataType to test, the input Python object and the expected
 *	BinaryStream result as a BW::string.
 *
 *	It tests that DataType::createFromStream is able to convert the result
 *	into an object matching the input.
 *
 *	It tests that DataType::addToStream is able to convert the input into a
 *	stream matching the result, and that DataType::createFromStream is
 *	then able to convert that stream back into an object matching the
 *	input.
 */
class CheckBSConversion
{
public:
	/**
	 *	Constructor
	 *
	 *	@param	typeDataSectionStr	An XML string of a DataType
	 *	@param	pythonStr			An evalable string of Python producing the
	 * 								input object.
	 *	@param	streamStr			A string of the Binary*Stream format we
	 *								expect.
	 */
	CheckBSConversion( const BW::string & typeDataSectionStr,
		const BW::string & pythonStr, const char * blob, size_t blobLen,
		bool isPersistentOnly = false ) :
			pDataType_( DataType::buildDataType(
				getDataSectionForXML( typeDataSectionStr ) ) ),
			inputObject_( Script::runString(
					pythonStr.c_str(), /* printResult */ false ),
				ScriptObject::FROM_NEW_REFERENCE ),
			resultString_( blob, blobLen ),
			isPersistentOnly_( isPersistentOnly )
	{
	}

	TEST_SUB_FUNCTOR_OPERATOR
	{
		CHECK( !Script::hasError() );
		Script::printError();

		// Check all our fixed inputs
		RETURN_ON_FAIL_CHECK( pDataType_.exists() );
		RETURN_ON_FAIL_CHECK( inputObject_.exists() );
		MemoryIStream resultStream( resultString_.data(),
			static_cast<int>(resultString_.size()) );
#ifdef TEST_STREAM_HACK_MSG
		HACK_MSG( "inputObject_: %s\n",
			inputObject_.str( ScriptErrorPrint() ).c_str() );
		HACK_MSG( "resultStream: %s\n",
			resultStream.remainingBytesAsDebugString( 256,
				/* shouldConsume */ false ).c_str() );
#endif // TEST_STREAM_HACK_MSG

		// Check that the given XMLSection produces a matching object
		ScriptDataSink resultSink;
		CHECK( pDataType_->createFromStream( resultStream, resultSink,
			isPersistentOnly_ ) );
		ScriptObject resultObject = resultSink.finalise();
		CHECK( !resultStream.error() );
		CHECK_EQUAL( 0, resultStream.remainingLength() );
		CHECK( !Script::hasError() );
		Script::printError();
		RETURN_ON_FAIL_CHECK( resultObject.exists() );
#ifdef TEST_STREAM_HACK_MSG
		HACK_MSG( "resultStream->ScriptObject: %s\n",
			resultObject.str( ScriptErrorPrint() ).c_str() );
#endif // TEST_STREAM_HACK_MSG

		CHECK( pDataType_->isSameType( inputObject_ ) );
		CHECK( pDataType_->isSameType( resultObject ) );

		bool result;
		CHECK( isEqual( inputObject_, resultObject, result ) );
		CHECK_EQUAL( true, result );
		CHECK( !Script::hasError() );

		// Check that the given object produces a matching XMLSection
		MemoryOStream outputStream;
		ScriptDataSource source( inputObject_ );
		CHECK( pDataType_->addToStream( source, outputStream,
			isPersistentOnly_ ) );
#ifdef TEST_STREAM_HACK_MSG
		HACK_MSG( "inputObject_->Binary*Stream: %s\n",
			outputStream.remainingBytesAsDebugString( 256,
				/* shouldConsume */ false ).c_str() );
#endif // TEST_STREAM_HACK_MSG

		outputStream.rewind();

		size_t outputSize = outputStream.remainingLength();
		BW::string outputStreamString(
			static_cast< const char* >(outputStream.retrieve(
				static_cast<int>(outputSize)) ),
			outputSize );

		CHECK_EQUAL( resultString_, outputStreamString );

		// Check that the generated BinaryStream produces a matching
		// object
		outputStream.rewind();
		ScriptDataSink testSink;
		CHECK( pDataType_->createFromStream( outputStream, testSink,
			isPersistentOnly_ ) );
		ScriptObject testObject = testSink.finalise();
		CHECK( !Script::hasError() );
		Script::printError();
		RETURN_ON_FAIL_CHECK( testObject.exists() );

#ifdef TEST_STREAM_HACK_MSG
		HACK_MSG( "inputObject_->Binary*Stream->ScriptObject: %s\n",
			testObject.str( ScriptErrorPrint() ).c_str() );
#endif // TEST_STREAM_HACK_MSG
		CHECK( isEqual( inputObject_, testObject, result ) );
		CHECK_EQUAL( true, result );

		CHECK( !Script::hasError() );
		Script::printError();
	}

private:
	DataTypePtr pDataType_;
	ScriptObject inputObject_;
	const BW::string resultString_;
	bool isPersistentOnly_;
};

} // Anonymous namespace


TEST_F( EntityDefUnitTestHarness, Script_BinaryStream_basictypes )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> BLOB </Type>", "'\\0\\1\\2'",
			"\x03\x00\x01\x02", 4 ), "BLOB_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> FLOAT32 </Type>", "56.75",
			"\x00\x00\x63\x42", 4 ), "FLOAT32_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> FLOAT64 </Type>", "123456.789",
			"\xc9\x76\xbe\x9f\x0c\x24\xfe\x40", 8 ), "FLOAT64_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT8 </Type>", "127",
			"\x7f", 1 ), "INT8_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT8 </Type>", "-128",
			"\x80", 1 ), "INT8_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT16 </Type>", "32767",
			"\xff\x7f", 2 ), "INT16_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT16 </Type>", "-32768",
			"\x00\x80", 2 ), "INT16_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT32 </Type>", "2147483647",
			"\xff\xff\xff\x7f", 4 ), "INT32_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT32 </Type>", "-2147483648",
			"\x00\x00\x00\x80", 4 ), "INT32_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT64 </Type>", "9223372036854775807L",
			"\xff\xff\xff\xff\xff\xff\xff\x7f", 8 ), "INT64_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT64 </Type>", "-9223372036854775808L",
			"\x00\x00\x00\x00\x00\x00\x00\x80", 8 ), "INT64_2" );

#if 0 && defined( MF_SERVER )
	// TODO: Create Mailbox from Python object string? Can we do that?
	// Or just a specifialised functor?
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> MAILBOX </Type>", "None",
			"<value>  </value>" ), "MAILBOX_1" );
#endif // defined( MF_SERVER )

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> PYTHON </Type>",
			"'A string'",
			"\x0f\x80\x02\x55\x08\x41\x20\x73\x74\x72\x69\x6e\x67\x71\x01\x2e",
			16 ),
		"PYTHON_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> PYTHON </Type>", "543",
			"\x06\x80\x02\x4d\x1f\x02\x2e", 7 ), "PYTHON_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> PYTHON </Type>",
			"4.2325",
			"\x0c\x80\x02\x47\x40\x10\xee\x14\x7a\xe1\x47\xae\x2e", 13 ),
		"PYTHON_3" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> PYTHON </Type>", "[ 3, 'a', 4.5 ]",
			"\x16\x80\x02\x5d\x71\x01\x28\x4b\x03\x55\x01\x61\x47\x40"
				"\x12\x00\x00\x00\x00\x00\x00\x65\x2e",
			23 ),
		"PYTHON_4" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> PYTHON </Type>",
			"[ 3, 'a', 4.5, [ 'b', -0.01 ] ]",
			"\x27\x80\x02\x5d\x71\x01\x28\x4b\x03\x55\x01\x61\x47\x40"
				"\x12\x00\x00\x00\x00\x00\x00\x5d\x71\x02\x28\x55\x01\x62"
				"\x47\xbf\x84\x7a\xe1\x47\xae\x14\x7b\x65\x65\x2e", 40 ),
		"PYTHON_5" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> PYTHON </Type>",
			"( 'hello', 3, -4.56 )",
			"\x18\x80\x02\x55\x05\x68\x65\x6c\x6c\x6f\x4b\x03\x47\xc0"
				"\x12\x3d\x70\xa3\xd7\x0a\x3d\x87\x71\x01\x2e",
			25 ),
		"PYTHON_6" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> STRING </Type>", "'hello!'",
			"\x06hello!", 7 ), "STRING_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> UINT8 </Type>", "255",
			"\xff", 1 ), "UINT8_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> UINT16 </Type>", "65535",
			"\xff\xff", 2 ), "UINT16_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> UINT32 </Type>", "4294967295",
			"\xff\xff\xff\xff", 4 ), "UINT32_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> UINT64 </Type>",
			"18446744073709551615L",
			"\xff\xff\xff\xff\xff\xff\xff\xff", 8 ),
		"UINT64_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> UNICODE_STRING </Type>", "u'hello!'",
			"\x06hello!", 7 ), "UNICODE_STRING_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> UNICODE_STRING </Type>", "u'hello'",
			"\x05hello", 6 ), "UNICODE_STRING_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> VECTOR2 </Type>",
			"( -1.23456, 34567.89 )",
			"\x10\x06\x9e\xbf\xe4\x07\x07\x47", 8 ),
		"VECTOR2_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> VECTOR3 </Type>",
			"( 1.0, 2.0, 3.0 )",
			"\x00\x00\x80\x3f\x00\x00\x00\x40\x00\x00\x40\x40", 12 ),
		"VECTOR3_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> VECTOR4 </Type>",
			"( 3, 4, 5, 6 )",
			"\x00\x00\x40\x40\x00\x00\x80\x40\x00\x00\xa0\x40\x00\x00\xc0\x40", 16 ),
		"VECTOR4_1" );

	Script::runString( "import Math", /* printResult */ true );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> VECTOR2 </Type>",
			"Math.Vector2( -1.23456, 34567.9 )",
			"\x10\x06\x9e\xbf\xe6\x07\x07\x47", 8 ),
		"VECTOR2_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> VECTOR3 </Type>",
			"Math.Vector3( 1.0, 2.0, 3.0 )",
			"\x00\x00\x80\x3f\x00\x00\x00\x40\x00\x00\x40\x40", 12 ),
		"VECTOR3_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> VECTOR4 </Type>",
			"Math.Vector4( 3, 4, 5, 6 )",
			"\x00\x00\x40\x40\x00\x00\x80\x40\x00\x00\xa0\x40\x00\x00\xc0\x40", 16 ),
		"VECTOR4_2" );
}


TEST_F( EntityDefUnitTestHarness, Script_BinaryStream_basictypes_persistentOnly )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> BLOB </Type>", "'\\0\\1\\2'",
			"\x03\x00\x01\x02", 4, true ), "BLOB_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> FLOAT32 </Type>", "56.75",
			"\x00\x00\x63\x42", 4, true ), "FLOAT32_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> FLOAT64 </Type>", "123456.789",
			"\xc9\x76\xbe\x9f\x0c\x24\xfe\x40", 8, true ), "FLOAT64_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT8 </Type>", "127",
			"\x7f", 1, true ), "INT8_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT8 </Type>", "-128",
			"\x80", 1, true ), "INT8_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT16 </Type>", "32767",
			"\xff\x7f", 2, true ), "INT16_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT16 </Type>", "-32768",
			"\x00\x80", 2, true ), "INT16_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT32 </Type>", "2147483647",
			"\xff\xff\xff\x7f", 4, true ), "INT32_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT32 </Type>", "-2147483648",
			"\x00\x00\x00\x80", 4, true ), "INT32_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT64 </Type>", "9223372036854775807L",
			"\xff\xff\xff\xff\xff\xff\xff\x7f", 8, true ), "INT64_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> INT64 </Type>", "-9223372036854775808L",
			"\x00\x00\x00\x00\x00\x00\x00\x80", 8, true ), "INT64_2" );

#if 0 && defined( MF_SERVER )
	// TODO: Create Mailbox from Python object string? Can we do that?
	// Or just a specifialised functor?
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> MAILBOX </Type>", "None",
			"<value>  </value>", true ), "MAILBOX_1" );
#endif // defined( MF_SERVER )

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> PYTHON </Type>",
			"'A string'",
			"\x0f\x80\x02\x55\x08\x41\x20\x73\x74\x72\x69\x6e\x67\x71\x01\x2e",
			16, true ),
		"PYTHON_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> PYTHON </Type>", "543",
			"\x06\x80\x02\x4d\x1f\x02\x2e", 7, true ), "PYTHON_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> PYTHON </Type>",
			"4.2325",
			"\x0c\x80\x02\x47\x40\x10\xee\x14\x7a\xe1\x47\xae\x2e", 13, true ),
		"PYTHON_3" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> PYTHON </Type>", "[ 3, 'a', 4.5 ]",
			"\x16\x80\x02\x5d\x71\x01\x28\x4b\x03\x55\x01\x61\x47\x40"
				"\x12\x00\x00\x00\x00\x00\x00\x65\x2e",
			23, true ),
		"PYTHON_4" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> PYTHON </Type>",
			"[ 3, 'a', 4.5, [ 'b', -0.01 ] ]",
			"\x27\x80\x02\x5d\x71\x01\x28\x4b\x03\x55\x01\x61\x47\x40"
				"\x12\x00\x00\x00\x00\x00\x00\x5d\x71\x02\x28\x55\x01\x62"
				"\x47\xbf\x84\x7a\xe1\x47\xae\x14\x7b\x65\x65\x2e", 40, true ),
		"PYTHON_5" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> PYTHON </Type>",
			"( 'hello', 3, -4.56 )",
			"\x18\x80\x02\x55\x05\x68\x65\x6c\x6c\x6f\x4b\x03\x47\xc0"
				"\x12\x3d\x70\xa3\xd7\x0a\x3d\x87\x71\x01\x2e",
			25, true ),
		"PYTHON_6" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> STRING </Type>", "'hello!'",
			"\x06hello!", 7, true ), "STRING_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> UINT8 </Type>", "255",
			"\xff", 1, true ), "UINT8_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> UINT16 </Type>", "65535",
			"\xff\xff", 2, true ), "UINT16_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> UINT32 </Type>", "4294967295",
			"\xff\xff\xff\xff", 4, true ), "UINT32_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> UINT64 </Type>",
			"18446744073709551615L",
			"\xff\xff\xff\xff\xff\xff\xff\xff", 8, true ),
		"UINT64_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> UNICODE_STRING </Type>", "u'hello!'",
			"\x06hello!", 7, true ), "UNICODE_STRING_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> UNICODE_STRING </Type>", "u'hello'",
			"\x05hello", 6, true ), "UNICODE_STRING_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> VECTOR2 </Type>",
			"( -1.23456, 34567.89 )",
			"\x10\x06\x9e\xbf\xe4\x07\x07\x47", 8, true ),
		"VECTOR2_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> VECTOR3 </Type>",
			"( 1.0, 2.0, 3.0 )",
			"\x00\x00\x80\x3f\x00\x00\x00\x40\x00\x00\x40\x40", 12, true ),
		"VECTOR3_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> VECTOR4 </Type>",
			"( 3, 4, 5, 6 )",
			"\x00\x00\x40\x40\x00\x00\x80\x40\x00\x00\xa0\x40\x00\x00\xc0\x40", 16,
			true ),
		"VECTOR4_1" );

	Script::runString( "import Math", /* printResult */ true );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> VECTOR2 </Type>",
			"Math.Vector2( -1.23456, 34567.9 )",
			"\x10\x06\x9e\xbf\xe6\x07\x07\x47", 8, true ),
		"VECTOR2_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> VECTOR3 </Type>",
			"Math.Vector3( 1.0, 2.0, 3.0 )",
			"\x00\x00\x80\x3f\x00\x00\x00\x40\x00\x00\x40\x40", 12, true ),
		"VECTOR3_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion( "<Type> VECTOR4 </Type>",
			"Math.Vector4( 3, 4, 5, 6 )",
			"\x00\x00\x40\x40\x00\x00\x80\x40\x00\x00\xa0\x40\x00\x00\xc0\x40", 16, true ),
		"VECTOR4_2" );
}


TEST_F( EntityDefUnitTestHarness, Script_BinaryStream_ARRAY )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> ARRAY <of> UINT32 </of> "
				"<persistAsBlob> false </persistAsBlob> </Type>",
			"[ 8, 16, 400 ]",
			"\x03\x08\x00\x00\x00\x10\x00\x00\x00\x90\x01\x00\x00",
			13 ),
		"ARRAY_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> ARRAY <of> ARRAY <of> UINT32 </of> "
				"<persistAsBlob> false </persistAsBlob> </of> "
				"<persistAsBlob> false </persistAsBlob> </Type>",
			"[ [ 8, 16, 400 ], [ 5, 7 ] ]",
			"\x02"
				"\x03\x08\x00\x00\x00\x10\x00\x00\x00"
				"\x90\x01\x00\x00"
				"\x02\x05\x00\x00\x00\x07\x00\x00\x00",
			23 ),
		"ARRAY_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> ARRAY <of> UINT32 </of> <size> 3 </size> "
				"<persistAsBlob> false </persistAsBlob> </Type>",
			"[ 8, 16, 400 ]",
			"\x08\x00\x00\x00\x10\x00\x00\x00\x90\x01\x00\x00",
			12 ),
		"ARRAY_3" );
}


TEST_F( EntityDefUnitTestHarness, Script_BinaryStream_ARRAY_persistentOnly )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> ARRAY <of> UINT32 </of> "
				"<persistAsBlob> false </persistAsBlob> </Type>",
			"[ 8, 16, 400 ]",
			"\x03\x08\x00\x00\x00\x10\x00\x00\x00\x90\x01\x00\x00",
			13, true ),
		"ARRAY_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> ARRAY <of> ARRAY <of> UINT32 </of> "
				"<persistAsBlob> false </persistAsBlob> </of> "
				"<persistAsBlob> false </persistAsBlob> </Type>",
			"[ [ 8, 16, 400 ], [ 5, 7 ] ]",
			"\x02"
				"\x03\x08\x00\x00\x00\x10\x00\x00\x00"
				"\x90\x01\x00\x00"
				"\x02\x05\x00\x00\x00\x07\x00\x00\x00",
			23, true ),
		"ARRAY_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> ARRAY <of> UINT32 </of> <size> 3 </size> "
				"<persistAsBlob> false </persistAsBlob> </Type>",
			"[ 8, 16, 400 ]",
			"\x08\x00\x00\x00\x10\x00\x00\x00\x90\x01\x00\x00",
			12, true ),
		"ARRAY_3" );
}


TEST_F( EntityDefUnitTestHarness, Script_BinaryStream_ARRAY_persistentOnly_persistAsBlob )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> ARRAY <of> UINT32 </of> "
				"<persistAsBlob> true </persistAsBlob> </Type>",
			"[ 8, 16, 400 ]",
			"\x0d\x03\x08\x00\x00\x00\x10\x00\x00\x00\x90\x01\x00\x00",
			14, true ),
		"ARRAY_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> ARRAY <of> ARRAY <of> UINT32 </of> "
				"<persistAsBlob> true </persistAsBlob> </of> "
				"<persistAsBlob> true </persistAsBlob> </Type>",
			"[ [ 8, 16, 400 ], [ 5, 7 ] ]",
			"\x19\x02"
				"\x0d\x03\x08\x00\x00\x00\x10\x00\x00\x00"
				"\x90\x01\x00\x00"
				"\x09\x02\x05\x00\x00\x00\x07\x00\x00\x00",
			26, true ),
		"ARRAY_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> ARRAY <of> UINT32 </of> <size> 3 </size> "
				"<persistAsBlob> true </persistAsBlob> </Type>",
			"[ 8, 16, 400 ]",
			"\x0c\x08\x00\x00\x00\x10\x00\x00\x00\x90\x01\x00\x00",
			13, true ),
		"ARRAY_3" );
}


TEST_F( EntityDefUnitTestHarness, Script_BinaryStream_TUPLE )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> TUPLE <of> UINT32 </of> "
				"<persistAsBlob> false </persistAsBlob> </Type>",
			"( 8, 16, 400 )",
			"\x03\x08\x00\x00\x00\x10\x00\x00\x00\x90\x01\x00\x00",
			13 ),
		"TUPLE_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> TUPLE <of> TUPLE <of> UINT32 </of> "
				"<persistAsBlob> false </persistAsBlob> </of> "
				"<persistAsBlob> false </persistAsBlob> </Type>",
			"( ( 8, 16, 400 ), ( 5, 7 ) )",
			"\x02"
				"\x03\x08\x00\x00\x00\x10\x00\x00\x00"
				"\x90\x01\x00\x00"
				"\x02\x05\x00\x00\x00\x07\x00\x00\x00",
			23 ),
		"TUPLE_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> TUPLE <of> UINT32 </of> <size> 3 </size> "
				"<persistAsBlob> false </persistAsBlob> </Type>",
			"( 8, 16, 400 )",
			"\x08\x00\x00\x00\x10\x00\x00\x00\x90\x01\x00\x00",
			12 ),
		"TUPLE_3" );
}


TEST_F( EntityDefUnitTestHarness, Script_BinaryStream_TUPLE_persistentOnly )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> TUPLE <of> UINT32 </of> "
				"<persistAsBlob> false </persistAsBlob> </Type>",
			"( 8, 16, 400 )",
			"\x03\x08\x00\x00\x00\x10\x00\x00\x00\x90\x01\x00\x00",
			13, true ),
		"TUPLE_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> TUPLE <of> TUPLE <of> UINT32 </of> "
				"<persistAsBlob> false </persistAsBlob> </of> "
				"<persistAsBlob> false </persistAsBlob> </Type>",
			"( ( 8, 16, 400 ), ( 5, 7 ) )",
			"\x02"
				"\x03\x08\x00\x00\x00\x10\x00\x00\x00"
				"\x90\x01\x00\x00"
				"\x02\x05\x00\x00\x00\x07\x00\x00\x00",
			23, true ),
		"TUPLE_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> TUPLE <of> UINT32 </of> <size> 3 </size> "
				"<persistAsBlob> false </persistAsBlob> </Type>",
			"( 8, 16, 400 )",
			"\x08\x00\x00\x00\x10\x00\x00\x00\x90\x01\x00\x00",
			12, true ),
		"TUPLE_3" );
}


TEST_F( EntityDefUnitTestHarness, Script_BinaryStream_TUPLE_persistentOnly_persistAsBlob )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> TUPLE <of> UINT32 </of> "
				"<persistAsBlob> true </persistAsBlob> </Type>",
			"( 8, 16, 400 )",
			"\x0d\x03\x08\x00\x00\x00\x10\x00\x00\x00\x90\x01\x00\x00",
			14, true ),
		"TUPLE_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> TUPLE <of> TUPLE <of> UINT32 </of> "
				"<persistAsBlob> true </persistAsBlob> </of> "
				"<persistAsBlob> true </persistAsBlob> </Type>",
			"( ( 8, 16, 400 ), ( 5, 7 ) )",
			"\x19\x02"
				"\x0d\x03\x08\x00\x00\x00\x10\x00\x00\x00"
				"\x90\x01\x00\x00"
				"\x09\x02\x05\x00\x00\x00\x07\x00\x00\x00",
			26, true ),
		"TUPLE_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> TUPLE <of> UINT32 </of> <size> 3 </size> "
				"<persistAsBlob> true </persistAsBlob> </Type>",
			"( 8, 16, 400 )",
			"\x0c\x08\x00\x00\x00\x10\x00\x00\x00\x90\x01\x00\x00",
			13, true ),
		"TUPLE_3" );
}


TEST_F( EntityDefUnitTestHarness, Script_BinaryStream_FIXED_DICT )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> </intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> false </AllowNone> </Type>",
			"{ 'intVal': 5, 'strVal': 'string' }",
			"\x05\x00\x00\x00\x06\x73\x74\x72\x69\x6e\x67", 11 ),
		"FIXED_DICT_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> </intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> true </AllowNone> </Type>",
			"{ 'intVal': 5, 'strVal': 'string' }",
			"\x01\x05\x00\x00\x00\x06\x73\x74\x72\x69\x6e\x67", 12 ),
		"FIXED_DICT_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> </intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> true </AllowNone> </Type>",
			"None",
			"\x00", 1 ),
		"FIXED_DICT_3" );
}

TEST_F( EntityDefUnitTestHarness, Script_BinaryStream_FIXED_DICT_persistentOnly )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> </intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> false </AllowNone> </Type>",
			"{ 'intVal': 5, 'strVal': 'string' }",
			"\x05\x00\x00\x00\x06\x73\x74\x72\x69\x6e\x67", 11, true ),
		"FIXED_DICT_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> </intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> true </AllowNone> </Type>",
			"{ 'intVal': 5, 'strVal': 'string' }",
			"\x01\x05\x00\x00\x00\x06\x73\x74\x72\x69\x6e\x67", 12, true ),
		"FIXED_DICT_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> </intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> true </AllowNone> </Type>",
			"None",
			"\x00", 1, true ),
		"FIXED_DICT_3" );
}

TEST_F( EntityDefUnitTestHarness, Script_BinaryStream_FIXED_DICT_nonPersistentInt )
{
	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> "
					"<Persistent> false </Persistent> "
				"</intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> false </AllowNone> </Type>",
			"{ 'intVal': 5, 'strVal': 'string' }",
			"\x05\x00\x00\x00\x06\x73\x74\x72\x69\x6e\x67", 11 ),
		"FIXED_DICT_1" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> "
					"<Persistent> false </Persistent> "
				"</intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> true </AllowNone> </Type>",
			"{ 'intVal': 5, 'strVal': 'string' }",
			"\x01\x05\x00\x00\x00\x06\x73\x74\x72\x69\x6e\x67", 12 ),
		"FIXED_DICT_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> "
					"<Persistent> false </Persistent> "
				"</intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> true </AllowNone> </Type>",
			"None",
			"\x00", 1 ),
		"FIXED_DICT_3" );
}

TEST_F( EntityDefUnitTestHarness, Script_BinaryStream_FIXED_DICT_nonPersistentInt_persistentOnly )
{
	TEST_SUB_FUNCTOR_NAMED(
		// Usage note: intVal is non-persistent so will always be 0, no matter
		// what is passed in.
		CheckBSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> "
					"<Persistent> false </Persistent> "
				"</intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> false </AllowNone> </Type>",
			"{ 'intVal': 0, 'strVal': 'string' }",
			"\x06\x73\x74\x72\x69\x6e\x67", 7, true ),
		"FIXED_DICT_1" );

	TEST_SUB_FUNCTOR_NAMED(
		// Usage note: intVal is non-persistent so will always be 0, no matter
		// what is passed in.
		CheckBSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> "
					"<Persistent> false </Persistent> "
				"</intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> true </AllowNone> </Type>",
			"{ 'intVal': 0, 'strVal': 'string' }",
			"\x01\x06\x73\x74\x72\x69\x6e\x67", 8, true ),
		"FIXED_DICT_2" );

	TEST_SUB_FUNCTOR_NAMED(
		CheckBSConversion(
			"<Type> FIXED_DICT <Properties> "
				"<intVal> <Type> UINT32 </Type> "
					"<Persistent> false </Persistent> "
				"</intVal> "
				"<strVal> <Type> STRING </Type> </strVal> "
			"</Properties> <AllowNone> true </AllowNone> </Type>",
			"None",
			"\x00", 1, true ),
		"FIXED_DICT_3" );
}

BW_END_NAMESPACE

// test_stream.cpp
