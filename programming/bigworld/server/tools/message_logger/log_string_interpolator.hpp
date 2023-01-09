#ifndef LOG_STRING_INTERPOLATOR_HPP
#define LOG_STRING_INTERPOLATOR_HPP

#include "string_offset.hpp"
#include "format_data.hpp"
#include "types.hpp"

#include "network/bsd_snprintf.h"
#include "network/file_stream.hpp"
#include "network/format_string_handler.hpp"
#include "network/logger_message_forwarder.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class BinaryFile;
class LogStringWriter;

/**
 * This class handles both reading and writing bwlogs.
 */
class LogStringInterpolator : public FormatStringHandler
{
public:
	LogStringInterpolator();
	LogStringInterpolator( const BW::string & formatString );
	virtual ~LogStringInterpolator();

	bool isOk() { return isOk_; }

	// FormatStringHandler interface
	virtual void onString( size_t start, size_t end );
	virtual void onToken( char type, int cflags, int min, int max,
		int flags, uint8 base, int vflags );

	MessageLogger::FormatStringOffsetId fileOffset() const;
	bool write( FileStream &fs );
	void read( FileStream &fs );

	const BW::string & formatString() const;

	bool streamToLog( LogStringWriter &writer, 
		BinaryIStream & inputStream,
		MessageLogger::NetworkVersion version = MESSAGE_LOGGER_VERSION );

	bool streamToString( BinaryIStream & inputStream, BW::string &str,
		MessageLogger::NetworkVersion version = MESSAGE_LOGGER_VERSION );

protected:
	BW::string formatString_;
	BW::string components_;
	StringOffsetList stringOffsets_;
	BW::vector< FormatData > fmtData_;
	MessageLogger::FormatStringOffsetId fileOffset_;

private:
	bool isOk_;
	template < class Handler >
	bool interpolate( Handler &handler, BinaryIStream &is, uint8 version );

};

BW_END_NAMESPACE

#endif // LOG_STRING_INTERPOLATOR_HPP
