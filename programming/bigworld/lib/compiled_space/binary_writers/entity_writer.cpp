#include "pch.hpp"

#include "entity_writer.hpp"
#include "chunk_converter.hpp"
#include "../entity_list_types.hpp"

#include "cstdmf/command_line.hpp"

namespace BW {
namespace CompiledSpace {

namespace {

	const char * INCLUDE_SERVER_ENTITIES = "includeServerEntities";

	void readIncludeServerEntities( const CommandLine& cmdLine, bool * includeServerEntities )
	{
		if (cmdLine.hasParam( INCLUDE_SERVER_ENTITIES ))
		{
			*includeServerEntities = true;
		}
	}
}

// ----------------------------------------------------------------------------
EntityWriter::EntityWriter(  )
{

}


// ----------------------------------------------------------------------------
EntityWriter::~EntityWriter()
{

}


// ----------------------------------------------------------------------------
bool EntityWriter::initialize( const DataSectionPtr& pSpaceSettings,
	const CommandLine& commandLine )
{
	includeServerEntities_ = false;
	readIncludeServerEntities( commandLine, &includeServerEntities_ );
	return true;
}

// ----------------------------------------------------------------------------
bool EntityWriter::write( BinaryFormatWriter& writer )
{
	BW::vector<BW::string> tagsToFilterOut;
	tagsToFilterOut.push_back( "clientOnly" );
	tagsToFilterOut.push_back( "editorOnly" );
	tagsToFilterOut.push_back( "metaData" );

	clientEntities_.writeToBinary( writer,
		CompiledSpace::EntityListTypes::CLIENT_ENTITIES_MAGIC, 
		CompiledSpace::EntityListTypes::FORMAT_VERSION, 
		tagsToFilterOut );

	if (includeServerEntities_)
	{
		serverEntities_.writeToBinary( writer,
			CompiledSpace::EntityListTypes::SERVER_ENTITIES_MAGIC, 
			CompiledSpace::EntityListTypes::FORMAT_VERSION, 
			tagsToFilterOut );
	}

	udos_.writeToBinary( writer,
		CompiledSpace::EntityListTypes::UDOS_MAGIC, 
		CompiledSpace::EntityListTypes::FORMAT_VERSION, 
		tagsToFilterOut );	

	return true;
}


// ----------------------------------------------------------------------------
void EntityWriter::convertEntity( const ConversionContext& ctx,
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	addEntity( pItemDS, ctx.chunkTransform );
}


// ----------------------------------------------------------------------------
void EntityWriter::convertUDO( const ConversionContext& ctx,
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	addUDO( pItemDS, ctx.chunkTransform );
}


// ----------------------------------------------------------------------------
void EntityWriter::addEntity( const DataSectionPtr& pItemDS, 
	const Matrix& baseTransform )
{
	if (pItemDS->readBool( "clientOnly" ))
	{
		clientEntities_.add( pItemDS, baseTransform );
	}
	else
	{
		serverEntities_.add( pItemDS, baseTransform  );
	}
}


// ----------------------------------------------------------------------------
void EntityWriter::addUDO( const DataSectionPtr& pItemDS, 
	const Matrix& baseTransform  )
{
	udos_.add( pItemDS, baseTransform );
}


// ----------------------------------------------------------------------------


} // namespace CompiledSpace
} // namespace BW
