#include "pch.hpp"
#include "terrain_lod_controller.hpp"

#include "cstdmf/profiler.hpp"
#include "cstdmf/watcher.hpp"
#include "../manager.hpp"
#include "../terrain_data.hpp"
#include "../terrain_settings.hpp"
#include "terrain_block2.hpp"


DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

using namespace Terrain;


#if 0 // ununsed?
TerrainLodController::TerrainLodController() : 
	cameraPosition_( 0, 0, 0 ),
	blockFocusX_( 0 ),
	blockFocusZ_( 0 ),
	initialised_( false ),
	blockWindowX_( 0 ),
	blockWindowZ_( 0 ),
	windowDims_( 30 )
{
	activeBlocks_.resize( windowDims_ * windowDims_, NULL );
	blockWindowX_ = blockFocusX_ - (windowDims_ / 2);
	blockWindowZ_ = blockFocusZ_ - (windowDims_ / 2);
}

TerrainLodController::~TerrainLodController()
{

}

namespace
{
	// This function calculates an index into the activeBlocks
	// with dimensions dims
	uint32 blockIndex( int32 x, int32 z, int32 dims )
	{
		// Find the x and z gridpositions
		int32 xGridPos = x % dims;
		if (xGridPos < 0) xGridPos += dims;
		int32 zGridPos = z % dims;
		if (zGridPos < 0) zGridPos += dims;

		// index into our block grid
		uint32 index = xGridPos + zGridPos * dims;

		return index;
	}
};

/**
 *	This method sets the camera postition as it relates to lod.
 *	(this does not necessarily have to be the camera position
 *	for every render call, i.e. for rendering the water reflections
 *	we would want to still use the same camera position as the main
 *	view.
 */
