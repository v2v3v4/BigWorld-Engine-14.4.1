#include "log_string_interpolator.hpp"

#include "log_string_printer.hpp"
#include "log_string_writer.hpp"


DECLARE_DEBUG_COMPONENT( 0 );


BW_BEGIN_NAMESPACE

namespace
{

// Corresponds to MESSAGE_LOGGER_VERSION from
// src/lib/network/logger_message_forwarder.hpp
const MessageLogger::NetworkVersion MESSAGE_LOGGER_VERSION_64BIT = 7;

const MessageLogger::FormatStringOffsetId FORMAT_STRING_EOF_MARKER( 0xffffffff );
}


/**
 *	Default constructor.
 */
LogStringInterpolator::LogStringInterpolator():
	isOk_( true )
{
}


/**
 *	Constructor.
 */
LogStringInterpolator::LogStringInterpolator( const BW::string &formatString ) :
	formatString_( formatString ),
	fileOffset_( FORMAT_STRING_EOF_MARKER )
{
	isOk_ = handleFormatString( formatString_.c_str(), *this );
}


/**
 *	Destructor.
 */
LogStringInterpolator::~LogStringInterpolator()
{
}


/*
 *	Override from FormatStringHandler.
 */
void LogStringInterpolator::onString( size_t start, size_t end )
{
	stringOffsets_.push_back( StringOffset( start, end ) );
	components_.push_back( 's' );
}


/*
 *	Override from FormatStringHandler.
 */
void LogStringInterpolator::onToken( char type, int cflags, int min,
	int max, int flags, uint8 base, int vflags )
{
	fmtData_.push_back( FormatData( type, cflags, base, min,
			max, flags, vflags ) );
	components_.push_back( 't' );
}


/**
 *	This method returns the current offset into the format string file.
 *
 *	@returns The current read/write offset into the format string file.
 */
MessageLogger::FormatStringOffsetId LogStringInterpolator::fileOffset() const
{
	return fileOffset_;
}


/**
 *	This method writes the broken down format string to the provided FileStream.
 *
 *	@see LogStringInterpolator::read
 *
 *	@param fileStream  The FileStream to write the format string data to.
 *
 *	@returns true if the format string details were successfully written,
 *		false otherwise.
 */
bool LogStringInterpolator::write( FileStream &fileStream )
{
	if (fileOffset_ != FORMAT_STRING_EOF_MARKER)
	{
		return false;
	}

	fileOffset_ = fileStream.tell();
	fileStream << formatString_ << components_ << stringOffsets_ << fmtData_;

	if (!fileStream.commit())
	{
		ERROR_MSG( "LogStringInterpolator::write: "
			"FileStream::commit failed: %s\n", fileStream.strerror() );
		return false;
	}

	return true;
}


/**
 *	This method reads the broken down format string data from a FileStream.
 *
 *	@see LogStringInterpolator::write
 *
 *	@param fileStream  The FileStream to read the format string data from.
 */
void LogStringInterpolator::read( FileStream & fileStream )
{
	fileOffset_ = fileStream.tell();
	fileStream >> formatString_ >> components_ >> stringOffsets_ >> fmtData_;
}


/**
 *	This method returns the format string for the current interpolator.
 *
 *	@returns The format string associated with the log line.
 */
const BW::string & LogStringInterpolator::formatString() const
{
	return formatString_;
}


/**
 *
 */
bool LogStringInterpolator::streamToString( BinaryIStream & inputStream,
	BW::string & resultString,
	MessageLogger::NetworkVersion version /*= MESSAGE_LOGGER_VERSION */ )
{
	static LogStringPrinter printer;
	printer.setResultString( resultString );

	return this->interpolate( printer, inputStream, version );
}


/**
 *
 */
bool LogStringInterpolator::streamToLog( LogStringWriter & writer, 
		BinaryIStream & inputStream,
		MessageLogger::NetworkVersion version /*= MESSAGE_LOGGER_VERSION */ )
{
	if (!this->interpolate( writer, inputStream, version ))
	{
		return false;
	}

	return writer.isGood();
}


/**
 *
 */
