#include "log_entry_address.hpp"

#include "cstdmf/binary_stream.hpp"


BW_BEGIN_NAMESPACE

LogEntryAddress::LogEntryAddress()
{ }

LogEntryAddress::LogEntryAddress( const char *suffix, int index ) :
	suffix_( suffix ),
	index_( index )
{ }


LogEntryAddress::LogEntryAddress( const BW::string &suffix, int index ) :
	suffix_( suffix ),
	index_( index )
{ }


void LogEntryAddress::write( BinaryOStream &os ) const
{
	os << suffix_ << index_;
}


// Note: the read functionality is required by the log writer
void LogEntryAddress::read( BinaryIStream &is )
{
	is >> suffix_ >> index_;
}


bool LogEntryAddress::operator<( const LogEntryAddress &other ) const
{
	return suffix_ < other.suffix_ ||
		(suffix_ == other.suffix_ && index_ < other.index_);
}


const char *LogEntryAddress::getSuffix() const
{
	return suffix_.c_str();
}


int LogEntryAddress::getIndex() const
{
	return index_;
}

BW_END_NAMESPACE

// log_entry_address.cpp
