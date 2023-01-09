#ifndef LOG_TIME_HPP
#define LOG_TIME_HPP

#include "constants.hpp"

#include "cstdmf/stdmf.hpp"

#include <time.h>


BW_BEGIN_NAMESPACE

#pragma pack( push, 1 )
class LogTime
{
public:
	typedef int64 Seconds;
	typedef uint16 Milliseconds;

	LogTime();
	LogTime( double ftime );

	bool operator>( const LogTime &other ) const;
	bool operator>=( const LogTime &other ) const;
	bool operator<( const LogTime &other ) const;
	bool operator<=( const LogTime &other ) const;

	double asDouble() const;

	Seconds secs_;
	Milliseconds msecs_;
};
#pragma pack( pop )

BW_END_NAMESPACE

#endif // LOG_TIME_HPP
