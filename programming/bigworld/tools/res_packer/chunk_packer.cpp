

#include "packers.hpp"
#include "packer_helper.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/packed_section.hpp"
#include "chunk_packer.hpp"


BW_BEGIN_NAMESPACE

IMPLEMENT_PACKER( ChunkPacker )

int ChunkPacker_token = 0;


bool ChunkPacker::prepare( const BW::string& src, const BW::string& dst )
{
	BW::StringRef ext = BWResource::getExtension( src );
	if (!ext.equals_lower( "chunk" ))
		return false;

	src_ = src;
	dst_ = dst;

	return true;
}

bool ChunkPacker::print()
{
	if ( src_.empty() )
	{
		printf( "Error: ChunkPacker not initialised properly\n" );
		return false;
	}

	printf( "ChunkFile: %s\n", src_.c_str() );

	return true;
}

bool ChunkPacker::pack()
{
	if ( src_.empty() || dst_.empty() )
	{
		printf( "Error: ChunkPacker not initialised properly\n" );
		return false;
	}

	// Copy to a temp file in the destination folder before packing, in order
	// to be able to edit the file (a PackedSection is not editable)
	BW::string tempFile = dst_ + ".packerTemp";
	if ( !PackerHelper::copyFile( src_, tempFile ) )
		return false;

	PackerHelper::FileDeleter deleter( tempFile );

	DataSectionPtr root = BWResource::openSection(
		BWResolver::dissolveFilename( tempFile ) );
	if ( !root )
	{
		printf( "Error opening as a DataSection\n" );
		return false;
	}

	// loop through all entity sections in the chunk
	BW::vector<DataSectionPtr> sections;
	root->openSections( "entity", sections );
	for ( BW::vector<DataSectionPtr>::iterator i = sections.begin();
		i != sections.end(); )
	{
#ifndef MF_SERVER

		// In the client, remove server entities
		bool delSection = !(*i)->readBool( "clientOnly", false );

#else // MF_SERVER

		// In the server, remove client-only entities
		bool delSection = (*i)->readBool( "clientOnly", false );

#endif // MF_SERVER

		if ( delSection )
		{
			// delete the section
			root->delChild( *i );
			i = sections.erase( i );
		}
		else
			++i;
	}

	// save changes on the destination file
	if ( !root->save() )
	{
		printf( "Error saving DataSection\n" );
		return false;
	}

	// finally, convert to a packed section
	BW::vector< BW::string > stripSections;
	stripSections.push_back( "editorOnly" );
	stripSections.push_back( "metaData" );
	stripSections.push_back( "navmesh" );
#ifndef MF_SERVER
	// In the client, also remove navmesh section
	stripSections.push_back( "worldNavmesh" );
#endif // MF_SERVER

	return PackedSection::convert( tempFile, dst_, &stripSections, false,
									2 /* strip sections up to depth 2 */ );
}

BW_END_NAMESPACE
