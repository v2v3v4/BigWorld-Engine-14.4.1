#ifndef STRING_TABLE_WRITER_HPP
#define STRING_TABLE_WRITER_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

#include "../string_table_types.hpp"
#include "binary_format_writer.hpp"

namespace BW {
namespace CompiledSpace {

class BinaryFormatWriter;

class StringTableWriter
{

public:
	StringTableWriter();
	virtual ~StringTableWriter();
	
	bool write( BinaryFormatWriter& writer );
	bool writeToStream( BinaryFormatWriter::Stream* pStream );

	StringTableTypes::Index addString( const BW::string& str );

	size_t size() const;
	const BW::string& entry( size_t idx ) const;


private:
	BW::vector< BW::string > strings_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // STRING_TABLE_WRITER_HPP
