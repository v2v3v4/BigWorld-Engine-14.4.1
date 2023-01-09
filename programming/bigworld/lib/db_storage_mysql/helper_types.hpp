#ifndef MYSQL_HELPER_TYPES_HPP
#define MYSQL_HELPER_TYPES_HPP

#include "cstdmf/bw_namespace.hpp"

#include "cstdmf/bw_string.hpp"
#include <mysql/mysql.h>


BW_BEGIN_NAMESPACE

class MySql;

/**
 *	This class is used to represent values that might also be null.
 */
template <class T>
class MySqlValueWithNull
{
public:
	MySqlValueWithNull() : isNull_( true ) {}
	MySqlValueWithNull( const T& x ) : value_(x), isNull_( false ) {}

	// this is the public interface
	void nullify() { isNull_ = true; }
	void valuefy() { isNull_ = false; }
	void set( const T& x )
	{
		value_ = x;
		isNull_ = true;
	}
	const T * get() const { return isNull_ ? 0 : &value_; }
	T& getBuf() { return value_; }

private:
	T value_;
	bool isNull_;
};


/**
 *	This class is used to represent a time value that might be null.
 */
class MySqlTimestampNull : public MySqlValueWithNull< MYSQL_TIME >
{
public:
	typedef MySqlValueWithNull< MYSQL_TIME > BaseClass;
	MySqlTimestampNull() {}
	MySqlTimestampNull( const MYSQL_TIME& x ) : BaseClass(x) {}
};


/**
 *	This class initialises itself to a MySQL escaped string from any string.
 */
class MySqlEscapedString
{
public:
	MySqlEscapedString( MySql & connection, const BW::string& string );
	~MySqlEscapedString();

	operator char*()	{ return escapedString_; }

private:
	char *	escapedString_;
};

BW_END_NAMESPACE

#endif // MYSQL_HELPER_TYPES_HPP
