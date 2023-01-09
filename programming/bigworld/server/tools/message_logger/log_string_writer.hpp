#ifndef LOG_STRING_WRITER_HPP
#define LOG_STRING_WRITER_HPP

#include "format_data.hpp"

#include "network/file_stream.hpp"
#include "network/forwarding_string_handler.hpp"
#include "network/bsd_snprintf.h"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class LogStringWriter
{
public:
	LogStringWriter( FileStream &blobFile );

	bool isGood() const;

	void onFmtStringSection( const BW::string &fmt, int start, int end );

	void onParseComplete();
	void onError();

	void onMinWidth( WidthType w, FormatData &fd );
	void onMaxWidth( WidthType w, FormatData &fd );

	void onChar( char c, const FormatData &fd );
	void onString( const char *s, const FormatData &fd );
	void onPointer( int64 ptr, const FormatData &fd );

	template < class IntType >
	void onInt( IntType i, const FormatData & /* fd */ )
	{
		blobFile_ << i;
	}

	template < class FloatType >
	void onFloat( FloatType f, const FormatData & /* fd */ )
	{
		blobFile_ << f;
	}

private:
	bool isGood_;

	FileStream &blobFile_;
};

BW_END_NAMESPACE

#endif // LOG_STRING_WRITER_HPP
