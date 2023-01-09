#ifndef DATE_TIME_UTILS_HPP
#define DATE_TIME_UTILS_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class helps in formating time and dates.
 */
class CSTDMF_DLL DateTimeUtils
{
public:
	struct DateTime
	{
		uint16 year;
		uint8 month;
		uint8 day;
		uint8 hour;
		uint8 minute;
		uint8 second;
	};

	static void format( BW::string & retFormattedTime,
						time_t time, bool fullDateFormat = false );

	static bool parse( DateTime & retDateTime,
						const BW::string & formattedTime );
};

BW_END_NAMESPACE

#endif // DATE_TIME_UTILS_HPP
