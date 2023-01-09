#ifndef LOG_STRING_PRINTER_HPP
#define LOG_STRING_PRINTER_HPP

#include "format_data.hpp"

#include "network/forwarding_string_handler.hpp"
#include "network/bsd_snprintf.h"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 * StreamParsers - for reading either network or file streams of log data
 */
class LogStringPrinter
{
public:
	void setResultString( BW::string &result );

	// Parsing methods
	void onFmtStringSection( const BW::string &fmt, int start, int end );

	void onParseComplete();
	void onError();

	void onMinWidth( WidthType w, FormatData &fd );
	void onMaxWidth( WidthType w, FormatData &fd );

	void onChar( char c, const FormatData &fd );
	void onString( const char *s, const FormatData &fd );
	void onPointer( int64 ptr, const FormatData &fd );

	template < class IntType >
	void onInt( IntType i, const FormatData &fd )
	{
		bsdFormatInt( i, fd.base_, fd.min_, fd.max_, fd.flags_, *pStr_ );
	}

	template < class FloatType >
	void onFloat( FloatType f, const FormatData &fd )
	{
		bsdFormatFloat( f, fd.min_, fd.max_, fd.flags_, *pStr_ );
	}

private:
	// We are passed a reference to a string to modify. This pointer
	// should never be deleted.
	BW::string *pStr_;
};

BW_END_NAMESPACE

#endif // LOG_STRING_PRINTER_HPP
