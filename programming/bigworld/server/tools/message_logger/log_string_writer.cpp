#include "log_string_writer.hpp"


BW_BEGIN_NAMESPACE

LogStringWriter::LogStringWriter( FileStream &blobFile ) :
	isGood_( false ),
	blobFile_( blobFile )
{ }


bool LogStringWriter::isGood() const
{
	return (isGood_ && blobFile_.good());
}

void LogStringWriter::onParseComplete()
{
	blobFile_.commit();
	isGood_ = true;
}


void LogStringWriter::onError()
{
	isGood_ = false;
}


void LogStringWriter::onFmtStringSection( const BW::string &fmt,
	int start, int end )
{ }


void LogStringWriter::onMinWidth( WidthType w, FormatData & /* fd */ )
{
	blobFile_ << w;
}

void LogStringWriter::onMaxWidth( WidthType w, FormatData & /* fd */ )
{
	blobFile_ << w;
}


void LogStringWriter::onString( const char *s, const FormatData & /* fd */ )
{
	blobFile_ << s;
}


void LogStringWriter::onPointer( int64 ptr, const FormatData & /* fd */ )
{
	blobFile_ << ptr;
}


void LogStringWriter::onChar( char c, const FormatData & /* fd */ )
{
	blobFile_ << c;
}

BW_END_NAMESPACE

// log_string_writer.cpp
