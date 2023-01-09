#include "pch.hpp"
#include "worldeditor/project/chunk_walker.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/project/space_map.hpp"
#include "worldeditor/project/space_map_debug.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_space.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/binary_block.hpp"

BW_BEGIN_NAMESPACE

LinearChunkWalker::LinearChunkWalker():	
	x_( 0 ),
	z_( 0 )	
{
}


void LinearChunkWalker::spaceInformation( const SpaceInformation& info )
{
	BW_GUARD;

	//reset the chunk walker.
	info_ = info;	
	this->reset();
}


void LinearChunkWalker::reset()
{
	x_ = z_ = 0;
}


bool LinearChunkWalker::nextTile( BW::string& retChunkName, int16& gridX, int16& gridZ )
{
	BW_GUARD;

	if ( z_ == info_.gridHeight_ )
		return false;

	offsetGrid( info_.localToWorld_, x_, z_, gridX, gridZ );
	chunkID( retChunkName, gridX, gridZ );

	x_++;
	if ( x_ == info_.gridWidth_ )
	{
		x_ = 0;
		z_++;
	}

	return true;
}

void LinearChunkWalker::load( const DataSectionPtr& pLocalSettings )
{
	BW_GUARD;

	if ( pLocalSettings )
	{
		x_ = pLocalSettings->readInt( "nextGridX", 0 );
		z_ = pLocalSettings->readInt( "nextGridZ", 0 );
	}
	else
	{
		this->reset();
	}
}


void LinearChunkWalker::save( const DataSectionPtr& pLocalSettings )
{
	BW_GUARD;

	if (pLocalSettings)
	{
		pLocalSettings->writeInt( "nextGridX", x_ );
		pLocalSettings->writeInt( "nextGridZ", z_ );
	}
}

/**
  * Constructs a ChunkWalker that iterates over the chunks between 
  * [left, bottom] -> (right, top). 
  * That is (0,0) -> (2,2) will examine (0,0) (1,0) (0,1) (1,1) 
  * Note that space information must be passed in before this iterator is valid
  * for use.
  * @param left the leftmost grid coordinate
  * @param bottom the lowest grid coordinate
  * @param right The rightmost column of the bounding area (not inclusive)
  * @param top The top row of the bounding area (not inclusive)  
  */
BoundedChunkWalker::BoundedChunkWalker( int16 left, int16 bottom, int16 right, int16 top ):	
	x_( left ),
	z_( bottom ),
	left_( left ), 
	bottom_( bottom ),
	right_( right ),
	top_( top ),
	isSet_( false )
{
	MF_ASSERT( right_ >= left_ );
	MF_ASSERT( top_ >= bottom_ );
}

/* Pass in the space information */
void BoundedChunkWalker::spaceInformation( const SpaceInformation& info )
{
	BW_GUARD;

	//reset the chunk walker.
	info_ = info;	
	this->reset();
	isSet_ = true;

}


void BoundedChunkWalker::reset()
{
	if (left_ >= info_.gridWidth_ || 
		right_ <= 0 ||
		bottom_ >= info_.gridHeight_ ||
		top_ <= 0 )
	{
		// If the walk range doesn't intersect with the space bound,
		// we just fail it so no chunk returned.
		this->stop();
		return;
	}
	left_ = std::max <int16> ( left_,0 );
	bottom_ = std::max <int16> ( bottom_,0 );
	right_ = std::min <int16> ( right_, info_.gridWidth_ );
	top_ = std::min <int16> ( top_, info_.gridHeight_ );
	x_ = left_;
	z_ = bottom_;
}

/**
  * stop further walk
  */
void BoundedChunkWalker::stop()
{
	z_ = top_;
}

bool BoundedChunkWalker::nextTile( BW::string& retChunkName, int16& gridX, int16& gridZ )
{
	BW_GUARD;

	MF_ASSERT( isSet_ );
	if (z_ == top_)
	{
		return false;
	}

	offsetGrid( info_.localToWorld_, x_, z_, gridX, gridZ );
	chunkID( retChunkName, gridX, gridZ );

	x_++;
	if (x_ == right_)
	{
		x_ = left_;
		z_++;
	}
	return true;
}


