#ifndef LOG_ENTRY_ADDRESS_HPP
#define LOG_ENTRY_ADDRESS_HPP

#include "cstdmf/stdmf.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;

#pragma pack( push, 1 )

/**
 * This class represents the on disk address of a log entry. Notice that we
 * reference by suffix instead of segment number to handle segment deletion on
 * disk.
 */
class LogEntryAddress
{
public:
	LogEntryAddress();
	LogEntryAddress( const char *suffix, int index );
	LogEntryAddress( const BW::string &suffix, int index );

	void write( BinaryOStream &os ) const;
	void read( BinaryIStream &is );

	bool operator<( const LogEntryAddress &other ) const;

	const char *getSuffix() const;
	int getIndex() const;

protected:
	BW::string suffix_;
	int32 index_;
};
#pragma pack( pop )

BW_END_NAMESPACE

#endif // LOG_ENTRY_ADDRESS_HPP
