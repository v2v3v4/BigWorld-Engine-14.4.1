#ifndef COMPILED_SPACE_SETTINGS_HPP
#define COMPILED_SPACE_SETTINGS_HPP

#include "compiled_space/forward_declarations.hpp"
#include "compiled_space/binary_format.hpp"

#include "compiled_space_settings_types.hpp"


namespace BW {
namespace CompiledSpace {


class COMPILED_SPACE_API CompiledSpaceSettings
{
public:
	CompiledSpaceSettings();
	~CompiledSpaceSettings();

	bool read( BinaryFormat& reader );
	void close();

	const AABB& boundingBox() const;

private:
	BinaryFormat* pReader_;
	BinaryFormat::Stream* pStream_;

	CompiledSpaceSettingsTypes::Header data_;
};


} // namespace CompiledSpace
} // namespace BW

#endif // COMPILED_SPACE_SETTINGS_HPP
