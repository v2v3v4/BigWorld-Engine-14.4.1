#ifndef COMPILED_SPACE_STRING_TABLE_HPP
#define COMPILED_SPACE_STRING_TABLE_HPP

#include "compiled_space/forward_declarations.hpp"
#include "compiled_space/binary_format.hpp"

#include "string_table_types.hpp"



namespace BW {
namespace CompiledSpace {


class COMPILED_SPACE_API StringTable
{
public:
	StringTable();
	~StringTable();

	bool read( BinaryFormat& reader );
	bool readFromStream( BinaryFormat::Stream* pStream_ );
	void close();

	bool isValid() const;
	size_t size() const;

	const char* entry( size_t idx ) const;
	size_t entryLength( size_t idx ) const;

private:
	BinaryFormat* pReader_;
	BinaryFormat::Stream* pStream_;

	ExternalArray<StringTableTypes::Entry> entries_;
	char* pStringData_;
};


} // namespace CompiledSpace
} // namespace BW

#endif // COMPILED_SPACE_STRING_TABLE_HPP
