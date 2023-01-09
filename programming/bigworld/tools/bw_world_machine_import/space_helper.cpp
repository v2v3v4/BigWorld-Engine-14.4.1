#include "pch.hpp"

#include "space_helper.hpp"

#include "chunk/chunk_grid_size.hpp"
#include "chunk/chunk_format.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/zip_section.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor
 */
SpaceHelper::SpaceHelper() :
xMin_( 0 ),
xMax_( 0 ),
zMin_( 0 ),
zMax_( 0 ),
chunkSize_( DEFAULT_GRID_RESOLUTION ),
singleDir_( false )
{
};


/**
 *	This method initialises the SpaceHelper class from the space.settings file
 *	@param spaceSettingsSpath the full path including the filename of the space.settings file
 *	@return true if the space.settings file is successfully loaded
 */
bool SpaceHelper::init( const BW::string& spaceSettingsPath )
{
	DataSectionPtr pSpaceSettings = BW::XMLSection::createFromFile( spaceSettingsPath.c_str() );

	spaceRoot_.clear();

	pFileSystem_ = NULL;

	if (pSpaceSettings)
	{
		xMin_ = pSpaceSettings->readInt( "bounds/minX" );
		xMax_ = pSpaceSettings->readInt( "bounds/maxX" );
		zMin_ = pSpaceSettings->readInt( "bounds/minY" );
		zMax_ = pSpaceSettings->readInt( "bounds/maxY" );

		chunkSize_ = pSpaceSettings->readFloat( "chunkSize", DEFAULT_GRID_RESOLUTION );

		singleDir_ = pSpaceSettings->readBool( "singleDir", false );

		spaceRoot_ = BW::BWResource::getFilePath( spaceSettingsPath );

		pFileSystem_ = BW::NativeFileSystem::create( spaceRoot_ );
	}

	return pSpaceSettings.exists();
}


/**
 *	This method opens the cdata for an  outside chunk in the current space
 *	@param x the x coording for the chunk to retrieve
 *	@param z the z coording for the chunk to retrieve
 *	@return DataSectionPtr to the cdata
 */
BW::DataSectionPtr SpaceHelper::getCDataForChunk( int x, int z )
{
	BW::string cDataName;

	if (x >= xMin_ &&
		x <= xMax_ &&
		z >= zMin_ &&
		z <= zMax_)
	{
		cDataName =  BW::ChunkFormat::outsideChunkIdentifier(x, z, singleDir_ ) +
			BW::string( ".cdata");
	}

	if (!cDataName.empty() &&
		pFileSystem_.exists() &&
		pFileSystem_->getFileTypeEx( cDataName ) == BW::IFileSystem::FT_ARCHIVE)
	{
		return new BW::ZipSection( cDataName, pFileSystem_ );
	}

	return NULL;
}


BW_END_NAMESPACE
