#include "pch.hpp"
#include "space_recreator.hpp"

#include "worldeditor/project/chunk_walker.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/terrain/terrain_texture_utils.hpp"
#include "worldeditor/world/editor_chunk_overlapper.hpp"
#include "worldeditor/world/editor_chunk_item_linker.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/world/items/editor_chunk_light.hpp"
#include "worldeditor/world/items/editor_chunk_station.hpp"
#include "worldeditor/world/items/editor_chunk_vlo.hpp"
#include "worldeditor/world/items/editor_chunk_user_data_object.hpp"
#include "worldeditor/world/items/editor_chunk_model_vlo.hpp"

#include "lib/asset_pipeline/asset_lock.hpp"

#include "lib/chunk/chunk_format.hpp"
#include "lib/chunk/chunk_item_amortise_delete.hpp"
#include "lib/chunk/chunk_manager.hpp"
#include "lib/chunk/chunk_terrain.hpp"
#include "lib/chunk/geometry_mapping.hpp"

#include "lib/terrain/terrain_texture_layer.hpp"
#include "lib/terrain/terrain_hole_map.hpp"
#include "lib/resmgr/auto_config.hpp"
#include "lib/resmgr/zip_section.hpp"
#include "lib/terrain/terrain2/editor_terrain_block2.hpp"
#include "lib/math/oriented_bbox.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	// The fraction values should total to 1.f.
	const static struct {
		const wchar_t* name;
		const float fraction;
	} STAGES[] = {
		{ L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/STAGE1", .01f },
		{ L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/STAGE2", .01f },
		{ L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/STAGE3", .35f },
		{ L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/STAGE4", .09f },
		{ L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/STAGE5", .09f },
		{ L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/STAGE6", .10f },
		{ L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RECREATE_SPACE/STAGE7", .35f }
	};

	static AutoConfigString s_spaceRecreatorBlankTexture(
		"system/emptyTerrainTextureBmp", "helpers/maps/aid_builder.tga" );

	static inline bool intervalsIntersect( const float chunkMin, const float chunkMax, const float otherMin, const float otherMax, bool includeStart, bool includeEnd ) {
		return (includeStart ? ( chunkMin <= otherMax ) : ( chunkMin < otherMax )) && (includeEnd ? ( otherMin <= chunkMax ) : ( otherMin < chunkMax ));
	}


	// Do these two chunks overlap? We ignore the bottom and right edges of both chunks
	static inline bool chunkTouchesChunk( const BoundingBox &chunkBB, const BoundingBox &otherBB ) {
		const Vector3 &chunkMin = chunkBB.minBounds();
		const Vector3 &chunkMax = chunkBB.maxBounds();
		const Vector3 &otherMin = otherBB.minBounds();
		const Vector3 &otherMax = otherBB.maxBounds();

		return intervalsIntersect( chunkMin.x, chunkMax.x, otherMin.x, otherMax.x, false, false )
			&& intervalsIntersect( chunkMin.y, chunkMax.y, otherMin.y, otherMax.y, false, false )
			&& intervalsIntersect( chunkMin.z, chunkMax.z, otherMin.z, otherMax.z, false, false );
	}


	// Does otherPos fall inside chunkBB? We ignore the bottom and right edges in all chunks except the last
	// row or column
	static inline bool fallsInChunk( const BoundingBox &chunkBB, const BoundingBox &otherBB, const bool finalXChunk, const bool finalZChunk ) {
		const Vector3 &chunkMin = chunkBB.minBounds();
		const Vector3 &chunkMax = chunkBB.maxBounds();
		const Vector3 &otherMin = otherBB.minBounds();
		const Vector3 &otherMax = otherBB.maxBounds();

		return intervalsIntersect( chunkMin.x, chunkMax.x, otherMin.x, otherMax.x, true, finalXChunk )
			&& intervalsIntersect( chunkMin.y, chunkMax.y, otherMin.y, otherMax.y, true, true )
			&& intervalsIntersect( chunkMin.z, chunkMax.z, otherMin.z, otherMax.z, true, finalZChunk );
	}


	// Does otherPos fall inside chunkBB? We ignore the bottom and right edges in all chunks except the last
	// row or column
	static inline bool fallsInChunk( const BoundingBox &chunkBB, const Vector3 &otherPos, const bool finalXChunk, const bool finalZChunk ) {
		const Vector3 &min = chunkBB.minBounds();
		const Vector3 &max = chunkBB.maxBounds();

		return otherPos.x >= min.x && otherPos.y >= min.y && otherPos.z >= min.z
			&& (finalXChunk ? (otherPos.x <= max.x) : (otherPos.x < max.x))
			&& otherPos.y <= max.y
			&& (finalZChunk ? (otherPos.z <= max.z) : (otherPos.z < max.z));
	}
}


enum class FixChunkLinkResult
{
	Fixed,
	Ignored,
	Deleted
};


class SpaceRecreator::Impl
{
public:
	Impl( SpaceRecreator& parent );

	FixChunkLinkResult fixChunkLink(
		DataSectionPtr chunkLinkDS, bool knownToBeChunkLinks = false );

	SpaceRecreator& parent_;
	BW::unordered_map<BW::string, BW::set<BW::string>> chunkVloReferences;
};


SpaceRecreator::SpaceRecreator(
		const BW::string& spaceName, float chunkSize,
		int width, int height,
		const TerrainInfo& terrainInfo )
		: srcSpaceName_( WorldManager::instance().getCurrentSpace() ),
		  spaceName_( spaceName ),
		  chunkSize_( chunkSize ),
		  terrainInfo_( terrainInfo ),
		  pImpl( new Impl( *this ) )
{
	BW_GUARD;

	float srcChunkSize;
	int srcMinX, srcMinZ, srcMaxX, srcMaxZ;
	TerrainUtils::terrainSize( "", srcChunkSize, srcMinX, srcMinZ, srcMaxX, srcMaxZ );
	srcSpaceBB_ = BoundingBox( Vector3( srcMinX * srcChunkSize, MIN_CHUNK_HEIGHT, srcMinZ * srcChunkSize ), 
		Vector3( ( srcMaxX + 1 ) * srcChunkSize, MAX_CHUNK_HEIGHT, ( srcMaxZ + 1 ) * srcChunkSize ) );
	changingChunkSize_ = srcChunkSize != chunkSize;

	calcBounds( srcMinX, srcMinZ, srcMaxX, srcMaxZ, width, height );
	xOffset_ = ( int ) ( ( srcMinX * srcChunkSize ) - ( minX_ * chunkSize_ ) );
	zOffset_ = ( int ) ( ( srcMinZ * srcChunkSize ) - ( minZ_ * chunkSize_ ) );
}


SpaceRecreator::SpaceRecreator( const SpaceRecreator& rhs )
	: srcSpaceName_( rhs.srcSpaceName_ )
	, spaceName_( rhs.spaceName_ )
	, chunkSize_( rhs.chunkSize_ )
	, terrainInfo_( rhs.terrainInfo_ )
{
	BW_GUARD;
}


SpaceRecreator& SpaceRecreator::operator=( const SpaceRecreator& rhs )
{
	BW_GUARD;
	return *this;
}


SpaceRecreator::~SpaceRecreator()
{
	BW_GUARD;
	bw_safe_delete( pImpl );
}


bool SpaceRecreator::create()
{
	BW_GUARD;
	bool result = createInternal();
	return result;
}


bool SpaceRecreator::createInternal()
{
	BW_GUARD;

	itemsWithPossibleBrokenLinks.clear();
	pImpl->chunkVloReferences.clear();

	if( WorldManager::instance().connection().enabled() &&
		!WorldManager::instance().connection().isAllLocked() )
	{
		if( !WorldManager::instance().connection().changeSpace( BWResource::dissolveFilename( spaceName_ ) )
			|| !WorldManager::instance().connection().lock( GridRect( GridCoord( minX_ - 1, minZ_ - 1 ),
				GridCoord( maxX_ + 1, maxZ_ + 1 ) ), "create space" ) )
		{
			errorMessage_ = "Unable to lock space";
			return false;
		}
	}
	errorMessage_ = createSpaceDirectory();
	if (shouldAbort())
		return false;

#if ENABLE_ASSET_PIPE
	AssetLock assetLock;
#endif // ENABLE_ASSET_PIPE

	errorMessage_ = createSpaceSettings();
	if (shouldAbort())
		return false;

	// True means purge after every column
	errorMessage_ = createChunks( true );
	if (shouldAbort())
		return false;

	errorMessage_ = copyAssets();
	if (shouldAbort())
		return false;

	// True means purge after every column
	errorMessage_ = fixChunks( true );
	if (shouldAbort())
		return false;

	return true;
}


bool SpaceRecreator::shouldAbort()
{
	BW_GUARD;

	if (!errorMessage_.empty())
		return true;

	if ( ProgressBar::getCurrentProgressBar() && ProgressBar::getCurrentProgressBar()->isCancelled() )
	{
		errorMessage_ = "Cancelled";
		return true;
	}
	return false;
}


void SpaceRecreator::calcBounds( int srcMinX, int srcMinZ, int srcMaxX, int srcMaxZ, int width, int height )
{
	BW_GUARD;
	//Space needs to be centered around 0, 0,
	//with even widths having one more negative than positive rows.
	//If this is not so, the height map calculations will not work.
	--width;
	maxX_ = width / 2;
	minX_ = maxX_ - width;

	--height;
	maxZ_ = height / 2;
	minZ_ = maxZ_ - height;
}


const BW::string SpaceRecreator::createSpaceDirectory()
{
	BW_GUARD;

	ProgressBarTask progressTask( LocaliseUTF8( STAGES[0].name ), 1, false );

	BWResource::ensurePathExists( spaceName_ + "/" );
	return BWResource::openSection( BWResource::dissolveFilename( spaceName_ ) ) ? "" :
		"Unable to create new space directory\n";
}


// Copy SPACE_SETTING_FILE_NAME from the old space, changing the values
// we've been given and keeping the rest.
const BW::string SpaceRecreator::createSpaceSettings()
{
	BW_GUARD;

	BW::string result = "";

	ProgressBarTask progressTask( LocaliseUTF8( STAGES[1].name ), 1, false );

	DataSectionPtr srcSpaceDS = BWResource::openSection( BWResource::dissolveFilename( srcSpaceName_ ) );
	DataSectionPtr spaceDS = BWResource::openSection( BWResource::dissolveFilename( spaceName_ ) );

	DataSectionPtr srcSpaceSettings = srcSpaceDS->openSection( SPACE_SETTING_FILE_NAME );
	DataSectionPtr spaceSettings = spaceDS->newSection( SPACE_SETTING_FILE_NAME, XMLSection::creator() );
	if (srcSpaceSettings && spaceSettings)
	{
		spaceSettings->copy( srcSpaceSettings );
		float srcChunkSize =
			spaceSettings->readFloat( "chunkSize", Terrain::DEFAULT_BLOCK_SIZE );
		spaceSettings->writeFloat( "chunkSize", chunkSize_ );
		spaceSettings->writeInt( "bounds/minX", minX_ );
		spaceSettings->writeInt( "bounds/maxX", maxX_ );
		spaceSettings->writeInt( "bounds/minY", minZ_ );
		spaceSettings->writeInt( "bounds/maxY", maxZ_ );

		// write terrain settings
		DataSectionPtr srcTerrainSettings = srcSpaceSettings->openSection( "terrain" );

		spaceSettings->deleteSection( "terrain" );
		DataSectionPtr terrainSettingsDS_ = spaceSettings->newSection( "terrain" );

		// Create a terrainsettings object with the new settings
		pTerrainSettings_ = new Terrain::TerrainSettings;
		pTerrainSettings_->init( chunkSize_, srcTerrainSettings, srcChunkSize );
		pTerrainSettings_->version( terrainInfo_.version_ );
		pTerrainSettings_->heightMapEditorSize( terrainInfo_.heightMapEditorSize_ );
		pTerrainSettings_->heightMapSize( terrainInfo_.heightMapSize_ );
		pTerrainSettings_->normalMapSize( terrainInfo_.normalMapSize_ );
		pTerrainSettings_->holeMapSize( terrainInfo_.holeMapSize_ );
		pTerrainSettings_->shadowMapSize( terrainInfo_.shadowMapSize_ );
		pTerrainSettings_->blendMapSize( terrainInfo_.blendMapSize_ );
		pTerrainSettings_->save( terrainSettingsDS_ );

		result = spaceSettings->save() ? "" : ("Unable to save " + SPACE_SETTING_FILE_NAME + "\n");
	} else {
		result = "Unable to copy " + SPACE_SETTING_FILE_NAME + "\n";
	}

	// srcLocations might not exist, that's fine.
	DataSectionPtr srcLocations = srcSpaceDS->openSection( "locations.xml" );
	DataSectionPtr locations = spaceDS->newSection( "locations.xml", XMLSection::creator() );
	if (srcLocations && locations)
	{
		locations->copy( srcLocations );
		result += locations->save() ? "" : "Unable to save locations.xml\n";
	} else {
		// But if we failed to create the new locations, we failed.
		result += (bool)locations? "" : "Unable to copy locations.xml\n";
	}

	return result;
}


BW::string SpaceRecreator::chunkName( int x, int z ) const
{
	BW_GUARD;

	return ChunkFormat::outsideChunkIdentifier( x,z );
}


// Loosely based on ImportVisitor::visit
bool SpaceRecreator::normaliseTerrainBlockBlends( Terrain::EditorBaseTerrainBlock &block )
{
	bool modified = false;

	// These values are used in fixed-point inside the painting calculations:
	static const uint32 STRENGTH_EXP	= 20;
	static const uint32 FULL_STRENGTH	= 1 << STRENGTH_EXP;
	static const uint32 HALF_STRENGTH	= 1 << (STRENGTH_EXP - 1);

	size_t numLayers = block.numberTextureLayers();

	MF_ASSERT( numLayers > 0 );

	for (size_t i = 0; i < numLayers; ++i){
		block.textureLayer( i ).lock( false );
	}

	// Assumes all the layers are the same dimensions... But that assumption appears everywhere
	// Plus we created these texture layers earlier, so they really should be.
	const uint32 height = block.textureLayer( 0 ).height();
	const uint32 width = block.textureLayer( 0 ).width();

	BW::vector<Terrain::TerrainTextureLayer::PixelType *> pLayerRows( numLayers );

	Terrain::TerrainHoleMap & rHoleMap = block.holeMap();
	Terrain::TerrainHoleMapHolder holeMapHolder(&rHoleMap, false);
	Terrain::TerrainHoleMap::ImageType & rHoleMapImage = rHoleMap.image();

	float holeFactorY = ((float)rHoleMap.height()) / height;
	float holeFactorX = ((float)rHoleMap.width()) / width;

	for ( uint32 y = 0; y < height; ++y ) {
		// Record the row pointer for each layer
		for (size_t i = 0; i < numLayers; ++i)
			pLayerRows[ i ] = block.textureLayer( i ).image().getRow( y );

		for ( uint32 x = 0; x < width; ++x ) {

			int sum = 0;

			for (size_t i = 0; i < numLayers; ++i ) {
				Terrain::TerrainTextureLayer::PixelType value = pLayerRows[ i ][ x ];
				sum += value;
			}

			if (sum == 0)
			{
				// There's no blends at this point, so cut a hole instead
				int holeXStart = int( floorf( holeFactorX * x ) );
				int holeXEnd = int( floorf( holeFactorX * ( x + 1 ) ) );
				int holeYStart = int( floorf( holeFactorY * y ) );
				int holeYEnd = int( floorf( holeFactorY * ( y + 1 ) ) );

				if (holeXEnd == holeXStart)
				{
					holeXEnd++;
				}

				if (holeYEnd == holeYStart)
				{
					holeYEnd++;
				}

				for (int holeX = holeXStart; holeX < holeXEnd; holeX++)
				{
					for (int holeY = holeYStart; holeY < holeYEnd; holeY++)
					{
						rHoleMapImage.set( holeX, holeY, true );
					}
				}

				// Full-blend the lowest layer
				pLayerRows[ 0 ][ x ] = std::numeric_limits<
					Terrain::TerrainTextureLayer::PixelType >::max();

				continue;
			}

			if ( sum == 255 )
				continue;

			modified = true;

			// So now we know our layer total is difference too low, or -difference too high
			// Spread the difference across all the non-zero layers, with any leftover going to
			// the highest weighted layer (the lowest one we see)
			size_t maxLayer = Terrain::EditorBaseTerrainBlock::INVALID_LAYER;
			int maxValue = 0;
			// sum = 255, ratio = 255/255;
			// sum = 254, ratio = 255/254;
			// sum = 1, ratio = 255/1;
			// sum = 256, ratio = 255/256;
			int ratio = (255 << STRENGTH_EXP) / sum;
			for (size_t i = 0; i < numLayers; ++i ) {
				Terrain::TerrainTextureLayer::PixelType* pPixel = pLayerRows[ i ] + x;
				if (*pPixel == 0 )
					continue;
				if (*pPixel > maxValue) {
					maxValue = *pPixel;
					maxLayer = i;
				}
				sum -= *pPixel;
				*pPixel = (Terrain::TerrainTextureLayer::PixelType)((*pPixel * ratio + HALF_STRENGTH) >> STRENGTH_EXP);
				sum += *pPixel;
			}
			MF_ASSERT( maxLayer != Terrain::EditorBaseTerrainBlock::INVALID_LAYER );
			// Remember, this is uint8 math. So can't use "+= (255-sum)"
			if (sum > 255)
				pLayerRows[ maxLayer ][ x ] -= (sum - 255);
			else if (sum < 255 )
				pLayerRows[ maxLayer ][ x ] += (255 - sum);
		}
	}

	for (size_t i = 0; i < numLayers; ++i)
		block.textureLayer( i ).unlock();

	return modified;
}


bool SpaceRecreator::createChunk( int x, int z )
{
	BW_GUARD;

	BW::set<BW::string> seenOverlappers;
	bool seenAmbient = false;
	BW::string seenAmbientSrcChunk;
	Moo::Colour seenAmbientColour;
	float seenAmbientMultiplier;
	bool hasTerrain = false;

	const BW::string name = chunkName( x, z );

	if( name.rfind( '/' ) != name.npos  )
	{
		BW::string path = name.substr( 0, name.rfind( '/' ) + 1 );
		if( subDir_.count( path ) == 0 )
		{
			BWResource::ensurePathExists( spaceName_ + "/" + path );
			subDir_.insert( path );
		}
	}

	GeometryMapping *srcMapping = WorldManager::instance().geometryMapping();

	DataSectionPtr spaceDS = BWResource::openSection( BWResource::dissolveFilename( spaceName_ ) );

	// OK, so create our chunk XMLSection and CData ZipSection.
	DataSectionPtr chunkDS = NULL;
	DataSectionPtr cdataDS = NULL;
	{
		//stop verifying tag name, this will disable unnecessary warning message
		XMLSectionTagCheckingStopper tagCheckingStopper;
		chunkDS = spaceDS->openSection( name + ".chunk", true, XMLSection::creator() );
		cdataDS = spaceDS->openSection( name + ".cdata", true, ZipSection::creator() );
	}
		

	// Now for the actual work. We need to look at every source chunk that overlaps this chunk
	// and copy any relevant information in.
	// Use a BoundedChunkWalker for the source chunks this chunk overlaps?
	// Overlaps are: [ ( x * chunkSize, z * chunkSize ), ( x + 1 * chunkSize, z + 1 * chunkSize ) )...
	// Bounded works on grid, not chunk.
	const BoundingBox chunkBB = BoundingBox(
		Vector3( x * chunkSize_ + xOffset_, MIN_CHUNK_HEIGHT, z * chunkSize_ + zOffset_ ),
		Vector3( x * chunkSize_ + xOffset_ + chunkSize_, MAX_CHUNK_HEIGHT, z * chunkSize_ + zOffset_ + chunkSize_ ));

	Matrix chunkTransform;
	chunkTransform.setTranslate( x * chunkSize_ + xOffset_, 0.f, z * chunkSize_ + zOffset_ );

	Matrix chunkTransformInv;
	chunkTransformInv.setTranslate( -x * chunkSize_ - xOffset_, 0.f, -z * chunkSize_ - zOffset_ );

	uint16 srcGridWidth, srcGridHeight;
	GridCoord srcLocalToWorld( 0, 0 );
	float srcGridSize;
	if (!TerrainUtils::spaceSettings( srcSpaceName_, srcGridSize, srcGridWidth, srcGridHeight, srcLocalToWorld) )
	{
		return false;
	}

	// Create a terrain block for this chunk
	const uint32 blendSize = pTerrainSettings_->blendMapSize();
	SmartPointer<Terrain::EditorTerrainBlock2> pTerrainBlock =
		new Terrain::EditorTerrainBlock2(pTerrainSettings_);
	BW::string error;
	uint32 underLayerIndex;
	bool wroteToUnderLayer = false;
	if (!pTerrainBlock->create( terrainSettingsDS_, &error ))
	{
		if (!error.empty())
		{
			ERROR_MSG( "createChunk %s: Failed to create terrain block: %s\n",
				name.c_str(), error.c_str() );
		}
		return false;
	}
	// Add a layer for texturing space outside the source space
	underLayerIndex = (uint32)pTerrainBlock->insertTextureLayer( blendSize, blendSize );
	if (underLayerIndex == -1)
	{
		ERROR_MSG( "createChunk %s: Failed to add default texture layer\n",
			name.c_str() );
		return false;
	}
	{ // Scope for the holders
		Terrain::TerrainTextureLayer& terrainTextureLayer = pTerrainBlock->textureLayer( underLayerIndex );
		terrainTextureLayer.textureName( s_spaceRecreatorBlankTexture.value() );
		// See PageTerrainTexture::updateProjection... Kinda.
		terrainTextureLayer.uProjection( Vector4( 1.f/chunkSize_, 0.f, 0.f, 0.f ) );
		terrainTextureLayer.vProjection( Vector4( 0.f, 0.f, -1.f/chunkSize_, 0.f ) );
		Terrain::TerrainTextureLayerHolder terrainLayerHolder(&terrainTextureLayer, false);
		Terrain::TerrainTextureLayer::ImageType &textureLayerImage = terrainTextureLayer.image();
		// Fill in any terrain that's outside our srcSpaceBB
		const float spacingX = terrainTextureLayer.blockSize() / terrainTextureLayer.width();
		const float halfSpacingX = spacingX / 2.f;
		const float spacingZ = terrainTextureLayer.blockSize() / terrainTextureLayer.height();
		const float halfSpacingZ = spacingZ / 2.f;

		for( uint32 x = 0; x < terrainTextureLayer.width(); ++x ) {
			const float xWorldPos = chunkBB.minBounds().x + (x * spacingX) + halfSpacingX;
			MF_ASSERT( xWorldPos <= chunkBB.maxBounds().x );
			for ( uint32 z = 0; z < terrainTextureLayer.height(); ++z ) {
				const float zWorldPos = chunkBB.minBounds().z + (z * spacingZ) + halfSpacingZ;
				MF_ASSERT( zWorldPos <= chunkBB.maxBounds().z );
				if (!srcSpaceBB_.intersects( Vector3( xWorldPos, 0.f, zWorldPos ) ))
				{
					textureLayerImage.set( x, z, std::numeric_limits<Terrain::TerrainTextureLayer::PixelType>::max() );
					wroteToUnderLayer = true;
				}
			}
		}
	}

	// Opencoded GeometryMapping::boundsToGrid without the bizarre rounding...
	int srcMinGridX = int(floorf( chunkBB.minBounds().x / srcGridSize ));
	int srcMaxGridX = int(floorf( chunkBB.maxBounds().x / srcGridSize ));
	int srcMinGridZ = int(floorf( chunkBB.minBounds().z / srcGridSize ));
	int srcMaxGridZ = int(floorf( chunkBB.maxBounds().z / srcGridSize ));

	// Walker's co-ords are in local grid space (i.e. [(0,0) -> (srcGridWidth, srcGridHeight))
	// We need to expand one chunk in each direction because if the old and new grid line up,
	// then any badly-positioned objects (i.e. with local transform X or Z < 0 or >= gridSize
	// will be lost. Highlands contains such objects at time of writing, for example:
	// 00020000o: fd_rock_sharp_medium.model has x >= 100.f
	// 0002ffffo: beast_entrance.model has x < 0
	BoundedChunkWalker walker( srcMinGridX - srcLocalToWorld.x - 1, srcMinGridZ - srcLocalToWorld.y - 1,
		srcMaxGridX - srcLocalToWorld.x + 1, srcMaxGridZ - srcLocalToWorld.y + 1 );
	walker.spaceInformation( SpaceInformation( srcSpaceName_, srcLocalToWorld, srcGridWidth, srcGridHeight ) );

	bool virginChunk = true;
	BW::string srcChunkName;
	int16 srcGridX, srcGridZ;
	while (walker.nextTile( srcChunkName, srcGridX, srcGridZ ))
	{
		// Process an overlapping source chunk...
		Chunk *srcChunk = ChunkManager::instance().findChunkByName( srcChunkName, srcMapping );
		MF_ASSERT( srcChunk );
		bool wasLoaded = srcChunk->loaded();
		if (!wasLoaded) {
			ChunkManager::instance().loadChunkNow( srcChunk );
			ChunkManager::instance().checkLoadingChunks();
		}
		MF_ASSERT( !srcChunk->loading() && srcChunk->loaded() && srcChunk->isBound() );
		//WorldManager::instance().workingChunk( srcChunk, false );

		const BoundingBox srcChunkBB = BoundingBox( Vector3( srcChunk->x() * srcGridSize, MIN_CHUNK_HEIGHT, srcChunk->z() * srcGridSize ), Vector3( srcChunk->x() * srcGridSize + srcGridSize, MAX_CHUNK_HEIGHT, srcChunk->z() * srcGridSize + srcGridSize ));
		/* This is always being true, because Visual Studio hates me...
			* So leave it commented out for now.
			* Because of the way we process individual terrain blocks, we need the two chunks to line up
			* correctly, or things get weird.
			* This also keeps srcChunkBB consistent with the ChunkWalker's calculations
			*/
#if 0
		if ( srcChunkBB != srcChunk->boundingBox() )
			WARNING_MSG( "createChunk: source chunk %s returns bounding box (%f,%f) => (%f,%f) but calculated bounding box (%f, %f) => (%f,%f)\n",
			srcChunkName.c_str(), 
			srcChunk->boundingBox().minBounds().x, srcChunk->boundingBox().minBounds().z,
			srcChunk->boundingBox().maxBounds().x, srcChunk->boundingBox().maxBounds().z,
			srcChunkBB.minBounds().x, srcChunkBB.minBounds().z,
			srcChunkBB.maxBounds().x, srcChunkBB.maxBounds().z );
#endif

		/*
		DEBUG_MSG( "createChunk %s (%f, %f) => (%f,%f): considering source chunk %s (%f,%f) => (%f,%f)\n",
			name.c_str(),
			chunkBB.minBounds().x, chunkBB.minBounds().z,
			chunkBB.maxBounds().x, chunkBB.maxBounds().z,
			srcChunkName.c_str(),
			srcChunkBB.minBounds().x, srcChunkBB.minBounds().z,
			srcChunkBB.maxBounds().x, srcChunkBB.maxBounds().z );
		*/

		// We need this to be able to skip it while iterating the static objects.
		EditorChunkTerrain* srcChunkTerrain = dynamic_cast<EditorChunkTerrain*>(ChunkTerrainCache::instance( *srcChunk ).pTerrain());
		MF_ASSERT( ChunkTerrainCache::instance( *srcChunk ).pTerrain() == NULL || srcChunkTerrain != NULL );

		typedef BW::vector<ChunkItemPtr> Items;
		const Items& staticItems = srcChunk->staticItems();

		for (Items::const_iterator it = staticItems.begin(); it != staticItems.end(); ++it)
		{
			bool wrotePos = false;

			ChunkItemPtr item = *it;
			// Portals are magic
			if ( item->isPortal() )
				continue;

			// EditorChunkLinks are created when their EditorChunkLinkables are created, they are
			// not stored as ChunkItems in the XML
			if ( item->isEditorChunkLink())
				continue;

			// Terrain is dealt with later
			if ( item.getObject() == srcChunkTerrain )
				continue;

			const Name itemClass = item->edClassName();
			const Matrix& srcTransform = item->edTransform();
			Matrix dstTransform = srcTransform;
			dstTransform.postMultiply( srcChunk->transform() );

			EditorChunkOverlapper* overlapper = dynamic_cast<EditorChunkOverlapper*>( item.getObject() );
			EditorChunkAmbientLight* ambient = dynamic_cast<EditorChunkAmbientLight*>( item.getObject() );
			if (overlapper != NULL) {
				// EditorChunkOverlappers need to appear in any chunk their internal chunk overlaps
				const BoundingBox &overlapperBB = overlapper->pOverlappingChunk()->boundingBox();
				if (!chunkBB.intersects( overlapperBB ))
					continue;

				const BW::string &overlapperID = overlapper->pOverlappingChunk()->identifier();
				if ( seenOverlappers.count( overlapperID ) != 0 )
					continue;
				overlappers_.insert( overlapperID );
				seenOverlappers.insert( overlapperID );

				// Overlappers don't have any position to update
				wrotePos = true;
			} else if (ambient != NULL) {
				// We're spreading the ChunkAmbientLight to anywhere that touches its
				// original chunk, as they don't actually have a real position.
				// TODO: Review this, perhaps we should be using chunk centre?
				if (!chunkTouchesChunk( chunkBB, srcChunkBB ))
					continue;

				// Need to avoid duplicate ChunkAmbientLights and log if we kill one
				// that doesn't match the one we did keep...
				if (seenAmbient) {
					if (seenAmbientColour != ambient->colour() || seenAmbientMultiplier != ambient->multiplier())
						WARNING_MSG( "createChunk %s: ChunkAmbientLight read from %s has colour %f,%f,%f and multipler %f, "
						"ignoring in favour of previous ChunkAmbientLight read from %s with colour %f,%f,%f and multipler %f\n",
						name.c_str(),
						srcChunkName.c_str(), ambient->colour().r * 255.f, ambient->colour().g * 255.f, ambient->colour().b * 255.f, ambient->multiplier(),
						seenAmbientSrcChunk.c_str(), seenAmbientColour.r * 255.f, seenAmbientColour.g * 255.f, seenAmbientColour.b * 255.f, seenAmbientMultiplier
						);
					continue;
				}
				seenAmbient = true;
				seenAmbientColour = ambient->colour();
				seenAmbientMultiplier = ambient->multiplier();
				seenAmbientSrcChunk = srcChunkName;

				// ChunkAmbientLights don't have any position to update
				wrotePos = true;
			} else if (item->edIsVLO()) {
				ChunkVLO *vlo = dynamic_cast< ChunkVLO* >( item.getObject() );
				MF_ASSERT( vlo );
				// VLOs need to appear in any chunk they overlap

				BoundingBox bb;
				vlo->edBounds( bb );
				Math::OrientedBBox vloBox( bb, vlo->object()->localTransform() );
				Math::OrientedBBox chunkBox( chunkBB, Matrix::identity );

				if (!vloBox.intersects( chunkBox ))
				{
					continue;
				}

				BW::set<BW::string>& vlos = pImpl->chunkVloReferences[name];
				BW::string vloUid = vlo->object()->getUID();
				bool inserted = vlos.insert( vloUid ).second;

				if (!inserted)
				{
					continue;
				}

				vlos.insert( vloUid );

				Matrix vloTransform = dstTransform;
				Matrix offset( Matrix::identity );
				offset.translation( Vector3( ( float ) -xOffset_,  0.0f, ( float ) -zOffset_ ) );
				vloTransform.postMultiply( offset );
				VLOMemento& vloData = vlos_[vloUid];
				vloData.transform = vloTransform;
				vloData.type = vlo->object()->type();
				wrotePos = true;
			} else if (!fallsInChunk( chunkBB, dstTransform.applyToOrigin(), x == maxX_, z == maxZ_ )) {
				// For anything else, check if they lie within the destination chunk
				// dstTransform is in WorldSpace here
				continue;
			}

			// dstTransform is in destination chunk space now.
			dstTransform.postMultiply( chunkTransformInv );

			/*
			DEBUG_MSG( "createChunk %s: (%f, %f, %f) <= %s (%f, %f, %f): %s\n", 
				name.c_str(), dstTransform._41, dstTransform._42, dstTransform._43,
				srcChunkName.c_str(), srcTransform._41, srcTransform._42, srcTransform._43,
				itemClass.c_str());
			*/

			if (item->isEditorChunkStationNode()) {
				EditorChunkStationNode *stationNode = dynamic_cast<EditorChunkStationNode*>( item.getObject() );
				MF_ASSERT( stationNode );
				const BW::string &nodeName = stationNode->graph()->name();
				if (!nodeName.empty())
					stationGraphs_.insert( nodeName );
			}

			if (item->isEditorEntity()) {
				EditorChunkEntity *entity = dynamic_cast<EditorChunkEntity*>( item.getObject() );
				MF_ASSERT( entity );
				const BW::string &nodeName = entity->patrolListGraphId();
				if (!nodeName.empty())
					stationGraphs_.insert( nodeName );
				// Need to fix up any references to this object later
				const BW::string guid = entity->chunkItemLinker()->guid();
				MF_ASSERT( guidChunks_.count( guid ) == 0 );
				guidChunks_.insert(std::make_pair( guid, std::make_pair( srcChunkName, name ) ) );
				itemsWithPossibleBrokenLinks.insert( item );
			}

			if (item->isEditorUserDataObject()){
				EditorChunkUserDataObject *udo = dynamic_cast<EditorChunkUserDataObject*>( item.getObject() );
				MF_ASSERT( udo );
				// Need to fix up any references to this object later
				const BW::string guid = udo->chunkItemLinker()->guid();
				MF_ASSERT( guidChunks_.count( guid ) == 0 );
				guidChunks_.insert(std::make_pair( guid, std::make_pair( srcChunkName, name ) ) );
				itemsWithPossibleBrokenLinks.insert( item );
			}

			// This is open-coded EditorChunkItem::edCloneSection
			DataSectionPtr srcItemDS = item->pOwnSect();
			if (!srcItemDS) {
				WARNING_MSG( "createChunk %s: %s: No pOwnSect()\n", name.c_str(), itemClass.c_str());
				continue;
			}

			DataSectionPtr dstItemDS = chunkDS->newSection( srcItemDS->sectionName() );
			dstItemDS->copy( srcItemDS );

			if (dstItemDS->openSection( "transform" )) {
				wrotePos |= true;
				dstItemDS->writeMatrix34( "transform", dstTransform );
			}
			if (dstItemDS->openSection( "position" )) {
				wrotePos |= true;
				dstItemDS->writeVector3( "position", dstTransform.applyToOrigin() );
			}
			if (dstItemDS->openSection( "direction" ))
				dstItemDS->writeVector3( "direction", dstTransform.applyToUnitAxisVector( 2 ) );

			if (!wrotePos)
				WARNING_MSG( "createChunk %s: %s: No transform or position attribute to update\n", name.c_str(), itemClass.c_str());

			// I'm not sure we need to do this, but there's no other way for the ChunkItem
			// to write out anything it wants to put into the cdata.
			// Mind you, every implementation of this is a no-op currently, and there's no
			// promise that any future implementation won't try to access srcChunk or any
			// other state which relates to the source space rather than the new space.
			item->edChunkSaveCData( cdataDS );
		}

		// Process the terrain from this chunk that overlaps the new chunk
		// We don't want to process terrain from a chunk whose min border happens to match
		// the max border of our new chunk.
		bool copyTerrain = chunkTouchesChunk( chunkBB, srcChunkBB );

		Terrain::EditorTerrainBlock2* pSrcTerrainBlock = nullptr;
		Terrain::BaseTerrainBlockPtr baseBlock = nullptr;

		if (copyTerrain)
		{
			virginChunk = false;

			if (srcChunkTerrain)
			{
				hasTerrain = true;
				pSrcTerrainBlock =
					dynamic_cast<Terrain::EditorTerrainBlock2*>( &srcChunkTerrain->block() );
			}
			else
			{
				// Attempt to load the texture manually.
				// We have to do this because the height map module needs the texture data
				// even if there is not texture on the chunk.
				const BW::string filename =
					BWResource::dissolveFilename( srcSpaceName_ ) + "/" +
					srcChunkName + ".cdata/terrain";
				baseBlock =	Terrain::BaseTerrainBlock::loadBlock( filename,
					Matrix::identity, Vector3::zero(),
					WorldManager::instance().pTerrainSettings() );
				pSrcTerrainBlock =
					dynamic_cast<Terrain::EditorTerrainBlock2*>( &(*baseBlock) );
			}

			copyTerrain = pSrcTerrainBlock != nullptr;
		}

		if (copyTerrain)
		{
			MF_ASSERT( pSrcTerrainBlock );

			// Copy maps: HeightMap
			{ // Scope for the holders
				Terrain::TerrainHeightMap &srcTerrainHeightMap = pSrcTerrainBlock->heightMap();

				Terrain::TerrainHeightMap &terrainHeightMap = pTerrainBlock->heightMap();
				Terrain::TerrainHeightMapHolder terrainHeightMapHolder(&terrainHeightMap, false);
				Terrain::TerrainHeightMap::ImageType &heightMapImage = terrainHeightMap.image();

				// TerrainUtils::patchEdges actually fills the blocksWidth()'d pole from the previous pole
				// which means we don't need to worry about overlapping if the chunk edges line up exactly
				// on that pole.
				// This does mean the maxX and maxZ rows of triangles in each space is flat
				for (uint32 x = 0; x < terrainHeightMap.blocksWidth(); ++x) {
					const float xWorldPos = chunkBB.minBounds().x + (x * terrainHeightMap.spacingX());
					MF_ASSERT( xWorldPos <= chunkBB.maxBounds().x );
					for (uint32 z = 0; z < terrainHeightMap.blocksHeight(); ++z) {
						const float zWorldPos = chunkBB.minBounds().z + (z * terrainHeightMap.spacingZ());
						MF_ASSERT( zWorldPos <= chunkBB.maxBounds().z );
						BoundingBox heightPoleBox;
						heightPoleBox.addBounds( Vector3( xWorldPos, 0.f, zWorldPos ) );
						heightPoleBox.expandSymmetrically( terrainHeightMap.spacingX() / 2.f, 0.f, terrainHeightMap.spacingZ() / 2.f );
						if ( !fallsInChunk( srcChunkBB, heightPoleBox, srcGridX == srcMapping->maxGridX(), srcGridZ == srcMapping->maxGridY() ) )
							continue;
						const float height = srcTerrainHeightMap.heightAt(
											xWorldPos - srcChunkBB.minBounds().x,
											zWorldPos - srcChunkBB.minBounds().z );
						heightMapImage.set( x + terrainHeightMap.xVisibleOffset(),
											z + terrainHeightMap.zVisibleOffset(),
											height );
					}
				}
			}
			// Holemap
			{ // Scope for the holders
				Terrain::TerrainHoleMap &srcTerrainHoleMap = pSrcTerrainBlock->holeMap();

				Terrain::TerrainHoleMap &terrainHoleMap = pTerrainBlock->holeMap();
				Terrain::TerrainHoleMapHolder terrainHoleMapHolder(&terrainHoleMap, false);
				Terrain::TerrainHoleMap::ImageType &holeMapImage = terrainHoleMap.image();

				const float spacingX = terrainHoleMap.blockSize() / terrainHoleMap.width();
				const float halfSpacingX = spacingX / 2.f;
				const float spacingZ = terrainHoleMap.blockSize() / terrainHoleMap.height();
				const float halfSpacingZ = spacingZ / 2.f;

				for (uint32 x = 0; x < terrainHoleMap.width(); ++x) {
					const float xWorldPos = chunkBB.minBounds().x + (x * spacingX) + halfSpacingX;
					MF_ASSERT( xWorldPos <= chunkBB.maxBounds().x );
					for (uint32 z = 0; z < terrainHoleMap.height(); ++z) {
						const float zWorldPos = chunkBB.minBounds().z + (z * spacingZ) + halfSpacingZ;
						MF_ASSERT( zWorldPos <= chunkBB.maxBounds().z );
						BoundingBox holeBox;
						holeBox.addBounds( Vector3( xWorldPos, 0.f, zWorldPos ) );
						holeBox.expandSymmetrically( halfSpacingX, 0.f, halfSpacingZ );
						if ( !fallsInChunk( srcChunkBB, holeBox, srcGridX == srcMapping->maxGridX(), srcGridZ == srcMapping->maxGridY() ) )
							continue;
						const bool hole = srcTerrainHoleMap.holeAt(
											holeBox.minBounds().x - srcChunkBB.minBounds().x,
											holeBox.minBounds().z - srcChunkBB.minBounds().z,
											holeBox.maxBounds().x - srcChunkBB.minBounds().x,
											holeBox.maxBounds().z - srcChunkBB.minBounds().z );
						holeMapImage.set( x, z, hole );
					}
				}
			}

			// Texture layers
			for (uint32 srcLayerIndex = 0; srcLayerIndex < pSrcTerrainBlock->numberTextureLayers(); ++srcLayerIndex)
			{ // Scope for the holders
				Terrain::TerrainTextureLayer &srcTerrainTextureLayer = pSrcTerrainBlock->textureLayer( srcLayerIndex );
				Terrain::TerrainTextureLayerHolder srcTerrainTextureLayerHolder(&srcTerrainTextureLayer, true);
				const Terrain::TerrainTextureLayer::ImageType &srcTextureLayerImage = srcTerrainTextureLayer.image();

				const BW::string &textureName = srcTerrainTextureLayer.textureName();
				const BW::string &bumpTextureName = srcTerrainTextureLayer.bumpTextureName();
				Vector4 uProjection = srcTerrainTextureLayer.uProjection();
				uProjection.w = 0.f;
				Vector4 vProjection = srcTerrainTextureLayer.vProjection();
				vProjection.w = 0.f;

				size_t layerIndex = pTerrainBlock->findLayer( 
					textureName, 
					bumpTextureName, 
					uProjection, vProjection,
					TerrainTextureUtils::PROJECTION_COMPARISON_EPSILON );
				bool newLayer = false;

				if (layerIndex == Terrain::EditorBaseTerrainBlock::INVALID_LAYER) {
					layerIndex = pTerrainBlock->insertTextureLayer( blendSize, blendSize );
					newLayer = true;
					MF_ASSERT( layerIndex != underLayerIndex );
				}
				MF_ASSERT( layerIndex != Terrain::EditorBaseTerrainBlock::INVALID_LAYER );

				bool removeTextureLayer = false;
				// This scope is for terrainTextureLayerHolder's live time ends
				// before srcTerrainTextureLayer is removed.
				{
					Terrain::TerrainTextureLayer &terrainTextureLayer = pTerrainBlock->textureLayer( layerIndex );
					Terrain::TerrainTextureLayerHolder terrainTextureLayerHolder(&terrainTextureLayer, false);
					Terrain::TerrainTextureLayer::ImageType &textureLayerImage = terrainTextureLayer.image();

					if ( newLayer ) {
						terrainTextureLayer.textureName( textureName );
						terrainTextureLayer.uProjection( uProjection );
						terrainTextureLayer.vProjection( vProjection );
					}

					const float spacingX = terrainTextureLayer.blockSize() / terrainTextureLayer.width();
					const float halfSpacingX = spacingX / 2.f;
					const float spacingZ = terrainTextureLayer.blockSize() / terrainTextureLayer.height();
					const float halfSpacingZ = spacingZ / 2.f;

					bool wroteToBlendLayer = false;

					for( uint32 x = 0; x < terrainTextureLayer.width(); ++x ) {
						const float xWorldPos = chunkBB.minBounds().x + (x * spacingX) + halfSpacingX;
						MF_ASSERT( xWorldPos <= chunkBB.maxBounds().x );
						const float srcX = Math::lerp<float, float>( xWorldPos, srcChunkBB.minBounds().x, srcChunkBB.maxBounds().x, 0.f, (float)srcTextureLayerImage.width()) ;
						if (srcX < 0.f)
							continue;
						if (srcX >= (float)srcTextureLayerImage.width())
							break;
						for ( uint32 z = 0; z < terrainTextureLayer.height(); ++z ) {
							const float zWorldPos = chunkBB.minBounds().z + (z * spacingZ) + halfSpacingZ;
							MF_ASSERT( zWorldPos <= chunkBB.maxBounds().z );
							const float srcZ = Math::lerp<float, float>( zWorldPos, srcChunkBB.minBounds().z, srcChunkBB.maxBounds().z, 0.f, (float)srcTextureLayerImage.height());
							if (srcZ < 0.f)
								continue;
							if (srcZ >= (float)srcTextureLayerImage.height())
								break;
							BoundingBox blendBox;
							blendBox.addBounds( Vector3( xWorldPos, 0.f, zWorldPos ) );
							blendBox.expandSymmetrically( halfSpacingX, 0.f, halfSpacingZ );
							IF_NOT_MF_ASSERT_DEV( fallsInChunk( srcChunkBB, blendBox, srcGridX == srcMapping->maxGridX(), srcGridZ == srcMapping->maxGridY() ) )
								continue;
							
							uint32 oldBlend = textureLayerImage.get( x, z );
							MF_ASSERT(oldBlend <= std::numeric_limits<Terrain::TerrainTextureLayer::PixelType>::max());

							uint32 blend = (uint32)floor(srcTextureLayerImage.getBilinear(srcX, srcZ) + 0.5);
							MF_ASSERT(blend <= std::numeric_limits<Terrain::TerrainTextureLayer::PixelType>::max());

							MF_ASSERT(blend + oldBlend <= std::numeric_limits<Terrain::TerrainTextureLayer::PixelType>::max());
							textureLayerImage.set( x, z, (Terrain::TerrainTextureLayer::PixelType)(blend + oldBlend) );

							wroteToBlendLayer = true;
						}
					}

					if (wroteToBlendLayer && layerIndex == underLayerIndex)
					{
						wroteToUnderLayer = true;
					}

					removeTextureLayer = newLayer && 
											layerIndex != underLayerIndex &&
											!wroteToBlendLayer;
				}//scope end for terrainTextureLayerHolder

				if (removeTextureLayer)
				{
					pTerrainBlock->removeTextureLayer( layerIndex );
				}
			}
		}

		if (!wasLoaded && srcChunk->removable())
		{
			srcChunk->unbind(false);
			srcChunk->unload();
			AmortiseChunkItemDelete::instance().purge();
		}
		// TODO: Is there a better way than this? Can we tick the BigWorld render loop?
		Moo::rc().preloadDeviceResources( 10000000 );
	}

	bool good = true;

	if (!wroteToUnderLayer)
	{
		pTerrainBlock->removeTextureLayer( underLayerIndex );
	}

	if (pTerrainBlock->numberTextureLayers() > 0)
	{
		// Finish terrain texturing and write it to .cdata
		normaliseTerrainBlockBlends( *pTerrainBlock );
		TerrainTextureUtils::deleteEmptyLayers( *pTerrainBlock, TerrainTextureUtils::REMOVE_LAYER_THRESHOLD );
		pTerrainBlock->optimiseLayers( TerrainTextureUtils::PROJECTION_COMPARISON_EPSILON );
		pTerrainBlock->rebuildCombinedLayers();
		pTerrainBlock->rebuildNormalMap( Terrain::NMQ_FAST );
		good &= pTerrainBlock->rebuildLodTexture( chunkTransform );
		good &= pTerrainBlock->saveToCData( cdataDS );

		if (hasTerrain || virginChunk)
		{
			// See EditorChunkTerrain::toss, oddly enough
			DataSectionPtr pTerrainDS = chunkDS->newSection( "terrain" );

			if (pTerrainDS)
			{
				// See EditorChunkTerrain::edSave
				good &= pTerrainDS->writeString( "resource",
					name + ".cdata/" + pTerrainBlock->dataSectionName() );
			}
			else
			{
				good = false;
			}
		}
	}

	// Write out the chunk files
	good &= chunkDS->save();
	good &= cdataDS->save();

	return good;
}


const BW::string SpaceRecreator::createChunks( bool withPurge )
{
	BW_GUARD;

	BW::string result = "";

	ProgressBarTask progressTask( LocaliseUTF8( STAGES[2].name ), (float)(maxX_-minX_+1) * (maxZ_-minZ_+1), true );

	subDir_.clear();

	int start = GetTickCount();
		
	for (int x = minX_; x <= maxX_ && result.empty(); ++x)
	{
		int loop = GetTickCount();
			
		for (int z = minZ_; z <= maxZ_ && result.empty(); ++z)
		{
			result += createChunk( x, z ) ? "" : ("Unable to create chunk " + chunkName( x, z ) + "\n");
			progressTask.step();
			if( shouldAbort() )
				return errorMessage_;
		}

		// Purge after every column
		if (withPurge)
			BWResource::instance().purgeAll();
			
		int last = GetTickCount();

		float average = float( (last - start) / (x - minX_ + 1) );

		DEBUG_MSG( "createChunks: Output row %d of %d, %dms, %2.0fms average\n", x-minX_+1, maxX_-minX_+1, last - loop,  average);
	}

	return result;
}

bool copyVLODataSectionAndChangePosition( DataSectionPtr srcDS, DataSectionPtr dstDS,
                                          const BW::string& vloType,
                                          const Vector3& newPosition)
{
	if ( srcDS )
	{
		dstDS->copy( srcDS );
		DataSectionPtr typeSpecificSection = dstDS->openSection( vloType );
		if ( typeSpecificSection )
		{
			typeSpecificSection->writeVector3("position", newPosition);
			return dstDS->save();
		}
	}
	return false;
}

const BW::string SpaceRecreator::copyAssets()
{
	BW_GUARD;
	BW::string result = "";

	GeometryMapping *srcMapping = WorldManager::instance().geometryMapping();

	DataSectionPtr srcSpaceDS = BWResource::openSection( BWResource::dissolveFilename( srcSpaceName_ ) );
	DataSectionPtr spaceDS = BWResource::openSection( BWResource::dissolveFilename( spaceName_ ) );

	ScopedProgressBar hepler( 3 );

	{
		ProgressBarTask  task( LocaliseUTF8( STAGES[3].name ), (float)overlappers_.size(), true );

		BW::set<BW::string>::const_iterator it;
		for( it = overlappers_.begin(); it != overlappers_.end() && result.empty(); ++it )
		{
			const BW::string &name = *it;
			DataSectionPtr srcChunkDS = srcSpaceDS->openSection( name + ".chunk" );
			DataSectionPtr chunkDS = spaceDS->newSection( name + ".chunk", XMLSection::creator() );
			if (srcChunkDS && chunkDS)
			{
				chunkDS->copy( srcChunkDS );
				const char * TRANSFORM_PROP_NAME = "transform";
				if (chunkDS->openSection( TRANSFORM_PROP_NAME ))
				{
					Matrix origTransform = chunkDS->readMatrix34( TRANSFORM_PROP_NAME );
					Matrix offset( Matrix::identity );
					offset.translation( Vector3( (float ) -xOffset_,  0.0f, (float ) -zOffset_ ) );
					origTransform.postMultiply( offset );
					chunkDS->writeMatrix34( TRANSFORM_PROP_NAME, origTransform );
				}
				result += chunkDS->save() ? "" : ("Failed to save " + name + ".chunk\n");
			} else {
				result += "Failed to copy " + name + ".chunk\n";
			}

			DataSectionPtr srcCdataDS = srcSpaceDS->openSection( name + ".cdata" );
			DataSectionPtr cdataDS = spaceDS->newSection( name + ".cdata", ZipSection::creator() );
			if (srcCdataDS && cdataDS)
			{
				cdataDS->copy( srcCdataDS );
				result += cdataDS->save() ? "" : ("Failed to save " + name + ".cdata\n");
			} else {
				result += "Failed to copy " + name + ".cdata\n";
			}

			// Need to scan the overlapper for VLOs, StationGraph references and EditorChunkItemLinkables
			Chunk *srcChunk = ChunkManager::instance().findChunkByName( name, srcMapping );
			MF_ASSERT( srcChunk );
			bool wasLoaded = srcChunk->loaded();
			if (!wasLoaded) {
				ChunkManager::instance().loadChunkNow( srcChunk );
				ChunkManager::instance().checkLoadingChunks();
			}
			MF_ASSERT( !srcChunk->loading() && srcChunk->loaded() && srcChunk->isBound() );

			// EditorChunkItemLinkable::getOutsideChunkId
			const Vector3 overlapperOwnerPosition = srcChunk->transform().applyToOrigin();
			const BW::string srcOutsideChunkName = srcMapping->outsideChunkIdentifier( overlapperOwnerPosition );
			// Open-coded GeometryMapping::outsideChunkIdentifier
			const int outsideChunkGridX = int(floorf( (overlapperOwnerPosition.x - xOffset_) / chunkSize_ ));
			const int outsideChunkGridZ = int(floorf( (overlapperOwnerPosition.z - zOffset_) / chunkSize_ ));
			const BW::string outsideChunkName = chunkName( outsideChunkGridX, outsideChunkGridZ );

			typedef BW::vector<ChunkItemPtr> Items;
			const Items& staticItems = srcChunk->staticItems();

			for (Items::const_iterator it = staticItems.begin(); it != staticItems.end(); ++it)
			{
				ChunkItemPtr item = *it;

				if (item->edIsVLO())
				{
					ChunkVLO* vlo = dynamic_cast<ChunkVLO*>( item.getObject() );
					MF_ASSERT( vlo );
					// Doesn't need a bounds check, as indoor chunks are not changed.
					Matrix dstTransform = item->edTransform();
					dstTransform.postMultiply( srcChunk->transform() );
					Matrix vloTransform = dstTransform;
					Matrix offset( Matrix::identity );
					offset.translation(
						Vector3( float( -xOffset_ ), 0.0f, float( -zOffset_ ) ) );
					vloTransform.postMultiply( offset );
					VLOMemento& vloData = vlos_[vlo->object()->getUID()];
					vloData.transform = vloTransform;
					vloData.type = vlo->object()->type();
				}

				if (item->isEditorEntity()) {
					EditorChunkEntity *entity = dynamic_cast<EditorChunkEntity*>( item.getObject() );
					MF_ASSERT( entity );
					const BW::string &nodeName = entity->patrolListGraphId();
					if (!nodeName.empty())
						stationGraphs_.insert( nodeName );
					// Need to fix up any references to this object later
					const BW::string guid = entity->chunkItemLinker()->guid();
					MF_ASSERT( guidChunks_.count( guid ) == 0 );
					guidChunks_.insert(std::make_pair( guid, std::make_pair( srcOutsideChunkName, outsideChunkName ) ) );
					itemsWithPossibleBrokenLinks.insert( item );
				}

				if (item->isEditorUserDataObject()){
					EditorChunkUserDataObject *udo = dynamic_cast<EditorChunkUserDataObject*>( item.getObject() );
					MF_ASSERT( udo );
					// Need to fix up any references to this object later
					const BW::string guid = udo->chunkItemLinker()->guid();
					MF_ASSERT( guidChunks_.count( guid ) == 0 );
					guidChunks_.insert(std::make_pair( guid, std::make_pair( srcOutsideChunkName, outsideChunkName ) ) );
					itemsWithPossibleBrokenLinks.insert( item );
				}
			}

			if (!wasLoaded && srcChunk->removable())
			{
				srcChunk->unbind(false);
				srcChunk->unload();
				AmortiseChunkItemDelete::instance().purge();
			}
			task.step();
			if( shouldAbort() )
				return errorMessage_;
		}
	}

	{
		ProgressBarTask  task( LocaliseUTF8( STAGES[4].name ), (float)stationGraphs_.size(), true );
		BW::set<BW::string>::const_iterator it;
		for( it = stationGraphs_.begin(); it != stationGraphs_.end() && result.empty(); ++it )
		{
			const BW::string &name = *it;
			DataSectionPtr srcGraphDS = srcSpaceDS->openSection( name + ".graph" );
			DataSectionPtr graphDS = spaceDS->newSection( name + ".graph", XMLSection::creator() );
			if (srcGraphDS && graphDS)
			{
				graphDS->copy( srcGraphDS );
				result += graphDS->save() ? "" : ("Failed to save " + name + ".graph\n");
			} else {
				result += "Failed to copy " + name + ".graph\n";
			}
			task.step();
			if( shouldAbort() )
				return errorMessage_;
		}

	}

	{
		ProgressBarTask  task( LocaliseUTF8( STAGES[5].name ), (float)vlos_.size(), true );
		StringHashMap< VLOMemento >::const_iterator it = vlos_.begin(),
		                                      vlos_end = vlos_.end();
		for( ; it != vlos_end && result.empty(); ++it )
		{
			const BW::string& name = it->first;
			const VLOMemento& vloData = it->second;
			VeryLargeObjectPtr pVLO = VeryLargeObject::getObject(name);
			bool successfulSave = false;

			DataSectionPtr legacyVLODS = spaceDS->newSection( name + ".vlo", XMLSection::creator() );
			if ( legacyVLODS )
			{
				// LEGACY MODE:
				// Regardless of whether the VLO is loaded or not, we'll copy
				// over the old VLO DataSection and change its position values
				// directly.
				DataSectionPtr srcLegacyVLODS = srcSpaceDS->openSection( name + ".vlo" );
				if (!srcLegacyVLODS)
					srcLegacyVLODS = pVLO->section();
				successfulSave = copyVLODataSectionAndChangePosition(
					srcLegacyVLODS, legacyVLODS,
					vloData.type, vloData.transform[3] );
				if ( ! successfulSave )
				{
					result += "Failed to save " + name + ".vlo\n";
				}
			}

			DataSectionPtr VLODS = spaceDS->newSection( "_" + name + ".vlo", XMLSection::creator() );
			if ( VLODS && ! successfulSave )
			{
				// First we'll check if the VLO is accessible and if we can
				// write the new .vlo directly from that object.
				if ( pVLO )
				{
					EditorChunkModelVLO * chunkModelVLO = dynamic_cast< EditorChunkModelVLO * >( pVLO.get() );
					if ( chunkModelVLO != NULL )
					{
						chunkModelVLO->save( VLODS, vloData.transform );
						successfulSave = true;
					}
				}
				if ( ! successfulSave )
				{
					// Copy information from previous .vlo and adjust position
					// in-place.
					DataSectionPtr srcVLODS = srcSpaceDS->openSection( "_" + name + ".vlo" );\
						successfulSave = copyVLODataSectionAndChangePosition(
						srcVLODS, VLODS,
						vloData.type, vloData.transform[3] );
					if ( ! successfulSave )
					{
						result += "Failed to save _" + name + ".vlo\n";
					}
				}
			}

			if ( pVLO && successfulSave )
			{
				// VLO is loaded, make sure we class it as dirty.
				//pVLO->updateLocalVars( vloData.transform );
				pVLO->dirty();
			}

			// The following two are specific to the water VLO.
			// The filenames are created in ChunkWater::load but used elsewhere
			DataSectionPtr srcWaterLegacyTransparencyDS = srcSpaceDS->openSection( name + ".odata" );
			if (srcWaterLegacyTransparencyDS)
			{
				DataSectionPtr waterLegacyTransparencyDS = spaceDS->newSection( name + ".odata", BinSection::creator() );
				if (waterLegacyTransparencyDS)
				{
					waterLegacyTransparencyDS->copy( srcWaterLegacyTransparencyDS );
					result += waterLegacyTransparencyDS->save() ? "" : ("Failed to save " + name + ".odata\n");
				} else {
					result += "Failed to copy " + name + ".odata\n";
				}
			}

			DataSectionPtr srcWaterTransparencyDS = srcSpaceDS->openSection( "_" + name + ".odata" );
			if (srcWaterTransparencyDS)
			{
				DataSectionPtr waterTransparencyDS = spaceDS->newSection( "_" + name + ".odata", BinSection::creator() );
				if (waterTransparencyDS)
				{
					waterTransparencyDS->copy( srcWaterTransparencyDS );
					result += waterTransparencyDS->save() ? "" : ("Failed to save _" + name + ".odata\n");
				} else {
					result += "Failed to copy _" + name + ".odata\n";
				}
			}
			task.step();
			if( shouldAbort() )
				return errorMessage_;
		}
	}

	BWResource::instance().purgeAll();

	return result;
}


void SpaceRecreator::fixChunkLinkArray( DataSectionPtr arrayDS, bool knownToBeChunkLinks /*= false */)
{
	BW_GUARD;
	// We might be guessing that this's an array
	if (arrayDS->countChildren() == 0)
		return;

	for (DataSectionIterator it = arrayDS->begin(); it != arrayDS->end();)
	{
		FixChunkLinkResult fixChunkLinkResult =
				pImpl->fixChunkLink( *it, knownToBeChunkLinks );

		if (fixChunkLinkResult == FixChunkLinkResult::Ignored &&
				!knownToBeChunkLinks)
		{
			fixChunkLinkArray( *it );
		}
		else if (fixChunkLinkResult == FixChunkLinkResult::Deleted
				&& knownToBeChunkLinks)
		{
			arrayDS->delChild( *it );
			continue;
		}

		++it;
	}
}


bool SpaceRecreator::fixChunkLinksInChunk( DataSectionPtr chunkDS )
{
	BW_GUARD;
	for (DataSectionIterator it = chunkDS->begin(); it != chunkDS->end(); ++it)
	{
		DataSectionPtr childDS = *it;
		// EditorChunkItemLinkable::loadBackLinks and EditorChunkItemLinkable::saveBackLinks
		DataSectionPtr backLinksDS = childDS->openSection( "backLinks" );
		if (backLinksDS)
			fixChunkLinkArray( backLinksDS, true );
		// Brute-force properties search for ChunkLinks or arrays of ChunkLinks (UDO_REFs)
		// EditorChunkUserDataObject::edLoad and EditorChunkEntity::edLoad
		DataSectionPtr propertiesDS = childDS->openSection( "properties" );
		if (propertiesDS)
			for(DataSectionIterator propIt = propertiesDS->begin();
				propIt != propertiesDS->end(); ++propIt)
				if (pImpl->fixChunkLink( *propIt ) ==
						FixChunkLinkResult::Ignored)
				{
					fixChunkLinkArray( *propIt );
				}
	}
	return chunkDS->save();
}


Terrain::EditorTerrainBlock2Ptr SpaceRecreator::readTerrainBlock( DataSectionPtr spaceDS, int x, int z )
{
	BW_GUARD;
	DataSectionPtr chunkDS = spaceDS->openSection( chunkName( x, z ) + ".chunk" );
	if (!chunkDS)
		return NULL;

	//chunks may have no terrain.
	const BW::string &terrainPath = chunkDS->readString( "terrain/resource" );
	if (terrainPath.empty())
	{
		return NULL;
	}

	Matrix chunkTransform;
	chunkTransform.setTranslate( x * chunkSize_, 0.f, z * chunkSize_ );

	Terrain::EditorTerrainBlock2Ptr result = new Terrain::EditorTerrainBlock2( pTerrainSettings_ );
		
	BW::string error = "Failed to allocate Terrain::EditorTerrainBlock2";
	if ( !result || !result->load( BWResource::dissolveFilename( spaceName_ ) + "/" + terrainPath, chunkTransform, chunkTransform.applyToOrigin(), &error ) ) {
		ERROR_MSG( "readTerrainBlock( %d, %d ): %s\n", x, z, error.c_str() );
		return NULL;
	}
	return result;
}


bool SpaceRecreator::writeTerrainBlock( DataSectionPtr spaceDS, int x, int z, Terrain::EditorTerrainBlock2Ptr pTerrainBlock )
{
	BW_GUARD;
	DataSectionPtr cdataDS = spaceDS->openSection( chunkName( x, z ) + ".cdata" );

	cdataDS->deleteSection( pTerrainBlock->dataSectionName() );
	BWResource::instance().purge( BWResource::dissolveFilename( spaceName_ ) + "/" + chunkName( x, z ) + ".cdata/" + pTerrainBlock->dataSectionName(), true );

	bool good = pTerrainBlock->saveToCData( cdataDS );
	return good && cdataDS->save();
}


void SpaceRecreator::blitHeights( Terrain::TerrainHeightMap::ImageType &heightMapImage,
	Terrain::EditorTerrainBlock2Ptr pNeighbourBlock,
	uint32 srcX, uint32 srcY, uint32 srcW, uint32 srcH, uint32 dstX, uint32 dstY )
{
	BW_GUARD;
	if (!pNeighbourBlock)
		return;

	Terrain::TerrainHeightMap &neighbourHeightMap = pNeighbourBlock->heightMap();
	Terrain::TerrainHeightMapHolder neighbourHeightMapHolder(&neighbourHeightMap, true);
	Terrain::TerrainHeightMap::ImageType &neighbourHeightMapImage = neighbourHeightMap.image();
	heightMapImage.blit( neighbourHeightMapImage, srcX, srcY, srcW, srcH, dstX, dstY );
}



bool SpaceRecreator::patchHeightMapEdges( DataSectionPtr spaceDS, int x, int z )
{
	BW_GUARD;

	bool good = true;

	//Chunks may have no terrain.
	Terrain::EditorTerrainBlock2Ptr pTerrainBlock = readTerrainBlock( spaceDS, x, z );
	if(pTerrainBlock == NULL)
	{
		return true;
	}

	{ // Scope for the Terrain::TerrainHeightMapHolder
		Terrain::TerrainHeightMap &terrainHeightMap = pTerrainBlock->heightMap();
		Terrain::TerrainHeightMapHolder terrainHeightMapHolder(&terrainHeightMap, false);
		Terrain::TerrainHeightMap::ImageType &heightMapImage = terrainHeightMap.image();

		// Theory for a 128-sized height map: (See TerrainUtils::patchEdges)
		// Top-left is blitting ( 127, 127 ) +2+2 => 0,0
		// Left is blitting ( 127, 2 ) +2+127 => 0, 2
		// Bottom-left is blitting ( 127, 2 ) +2+3 => 0, 129
		const int xOff = terrainHeightMap.xVisibleOffset(); // 2
		const int xSize = terrainHeightMap.blocksWidth(); // 127

		const int zOff = terrainHeightMap.zVisibleOffset(); // 2
		const int zSize = terrainHeightMap.blocksHeight(); // 127

		// src, width, dst
		// -1: Size, Off, 0
		// 0 : Off, Size, Off
		// +1: Off, Off+1, Size + Off
		// Fill the top-left area
		blitHeights( heightMapImage, readTerrainBlock( spaceDS, x-1, z-1 ), xSize, zSize, xOff, zOff, 0, 0 );
		// Fill the left area
		blitHeights( heightMapImage, readTerrainBlock( spaceDS, x-1, z ), xSize, zOff, xOff, zSize, 0, zOff );
		// Fill the bottom-left area
		blitHeights( heightMapImage, readTerrainBlock( spaceDS, x-1, z+1 ), xSize, zOff, xOff, zOff + 1, 0, zSize + zOff );

		// Fill the top area
		blitHeights( heightMapImage, readTerrainBlock( spaceDS, x, z-1 ), xOff, zSize, xSize, zOff, xOff, 0 );
		// Fill the bottom area
		blitHeights( heightMapImage, readTerrainBlock( spaceDS, x, z+1 ), xOff, zOff, xSize, zOff + 1, xOff, zSize + zOff );


		// Fill the top-right area
		blitHeights( heightMapImage, readTerrainBlock( spaceDS, x+1, z-1 ), xOff, zSize, xOff + 1, zOff, xSize + xOff, 0 );
		// Fill the right area
		blitHeights( heightMapImage, readTerrainBlock( spaceDS, x+1, z ), xOff, zOff, xOff + 1, zSize, xSize + xOff, zOff );
		// Fill the bottom-right area
		blitHeights( heightMapImage, readTerrainBlock( spaceDS, x+1, z+1 ), xOff, zOff, xOff + 1, zOff + 1, xSize + xOff, zSize + zOff );
		// This's abusing knowledge of TerrainUtils::patchEdges a little.
		// The only part of TerrainGetInfo patchEdges uses is the EditorBaseTerrainBlockPtr
		TerrainUtils::TerrainGetInfo info;
		info.block_ = pTerrainBlock;
		// TerrainUtils::patchEdges is assuming that the grid is centrered
		// The left edge should be x + xWidth/2 == 0
		// The right edge shoudl be x + xWidth/2 == xWidth - 1
		TerrainUtils::patchEdges( x, z, maxX_ - minX_ + 1, maxZ_ - minZ_ + 1, info, heightMapImage );
	}

	pTerrainBlock->rebuildNormalMap( Terrain::NMQ_NICE );

	Matrix chunkTransform;
	chunkTransform.setTranslate( x * chunkSize_, 0.f, z * chunkSize_ );
	good &= pTerrainBlock->rebuildLodTexture( chunkTransform );

	good &= writeTerrainBlock( spaceDS, x, z, pTerrainBlock );

	return good;

}


const BW::string SpaceRecreator::fixChunks( bool withPurge )
{
	BW_GUARD;

	BW::string result = "";

	ProgressBarTask progressTask( LocaliseUTF8( STAGES[6].name ), (float)(maxX_-minX_+1) * (maxZ_-minZ_+1), true );

	DataSectionPtr spaceDS = BWResource::openSection( BWResource::dissolveFilename( spaceName_ ) );

	// This one's a little nasty. Need to iterate all the chunks in the new space, and manually edit the
	// XML of each for anything that has a 'backlinks' child section.
	int start = GetTickCount();
		
	for (int x = minX_; x <= maxX_ && result.empty(); ++x)
	{
		int loop = GetTickCount();
			
		for (int z = minZ_; z <= maxZ_ && result.empty(); ++z)
		{
			const BW::string name = chunkName( x, z );
			DataSectionPtr chunkDS = spaceDS->openSection( name + ".chunk" );
			result += fixChunkLinksInChunk( chunkDS ) ? "" : ("Failed to update chunk links in " + name + ".chunk\n");
			if( shouldAbort() )
				return errorMessage_;
			result += patchHeightMapEdges( spaceDS, x, z ) ? "" : ("Failed to update chunk height map overlaps in " + name + ".chunk\n");
			progressTask.step();
			if( shouldAbort() )
				return errorMessage_;
		}

		// Purge after every column
		if (withPurge)
			BWResource::instance().purgeAll();
			
		int last = GetTickCount();

		float average = float( (last - start) / (x - minX_ + 1) );

		DEBUG_MSG( "fixChunks: Output row %d of %d, %dms, %2.0fms average\n", x-minX_+1, maxX_-minX_+1, last - loop,  average);
	}

	for( BW::set<BW::string>::const_iterator it = overlappers_.begin();
		it != overlappers_.end() && result.empty(); ++it )
	{
		const BW::string &name = *it;
		DataSectionPtr chunkDS = spaceDS->openSection( name + ".chunk" );
		result += fixChunkLinksInChunk( chunkDS ) ? "" : ("Failed to update chunk links in " + name + ".chunk\n");
	}
	
	// Remove links to items that were dropped due to a space reduction.
	for (auto itr = itemsWithPossibleBrokenLinks.begin();
			itr != itemsWithPossibleBrokenLinks.end(); ++itr)
	{
		ChunkItemPtr item = *itr;

		if (item->isEditorEntity())
		{
			EditorChunkEntity* entity =
					dynamic_cast<EditorChunkEntity*>( item.getObject() );
			MF_ASSERT( entity );

			EditorChunkItemLinkable* linker = entity->chunkItemLinker();
			linker->updateChunkLinks();
		}

		else if (item->isEditorUserDataObject())
		{
			EditorChunkUserDataObject* udo =
					dynamic_cast<EditorChunkUserDataObject*>( item.getObject() );
			MF_ASSERT( udo );

			EditorChunkItemLinkable* linker = udo->chunkItemLinker();
			linker->updateChunkLinks();
		}
	}
	
	return result;
}


SpaceRecreator::Impl::Impl( SpaceRecreator& parent )
	: parent_( parent )
{
}


FixChunkLinkResult SpaceRecreator::Impl::fixChunkLink(
	DataSectionPtr chunkLinkDS, bool knownToBeChunkLinks /*= false */)
{
	BW_GUARD;

	// A ChunkLink has two children, guid and chunkId.
	if (chunkLinkDS->countChildren() != 2)
	{
		return FixChunkLinkResult::Ignored;
	}

	DataSectionPtr guidDS = chunkLinkDS->openSection( "guid" );
	DataSectionPtr chunkIdDS = chunkLinkDS->openSection( "chunkId" );

	// We might be guessing that this is a ChunkLink
	MF_ASSERT( !knownToBeChunkLinks || (guidDS && chunkIdDS) );

	if (!guidDS || !chunkIdDS)
	{
		if (knownToBeChunkLinks)
		{
			ERROR_MSG( "fixChunkLink: Expected ChunkLink in %s\n",
					chunkLinkDS->sectionName().c_str() );
		}

		return FixChunkLinkResult::Ignored;
	}

	const BW::string guid = guidDS->asString();
	const BW::string chunkId = chunkIdDS->asString();
	MF_ASSERT( (guid == "") == (chunkId == "") );

	// We can't fix what isn't broken...
	if (chunkId == "")
	{
		return FixChunkLinkResult::Fixed;
	}

	// These have fallen away due to a space reduction.
	if (parent_.guidChunks_.count( guid ) == 0)
	{
		chunkLinkDS->writeString( "guid", "" );
		chunkLinkDS->writeString( "chunkId", "" );
		return FixChunkLinkResult::Deleted;
	}

	MF_ASSERT( parent_.guidChunks_.count( guid ) == 1 );
	const std::pair<BW::string, BW::string>& guidMove =
			parent_.guidChunks_[guid];
	MF_ASSERT( guidMove.first == chunkId )

	if(guidMove.first != guidMove.second)
	{
		chunkLinkDS->writeString( "chunkId", guidMove.second );
	}

	return FixChunkLinkResult::Fixed;
}


BW_END_NAMESPACE