CacheChunkWalker::CacheChunkWalker()
{
}


void CacheChunkWalker::spaceInformation( const SpaceInformation& info )
{
	info_ = info;
}



bool CacheChunkWalker::nextTile( BW::string& chunkName, int16& gridX, int16& gridZ )
{
	BW_GUARD;

	if (xs_.size()==0)
		return false;

	int i = static_cast<int>(xs_.size() - 1);

	gridX = xs_[i];
	gridZ = zs_[i];
	chunkID( chunkName, gridX, gridZ );
	
	// This will happen if we have cached chunk coordinates
	// which don't exist in the current geometry mapping. Have you
	// leaked cached chunks across changing space?
	MF_ASSERT( !chunkName.empty() );
	
	xs_.pop_back();
	zs_.pop_back();

	if (chunkName.empty())
	{
		return false;
	}

	return true;
}


void CacheChunkWalker::add( Chunk* pChunk )
{
	BW_GUARD;

	int16 gridX, gridZ;
	WorldManager::instance().geometryMapping()->gridFromChunkName( pChunk->identifier(), gridX, gridZ );
	this->add( gridX, gridZ );
}


void CacheChunkWalker::add( int16& gridX, int16& gridZ )
{
	BW_GUARD;

	xs_.push_back( gridX );
	zs_.push_back( gridZ );
}


bool CacheChunkWalker::added( const Chunk* pChunk ) const
{
	BW_GUARD;

	int16 gridX, gridZ;
	WorldManager::instance().geometryMapping()->gridFromChunkName( pChunk->identifier(), gridX, gridZ );
    return added(gridX, gridZ );
}


bool CacheChunkWalker::added( int16 gridX, int16 gridZ ) const
{
	BW_GUARD;

    for (size_t i = 0; i < xs_.size(); ++i)
    {
        if ((int16)xs_[i] == gridX && (int16)zs_[i] == gridZ)
            return true;
    }
    return false;
}


bool CacheChunkWalker::erase( const Chunk* pChunk )
{
	BW_GUARD;

	int16 gridX, gridZ;
	WorldManager::instance().geometryMapping()->gridFromChunkName( pChunk->identifier(), gridX, gridZ );
    return erase( gridX, gridZ );
}


bool CacheChunkWalker::erase( int16 gridX, int16 gridZ )
{
	BW_GUARD;

    for (size_t i = 0; i < xs_.size(); ++i)
    {
        if ((int16)xs_[i] == gridX && (int16)zs_[i] == gridZ)
        {
            xs_.erase(xs_.begin() + i);
            zs_.erase(zs_.begin() + i);
            return true;
        }
    }
    return false;
}


bool inRange( uint16 tileNum, uint16 minRange, uint16 maxRange )
{
	return ( min( max(tileNum,minRange), maxRange ) == tileNum );
}


void writeRange( DataSectionPtr pSection, uint16 minRange, uint16 maxRange )
{
	BW_GUARD;

	if ( minRange > maxRange )
		return;

	DataSectionPtr pSect = pSection->newSection( "tile" );
	if ( minRange == maxRange )
	{
		pSect->writeInt( "min", minRange );
	}
	else
	{
		pSect->writeInt( "min", minRange );
		pSect->writeInt( "max", maxRange );
	}
}


/**
 *	This method writes out a list of all the tiles that we know about.
 *	It converts the grid X,Z to a single tile number and saves them out.
 *	It also concatenates all adjacencies (RLE) because sometimes the
 *	number of entries can be very large ( e.g. 70km space ~ 1/2 million thumbnails )	
 */
void CacheChunkWalker::save( DataSectionPtr pSection )
{
	BW_GUARD;

	BW::vector<uint16> tileNumbers;
	
	size_t num = xs_.size();
	for ( size_t i=0; i<num; i++ )
	{
		uint16 biasedX, biasedZ;
		int16 x = xs_[i];
		int16 z = zs_[i];
		biasGrid( info_.localToWorld_, x, z, biasedX, biasedZ );
		uint16 tileNum = biasedX + biasedZ*info_.gridWidth_;
		tileNumbers.push_back(tileNum);
	}

	if ( num>0 )
		this->save( pSection, tileNumbers );
}


