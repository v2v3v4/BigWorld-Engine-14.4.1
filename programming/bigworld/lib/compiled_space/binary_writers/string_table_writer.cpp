#include "pch.hpp"

#include "string_table_writer.hpp"
#include "binary_format_writer.hpp"

#include "resmgr/quick_file_writer.hpp"

#include <limits>

namespace BW {
namespace CompiledSpace {


// ----------------------------------------------------------------------------
StringTableWriter::StringTableWriter()
{

}


// ----------------------------------------------------------------------------
StringTableWriter::~StringTableWriter()
{

}


// ----------------------------------------------------------------------------
bool StringTableWriter::write( BinaryFormatWriter& writer )
{
	BinaryFormatWriter::Stream* stream =
		writer.appendSection( 
			StringTableTypes::FORMAT_MAGIC,
			StringTableTypes::FORMAT_VERSION );

	return writeToStream( stream );
}

// ----------------------------------------------------------------------------
bool StringTableWriter::writeToStream( BinaryFormatWriter::Stream* stream )
{
	typedef StringTableTypes::Entry Entry;

	MF_ASSERT( stream != NULL );

	// If we hit this I guess we need bigger indices X_X
	MF_ASSERT( strings_.size() <=
		(size_t)std::numeric_limits<StringTableTypes::Index>::max() );

	// Entries
	uint32 offset = 0;
	BW::vector<Entry> entries;

	for (size_t i = 0; i < strings_.size(); ++i)
	{
		MF_ASSERT( offset <=
			(size_t)std::numeric_limits<uint32>::max() );

		Entry entry;
		entry.offset_ = offset;
		entry.length_ = (uint32)(strings_[i].size());
		offset += (uint32)(strings_[i].size() + 1);

		entries.push_back( entry );
	}

	stream->write( entries );

	// Write out the amount of data we're going to be outputting
	stream->write( offset );

	// String data
	for (size_t i = 0; i < strings_.size(); ++i)
	{
		// Making sure we include NULL terminator
		stream->writeRaw( &strings_[i][0], strings_[i].size()+1 );
	}

	return true;
}


// ----------------------------------------------------------------------------
StringTableTypes::Index StringTableWriter::addString( const BW::string& str )
{
	auto it = std::find( strings_.begin(), strings_.end(), str );
	if (it != strings_.end())
	{
		return static_cast<StringTableTypes::Index>(
			std::distance( strings_.begin(), it ) );
	}
	else
	{
		strings_.push_back( str );
		return static_cast<StringTableTypes::Index>( strings_.size()-1 );
	}
}


// ----------------------------------------------------------------------------
size_t StringTableWriter::size() const
{
	return strings_.size();
}


// ----------------------------------------------------------------------------
const BW::string& StringTableWriter::entry( size_t idx ) const
{
	return strings_[idx];
}

} // namespace CompiledSpace
} // namespace BW
