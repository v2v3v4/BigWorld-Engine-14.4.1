#ifndef FORMAT_STRINGS_MLDB_HPP
#define FORMAT_STRINGS_MLDB_HPP

#include "../binary_file_handler.hpp"
#include "../format_strings.hpp"
#include "../log_string_interpolator.hpp"
#include "../log_entry.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

typedef BW::vector< BW::string > FormatStringList;

/**
 * Handles the global format strings mapping and file.
 */
class FormatStringsMLDB : public BinaryFileHandler, public FormatStrings
{
public:
	virtual ~FormatStringsMLDB();
	bool init( const char *logLocation, const char *mode );
	const LogStringInterpolator * getHandlerForLogEntry(
			const LogEntry & entry );

protected:
	virtual bool read();
	virtual void flush();

	virtual void clear();

	bool canAppendToDB();
	bool writeFormatStringToDB( LogStringInterpolator *pHandler );

	void addFormatStringToMap( const BW::string & fmt,
		LogStringInterpolator *pHandler );

private:
	// Mapping from strings offset -> handler (for reading)
	typedef BW::map<
		MessageLogger::FormatStringOffsetId, LogStringInterpolator * > OffsetMap;
	OffsetMap offsetMap_;
};

BW_END_NAMESPACE

#endif // FORMAT_STRINGS_MLDB_HPP