void CacheChunkWalker::save( DataSectionPtr pSection, BW::vector<uint16>& modifiedTileNumbers )
{
	BW_GUARD;

	//to make the most of our span range algortithm, we sort the tiles.
	std::sort( modifiedTileNumbers.begin(), modifiedTileNumbers.end() );

	pSection->writeInt( "minX", info_.localToWorld_.x );
	pSection->writeInt( "minZ", info_.localToWorld_.y );
	pSection->writeInt( "width", info_.gridWidth_ );
	pSection->writeInt( "height", info_.gridHeight_ );

	uint16 minRange = 0xffff;
	uint16 maxRange = 0;

	size_t num = modifiedTileNumbers.size();
	for ( size_t i=0; i<num; i++ )
	{
		uint16 tileNum = modifiedTileNumbers[i];

		//allow new tiles to be in range, or adjacent to it.
		uint16 adjMin = (minRange>0) ? minRange-1 : (uint16)0;
		if ( inRange(tileNum,adjMin,maxRange+1) )
		{
			minRange = min(tileNum,minRange);
			maxRange = max(tileNum,maxRange);
			continue;
		}

		//new tile number doesn't fit in range.
		//write out the previous range.
		//note the writeRange method rejects invalid ranges,
		//which will always happen for the first tile.
		writeRange( pSection, minRange, maxRange );
		
		//...and begin the new one
		minRange = tileNum;
		maxRange = tileNum;
	}

	//write out the last one.
	writeRange( pSection, minRange, maxRange );
}


void CacheChunkWalker::load( DataSectionPtr pSection )
{
	BW_GUARD;

	int16 offsetX;
	int16 offsetZ;
	uint16 minRange,maxRange;

	BW::string chunkName;
	info_.localToWorld_.x = pSection->readInt( "minX", info_.localToWorld_.x );
	info_.localToWorld_.y = pSection->readInt( "minZ", info_.localToWorld_.y );
	info_.gridWidth_ = pSection->readInt( "width", info_.gridWidth_ );
	info_.gridHeight_ = pSection->readInt( "height", info_.gridHeight_ );

	int num = pSection->countChildren();
	for ( int i=0; i<num; i++ )
	{
		DataSectionPtr pSect = pSection->openChild(i);
		
		if ( pSect->findChild("min" ) )
		{
			minRange = pSect->readInt("min");
			maxRange = pSect->readInt("max",minRange);
		
			while ( minRange <= maxRange )
			{
				uint16 x = (minRange%info_.gridWidth_);
				uint16 z = (minRange/info_.gridWidth_);
				offsetGrid(info_.localToWorld_,x,z,offsetX,offsetZ);
				xs_.push_back(offsetX);
				zs_.push_back(offsetZ);			
				minRange++;
			} 
		}
	}
}


size_t CacheChunkWalker::size() const
{
    return xs_.size();
}

void CacheChunkWalker::reset()
{
	xs_.clear();
	zs_.clear();
}


/**
 *	Section - FileChunkWalker
 */
FileChunkWalker::FileChunkWalker( uint16 numIrrelevantPerFind ):
	fileHandle_( INVALID_HANDLE_VALUE ),
	folder_( "" ),
	root_( "" ),	
	numIrrelevantPerFind_( numIrrelevantPerFind ),
	reentryFolder_( "" )
{
}


FileChunkWalker::~FileChunkWalker()
{
	BW_GUARD;

	releaseFileHandle();
}


void FileChunkWalker::spaceInformation( const SpaceInformation& info )
{
	BW_GUARD;

	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	space_ = dirMap->path();
	root_ = BWResource::resolveFilename( space_ );
	folder_ = "";
	info_ = info;
	this->reset();
}


void FileChunkWalker::reset()
{
	BW_GUARD;

	this->releaseFileHandle();
	folders_.clear();
	folder_ = "";
	xs_.clear();
	zs_.clear();
}


bool FileChunkWalker::nextTile( BW::string& retChunkName, int16& gridX, int16& gridZ )
{
	BW_GUARD;

	if ( reentryFolder_ != "" )
	{
		this->findReentryFolder();
	}

	if ( !xs_.size() )
	{
		this->findNextFile();
	}

	if ( xs_.size() )
	{
		gridX = xs_.back();
		gridZ = zs_.back();
		xs_.pop_back();
		zs_.pop_back();
		chunkID( retChunkName, gridX, gridZ );
		return true;
	}

	return false;
}