void TerrainLodController::cameraPosition( const Vector3& value )
{
	BW_GUARD;
	cameraPosition_ = value;

	int32 focusX = int32(floorf(cameraPosition_.x / BLOCK_SIZE_METRES));
	int32 focusZ = int32(floorf(cameraPosition_.z / BLOCK_SIZE_METRES));

	// if our focus block has changed, update our active window
	if (focusX != blockFocusX_ || 
		focusZ != blockFocusZ_ ||
		!initialised_)
	{
		int32 diffZ = focusZ - blockFocusZ_;
		int32 diffX = focusX - blockFocusX_;

		// calculate our new window corner on the x and z axis
		int32 bwx = focusX - (windowDims_ / 2);
		int32 bwz = focusZ - (windowDims_ / 2);

		// If the entire window has changed or the lod controller has not been
		// initialised we need to update the entire window
		if (abs(diffZ) > windowDims_ ||
			abs(diffX) > windowDims_ ||
			!initialised_)
		{
			// If we are initialised already, clear the active blocks and
			// place them in the homeless list
			if (initialised_)
			{
				for (int32 z = 0; z < windowDims_; z++)
				{
					int32 realZ = z + blockWindowZ_;
					for (int32 x = 0; x < windowDims_; x++)
					{
						// get the block at the current x/z position
						int32 realX = x + blockWindowX_;
						uint32 index = blockIndex( realX, realZ, windowDims_ );
						TerrainBlock2* pBlock =  activeBlocks_[index];
						activeBlocks_[index] = NULL;

						// If there was an actual block at the position
						if (pBlock)
						{
							homeless_[LocationToken( realX, realZ )] = pBlock;
						}
					}
				}
			}

			// Attempt to update all the elements in the active block window
			for (int32 z = 0; z < windowDims_; z++)
			{
				int realZ = z + bwz;
				for (int32 x = 0; x < windowDims_; x++)
				{
					// see if we have the block at the current position in
					// the homeless map
					int realX = x + bwx;
					BlockMap::iterator it = 
						homeless_.find( LocationToken(realX, realZ) );

					// if the block has been added, set it as one of
					// the active blocks
					if (it != homeless_.end())
					{
						uint32 index = blockIndex( realX, realZ, windowDims_);
						activeBlocks_[index] = it->second;
						homeless_.erase( it );
					}
				}
			}
			initialised_ = true;
		}
		else
		{
			// Calculate the offset on the x and z axis of the new
			// grid position relative to the old one
			int32 xSense = diffX < 0 ? -windowDims_ : windowDims_;
			int32 zSense = diffZ < 0 ? -windowDims_ : windowDims_;

			// If both are different, we have a corner rectangle that is
			// offset in both directions
			if (diffZ != 0 && diffX != 0)
			{
				// Get the start of the window in the x direction
				int32 startX = 0;
				if (diffX < 0)
				{
					startX = bwx;
				}
				else
				{
					startX = blockWindowX_ + windowDims_;
				}

				// Get the start of the window in the z direction
				int32 startZ = 0;
				if (diffZ < 0)
				{
					startZ = bwz;
				}
				else
				{
					startZ = blockWindowZ_ + windowDims_;
				}

				// Iterate over the rectangular part of the window
				// and move blocks that are there to the homeless list
				for (int32 z = startZ; z < (startZ + abs(diffZ)); z++)
				{
					for (int32 x = startX; x < (startX + abs(diffX)); x++)
					{
						// get the index of the current block
						uint32 index = blockIndex( x, z, windowDims_);

						// If we have an old block, add it to the homeless list
						TerrainBlock2* pOld = activeBlocks_[index];
						if (pOld)
						{
							homeless_[LocationToken( x - xSense, z - zSense )] = pOld;
						}
						// If we have the needed block in the homeless list, add it to
						// the active blocks
						BlockMap::iterator it = homeless_.find(LocationToken(x, z));
						if (it != homeless_.end())
						{
							activeBlocks_[index] = it->second;
							homeless_.erase( it );
						}
						else
						{
							activeBlocks_[index] = NULL;
						}
					}
				}
			}

			// If the window movement is on the z axis update the blocks
			// that have changed
			if (diffZ != 0)
			{
				// Make sure that we only update portions of the window that have not
				// already been updated in the rectangular section
				int32 startX = max( bwx, blockWindowX_ );
				int32 endX = min( bwx + windowDims_, blockWindowZ_ + windowDims_ );

				// Get the top of the updated portion on the z axis.
				int32 startZ = 0;
				if (diffZ < 0)
				{
					startZ = bwz;
				}
				else
				{
					startZ = blockWindowZ_ + windowDims_;
				}

				// Iterate over the changed blocks that have only got z movement
				for (int32 z = startZ; z < (startZ + abs(diffZ)); z++)
				{
					for (int32 x = startX; x < endX; x++)
					{
						uint32 index = blockIndex( x, z, windowDims_);
						TerrainBlock2* pOld = activeBlocks_[index];
						if (pOld)
						{
							homeless_[LocationToken( x, z - zSense )] = pOld;
						}
						BlockMap::iterator it = homeless_.find(LocationToken(x, z));
						if (it != homeless_.end())
						{
							activeBlocks_[index] = it->second;
							homeless_.erase( it );
						}
						else
						{
							activeBlocks_[index] = NULL;
						}
					}
				}
			}

			// If the window movement is on the x axis update the blocks
			// that have changed
			if (diffX != 0)
			{
				// Make sure that we only update portions of the window that have not
				// already been updated in the rectangular section
				int32 startZ = max( bwz, blockWindowZ_ );
				int32 endZ = min( bwz + windowDims_, blockWindowZ_ + windowDims_ );

				// Get the side of the updated portion on the x axis.
				int32 startX = 0;
				if (diffX < 0)
				{
					startX = bwx;
				}
				else
				{
					startX = blockWindowX_ + windowDims_;
				}

				// Iterate over the changed blocks that have only got z movement
				for (int32 z = startZ; z < endZ; z++)
				{
					for (int32 x = startX; x < (startX + abs(diffX)); x++)
					{
						uint32 index = blockIndex( x, z, windowDims_);
						TerrainBlock2* pOld = activeBlocks_[index];
						if (pOld)
						{
							homeless_[LocationToken( x - xSense, z )] = pOld;
						}
						BlockMap::iterator it = homeless_.find(LocationToken(x, z));
						if (it != homeless_.end())
						{
							activeBlocks_[index] = it->second;
							homeless_.erase( it );
						}
					}
				}
			}
		}

		blockFocusX_ = focusX;
		blockFocusZ_ = focusZ;
		blockWindowX_ = focusX - (windowDims_ / 2);
		blockWindowZ_ = focusZ - (windowDims_ / 2);
	}
}

/**
 *	This method adds a block to the lod controller
 *	@param pBlock the terrain block to add
 *	@param centre the centre of the block to add
 */
void TerrainLodController::addBlock( TerrainBlock2* pBlock, 
									const Vector3& centre )
{
	BW_GUARD;
	int32 x = int32(floorf(centre.x / pBlock->blockSize()));
	int32 z = int32(floorf(centre.z / pBlock->blockSize()));

	int32 xLocal = x - blockWindowX_;
	int32 zLocal = z - blockWindowZ_;

	// Check if the position of the block is in the current 
	// local grid space
	if (xLocal >= 0 && xLocal < windowDims_ &&
		xLocal >= 0 && xLocal < windowDims_ && 
		initialised_)
	{
		uint32 index = blockIndex( x, z, windowDims_ );

		MF_ASSERT(index  < activeBlocks_.size());
		MF_ASSERT(activeBlocks_[index] == NULL);

		// set the block on the grid
		activeBlocks_[index] = pBlock;
	}
	else
	{
		// We are a homeless block, store it in the homeless list
		homeless_[LocationToken(x,z)] = pBlock;
	}
}
/*
 *	This is a helper class that aids in searching for a block in the block map
 */
