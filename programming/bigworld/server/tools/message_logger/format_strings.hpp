#ifndef FORMAT_STRINGS_HPP
#define FORMAT_STRINGS_HPP

#include "cstdmf/bw_map.hpp"

#include "log_string_interpolator.hpp"
#include "log_entry.hpp"


BW_BEGIN_NAMESPACE

typedef BW::vector< BW::string > FormatStringList;

/**
 * Handles the global format strings mapping and file.
 */
class FormatStrings
{
public:
	virtual ~FormatStrings();

	virtual bool writeFormatStringToDB( LogStringInterpolator *pHandler ) = 0;

	virtual bool canAppendToDB() = 0;

	LogStringInterpolator * resolve( const BW::string & fmt );

	FormatStringList getFormatStrings() const;

protected:
	virtual void clear();

	void addFormatStringToMap( const BW::string & fmt,
		LogStringInterpolator *pHandler );

private:
	// Mapping from format string -> handler (used when writing log entries)
	typedef BW::map< BW::string, LogStringInterpolator* > FormatMap;
	FormatMap formatMap_;
};

BW_END_NAMESPACE

#endif // FORMAT_STRINGS_HPP