void FileChunkWalker::findNextFile()
{
	BW_GUARD;

	for ( int i=0; i<numIrrelevantPerFind_; i++ )
	{
		this->advanceFileHandle();
		
		if (fileHandle_ != INVALID_HANDLE_VALUE)
		{
			if ( searchingFiles_ )
			{
				if ( this->isRelevant( findFileData_ ) )
				{
					int16 x,z;
					BW::string fileName;

					bw_wtoacp( findFileData_.cFileName, fileName );

					if (gridFromThumbnailFileName(fileName.c_str(),x,z))
					{
						xs_.push_back( x );
						zs_.push_back( z );
					}
					return;
				}
			}
			else
			{
				if ( isValidFolder( findFileData_ ) )
				{
					BW::string nfileName;
					bw_wtoutf8( findFileData_.cFileName, nfileName );
					folders_.push_back( folder_ + nfileName + "/sep/" );
				}
				return;
			}
		}
	}
}


void FileChunkWalker::releaseFileHandle()
{
	BW_GUARD;

	if ( fileHandle_ != INVALID_HANDLE_VALUE )
		FindClose(fileHandle_);
	fileHandle_ = INVALID_HANDLE_VALUE;
}


/**
 *	This special case method is used to kick-start
 *	the chunk walking process off from a given folder.
 *
 *	This allows the file scanning process to be
 *	interruptible, without favouring files towards
 *	the start of the default search.
 */
void FileChunkWalker::findReentryFolder()
{
	BW_GUARD;

	this->releaseFileHandle();

	if ( reentryFolder_ == "" )
		return;

	this->findFolder( reentryFolder_ );

	reentryFolder_ = "";
}


//returns true if the two folders match to as many partial paths as exist
//
//e.g.	"n1/n2" and "n1/n2/n3/n4" returns true
//		"n1/n2" and "n1/n3/n2/n4" returns false
bool FileChunkWalker::partialFolderMatch( const BW::string& folder1, const BW::string& folder2 )
{
	BW_GUARD;

	return ( folder1 == folder2.substr(0,folder1.size()) );
}


bool FileChunkWalker::findFolder( const BW::string& target )
{
	BW_GUARD;

	this->findFolders( "" );

	//Now search the folders just as we would searching for thumbnails.
	while ( folders_.size() )
	{
		BW::string folderName = folders_.back();
		folders_.pop_back();

		if ( folderName == target )
		{
			folders_.push_back( folderName );
			return true;
		}

		if ( partialFolderMatch( folderName, target ) )
			this->findFolders( folderName );
	}

	//Bad luck.  No folders in the current folder match our criteria.
	return false;
}


//This method advances the search 
void FileChunkWalker::advanceFileHandle()
{
	BW_GUARD;

	//If this is a new search, or we have no more files,
	//advance the file handle to the next folder.
	if (fileHandle_ == INVALID_HANDLE_VALUE )
	{
		if ( folders_.size() )
		{
			folder_ = folders_.back();
			folders_.pop_back();
		}
		else
		{
			folder_ = "";
		}

		searchingFiles_ = true;
		BW::wstring wfullName;
		bw_utf8tow( root_ + folder_ + "*.cdata", wfullName );
		fileHandle_ = FindFirstFile( wfullName.c_str(), &findFileData_ );
		return;
	}

	//Next!
	if ( FindNextFile(fileHandle_,&findFileData_) )
	{
		//found another one ( file or folder ).
		return;
	}
	
	//No more found.
	//If we were doing a file search, begin a folder search.
	this->releaseFileHandle();
	if ( searchingFiles_ )
	{
		searchingFiles_ = false;
		BW::wstring wfullName;
		bw_utf8tow( root_ + folder_ + "*.", wfullName );
		fileHandle_ = FindFirstFile( wfullName.c_str(), &findFileData_ );
	}
}


