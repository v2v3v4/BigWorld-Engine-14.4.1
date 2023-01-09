#ifndef FORWARDING_STRING_HANDLER_H
#define FORWARDING_STRING_HANDLER_H

#include <stdarg.h>
#include "cstdmf/bw_vector.hpp"

#include "format_string_handler.hpp"
#include "cstdmf/memory_stream.hpp"
#include "bsd_snprintf.h"

BW_BEGIN_NAMESPACE

void TestStrings();

/**
 *  This object handles log message forwarding for a single format string.
 */
class ForwardingStringHandler : public FormatStringHandler
{
public:
	ForwardingStringHandler( const char *fmt, bool isSuppressible = false );

	virtual void onToken( char type, int cflags, int min, int max,
		int flags, uint8 base, int vflags );

	void parseArgs( va_list argPtr, MemoryOStream &os );
	const BW::string & fmt() const { return fmt_; }

	unsigned numRecentCalls() const { return numRecentCalls_; }
	void addRecentCall() { ++numRecentCalls_; }
	void clearRecentCalls() { numRecentCalls_ = 0; }

	bool isSuppressible() const { return isSuppressible_; }
	void isSuppressible( bool b ) { isSuppressible_ = b; }

	bool isOk() { return isOk_ ; }

protected:
	/**
	 *  This helper class holds the various numbers needed by the bsd_snprintf
	 *  family to correctly format this message.  We cache this stuff to avoid
	 *  re-parsing the format string each time a message of this type is sent.
	 */
	class FormatData
	{
	public:
		FormatData( char type, uint8 cflags, uint8 vflags ) :
			type_( type ), cflags_( cflags ), vflags_( vflags ) {}

		char type_;
		unsigned cflags_:4;
		unsigned vflags_:4;
	};

	void processToken( char type, int cflags );

	/// The format string associated with this object
	BW::string fmt_;

	/// Cached printf data structures, allowing us to avoid re-parsing the
	/// format string each time we call parseArgs()
	BW::vector< FormatData > fmtData_;

	/// The number of calls since the last time clearRecentCalls() was called.
	unsigned numRecentCalls_;

	/// Whether or not this format string should be suppressed if it is
	/// spamming.
	bool isSuppressible_;

	// whether this string handler is ok, primarily whether the format string is
	// valid
	bool isOk_;
};

BW_END_NAMESPACE

#endif
