#ifndef INTEGER_RANGE_CHECKER_HPP
#define INTEGER_RANGE_CHECKER_HPP

#include "cstdmf/stdmf.hpp"

#include "cstdmf/bw_vector.hpp"
#include <Python.h>


BW_BEGIN_NAMESPACE

template <class TYPE> class SmartPointer;
typedef SmartPointer< PyObject > PyObjectPtr;

/**
 *	This interface is used by IntegerRangeCheckerVisitor::findSameRange.
 */
class IntegerRangeCheckerVisitor
{
public:
	virtual bool visit( PyObject * pObject ) const = 0;
};


/**
 *	This class is used to identify a range of Python integers that a condition
 *	is true over. For example, it is used to test what range works for the
 *	different integer Script::setData functions.
 */
class IntegerRangeChecker
{
public:
	IntegerRangeChecker();

	bool findSameRange( const IntegerRangeCheckerVisitor & visitor,
		size_t & start, size_t & end ) const;

#if 0
	// Something like this might be nice.
	bool visitRange( const IntegerRangeCheckerVisitor & visitor,
			size_t start, size_t end ) const;
#endif

private:
	BW::vector< PyObjectPtr > values_;
};


/**
 *	This class is used to indicate the appropriate start and end indices for
 *	different integer types.
 */
template <class TYPE>
class IntegerRangeCheckerResult
{
public:
	static inline size_t start()	{ return 0; }
	static inline size_t end()		{ return 0; }
};

#define SPECIALISE_RESULT( TYPE, START, END )						\
	template <>														\
	class IntegerRangeCheckerResult< TYPE >							\
	{																\
	public:															\
		static inline size_t start()	{ return START##U; }		\
		static inline size_t end()		{ return END##U; }			\
	};																\

// These values indicate the start and end of the valid range of indices for
// a given integer type. The end is one after the last valid index. See
// IntegerRangeChecker::findSameRange for more info.
SPECIALISE_RESULT(  int8, 15, 19 )
SPECIALISE_RESULT( int16, 11, 23 )
SPECIALISE_RESULT( int32,  7, 27 )
SPECIALISE_RESULT( int64,  3, 31 )

SPECIALISE_RESULT(  uint8, 17, 21 )
SPECIALISE_RESULT( uint16, 17, 25 )
SPECIALISE_RESULT( uint32, 17, 29 )
SPECIALISE_RESULT( uint64, 17, 33 )

#undef SPECIALISE_RESULT

BW_END_NAMESPACE

#endif // INTEGER_RANGE_CHECKER_HPP
