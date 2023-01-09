#include "pch.hpp"

#include "compiled_space_settings.hpp"

#include "cstdmf/string_builder.hpp"
#include <limits>

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
CompiledSpaceSettings::CompiledSpaceSettings() :
	pReader_( NULL ),
	pStream_( NULL )
{
	data_.spaceBounds_ = AABB();
}


// ----------------------------------------------------------------------------
CompiledSpaceSettings::~CompiledSpaceSettings()
{
	this->close();
}


// ----------------------------------------------------------------------------
bool CompiledSpaceSettings::read( BinaryFormat& reader )
{
	typedef CompiledSpaceSettingsTypes::Header Header;

	MF_ASSERT( pReader_ == NULL );
	pReader_ = &reader;

	pStream_ = pReader_->findAndOpenSection(
		CompiledSpaceSettingsTypes::FORMAT_MAGIC,
		CompiledSpaceSettingsTypes::FORMAT_VERSION,
		"CompiledSpaceSettings" );
	if (!pStream_)
	{
		ASSET_MSG( "Failed to map compiled space settings into memory.\n" );
		this->close();
		return false;
	}

	// Now read the header
	if (!pStream_->read( data_ ))
	{
		ASSET_MSG( "CompiledSpaceSettings in '%s' is malformed.\n",
			pReader_->resourceID() );
		this->close();
		return false;
	}
	
	// No need to keep this in memory
	this->close();
	return true;
}


// ----------------------------------------------------------------------------
void CompiledSpaceSettings::close()
{
	// Nothing remains mapped in memory so close is often called
	if (pStream_ && pReader_)
	{
		pReader_->closeSection( pStream_ );
	}

	pReader_ = NULL;
	pStream_ = NULL;
}


// ----------------------------------------------------------------------------
const AABB& CompiledSpaceSettings::boundingBox() const
{
	return data_.spaceBounds_;
}

} // namespace CompiledSpace
} // namespace BW
