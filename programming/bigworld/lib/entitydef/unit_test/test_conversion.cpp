#include "pch.hpp"

#include "test_harness.hpp"
#include "integer_range_checker.hpp"

#include "entitydef/data_description.hpp"


BW_BEGIN_NAMESPACE

TEST_F( EntityDefUnitTestHarness, EntityDef_intConversion )
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


namespace
{

class Visitor : public IntegerRangeCheckerVisitor
{
public:
	Visitor( const char * typeName ) : pType_( NULL )
	{
		MetaDataType * pMetaDataType = MetaDataType::find( typeName );
		MF_ASSERT( pMetaDataType != NULL );
		if (pMetaDataType == NULL)
		{
			ERROR_MSG( "findSameRange: Could not find %s\n", typeName );
		}
		else
		{
			pType_ = pMetaDataType->getType( NULL );
		}

		if (pType_ == NULL)
		{
			ERROR_MSG( "findSameRange: Could not create %s\n", typeName );
		}
	}

private:
	virtual bool visit( PyObject * pObject ) const
	{
		return pType_ ? pType_->isSameType( ScriptObject( pObject, 
			ScriptObject::FROM_BORROWED_REFERENCE ) ) : false;
	};

	DataTypePtr pType_;
};

} // anonymous namespace


TEST_F( EntityDefUnitTestHarness, Script_checkRange )
{
	IntegerRangeChecker checker;
	size_t start = 0;
	size_t end = 0;

	// Signed integers

	CHECK( checker.findSameRange( Visitor( "INT8" ), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<int8>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<int8>::end(), end );

	CHECK( checker.findSameRange( Visitor( "INT16" ), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<int16>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<int16>::end(), end );

	CHECK( checker.findSameRange( Visitor( "INT32" ), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<int32>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<int32>::end(), end );

	CHECK( checker.findSameRange( Visitor( "INT64" ), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<int64>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<int64>::end(), end );

	// Unsigned integers

	CHECK( checker.findSameRange( Visitor( "UINT8" ), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint8>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint8>::end(), end );

	CHECK( checker.findSameRange( Visitor( "UINT16" ), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint16>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint16>::end(), end );

	CHECK( checker.findSameRange( Visitor( "UINT32" ), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint32>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint32>::end(), end );

	CHECK( checker.findSameRange( Visitor( "UINT64" ), start, end ) );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint64>::start(), start );
	CHECK_EQUAL( IntegerRangeCheckerResult<uint64>::end(), end );
}

BW_END_NAMESPACE

// test_conversion.cpp
