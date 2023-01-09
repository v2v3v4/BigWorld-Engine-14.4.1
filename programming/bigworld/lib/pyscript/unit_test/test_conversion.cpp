#include "pch.hpp"
#include "test_harness.hpp"
#include "cstdmf/bw_namespace.hpp"

#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

TEST_F( PyScriptUnitTestHarness, Script_intConversion )
{
	int values[] = { -8000000, -1, 0, 10, 0x7fffffff };

	for (size_t i = 0; i < sizeof( values )/sizeof( values[0] ); ++i)
	{
		int value = values[i];
		PyObject * pObject  = Script::getData( value );
			CHECK( pObject != NULL );

			int newValue;
			int result = Script::setData( pObject, newValue );
			CHECK_EQUAL( 0, result );
			Py_DECREF( pObject );

			CHECK_EQUAL( value, newValue );
		}
}


template <class TYPE>
bool testLong( const char * asHexString, TYPE & result )
{
	PyObject * pValue = PyLong_FromString( (char *)asHexString, NULL, 16 );

	if (pValue == NULL)
	{
		PyErr_Clear();
		return false;
	}

	int status = Script::setData( pValue, result );
	Py_DECREF( pValue );

	if (status == -1)
	{
		PyErr_Clear();
		return false;
	}

	return (status == 0);
}


TEST_F( PyScriptUnitTestHarness, Script_intOverflow )
{
	// Currently still support the deprecated conversion from uint32
	const bool SUPPORTS_DEPRECATED = false;
	int intValue;

	CHECK_EQUAL( SUPPORTS_DEPRECATED, testLong( "80000000", intValue ) );

	CHECK_EQUAL( true,  testLong( "7fffffff", intValue ) );
	CHECK_EQUAL( false, testLong( "100000000", intValue ) );

	CHECK_EQUAL( true, testLong( "-1", intValue ) );
	CHECK_EQUAL( true, testLong( "-80000000", intValue ) );
	CHECK_EQUAL( false, testLong( "-80000001", intValue ) );

	uint32 uintValue;

	CHECK_EQUAL( true,  testLong( "80000000", uintValue ) );
	CHECK_EQUAL( true,  testLong( "7fffffff", uintValue ) );
	CHECK_EQUAL( false, testLong( "100000000", uintValue ) );

	CHECK_EQUAL( SUPPORTS_DEPRECATED, testLong( "-1", uintValue ) );
	CHECK_EQUAL( SUPPORTS_DEPRECATED, testLong( "-80000000", uintValue ) );
	CHECK_EQUAL( false, testLong( "-80000001", uintValue ) );
}


#if 0
/**
 *	This function finds the range of values that isSameType returns true for
 *	an indicated type.
 *
 *	@param typeName The name of the type. e.g. UINT16
 *	@param pValues An array of the Python integers to test against.
 *	@param numValues The size of the pValues array.
 *	@param start A reference that will be populated with the index of the first
 *		value that isSameType return true for.
 *	@param end A reference that will be populated with the index of the first
 *			value that isSameType return false for after start.
 *
 *	@return true if successful, otherwise false. This includes having more or
 *		less than one range that isSameType returns true for.
 */
template <class TYPE>
bool findSameRange( PyObjectPtr * pValues, size_t numValues,
		size_t & start, size_t & end )
{
	bool wasOkay = false;
	bool isFinished = false;

	TYPE value;

	for (size_t i = 0; i < numValues; ++i)
	{
		bool isOkay = (Script::setData( pValues[i].get(), value ) == 0);

		if (!isOkay)
		{
			PyErr_Clear();
		}

		if (PyErr_Occurred())
		{
			ERROR_MSG( "findSameRange: PyErr_Occurred\n" );
			return false;
		}

		if (!wasOkay && isOkay)
		{
			if (isFinished)
			{
				ERROR_MSG( "findSameRange: isOkay at index %zd\n", i );
				return false;
			}

			start = i;
		}
		else if (wasOkay && !isOkay)
		{
			end = i;
			isFinished = true;
		}
		else
		{
			MF_ASSERT( wasOkay == isOkay );
		}

		wasOkay = isOkay;
	}

	return isFinished;
}