template < class Handler >
bool LogStringInterpolator::interpolate( Handler & handler, 
		BinaryIStream & inputStream, MessageLogger::NetworkVersion version )
{
	StringOffsetList::iterator sit = stringOffsets_.begin();
	BW::vector< FormatData >::iterator fit = fmtData_.begin();

	for (unsigned i=0; i < components_.size(); i++)
	{
		if (components_[i] == 's')
		{
			StringOffset &so = *sit++;
			handler.onFmtStringSection( formatString_, so.start_, so.end_ );
		}
		else
		{
			FormatData &fd = *fit++;

			// Macro to terminate parsing on stream error
#			define CHECK_STREAM()											\
			if (inputStream.error())										\
			{																\
				ERROR_MSG( "Stream too short for '%s'\n",					\
					formatString_.c_str() );								\
				handler.onError();											\
				return false;												\
			}

			if (fd.vflags_ & VARIABLE_MIN_WIDTH)
			{
				WidthType w;
				inputStream >> w;
				CHECK_STREAM();
				handler.onMinWidth( w, fd );
			}

			if (fd.vflags_ & VARIABLE_MAX_WIDTH)
			{
				WidthType w;
				inputStream >> w;
				CHECK_STREAM();
				handler.onMaxWidth( w, fd );
			}

			switch (fd.type_)
			{
			case 'd':
			{
				switch (fd.cflags_)
				{
				case DP_C_SHORT:
				{
					int16 val; inputStream >> val; CHECK_STREAM();
					handler.onInt( val, fd );
					break;
				}

				case DP_C_LONG:
				{
					int64 finalVal;
					if (version < MESSAGE_LOGGER_VERSION_64BIT)
					{
						int32 val;
						inputStream >> val; CHECK_STREAM();

						// We've just read from a 32-bit version client, but
						// write it out as if it were 64-bit version.
						finalVal = val;
					}
					else
					{
						inputStream >> finalVal; CHECK_STREAM();
					}
					handler.onInt( (int64)finalVal, fd );
					break;
				}

				case DP_C_LLONG:
				{
					int64 val; inputStream >> val; CHECK_STREAM();
					handler.onInt( val, fd );
					break;
				}

				default:
				{
					int32 val; inputStream >> val; CHECK_STREAM();
					handler.onInt( val, fd );
					break;
				}
				}
				break;
			}

			case 'o':
			case 'u':
			case 'x':
			{
				switch (fd.cflags_)
				{
				case DP_C_SHORT:
				{
					uint16 val; inputStream >> val; CHECK_STREAM();
					handler.onInt( val, fd );
					break;
				}

				case DP_C_LONG:
				{
					uint64 finalVal;

					if (version < MESSAGE_LOGGER_VERSION_64BIT)
					{
						uint32 val;
						inputStream >> val; CHECK_STREAM();

						// We've just read from a 32-bit version client, but
						// write it out as if it were 64-bit version.
						finalVal = val;
					}
					else
					{
						inputStream >> finalVal; CHECK_STREAM();
					}

					handler.onInt( finalVal, fd );
					break;
				}

				case DP_C_LLONG:
				{
					uint64 val; inputStream >> val; CHECK_STREAM();
					handler.onInt( val, fd );
					break;
				}

				default:
				{
					uint32 val; inputStream >> val; CHECK_STREAM();
					handler.onInt( val, fd );
					break;
				}
				}
				break;
			}

			case 'f':
			case 'e':
			case 'g':
			{
				switch (fd.cflags_)
				{
				case DP_C_LDOUBLE:
				{
					LDOUBLE val; inputStream >> val; CHECK_STREAM();
					handler.onFloat( val, fd );
					break;
				}
				default:
				{
					double val; inputStream >> val; CHECK_STREAM();
					handler.onFloat( val, fd );
					break;
				}
				}
				break;
			}

			case 's':
			{
				BW::string val; inputStream >> val; CHECK_STREAM();
				handler.onString( val.c_str(), fd );
				break;
			}

			case 'p':
			{
				int64 ptr; inputStream >> ptr; CHECK_STREAM();
				handler.onPointer( ptr, fd );
				break;
			}

			case 'c':
			{
				char c; inputStream >> c; CHECK_STREAM();
				handler.onChar( c, fd );
				break;
			}

			default:
				ERROR_MSG( "Unhandled format: '%c'\n", fd.type_ );
				handler.onError();
				return false;
			}
		}
	}

	handler.onParseComplete();

	return true;
}

BW_END_NAMESPACE

// log_string_interpolator.cpp
