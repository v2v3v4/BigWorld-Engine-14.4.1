#include "pch.hpp"

#include "settings_writer.hpp"
#include "binary_format_writer.hpp"

#include "cstdmf/command_line.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
SettingsWriter::SettingsWriter()
{

}


// ----------------------------------------------------------------------------
SettingsWriter::~SettingsWriter()
{

}


// ----------------------------------------------------------------------------
bool SettingsWriter::initialize( const DataSectionPtr& pSpaceSettings,
	const CommandLine& commandLine )
{
	// Nothing to be done here
	return true;
}


// ----------------------------------------------------------------------------
void SettingsWriter::postProcess()
{
	// Extract bounds from all bound generators
	for (GeneratorList::iterator iter = generators_.begin();
		iter != generators_.end(); ++iter)
	{
		const BoundGeneratorFunc& generator = *iter;

		AABB bounds = generator();
		addBounds( bounds );
	}
}


// ----------------------------------------------------------------------------
bool SettingsWriter::write( BinaryFormatWriter& writer )
{
	if (header_.spaceBounds_.insideOut())
	{
		header_.spaceBounds_.addBounds( Vector3::ZERO );
	}

	BinaryFormatWriter::Stream* stream =
		writer.appendSection( 
			CompiledSpaceSettingsTypes::FORMAT_MAGIC,
			CompiledSpaceSettingsTypes::FORMAT_VERSION );
	MF_ASSERT( stream != NULL );

	stream->write( header_ );
	return true;
}

// ----------------------------------------------------------------------------
void SettingsWriter::addBounds( const AABB& bb )
{
	if (!bb.insideOut())
	{
		header_.spaceBounds_.addBounds( bb );
	}
}


// ----------------------------------------------------------------------------
void SettingsWriter::addBoundGenerator( BoundGeneratorFunc callback )
{
	generators_.push_back( callback );
}


// ----------------------------------------------------------------------------

} // namespace CompiledSpace
} // namespace BW
