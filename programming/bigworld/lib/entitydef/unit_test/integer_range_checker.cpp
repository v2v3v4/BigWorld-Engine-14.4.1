#include "pch.hpp"

#include "integer_range_checker.hpp"

#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

IntegerRangeChecker::IntegerRangeChecker()
{
	values_.reserve( 34 );
	values_.push_back( PyLong_FromString( (char *)"-10000000000000001", NULL, 16 ) ); //  0: -2**64 - 1
	values_.push_back( PyLong_FromString( (char *)"-10000000000000000", NULL, 16 ) ); //  1: -2**64
	values_.push_back( PyLong_FromString( (char *)"-8000000000000001", NULL, 16 ) );  //  2: -2** 63 - 1
	values_.push_back( PyLong_FromString( (char *)"-8000000000000000", NULL, 16 ) );  //  3: -2** 63
	values_.push_back( PyLong_FromString( (char *)"-100000001", NULL, 16 ) );         //  4: -2*32 - 1
	values_.push_back( PyLong_FromString( (char *)"-100000000", NULL, 16 ) );         //  5: -2*32
	values_.push_back( PyLong_FromString( (char *)"-80000001", NULL, 16 ) );          //  6: -2*31 - 1
	values_.push_back( Script::getData( int( 0x80000000 ) ) );                //  7: -2*31
	values_.push_back( Script::getData( -0x10001 ) );                         //  8: -2*16 -1
	values_.push_back( Script::getData( -0x10000 ) );                         //  9: -2*16
	values_.push_back( Script::getData( -0x8001 ) );                          // 10: -2*15 - 1
	values_.push_back( Script::getData( -0x8000 ) );                          // 11: 2*15
	values_.push_back( Script::getData( -0x101 ) );                           // 12: -2**8 - 1
	values_.push_back( Script::getData( -0x100 ) );                           // 13: -2**8
	values_.push_back( Script::getData( -0x81 ) );                            // 14: -2**7 - 1
	values_.push_back( Script::getData( -0x80 ) );                            // 15: -2**7
	values_.push_back( Script::getData( -0x1 ) );                             // 16: -1
	values_.push_back( Script::getData( 0 ) );                                // 17: 0
	values_.push_back( Script::getData( 0x7f ) );                             // 18: 2**7 - 1
	values_.push_back( Script::getData( 0x80 ) );                             // 19: 2**7
	values_.push_back( Script::getData( 0xff ) );                             // 20: 2**8 - 1
	values_.push_back( Script::getData( 0x100 ) );                            // 21: 2**8
	values_.push_back( Script::getData( 0x7fff ) );                           // 22: 2**15 - 1
	values_.push_back( Script::getData( 0x8000 ) );                           // 23: 2**15
	values_.push_back( Script::getData( 0xffff ) );                           // 24: 2**16 - 1
	values_.push_back( Script::getData( 0x10000 ) );                          // 25: 2**16
	values_.push_back( Script::getData( 0x7fffffff ) );                       // 26: 2**31 - 1
	values_.push_back( Script::getData( 0x80000000 ) );                       // 27: 2**31
	values_.push_back( Script::getData( 0xffffffff ) );                       // 28: 2**32 - 1
	values_.push_back( PyLong_FromString( (char *)"100000000", NULL, 16 ) );          // 29: 2**32
	values_.push_back( PyLong_FromString( (char *)"7fffffffffffffff", NULL, 16 ) );   // 30: 2**63 - 1
	values_.push_back( PyLong_FromString( (char *)"8000000000000000", NULL, 16 ) );   // 31: 2**63
	values_.push_back( PyLong_FromString( (char *)"ffffffffffffffff", NULL, 16 ) );   // 32: 2**64 - 1
	values_.push_back( PyLong_FromString( (char *)"10000000000000000", NULL, 16 ) );  // 33: 2**64

	MF_ASSERT( !PyErr_Occurred() );
}


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
bool IntegerRangeChecker::findSameRange(
		const IntegerRangeCheckerVisitor & visitor,
		size_t & start, size_t & end ) const
{
	bool wasOkay = false;
	bool isFinished = false;

	for (size_t i = 0; i < values_.size(); ++i)
	{
		bool isOkay = visitor.visit( values_[i].get() );

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

BW_END_NAMESPACE

// integer_range_checker.cpp
