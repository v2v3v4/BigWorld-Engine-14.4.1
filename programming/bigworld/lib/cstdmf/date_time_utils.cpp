
#include "pch.hpp"
#include "date_time_utils.hpp"
#include "guard.hpp"

#include <time.h>

BW_BEGIN_NAMESPACE

namespace
{
	// Statics to facilitate date formatting and parsing according to the user
	// locale.
	enum DATE_STYLE
	{
		DAY_FIRST,
		MONTH_FIRST,
		YEAR_FIRST
	};

	bool s_dateInited = false;

	DATE_STYLE s_dateStyle = DAY_FIRST;
	int s_dateYearPos = 16;
	int s_dateMonthPos = 13;
	int s_dateDayPos = 10;
	int s_dateFirstSlashPos = 12;
	int s_dateSecondSlashPos = 15;


	/**
	 *	Swaps date items depending on the locale date format.
	 *	NOTE:  The input is allways day, month, year.
	 *
	 *	@param item1	receives the day as input, and returns the date element
	 *	to be put first if formating to a string.
	 *	@param item2	receives the month as input, and returns the date
	 *	element to be put second if formating to a string.
	 *	@param item2	receives the year as input, and returns the date
	 *	element to be put last if formating to a string.
	 */
	void shuffleDateForLocale( int & item1, int & item2, int & item3 )
	{
		// Initialise if not initialised already
		if (!s_dateInited)
		{
			 // Defaults
			s_dateStyle = DAY_FIRST;
			s_dateYearPos = 16;
			s_dateMonthPos = 13;
			s_dateDayPos = 10;
			s_dateFirstSlashPos = 12;
			s_dateSecondSlashPos = 15;
#ifdef WIN32
			char infoBuf[2];
			if (GetLocaleInfoA( LOCALE_USER_DEFAULT, LOCALE_IDATE, infoBuf, 2 ))
			{
				if (infoBuf[0] == '2')
				{
					// Japanese format, YEAR_FIRST
					s_dateStyle = YEAR_FIRST;
					s_dateYearPos = 10;
					s_dateMonthPos = 15;
					s_dateDayPos = 18;
					s_dateFirstSlashPos = 14;
					s_dateSecondSlashPos = 17;
				}
				else if (infoBuf[0] == '0')
				{
					// U.S. format, MONTH_FIRST
					s_dateStyle = MONTH_FIRST;
					s_dateYearPos = 16;
					s_dateMonthPos = 10;
					s_dateDayPos = 13;
					s_dateFirstSlashPos = 12;
					s_dateSecondSlashPos = 15;
				}
			}
#endif // WIN32
			s_dateInited = true;
		}

		if (s_dateStyle == MONTH_FIRST)
		{
			std::swap( item1, item2 );
		}
		else if (s_dateStyle == YEAR_FIRST)
		{
			std::swap( item1, item3 );
		}
	}
};


/**
 *	Returns the time in string format.
 *
 *	@param retFormattedTime	Returned formatted date and/or time string.
 *	@param time				Date and time to be formatted.
 *	@param fullDateFormat	True for returning the full date and time, false to
 *	return either the time (if the date is from today) or the date.
 */
/*static*/
void DateTimeUtils::format( BW::string & retFormattedTime,
							time_t time, bool fullDateFormat )
{
	BW_GUARD;

	if (time == 0)
	{
		retFormattedTime = "-";
		return;
	}

	time_t n = ::time( NULL );
	tm now = *::localtime( &n );
	tm then = *::localtime( &time );

	char result[ 256 ];

	if (fullDateFormat)
	{
		int dateItem1 = then.tm_mday;
		int dateItem2 = then.tm_mon + 1;
		int dateItem3 = then.tm_year + 1900;
		shuffleDateForLocale( dateItem1, dateItem2, dateItem3 );

		bw_snprintf( result, ARRAY_SIZE( result ),
			"%02d:%02d:%02d, %02d/%02d/%02d",
			then.tm_hour, then.tm_min, then.tm_sec,
			dateItem1, dateItem2, dateItem3 );
	}
	else
	{
		if (now.tm_year != then.tm_year ||
			now.tm_mon != then.tm_mon ||
			now.tm_mday != then.tm_mday)
		{
			int dateItem1 = then.tm_mday;
			int dateItem2 = then.tm_mon + 1;
			int dateItem3 = then.tm_year + 1900;
			shuffleDateForLocale( dateItem1, dateItem2, dateItem3 );

			bw_snprintf( result, ARRAY_SIZE( result ), "%02d/%02d/%02d",
				dateItem1, dateItem2, dateItem3 );
		}
		else
		{
			bw_snprintf( result, ARRAY_SIZE( result ), "%02d:%02d:%02d",
				then.tm_hour, then.tm_min, then.tm_sec );
		}
	}

	retFormattedTime = result;
}


/**
 *	Parses a date/time string and returns a DateTime struct.
 *	NOTE: At the moment, only "fullDateFormat" strings, as described in the 
 *	above "format" method, can be parsed.
 *
 *	@param retDateTime		Returned date and time.
 *	@param formattedTime	Date and time in string format.
 */
/*static*/
bool DateTimeUtils::parse( DateTime & retDateTime,
							const BW::string & formattedTime )
{
	bool parsedOK = false;

	if (formattedTime.length() == 20 &&
		formattedTime[  2 ] == ':' && formattedTime[  5 ] == ':' &&
		formattedTime[ s_dateFirstSlashPos ] == '/' && formattedTime[ s_dateSecondSlashPos ] == '/')
	{
		retDateTime.year = atoi( formattedTime.substr( s_dateYearPos, 4 ).c_str() );
		retDateTime.month = atoi( formattedTime.substr( s_dateMonthPos, 2 ).c_str() );
		retDateTime.day = atoi( formattedTime.substr( s_dateDayPos, 2 ).c_str() );
		retDateTime.hour = atoi( formattedTime.substr( 0, 2 ).c_str() );
		retDateTime.minute = atoi( formattedTime.substr( 3, 2 ).c_str() );
		retDateTime.second = atoi( formattedTime.substr( 6, 2 ).c_str() );

		parsedOK = true;
	}

	return parsedOK;
}

BW_END_NAMESPACE
