#include "log_string_printer.hpp"


BW_BEGIN_NAMESPACE

void LogStringPrinter::setResultString( BW::string &result )
{
	pStr_ = &result;
}


void LogStringPrinter::onParseComplete()
{
	pStr_ = NULL;
}


void LogStringPrinter::onError()
{
	pStr_ = NULL;
}


void LogStringPrinter::onFmtStringSection( const BW::string &fmt,
	int start, int end )
{
	bsdFormatString( fmt.c_str() + start, 0, 0, end-start, *pStr_ );
}


void LogStringPrinter::onMinWidth( WidthType w, FormatData &fd )
{
	fd.min_ = w;
}


void LogStringPrinter::onMaxWidth( WidthType w, FormatData &fd )
{
	fd.max_ = w;
}


void LogStringPrinter::onString( const char *s, const FormatData &fd )
{
	bsdFormatString( s, fd.flags_, fd.min_, fd.max_, *pStr_ );
}


void LogStringPrinter::onPointer( int64 ptr, const FormatData &fd )
{
	char buf[ 128 ];
	bw_snprintf( buf, sizeof( buf ), "0x%" PRIx64, uint64( ptr ) );
	this->onString( buf, fd );
}


void LogStringPrinter::onChar( char c, const FormatData &fd )
{
	char buf[2] = { c, 0 };
	this->onString( buf, fd );
}

BW_END_NAMESPACE

// log_string_printer.cpp
