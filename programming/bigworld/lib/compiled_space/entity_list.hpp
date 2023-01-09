#ifndef COMPILED_ENTITY_LIST_HPP
#define COMPILED_ENTITY_LIST_HPP

#include "compiled_space/forward_declarations.hpp"
#include "compiled_space/binary_format.hpp"

#include "resmgr/datasection.hpp"


namespace BW {
namespace CompiledSpace {


class COMPILED_SPACE_API EntityList
{
public:
	EntityList();
	~EntityList();

	bool read( BinaryFormat& reader, const FourCC& sectionMagic );
	void close();

	bool isValid() const;

	const DataSectionPtr& dataSection();

private:
	BinaryFormat* pReader_;
	BinaryFormat::Stream* pStream_;

	DataSectionPtr pPackedSection_;
};


} // namespace CompiledSpace
} // namespace BW

#endif // COMPILED_SPACE_STRING_TABLE_HPP