bool FileChunkWalker::isValidFolder( const WIN32_FIND_DATA& fileInfo )
{
	BW_GUARD;

	if ( findFileData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{
		//don't allow "." and ".." folders.  that be bad.  also don't bother looking in the "CVS" folder
		if ( !findFileData_.cFileName[0] )
			return false;
		if ( !findFileData_.cFileName[1] )
			return false;
		if ( !findFileData_.cFileName[2] )
			return false;
		if ( findFileData_.cFileName[3]  )
		{
			return true;
		}

		//3 letter folder name : could be "CVS" or "sep".
		if ( findFileData_.cFileName[0] == 's' )
			return true;

		//first letter of 3 letter folder name is not "s" - assume it is "CVS"
		return false;
	}
	return false;
}


/**
 *	This method adds all of the folders contained in the given
 *	directory to the folders_ vector.
 */
void FileChunkWalker::findFolders( const BW::string& folder )
{
	BW_GUARD;

	//Now add all sub-folders to our list of folders to search.
	BW::wstring wfullName;
	bw_utf8tow( root_ + folder + "*.", wfullName );
	fileHandle_ = FindFirstFile( wfullName.c_str(), &findFileData_ );
	while (fileHandle_ != INVALID_HANDLE_VALUE)
	{
		if ( isValidFolder( findFileData_ ) )
		{
			BW::string nfilename;
			bw_wtoutf8( findFileData_.cFileName, nfilename );
			folders_.push_back( folder + nfilename + "/" );
		}
		
		if ( !FindNextFile(fileHandle_,&findFileData_) )
		{
			this->releaseFileHandle();
			break;
		}
	}
}


void FileChunkWalker::load( const DataSectionPtr& pLocalSettings )
{
	BW_GUARD;

	if ( pLocalSettings )
	{
		reentryFolder_ = pLocalSettings->readString( "reentryFolder", "" );
	}
	else
	{
		reentryFolder_ = "";
	}
}


void FileChunkWalker::save( const DataSectionPtr& pLocalSettings )
{
	BW_GUARD;

	if (pLocalSettings)
	{
		pLocalSettings->writeString( "reentryFolder", folder_ );
	}
}



/**
 *	Section - ModifiedFileChunkWalker
 */
ModifiedFileChunkWalker::ModifiedFileChunkWalker( uint16 numIrrelevantPerFind ):
FileChunkWalker( numIrrelevantPerFind )
{
}


ModifiedFileChunkWalker::~ModifiedFileChunkWalker()
{
}


void ModifiedFileChunkWalker::spaceInformation( const SpaceInformation& info )
{	
	BW_GUARD;

	FileChunkWalker::spaceInformation( info );
}


bool ModifiedFileChunkWalker::isRelevant( const WIN32_FIND_DATA& fileInfo )
{
	BW_GUARD;

	int16 gridX, gridZ;
	BW::string fileName;

	bw_wtoacp( fileInfo.cFileName, fileName );

	if (gridFromThumbnailFileName(fileName.c_str(),gridX,gridZ))
	{
		BW::string pathName = WorldManager::instance().geometryMapping()->path();
		BW::string chunkName;
		chunkID( chunkName, gridX, gridZ );

		if (gridX >= info_.localToWorld_.x &&
			gridZ >= info_.localToWorld_.y &&
			gridX < info_.localToWorld_.x + info_.gridWidth_ &&
			gridZ < info_.localToWorld_.y + info_.gridHeight_)
		{
			if( thumbnailExists( pathName, chunkName ) )
			{			
				uint64 fileTime =
					SpaceMap::instance().timestampCache().getUint64(fileInfo.ftLastWriteTime);

				uint64 cacheTime =
					SpaceMap::instance().timestampCache().cacheTime(gridX, gridZ);

				if ( cacheTime < fileTime )
				{				
					SpaceMapDebug::instance().onConsidered(gridX,gridZ,0x00ff0000);
					return true;
				}
				else
				{
					//Green - thumbnail is ok
					SpaceMapDebug::instance().onConsidered(gridX,gridZ,0x0000ff00);
				}
			}
			else
			{
				//Black - thumbnail does not exist on disk!
				SpaceMapDebug::instance().onConsidered(gridX,gridZ,0x00000000);
			}
		}
	}

	return false;
}
BW_END_NAMESPACE
