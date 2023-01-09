#include "pch.hpp"

#include "chunk_converter.hpp"
#include "chunk/chunk_grid_size.hpp"

#include "chunk/chunk_format.hpp"
#include "cstdmf/command_line.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/bwresource.hpp"

namespace BW {
namespace CompiledSpace {

namespace {

	const StringRef SPACE_SETTINGS_IDENTIFIER = "/space.settings";
	const char * INCLUDE_INSIDE_CHUNKS = "includeInsideChunks";

	BW::string spaceDirFromSettings( const BW::string& settingsFile )
	{
		return settingsFile.substr( 0, 
			settingsFile.length() - SPACE_SETTINGS_IDENTIFIER.length() );
	}

	void readIncludeInsideChunks( const CommandLine& cmdLine, bool * readInsideChunks )
	{
		if (cmdLine.hasParam( INCLUDE_INSIDE_CHUNKS ))
		{
			*readInsideChunks = true;
		}
	}

	const char * BOUNDS_MIN_X_PARAM = "minX";
	const char * BOUNDS_MAX_X_PARAM = "maxX";
	const char * BOUNDS_MIN_Z_PARAM = "minZ";
	const char * BOUNDS_MAX_Z_PARAM = "maxZ";

	bool readBounds( const CommandLine& cmdLine, 
		const DataSectionPtr& pSettings, 
		int &boundsMinX, int &boundsMaxX, int &boundsMinZ, int &boundsMaxZ )
	{
		MF_ASSERT( pSettings );

		const char * minX = cmdLine.getParam( BOUNDS_MIN_X_PARAM );
		boundsMinX = (strlen( minX ) > 0) ? atoi( minX ) : pSettings->readInt( 
			"bounds/minX", std::numeric_limits<int>::max() );

		const char * maxX = cmdLine.getParam( BOUNDS_MAX_X_PARAM );
		boundsMaxX = (strlen( maxX ) > 0) ? atoi( maxX ) : pSettings->readInt( 
			"bounds/maxX", std::numeric_limits<int>::min() );

		const char * minZ = cmdLine.getParam( BOUNDS_MIN_Z_PARAM );
		boundsMinZ = (strlen( minZ ) > 0) ? atoi( minZ ) : pSettings->readInt( 
			"bounds/minY", std::numeric_limits<int>::max() );

		const char * maxZ = cmdLine.getParam( BOUNDS_MAX_Z_PARAM );
		boundsMaxZ = (strlen( maxZ ) > 0) ? atoi( maxZ ) : pSettings->readInt( 
			"bounds/maxY", std::numeric_limits<int>::min() );

		if (boundsMinX > boundsMaxX || boundsMinZ > boundsMaxZ)
		{
			return false;
		}

		return true;
	}
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
ChunkConverter::ChunkConverter() :
	gridSize_( DEFAULT_GRID_RESOLUTION ),
	boundsMinX_( 0 ),
	boundsMinZ_( 0 ),
	boundsMaxX_( 0 ),
	boundsMaxZ_( 0 )
{
	addItemHandler( "overlapper", this, &ChunkConverter::convertOverlapper );
	addItemHandler( "vlo", this, &ChunkConverter::convertVLO );
}


// ----------------------------------------------------------------------------
bool ChunkConverter::initialize( const BW::string& spaceDir,
	const DataSectionPtr& pSpaceSettings, CommandLine& commandLine )
{
	spaceDir_ = spaceDir;
	pSpaceSettings_ = pSpaceSettings;

	if (!readBounds( commandLine, pSpaceSettings,
		boundsMinX_, boundsMaxX_, boundsMinZ_, boundsMaxZ_ ))
	{
		ERROR_MSG( "%s has invalid bounds.\n", 
			pSpaceSettings->sectionName().c_str() );
		return false;
	}

	convertInsideChunks_ = false;
	readIncludeInsideChunks( commandLine, &convertInsideChunks_ );

	gridSize_ = 
		pSpaceSettings->readFloat( "chunkSize", DEFAULT_GRID_RESOLUTION );

	return true;
}


// ----------------------------------------------------------------------------
void ChunkConverter::process()
{
	size_t numSuccessfulOutdoorChunks = 0;

	for (int x = boundsMinX_; x <= boundsMaxX_; ++x)
	{
		for (int z = boundsMinZ_; z <= boundsMaxZ_; ++z)
		{
			BW::string chunkID = ChunkFormat::outsideChunkIdentifier( x, z, false );
			BW::string chunkFile = spaceDir_ + "/" + chunkID + ".chunk";
			BW::string cdataFile = spaceDir_ + "/" + chunkID + ".cdata";

			ConversionContext ctx;
			ctx.includeInsideChunks = convertInsideChunks_;
			ctx.gridX = x;
			ctx.gridZ = z;
			ctx.gridSize = gridSize_;
			ctx.spaceDir = spaceDir_;
			ctx.chunkID = chunkID;
			ctx.pChunkDS = BWResource::openSection( chunkFile );
			ctx.pCDataDS = BWResource::openSection( cdataFile );
			ctx.pSpaceSettings = pSpaceSettings_;
			ctx.pOverlaps = NULL;
			ctx.chunkTransform.setTranslate(
				float(x) * gridSize_, 0.f, float(z) * gridSize_ );

			if (ctx.pChunkDS && ctx.pCDataDS)
			{
				convertChunk( ctx );
				++numSuccessfulOutdoorChunks;
			}
			else
			{
				ERROR_MSG( "Failed to load chunk %s\n", chunkID.c_str() );			
			}
		}
	}

	TRACE_MSG( "Successfully converted %d outdoor chunks.\n",
		numSuccessfulOutdoorChunks);
}


// ----------------------------------------------------------------------------
void ChunkConverter::processItem( 
	const BW::string& itemTypeName, 
	const ConversionContext& ctx, const DataSectionPtr& pItemDS, 
	const BW::string& uid )
{
	// Determine a type handler to call for this type
	HandlerList& handlers = typeHandlers_[ itemTypeName ];

	if (handlers.empty())
	{
		WARNING_MSG( "Unsupported chunk item type: %s\n",
			itemTypeName.c_str() );
		return;
	}

	// Call all the type handlers
	for (HandlerList::iterator iter = handlers.begin();
		iter != handlers.end(); ++iter)
	{
		ItemHandler* pHandler = *iter;
		pHandler->handleItem( ctx, pItemDS, uid );
	}
}


// ----------------------------------------------------------------------------
void ChunkConverter::convertChunk( const ConversionContext& ctx )
{
	TRACE_MSG( "Converting chunk %s...\n", ctx.chunkID.c_str() );
	size_t totalConvertedItems = 0;

	for (int childIdx = 0; childIdx < ctx.pChunkDS->countChildren(); ++childIdx)
	{
		BW::string itemTypeName = ctx.pChunkDS->childSectionName( childIdx );
		DataSectionPtr pItemDS = ctx.pChunkDS->openChild( childIdx );
		MF_ASSERT( pItemDS );

		processItem( itemTypeName, ctx, pItemDS, BW::string() );

		++totalConvertedItems;
	}

	TRACE_MSG( "    %d items converted.\n", totalConvertedItems );
}


// ----------------------------------------------------------------------------
void ChunkConverter::convertOverlapper( const ConversionContext& ctx, 
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	MF_ASSERT( ctx.pOverlaps == NULL ); // indoor chunks inside indoor chunks?

	if (!ctx.includeInsideChunks)
	{
		// Not including inside chunks in conversions
		return;
	}

	BW::string chunkID = pItemDS->asString();
	BW::string chunkFile = ctx.spaceDir + "/" + chunkID + ".chunk";
	BW::string cdataFile = ctx.spaceDir + "/" + chunkID + ".cdata";

	ConversionContext indoorCtx;
	indoorCtx.gridX = ctx.gridX;
	indoorCtx.gridZ = ctx.gridZ;
	indoorCtx.spaceDir = ctx.spaceDir;
	indoorCtx.chunkID = chunkID;
	indoorCtx.pChunkDS = BWResource::openSection( chunkFile );
	indoorCtx.pCDataDS = BWResource::openSection( cdataFile );
	indoorCtx.pOverlaps = &ctx;

	if (indoorCtx.pChunkDS && indoorCtx.pCDataDS)
	{
		indoorCtx.chunkTransform =
			indoorCtx.pChunkDS->readMatrix34( "transform", Matrix::identity );
		this->convertChunk( indoorCtx );
	}
	else
	{
		ERROR_MSG( "Failed to load chunk %s\n", chunkID.c_str() );
	}
}


// ----------------------------------------------------------------------------
void ChunkConverter::convertVLO( const ConversionContext& ctx, 
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	BW::string vloUID = pItemDS->readString( "uid" );
	if (vloUID.empty())
	{
		ERROR_MSG( "VLO specified without a uid.\n" );
		return;
	}

	BW::string vloTypeName = pItemDS->readString( "type" );
	if (vloTypeName.empty())
	{
		ERROR_MSG( "VLO specified without a type. UID: %s\n",
			vloUID.c_str() );
		return;
	}

	VLOTypeList & typeList = vlos_[vloUID];
	if (std::find( typeList.begin(), typeList.end(), vloTypeName ) != typeList.end())
	{
		return; // have already processed this vlo
	}
	typeList.insert( typeList.end(), vloTypeName );

	BW::string vloFile = ctx.spaceDir + "/_" + vloUID + ".vlo";
	DataSectionPtr pObjectDS = BWResource::openSection( vloFile + '/' + vloTypeName );
	if (!pObjectDS)
	{
		// support for legacy vlo files
		vloFile = ctx.spaceDir + "/" + vloUID + ".vlo";
		pObjectDS = BWResource::openSection( vloFile + '/' + vloTypeName );
	}
	if (!pObjectDS)
	{
		ERROR_MSG( "Could not open VLO section. UID: %s Type: %s\n",
			vloUID.c_str(), vloTypeName.c_str() );
		return;
	}

	// Determine a type handler to call for this type
	processItem( vloTypeName, ctx, pObjectDS, vloUID );
}


// ----------------------------------------------------------------------------
void ChunkConverter::ignoreHandler( const ConversionContext& ctx, 
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	// Do nothing placeholder for ignoring a type
	ASSET_MSG( "Chunk item type '%s' is being ignored.\n",
		pItemDS->sectionName().c_str() );
}


// ----------------------------------------------------------------------------
void ChunkConverter::addIgnoreHandler( const char* itemType )
{
	addItemHandler( itemType, this, &ChunkConverter::ignoreHandler );
}	

} // namespace CompiledSpace
} // namespace BW