class BlockFinder
{
public:
	BlockFinder( TerrainBlock2* pBlock )
	: pBlock_( pBlock )
	{
	}
	template <class Item>
	bool operator()( const Item& item )
	{
		return item.second == pBlock_;
	}
private:
	TerrainBlock2* pBlock_;
};

/**
 *	This method removes a block from the lod controller
 *	@param pBlock the terrain block to remove
 */
void TerrainLodController::delBlock( TerrainBlock2* pBlock )
{
	BW_GUARD_PROFILER( TerrainLodController_delBlock );
	// Look for the block in the homeless list
	BlockMap::iterator it = 
		std::find_if( homeless_.begin(), homeless_.end(), 
		BlockFinder( pBlock ) );

	// If the block is in the homeless list, delete it from the list
	if (it != homeless_.end())
	{
		homeless_.erase( it );
	}
	else
	{
		// Search for the block in the active list
		// if it's there set the active list entry to NULL
		Blocks::iterator bit =
			std::find( activeBlocks_.begin(), activeBlocks_.end(),
				pBlock );
		if (bit != activeBlocks_.end())
		{
			*bit = NULL;
		}
	}
}
#endif

// BasicTerrainLodController

BasicTerrainLodController::BasicTerrainLodController() 
	: closestUnstreamedBlock_( FLT_MAX )
{
	BW_GUARD;
	blocks_.reserve( 1024 );

	MF_WATCH( "Render/Terrain/Terrain2/numBlocksLoaded", *this,
		&BasicTerrainLodController::getNumBlocks,
		"The total number of terrain blocks loaded." );
}

BasicTerrainLodController::~BasicTerrainLodController() 
{
#if ENABLE_WATCHERS
	if (Watcher::hasRootWatcher())
	{
		Watcher::rootWatcher().removeChild("Render/Terrain/Terrain2/numBlocksLoaded");
	}
#endif
}

void BasicTerrainLodController::setCameraPosition(	const Vector3& camPosition, float zoomFactor )
{
	BW_GUARD_PROFILER( BasicTerrainLodController_setCameraPosition );
	// Wait till we've added a block to our list, this can happen in the loading
	// thread.
	SimpleMutexHolder smh( accessMutex_ ); 

	//-- change zoom factor to do correct lod selection in case if we change FOV angle of the camera.
	TerrainSettings::changeZoomFactor(zoomFactor);
	TerrainVertexLod::changeZoomFactor(zoomFactor);

	Vector3 camTemp;

	closestUnstreamedBlock_ = FLT_MAX;

	const size_t numBlocks = blocks_.size();
	for ( size_t i = 0; i < numBlocks; i++ )
	{
		blocks_[i].first.applyPoint( camTemp, camPosition );
		blocks_[i].second->evaluate( camTemp );
		blocks_[i].second->stream();

		if (!blocks_[i].second->readyToDraw())
		{
			camTemp.y = 0;

			float dist = camTemp.length();
			if (dist < closestUnstreamedBlock_)
			{
				closestUnstreamedBlock_ = dist;
			}
		}
	}
}

void BasicTerrainLodController::addBlock(	TerrainBlock2* pBlock, 
											const Matrix& worldTransform  )
{
	BW_GUARD;
	// Force an atomic add to list, in case we get halfway through and then
	// try to update an invalid entry.
	SimpleMutexHolder smh( accessMutex_ );

	Matrix invWorld;
	invWorld.invert(worldTransform);

	blocks_.push_back( std::make_pair( invWorld, pBlock ) );
}

bool BasicTerrainLodController::delBlock( TerrainBlock2* pBlock )
{
	BW_GUARD;
	// Force an atomic remove from list, in case we get halfway through and then
	// try to update an invalid entry.
	SimpleMutexHolder smh( accessMutex_ );

	for ( BlockContainer::iterator bit = blocks_.begin();
			bit != blocks_.end(); bit++ )
	{
		if ( bit->second == pBlock )
		{
			blocks_.erase( bit );
			return true;
		}
	}

	return false;
}


BasicTerrainLodController& BasicTerrainLodController::instance()
{
	return Manager::instance().lodController();
}

BW_END_NAMESPACE

// terrain_lod_controller.cpp