TEST_F( PyScriptUnitTestHarness, Script_checkRange )
{
	PyObjectPtr pValues[] =
	{
		PyLong_FromString( "-10000000000000001", NULL, 16 ), //  0: -2**64 - 1
		PyLong_FromString( "-10000000000000000", NULL, 16 ), //  1: -2**64
		PyLong_FromString( "-8000000000000001", NULL, 16 ),  //  2: -2** 63 - 1
		PyLong_FromString( "-8000000000000000", NULL, 16 ),  //  3: -2** 63
		PyLong_FromString( "-100000001", NULL, 16 ),         //  4: -2*32 - 1
		PyLong_FromString( "-100000000", NULL, 16 ),         //  5: -2*32
		PyLong_FromString( "-80000001", NULL, 16 ),          //  6: -2*31 - 1
		Script::getData( int( 0x80000000 ) ),                //  7: -2*31
		Script::getData( -0x10001 ),                         //  8: -2*16 -1
		Script::getData( -0x10000 ),                         //  9: -2*16
		Script::getData( -0x8001 ),                          // 10: -2*15 - 1
		Script::getData( -0x8000 ),                          // 11: 2*15
		Script::getData( -0x101 ),                           // 12: -2**8 - 1
		Script::getData( -0x100 ),                           // 13: -2**8
		Script::getData( -0x81 ),                            // 14: -2**7 - 1
		Script::getData( -0x80 ),                            // 15: -2**7
		Script::getData( -0x1 ),                             // 16: -1
		Script::getData( 0 ),                                // 17: 0
		Script::getData( 0x7f ),                             // 18: 2**7 - 1
		Script::getData( 0x80 ),                             // 19: 2**7
		Script::getData( 0xff ),                             // 20: 2**8 - 1
		Script::getData( 0x100 ),                            // 21: 2**8
		Script::getData( 0x7fff ),                           // 22: 2**15 - 1
		Script::getData( 0x8000 ),                           // 23: 2**15
		Script::getData( 0xffff ),                           // 24: 2**16 - 1
		Script::getData( 0x10000 ),                          // 25: 2**16
		Script::getData( 0x7fffffff ),                       // 26: 2**31 - 1
		Script::getData( 0x80000000 ),                       // 27: 2**31
		Script::getData( 0xffffffff ),                       // 28: 2**32 - 1
		PyLong_FromString( "100000000", NULL, 16 ),          // 29: 2**32
		PyLong_FromString( "7fffffffffffffff", NULL, 16 ),   // 30: 2**63 - 1
		PyLong_FromString( "8000000000000000", NULL, 16 ),   // 31: 2**63
		PyLong_FromString( "ffffffffffffffff", NULL, 16 ),   // 32: 2**64 - 1
		PyLong_FromString( "10000000000000000", NULL, 16 ),  // 33: 2**64
	};

	CHECK( !PyErr_Occurred() );

	const size_t numValues = sizeof( pValues )/sizeof( pValues[0] );

	size_t start = 0;
	size_t end = 0;

	// Signed integers

	CHECK( findSameRange<int8>( pValues, numValues, start, end ) );
	CHECK_EQUAL( 15U, start );
	CHECK_EQUAL( 19U, end );

	CHECK( findSameRange<int16>( pValues, numValues, start, end ) );
	CHECK_EQUAL( 11U, start );
	CHECK_EQUAL( 23U, end );

	CHECK( findSameRange<int32>( pValues, numValues, start, end ) );
	CHECK_EQUAL( 7U, start );
	CHECK_EQUAL( 27U, end );

	CHECK( findSameRange<int64>( pValues, numValues, start, end ) );
	CHECK_EQUAL( 3U, start );
	CHECK_EQUAL( 31U, end );

	// Unsigned integers

	CHECK( findSameRange<uint8>( pValues, numValues, start, end ) );
	CHECK_EQUAL( 17U, start );
	CHECK_EQUAL( 21U, end );

	CHECK( findSameRange<uint16>( pValues, numValues, start, end ) );
	CHECK_EQUAL( 17U, start );
	CHECK_EQUAL( 25U, end );

	CHECK( findSameRange<uint32>( pValues, numValues, start, end ) );
	CHECK_EQUAL( 17U, start );
	CHECK_EQUAL( 29U, end );

	CHECK( findSameRange<uint64>( pValues, numValues, start, end ) );
	CHECK_EQUAL( 17U, start );
	CHECK_EQUAL( 33U, end );
}
#endif

BW_END_NAMESPACE

#include "integer_range_checker.hpp"

BW_BEGIN_NAMESPACE

namespace
{

template <class TYPE>
class Visitor : public IntegerRangeCheckerVisitor
{
private:
	virtual bool visit( PyObject * pObject ) const
	{
		TYPE value;
		bool isOkay = (Script::setData( pObject, value ) == 0);
		if (!isOkay)
		{
			PyErr_Clear();
		}

		return isOkay;
	};
};

} // anonymous namespace


TEST_F( PyScriptUnitTestHarness, EntityDef_findRange )
{
	IntegerRangeChecker checker;
	size_t start = 0;
	size_t end = 0;

	// Signed integers

	CHECK( checker.findSameRange( Visitor<int8>(), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<int8>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<int8>::end(), end );

	CHECK( checker.findSameRange( Visitor<int16>(), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<int16>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<int16>::end(), end );

	CHECK( checker.findSameRange( Visitor<int32>(), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<int32>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<int32>::end(), end );

	CHECK( checker.findSameRange( Visitor<int64>(), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<int64>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<int64>::end(), end );

	// Unsigned integers

	CHECK( checker.findSameRange( Visitor<uint8>(), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint8>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint8>::end(), end );

	CHECK( checker.findSameRange( Visitor<uint16>(), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint16>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint16>::end(), end );

	CHECK( checker.findSameRange( Visitor<uint32>(), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint32>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint32>::end(), end );

	CHECK( checker.findSameRange( Visitor<uint64>(), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint64>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint64>::end(), end );

}

BW_END_NAMESPACE

// test_conversion.cpp
