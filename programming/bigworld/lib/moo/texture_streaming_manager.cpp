#include "pch.hpp"

#include "texture_streaming_manager.hpp"
#include "texture_streaming_scene_view.hpp"
#include "texture_usage.hpp"
#include "managed_texture.hpp"
#include "image_texture.hpp"
#include "streaming_texture.hpp"
#include "render_target.hpp"
#include "resmgr/file_streamer.hpp"
#include "resmgr/file_system.hpp"
#include "resmgr/multi_file_system.hpp"
#include "scene/scene.hpp"
#include "space/space_manager.hpp"
#include "space/client_space.hpp"

#include <math.h>
#include "math/math_extra.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

TextureStreamingManager::TextureStreamingManager( 
	TextureManager * pTextureManager ) : 
		pTextureManager_( pTextureManager ), 
		debugMode_( DebugTextures::DEBUG_TS_NONE ), 
		outstandingStreamingRequests_( 0 ), 
		nextEvaluationTime_( 0.0 ), 
		minMipLevel_( 7 ), 
		maxMipLevel_( 100 ), 
		maxMipsPerRequest_( 10 ), 
		memoryBudget_( 500 * 1024 * 1024 ), 
		evaluationPeriod_( 0.1f ), 
		streamingMode_( MODE_CONSERVATIVE_NO_BUDGET ), 
		prioritisationMode_( PM_ENABLE_COHERENT_SACRIFICE ), 
		initialLoadMipSkip_( 2 ),
		idealSetError_( 0.0f ),
		targetSetError_( 0.0f ),
		textureDataOnDisk_( 0 ),
		textureDataInMemory_( 0 ),
		textureDataPercentInMemory_( 0 )
{
	pStreamingManager_.reset( new StreamingManager() );
	pEvaluationTask_ = new StreamingTextureEvaluationTask( this );

#define TS_WATCHER_GROUP "TextureStreaming/"

	// Configuration
	MF_WATCH( TS_WATCHER_GROUP "Config_StreamingMode", *this, 
		&TextureStreamingManager::streamingMode, 
		&TextureStreamingManager::streamingMode,
		"The streaming mode currently employed "
		"(off, cons_nobudget, cons, demand)" );

	MF_WATCH( TS_WATCHER_GROUP "Config_PrioritisationMode", *this, 
		&TextureStreamingManager::prioritisationMode, 
		&TextureStreamingManager::prioritisationMode,
		"The prioritisation mode currently employed (0 = generic, "
		"flag 1 = enable streaming priority, "
		"flag 2 = enable coherent sacrifice)" );

	MF_WATCH( TS_WATCHER_GROUP "Config_DebugMode", *this, 
		&TextureStreamingManager::debugMode, 
		&TextureStreamingManager::debugMode,
		"The debug mode currently employed (off, current_working_set, "
		"missing detail, missing available detail, object bounds, "
		"num mips skipped, sacrificed detail, texture size, "
		"current texture size)" );

	MF_WATCH( TS_WATCHER_GROUP "Config_Data_Budget", *this,
		&TextureStreamingManager::memoryBudget,
		&TextureStreamingManager::memoryBudget,
		"Streaming texture memory budget" );

	MF_WATCH( TS_WATCHER_GROUP "Config_EvaluationPeriod", *this,
		&TextureStreamingManager::evaluationPeriod,
		&TextureStreamingManager::evaluationPeriod,
		"The number of seconds between re-evaluations of the scene" );

	MF_WATCH( TS_WATCHER_GROUP "Config_GlobalMinMip", *this,
		&TextureStreamingManager::minMipLevel,
		&TextureStreamingManager::minMipLevel,
		"The minimum mip level to load (from level 0 = 1x1)" );
	
	MF_WATCH( TS_WATCHER_GROUP "Config_GlobalMaxMip", *this,
		&TextureStreamingManager::maxMipLevel,
		&TextureStreamingManager::maxMipLevel,
		"The maximum mip level to load (from level 0 = 1x1)" );

	MF_WATCH( TS_WATCHER_GROUP "Config_GlobalMaxMipsPerRequest", *this,
		&TextureStreamingManager::maxMipsPerRequest,
		&TextureStreamingManager::maxMipsPerRequest,
		"The maximum number of mip levels to load per request" );

	MF_WATCH( TS_WATCHER_GROUP "Config_InitialLoadMipSkip", *this,
		&TextureStreamingManager::initialLoadMipSkip,
		&TextureStreamingManager::initialLoadMipSkip,
		"The number of mips to skip on initial load" );

	// Status queries
	MF_WATCH( TS_WATCHER_GROUP "Query_NumTextures", *this,
		&TextureStreamingManager::streamingTextureCount,
		"Total number of streaming textures" );

	MF_WATCH( TS_WATCHER_GROUP "Query_NumTexturesInCache", *this,
		&TextureStreamingManager::numTexturesInCache,
		"Total number of textures in reuse cache" );

	MF_WATCH( TS_WATCHER_GROUP "Query_NumTextureUsage", *this,
		&TextureStreamingManager::streamingTextureUsageCount,
		"Total number of usages of streaming textures" );

	MF_WATCH( TS_WATCHER_GROUP "Query_PendingRequests", *this,
		&TextureStreamingManager::pendingStreamingRequests,
		"The number of outstanding streaming requests" );

	MF_WATCH( TS_WATCHER_GROUP "Query_IdealSet_ErrorMetric", 
		idealSetError_, Watcher::WT_READ_ONLY, 
		"Ideal set error metric" );

	MF_WATCH( TS_WATCHER_GROUP "Query_TargetSet_ErrorMetric", 
		targetSetError_, Watcher::WT_READ_ONLY, 
		"Target set error metric" );

	// Memory usage information
	MF_WATCH( TS_WATCHER_GROUP "Query_Data_OnDisk", *this,
		&TextureStreamingManager::memoryUsageStreamingDisk, 
		"Amount of texture data stored on disk for the textures in memory."
		"This would be loaded without texture streaming." );
	
	MF_WATCH( TS_WATCHER_GROUP "Query_Data_PercentInMemory", 
		textureDataPercentInMemory_, Watcher::WT_READ_ONLY, 
		"Percentage of disk data in memory" );

	MF_WATCH( TS_WATCHER_GROUP "Query_MemUsage_StreamingTextures", *this,
		&TextureStreamingManager::memoryUsageStreaming,
		"Amount of streaming texture data currently loaded into memory" );

	MF_WATCH( TS_WATCHER_GROUP "Query_MemUsage_ManagedTextures", *this,
		&TextureStreamingManager::memoryUsageManagedTextures, 
		"Amount of texture data currently in managed textures "
		"(textures loaded from named disk assets)" );

	MF_WATCH( TS_WATCHER_GROUP "Query_MemUsage_TextureCacheGPU", *this,
		&TextureStreamingManager::memoryUsageGPUCache, 
		"Amount of GPU memory used by the texture reuse cache" );

	MF_WATCH( TS_WATCHER_GROUP "Query_MemUsage_TextureCacheMain", *this,
		&TextureStreamingManager::memoryUsageMainCache, 
		"Amount of Main memory used by the texture reuse cache" );

	MF_WATCH( TS_WATCHER_GROUP "Query_MemUsage_AllTextures", *this,
		&TextureStreamingManager::memoryUsageAllTextures, 
		"Amount of GPU memory used by ALL textures loaded "
		"(Requires resource counters enabled)" );

#undef TS_WATCHER_GROUP

	if (isCommandLineDisabled())
	{
		streamingMode( MODE_DISABLE );
	}

	// Register space create/destroy handlers. These introduce a
	// dependency on 'space' lib. Unfortunately, the other alternative
	// is to make similar pieces of code all throughout all other projects
	// to hook these things up. So for now, we will do it just once here
	// until we have some common startup code library available to put it in.
	if ( streamingMode_ != MODE_DISABLE )
	{
		// Only do this if we have enabled texture streaming. 
		// This allows it to be disabled and remove all processing of 
		// dynamic objects and the overheads that that may incur.
		SpaceManager& spaceManager = SpaceManager::instance();
		spaceManager.spaceCreated().add< TextureStreamingManager, 
			&TextureStreamingManager::onSpaceCreated>( this );
		spaceManager.spaceDestroyed().add< TextureStreamingManager, 
			&TextureStreamingManager::onSpaceDestroyed>( this );
	}

#ifdef EDITOR_ENABLED
	// Blanket-disable texture streaming in all tools. The only exception to
	// this will be the world editor, which will allow streaming.
	streamingMode( MODE_DISABLE );
#endif
}

TextureStreamingManager::~TextureStreamingManager()
{
	// Unregister space create/destroy handlers.
	SpaceManager& spaceManager = SpaceManager::instance();
	spaceManager.spaceCreated().remove< TextureStreamingManager, 
		&TextureStreamingManager::onSpaceCreated>( this );
	spaceManager.spaceDestroyed().remove< TextureStreamingManager, 
		&TextureStreamingManager::onSpaceDestroyed>( this );

	// TODO: When streaming manager becomes not just used for texture
	// streaming, we will need to keep a list of all tasks that are active
	// and deactivate them before destroying the manager.
	pStreamingManager_->clear();

	// Tell the evaluation task to skip it's work.
	pEvaluationTask_->manager( NULL );
	clearViewpoints();
}


void TextureStreamingManager::tryDestroy( const StreamingTexture * texture )
{
	// get the lock first and then delegate to default behaviour
	SimpleMutexHolder smh( textureLock_ );
	texture->tryDestroy();
}


bool TextureStreamingManager::isCommandLineDisabled() const
{
#if !CONSUMER_CLIENT_BUILD
	// Check the command line for the disable flag
	LPSTR commandLine = ::GetCommandLineA();
	if (strstr( commandLine, "-disableTextureStreaming" ))
	{
		return true;
	}
#endif

	return false;
}


bool TextureStreamingManager::enabled() const
{
	return streamingMode_ != MODE_OFF && 
		streamingMode_ != MODE_DISABLE;
}

bool TextureStreamingManager::canStreamResource( 
	const BW::StringRef & resourceID )
{
	// Check to see if we are ignoring all requests for textures
	if (streamingMode_ == MODE_DISABLE)
	{
		return false;
	}

	// Check that the file matches our requirements for streaming
	BW::StringRef ext = BWResource::getExtension( resourceID );
	bool isDDS = ext.equals_lower( "dds" );
	if (!isDDS)
	{
		return false;
	}

	// Open up the file
	FileStreamerPtr pStream = 
		BWResource::instance().fileSystem()->streamFile( resourceID );
	if (!pStream)
	{
		return false;
	}

	// Load the DDS header from the stream
	StreamingTextureLoader::HeaderInfo header;
	if (!StreamingTextureLoader::loadHeaderFromStream( pStream.get(), header ))
	{
		return false;
	}

	// Cannot stream the texture if there is only one mip level stored.
	if (header.numMipLevels_ == 1)
	{
		return false;
	}

	// We don't support streaming cube map textures
	if (!header.isFlatTexture_)
	{
		return false;
	}

	// We don't understand the format
	if (header.format_ == D3DFMT_UNKNOWN)
	{
		return false;
	}

	return true;
}

BaseTexturePtr TextureStreamingManager::createStreamingTexture( 
	TextureDetailLevel * detail,
	const BW::StringRef & resourceID, 
	const BaseTexture::FailurePolicy & notFoundPolicy,
	D3DFORMAT format,
	int mipSkip, bool noResize, bool noFilter,
	const BW::StringRef & allocator )
{
	if (detail->streamable() &&
		canStreamResource( resourceID ))
	{
		SmartPointer< StreamingTexture > res = new StreamingTexture( 
			detail, 
			resourceID,
			notFoundPolicy, 
			format, 
			mipSkip, 
			noResize, 
			noFilter, 
			allocator );

		// Register the texture with the streaming system.
		// Handles initial load at this point.
		if (!registerTexture( res.get(), detail ))
		{
			return NULL;
		}

		return res;
	}
	else
	{
		// Streaming unsupported for this texture
		return NULL;
	}
}

bool TextureStreamingManager::registerTexture( StreamingTexture * pTex,
	const TextureDetailLevel * detail )
{
	SimpleMutexHolder smh( textureLock_ );

	// Allocate a new streaming texture entry for this texture
	StreamingTextureHandle handle = streamingTextures_.create();
	StreamingTextureEntry& entry = streamingTextures_[handle];
	entry.pTexture = pTex;
	entry.currentMip = INVALID_MIP;
	entry.requestedMip = INVALID_MIP;
	entry.streamingPriority = detail->streamingPriority();

	pTex->streamingTextureHandle( handle );

	// Force the texture to load some data immediately
	requestTextureUpdate( entry, minMipLevel_, true);

	bool success =  entry.currentMip != INVALID_MIP &&
		entry.format != D3DFMT_UNKNOWN;

	if (success)
	{
		// We assign the ideal mip to the value of current mip here as
		// after the request, a load will have occured and the entry filled
		// with the relevant information
		setIdealMip( entry, entry.currentMip );
	}
	else
	{
		// Failed
		unregisterTexture( pTex );
	}

	return success;
}

void TextureStreamingManager::unregisterTexture( StreamingTexture * pTex )
{
	// TODO: MF_ASSERT( textureLock.isOwnedByCurrentThread() )

	StreamingTextureHandle handle = pTex->streamingTextureHandle();
	MF_ASSERT( handle.isValid() );
	
	MF_ASSERT( streamingTextures_[handle].pTexture == pTex );
	MF_ASSERT( !streamingTextures_[handle].streamingActive );

	// Remove the texture from the set of streaming textures
	pTex->streamingTextureHandle( StreamingTextureHandle() );
	streamingTextures_.release( handle );
}

void TextureStreamingManager::reloadTexture( StreamingTexture * pTex )
{
	SimpleMutexHolder smh( textureLock_ );

	StreamingTextureHandle handle = pTex->streamingTextureHandle();
	MF_ASSERT( handle.isValid() );
	StreamingTextureEntry& entry = streamingTextures_[handle];

	// Flag the current mip as not loaded to anything.
	entry.currentMip = INVALID_MIP;

	if (!entry.streamingActive)
	{
		entry.requestedMip = INVALID_MIP;
	}
}

uint32 TextureStreamingManager::getTextureWidth( 
	const StreamingTexture * pTex ) const
{
	const StreamingTextureEntry * pEntry = fetchEntry( pTex );
	if (!pEntry)
	{
		return 0;
	}

	return std::max< uint32 >( pEntry->width >> 
		(pEntry->maxAvailableMip - pEntry->currentMip), 1 );
}

uint32 TextureStreamingManager::getTextureHeight( 
	const StreamingTexture * pTex ) const
{
	const StreamingTextureEntry * pEntry = fetchEntry( pTex );
	if (!pEntry)
	{
		return 0;
	}

	return std::max< uint32 >( pEntry->height >> 
		(pEntry->maxAvailableMip - pEntry->currentMip), 1 );
}

D3DFORMAT TextureStreamingManager::getTextureFormat( 
	const StreamingTexture * pTex ) const
{
	const StreamingTextureEntry * pEntry = fetchEntry( pTex );
	if (!pEntry)
	{
		return D3DFMT_UNKNOWN;
	}

	return pEntry->format;
}

size_t TextureStreamingManager::getTextureMemoryUsed( 
	const StreamingTexture * pTex ) const
{
	const StreamingTextureEntry * pEntry = fetchEntry( pTex );
	if (!pEntry)
	{
		return 0;
	}

	return calculateTextureMemoryUsage( *pEntry );
}

uint32 TextureStreamingManager::getTextureMaxMipLevel( 
	const StreamingTexture * pTex ) const
{
	const StreamingTextureEntry * pEntry = fetchEntry( pTex );
	if (!pEntry)
	{
		return 0;
	}

	return pEntry->currentMip - pEntry->minAvailableMip;
}

void TextureStreamingManager::updateTextureMipSkip( 
	StreamingTexture * pTex , uint32 mipSkip )
{
	StreamingTextureEntry * pEntry = fetchEntry( pTex );
	if (!pEntry)
	{
		return;
	}

	pEntry->maxAvailableStreamingMip = pEntry->maxAvailableMip - mipSkip;
	pEntry->maxAvailableStreamingMip = 
		max(pEntry->maxAvailableStreamingMip, pEntry->minAvailableStreamingMip);
}

void TextureStreamingManager::updateViewpoints( 
	BW::vector<ViewpointData> & activePoints )
{
	BW::vector< Viewpoint * > unregisterList;

	for (ViewpointList::iterator iter = viewpoints_.begin(),
		end = viewpoints_.end(); iter != end; ++iter)
	{
		const SmartPointer< Viewpoint > & pViewpoint = *iter;

		ViewpointData tempData;
		Viewpoint::QueryReturnCode rc = pViewpoint->query( tempData );
		if (rc == Viewpoint::SUCCESS)
		{
			activePoints.push_back( tempData );
		}
		else if (rc == Viewpoint::UNREGISTER)
		{
			unregisterList.push_back( pViewpoint.get() );
		}
	}

	for (uint32 index = 0; index < unregisterList.size(); ++index)
	{
		Viewpoint * pViewpoint = unregisterList[index];
		unregisterViewpoint( pViewpoint );
	}
}

void TextureStreamingManager::calculateIdealMips()
{
	// TODO: MF_ASSERT( textureLock.isOwnedByCurrentThread() )

	BW::vector< ViewpointData > activeViewpoints;
	updateViewpoints( activeViewpoints );

	// TODO: Implement this whole process as a scatter/gather operation
	// Reset the usage of each texture
	// For each texture now update ideal available map to invalid handle
	for (StreamingTextureEntryVector::iterator iter = 
		streamingTextures_.begin(), end = streamingTextures_.end();
		iter != end; ++iter)
	{
		StreamingTextureEntry& entry = *iter;
		entry.idealMip = INVALID_MIP;
		entry.pixelsAssignedOnScreen = 0;
		entry.minAngleToFrustum = MATH_PI;
	}

	// Iterate over all scene views and calculate ideal mips
	{
		TextureStreamingUsageCollector usageCollector( this );
		SimpleMutexHolder smh_scenes( sceneLock_ );


		for (uint32 index = 0; index < activeViewpoints.size(); ++index)
		{
			ViewpointData& viewpointData = activeViewpoints[index];
			for (auto iter = sceneViews_.begin(); 
				iter != sceneViews_.end(); ++iter)
			{
				// Iterate through all usages of the textures and update idealMap entry
				TextureStreamingSceneView* pView = *iter;
				pView->calculateIdealMips( usageCollector, viewpointData );
			}
		}
	}

	// For each texture now update ideal available map to 
	for (StreamingTextureEntryVector::iterator iter = 
		streamingTextures_.begin(), end = streamingTextures_.end();
		iter != end; ++iter)
	{
		StreamingTextureEntry& entry = *iter;

		// Set everything to requiring the minimum mip level
		uint32 idealMipLevel = minMipLevel_;

		if (entry.idealMip == INVALID_MIP)
		{
			// Nothing has claimed to use this texture
			// or there are no valid view points to decide against
			// Yet it still exists, so we must assume it is being used
			// and required at highest detail
			idealMipLevel = entry.maxAvailableStreamingMip;
		}
		else
		{
			idealMipLevel = 
				Math::clamp( minMipLevel_, entry.idealMip , maxMipLevel_ );
		}

		if (entry.forcedMip != INVALID_MIP)
		{
			idealMipLevel = entry.forcedMip;
		}

		// Assign ideal level
		setIdealMip( entry, idealMipLevel );
	}
}

struct IndexPriority
{
	IndexPriority() :
		index_( TextureStreamingManager::INVALID_MIP ), 
		priority_( FLT_MIN )
	{
	}

	IndexPriority( uint32 index, float priority ): 
		index_( index ), 
		priority_( priority )
	{
	}

	bool operator < (const IndexPriority& other)
	{
		return this->priority_ < other.priority_;
	}

	uint32 index_;
	float priority_;
};

void TextureStreamingManager::calculateTargetMips()
{
	size_t proposedMemoryUsage = 0;
	if (streamingMode_ == MODE_OFF)
	{
		// For each texture now update ideal available map to 
		for (StreamingTextureEntryVector::iterator iter = 
			streamingTextures_.begin(), end = streamingTextures_.end();
			iter != end; ++iter)
		{
			StreamingTextureEntry& entry = *iter;

			// Maximum texture target set generation
			entry.targetMip = entry.maxAvailableStreamingMip;
		}
	} 
	else
	{
		// For each texture now update ideal available map to 
		for (StreamingTextureEntryVector::iterator iter = 
			streamingTextures_.begin(), end = streamingTextures_.end();
			iter != end; ++iter)
		{
			StreamingTextureEntry& entry = *iter;

			// Naive target set generation
			size_t usage = calculateTextureMemoryUsage(
				entry.width, 
				entry.height, 
				entry.maxAvailableMip - entry.idealAvailableMip, 
				entry.idealAvailableMip - entry.minAvailableMip + 1,
				entry.format);
			proposedMemoryUsage += usage;

			entry.targetMip = entry.idealAvailableMip;
		}
	}

	const bool isOversubscribed = proposedMemoryUsage > memoryBudget_;

	if (isOversubscribed &&
		(streamingMode_ == MODE_CONSERVATIVE ||
		streamingMode_ == MODE_ON_DEMAND))
	{
		executeOverSubscriptionPolicy( proposedMemoryUsage );
	}

	if (!isOversubscribed &&
		streamingMode_ == MODE_ON_DEMAND)
	{
		executeUnderSubscriptionPolicy( proposedMemoryUsage );
	}
}

void TextureStreamingManager::executeOverSubscriptionPolicy( 
	size_t proposedMemoryUsage )
{
	// Prioritise the textures, textures with highest priority will be 
	// considered for space in the budget first.
	
	StreamingTextureEntryVector::Container& textureEntries = 
		streamingTextures_.container();
	size_t numTextures = textureEntries.size();

	BW::vector< IndexPriority > priorityHeap;
	priorityHeap.reserve( numTextures );

	for (uint32 index = 0; index < numTextures; ++index)
	{
		StreamingTextureEntry& entry = textureEntries[index];

		bool canSacrifice = canSacrificeDetail( entry );
		if (!canSacrifice)
		{
			// Don't attempt to prioritise textures that MUST be loaded to max
			continue;
		}

		priorityHeap.push_back( 
			IndexPriority( index, sacrificePriority( entry ) ) );
		push_heap( priorityHeap.begin(), priorityHeap.end() );
	}

	while (proposedMemoryUsage > memoryBudget_ &&
		!priorityHeap.empty())
	{
		// Find a texture mip to remove from the list
		pop_heap( priorityHeap.begin(), priorityHeap.end() );
		// The entry is now at the back (since the pop)
		IndexPriority& minEntry = priorityHeap.back();

		StreamingTextureEntry& entry = textureEntries[minEntry.index_];

		// reduce the memory usage by the amount of this textures current 
		// proposed mip
		size_t usage = calculateTextureMemoryUsage(
			entry.width, entry.height, 
			entry.maxAvailableMip - entry.targetMip, 
			1,
			entry.format);

		proposedMemoryUsage -= usage;
		entry.targetMip -= 1;

		bool canSacrifice = canSacrificeDetail( entry );
		if (canSacrifice)
		{
			// Reduce it's target mip level by one
			minEntry.priority_ = sacrificePriority( entry );

			// Add it back into the heap
			push_heap( priorityHeap.begin(), priorityHeap.end() );
		}
		else
		{
			priorityHeap.pop_back();
		}
	}
}

void TextureStreamingManager::executeUnderSubscriptionPolicy( 
	size_t proposedMemoryUsage )
{
	// We only want to unload if we must to free space for textures in the
	// target set. Increase mips in the target set to values
	// that already exist in memory until we don't meet our budget

	StreamingTextureEntryVector::Container& textureEntries = 
		streamingTextures_.container();
	size_t numTextures = textureEntries.size();

	BW::vector<IndexPriority> priorityHeap;
	priorityHeap.reserve( numTextures );

	for (uint32 index = 0; index < numTextures; ++index)
	{
		StreamingTextureEntry& entry = textureEntries[index];

		// Only attempt to upgrade textures from their targets
		// to their current values
		if (entry.currentMip <= entry.targetMip)
		{
			continue;
		}

		priorityHeap.push_back( IndexPriority( index, loadPriority( entry )) );
		push_heap( priorityHeap.begin(), priorityHeap.end() );
	}

	while (proposedMemoryUsage < memoryBudget_ &&
		!priorityHeap.empty())
	{
		// Find a texture to push back to it's current level
		pop_heap( priorityHeap.begin(), priorityHeap.end() );
		// The entry is now at the back (since the pop)
		IndexPriority maxEntry = priorityHeap.back();
		StreamingTextureEntry & entry = textureEntries[maxEntry.index_];
		priorityHeap.pop_back();

		// reduce the memory usage by the amount of this textures current 
		// proposed mip
		size_t usage = calculateTextureMemoryUsage(
			entry.width, entry.height, 
			entry.maxAvailableMip - entry.currentMip, 
			entry.currentMip - entry.targetMip,
			entry.format );

		if (proposedMemoryUsage + usage > memoryBudget_)
		{
			// This is the end of our budget
			break;
		}
		else
		{
			proposedMemoryUsage += usage;

			// Update the texture's target
			entry.targetMip = entry.currentMip;
		}
	}
}

float TextureStreamingManager::sacrificePriority( 
	const StreamingTextureEntry& entry )
{
	float priority = -loadPriority( entry );

	// Modify the load priority according to previous sacrifice
	float adjustment = 0.0f;

	// Define the sizes of the bands that each of the adjustments
	// below will categorize the textures into on the priority spectrum.
	const float streamingPriorityBandWidth = 100.0f;
	const float coherantAdjustmentBandWidth = 10000.0f;

	// Now adjust this priority according to the user
	// priority placed upon this texture.
	// For each mip we sacrifice, we consider the texture as
	// if it is at a higher streaming priority to be considered 
	// against all the others in that streaming priority category
	uint32 currentSacrifice = entry.idealAvailableMip - entry.targetMip;
	if (prioritisationMode_ & PM_ENABLE_STREAMING_PRIORITY)
	{
		adjustment -= (float)(entry.streamingPriority + currentSacrifice) * 
			streamingPriorityBandWidth;
	}
	
	// Lower the cost of sacrificing textures that have been sacrificed to 
	// this level before. Unless this texture has recently requested to be 
	// upgraded due to proximity...
	uint32 originalIdeal = entry.requestedMip + entry.sacrificedDetail;
	if (entry.sacrificedDetail > currentSacrifice &&
		originalIdeal <= entry.idealAvailableMip &&
		prioritisationMode_ & PM_ENABLE_COHERENT_SACRIFICE)
	{
		uint32 category = entry.sacrificedDetail - currentSacrifice;
		adjustment += float(category) * coherantAdjustmentBandWidth;
	}

	return priority + adjustment;
}

float TextureStreamingManager::loadPriority(
	const StreamingTextureEntry & entry )
{
	// Calculate how much memory the top target mip is using
	size_t topMipMemoryUsage = calculateTextureMemoryUsage(
		entry.width, entry.height, 
		entry.maxAvailableMip -  entry.targetMip, 
		1,
		entry.format );

	float cost = static_cast< float >( BW::log2ceil( 
		static_cast< uint32 >( topMipMemoryUsage )) >> 1);

	float benefit = static_cast< float >(
		entry.idealAvailableMip - entry.targetMip + 1) * 
		(BW::log2ceil( entry.pixelsAssignedOnScreen ) >> 1 );

	// Adjust benifit to lower if the min angle > 0
	benefit *= (MATH_PI - entry.minAngleToFrustum)/ MATH_PI;

	float priority = benefit / cost;

	return priority;
}

bool TextureStreamingManager::canSacrificeDetail( 
	const StreamingTextureEntry & entry )
{
	if (entry.idealMip == INVALID_MIP)
	{
		// Don't attempt to prioritise textures that MUST be loaded to max
		return false;
	}

	if (entry.targetMip <= entry.minAvailableStreamingMip)
	{
		return false;
	}

	if (entry.streamingPriority != 0 &&
		(prioritisationMode_ & PM_ENABLE_STREAMING_PRIORITY_DISABLE_SACRIFICE) != 0)
	{
		return false;
	}

	return true;
}

void TextureStreamingManager::generateStreamingRequests()
{
	// For each texture now update ideal available map to 
	for (StreamingTextureEntryVector::iterator iter = 
		streamingTextures_.begin(), end = streamingTextures_.end();
		iter != end; ++iter)
	{
		StreamingTextureEntry & entry = *iter;

		if (entry.idealAvailableMip > entry.targetMip)
		{
			entry.sacrificedDetail = entry.idealAvailableMip - entry.targetMip;
		}
		else
		{
			entry.sacrificedDetail = 0;
		}
		
		// Make the request
		requestTextureUpdate( entry, entry.targetMip );
	}
}

void TextureStreamingManager::registerViewpoint( Viewpoint * pViewpoint )
{
	SimpleMutexHolder smh( viewpointLock_ );

	viewpoints_.push_back( pViewpoint );
	MF_ASSERT( pViewpoint->manager() == NULL );
	pViewpoint->manager( this );
}

void TextureStreamingManager::unregisterViewpoint( Viewpoint * pViewpoint )
{
	SimpleMutexHolder smh( viewpointLock_ );

	MF_ASSERT( pViewpoint->manager() == this );
	ViewpointList::iterator findResult = std::find( 
		viewpoints_.begin(), viewpoints_.end(), pViewpoint );
	MF_ASSERT( findResult != viewpoints_.end() );
	
	pViewpoint->manager( NULL );

	// swap with the back and erase
	std::swap( *findResult, viewpoints_.back() );
	viewpoints_.pop_back();
}

void TextureStreamingManager::clearViewpoints()
{
	while (!viewpoints_.empty())
	{
		unregisterViewpoint( viewpoints_.front().get() );
	}
}

void TextureStreamingManager::registerScene( Scene * pScene )
{
	SimpleMutexHolder smh( sceneLock_ );
	bool inserted = scenes_.insert( pScene ).second;

	if (!inserted)
	{
		return;
	}

	// Make sure the texture streaming scene view exists on 
	// the scene
	TextureStreamingSceneView* pView = 
		pScene->getView<TextureStreamingSceneView>();
	sceneViews_.insert( pView );
}

void TextureStreamingManager::unregisterScene( Scene * pScene )
{
	SimpleMutexHolder smh( sceneLock_ );
	scenes_.erase( pScene );

	TextureStreamingSceneView* pView = 
		pScene->getView<TextureStreamingSceneView>();
	sceneViews_.erase( pView );
}

void TextureStreamingManager::onSpaceCreated( SpaceManager * pManager, 
	ClientSpace * pSpace )
{
	registerScene( &pSpace->scene() );
}

void TextureStreamingManager::onSpaceDestroyed( SpaceManager * pManager, 
	ClientSpace * pSpace )
{
	unregisterScene( &pSpace->scene() );
}

void TextureStreamingManager::forceTextureSkipMips( 
	StreamingTexture * pTex, uint32 skipMips )
{
	SimpleMutexHolder smh( textureLock_ );

	StreamingTextureEntry* pEntry = fetchEntry( pTex );
	if (skipMips == INVALID_MIP)
	{
		pEntry->forcedMip = INVALID_MIP;
	}
	else
	{
		pEntry->forcedMip = Math::clamp(
			pEntry->minAvailableMip, 
			pEntry->maxAvailableMip - skipMips, 
			pEntry->maxAvailableMip);
	}
}

const TextureStreamingManager::StreamingTextureEntry * 
	TextureStreamingManager::fetchEntry( const StreamingTexture * pTex ) const
{
	// TODO: MF_ASSERT( textureLock_.isHeldByCurrentThread() )

	StreamingTextureHandle handle = pTex->streamingTextureHandle();
	if (!handle.isValid())
	{
		return NULL;
	}

	const StreamingTextureEntry& entry = streamingTextures_[handle];
	MF_ASSERT( entry.pTexture == pTex );
	return &entry;
}

TextureStreamingManager::StreamingTextureEntry * 
	TextureStreamingManager::fetchEntry( StreamingTexture * pTex )
{
	return const_cast< StreamingTextureEntry * >( 
		fetchEntry( (const StreamingTexture *)pTex ) );
}

bool TextureStreamingManager::getStreamingRequest( StreamingTexture * pTex, 
	StreamingTextureRequest & request )
{
	StreamingTextureEntry * pEntry = fetchEntry( pTex );
	if (!pEntry)
	{
		return false;
	}

	request = *pEntry;
	return true;
}

void TextureStreamingManager::setIdealMip( StreamingTextureEntry & entry, 
	uint32 mipLevel )
{
	// Abide by static constraints
	entry.idealAvailableMip = Math::clamp( entry.minAvailableStreamingMip, 
		mipLevel, entry.maxAvailableStreamingMip );
}

uint32 TextureStreamingManager::calculateMaxMips( uint32 width, uint32 height )
{
	uint32 maxDim = max( width, height );
	return BW::log2ceil( maxDim );	
}

BaseTexture * TextureStreamingManager::getDebugTexture( 
	DebugTextures::TextureSet debugSet, BaseTexture * pTex )
{
	const StreamingTextureEntry * pEntry = NULL;
	if (pTex->type() == BaseTexture::STREAMING)
	{
		pEntry = fetchEntry( static_cast< StreamingTexture * >( pTex ) );
	}

	if (pEntry)
	{
		uint32 mipSkip = pEntry->maxAvailableMip - pEntry->currentMip;
		if (debugSet == DebugTextures::DEBUG_TS_SACRIFICED_DETAIL)
		{
			mipSkip = pEntry->idealAvailableMip - pEntry->targetMip;
		}

		return debugTextures_.getDebugTexture( debugSet,
			pEntry->width,
			pEntry->height,
			mipSkip);
	}
	else
	{
		// Texture is not a streaming texture, technically shouldn't be here,
		// but if it is, then grab the information from the object itself
		return debugTextures_.getDebugTexture( debugSet, pTex );
	}
}

void TextureStreamingManager::streamingMode( uint32 mode )
{
	streamingMode_ = mode;
}

uint32 TextureStreamingManager::streamingMode() const
{
	return streamingMode_;
}

void TextureStreamingManager::prioritisationMode( uint32 mode )
{
	prioritisationMode_ = mode;
}

uint32 TextureStreamingManager::prioritisationMode() const
{
	return prioritisationMode_;
}

void TextureStreamingManager::debugMode(
	uint32 mode )
{
	if (debugMode_ == mode)
	{
		return;
	}

	SimpleMutexHolder smh( textureLock_ );

	// Assign new set
	debugMode_ = static_cast< DebugTextures::TextureSet >( mode );

	// Clear the previous set of debug textures (releasing them)
	debugTextures_.clear();

	// Lock the textures, iterate over them and assign textures
	for (StreamingTextureEntryVector::iterator iter = 
		streamingTextures_.begin(), end = streamingTextures_.end();
		iter != end; ++iter)
	{
		StreamingTextureEntry & entry = *iter;

		assignDebugTexture( entry );
	}
}

void TextureStreamingManager::assignDebugTexture( 
	StreamingTextureEntry & entry )
{
	// Default is no change
	ComObjectWrap< DeviceTexture > pReplacement = NULL;

	if (debugMode_ != DebugTextures::DEBUG_TS_NONE)
	{
		BaseTexture * pReplacementTex = 
			getDebugTexture( debugMode_, entry.pTexture );
		if (pReplacementTex)
		{
			pReplacement = pReplacementTex->pTexture();
		}
	}
	
	// Assign it a debug one 
	entry.pTexture->assignDebugTexture( pReplacement.pComObject() );
}

uint32 TextureStreamingManager::debugMode() const
{
	return debugMode_;
}

void TextureStreamingManager::minMipLevel( uint32 size )
{
	// Make sure it doesn't go above max mip level
	minMipLevel_ = min( size, maxMipLevel_ );
}

uint32 TextureStreamingManager::minMipLevel() const
{
	return minMipLevel_;
}

void TextureStreamingManager::maxMipLevel( uint32 size )
{
	// Make sure it doesn't go below min mip level
	maxMipLevel_ = max( size, minMipLevel_ );
}

uint32 TextureStreamingManager::maxMipLevel() const
{
	return maxMipLevel_;
}

void TextureStreamingManager::maxMipsPerRequest( uint32 size )
{
	maxMipsPerRequest_ = size;
}

uint32 TextureStreamingManager::maxMipsPerRequest() const
{
	return maxMipsPerRequest_;
}

void TextureStreamingManager::memoryBudget( uint32 budget )
{
	memoryBudget_ = budget;
}

uint32 TextureStreamingManager::memoryBudget() const
{
	return memoryBudget_;
}

void TextureStreamingManager::evaluationPeriod( float seconds )
{
	evaluationPeriod_ = seconds;
}

float TextureStreamingManager::evaluationPeriod() const
{
	return evaluationPeriod_;
}

void TextureStreamingManager::initialLoadMipSkip( uint32 mipSkip )
{
	initialLoadMipSkip_ = mipSkip;
}

uint32 TextureStreamingManager::initialLoadMipSkip() const
{
	return initialLoadMipSkip_;
}

uint32 TextureStreamingManager::streamingTextureCount() const
{
	return (uint32)streamingTextures_.size();
}

uint32 TextureStreamingManager::streamingTextureUsageCount() const
{
	// Iterate over all views and draw debug information
	uint32 count = 0;
	SimpleMutexHolder smh_scenes( sceneLock_ );
	for (auto iter = sceneViews_.begin(); 
		iter != sceneViews_.end(); ++iter)
	{
		// Iterate through all usages of the textures and update idealMap entry
		TextureStreamingSceneView* pView = *iter;
		count += pView->usageCount();
	}
	return count;
}

uint32 TextureStreamingManager::pendingStreamingRequests() const
{
	return outstandingStreamingRequests_;
}

size_t TextureStreamingManager::memoryUsageGPUCache() const
{
	return Moo::rc().textureReuseCache().memoryUsageGPU();
}

size_t TextureStreamingManager::memoryUsageMainCache() const
{
	return Moo::rc().textureReuseCache().memoryUsageMain();
}

size_t TextureStreamingManager::numTexturesInCache() const
{
	return Moo::rc().textureReuseCache().size();
}

size_t TextureStreamingManager::memoryUsageStreaming() const
{
	return textureDataInMemory_;
}

size_t TextureStreamingManager::memoryUsageStreamingDisk() const
{
	return textureDataOnDisk_;
}

size_t TextureStreamingManager::memoryUsageManagedTextures() const
{
	return TextureManager::instance()->textureMemoryUsed();
}

size_t TextureStreamingManager::memoryUsageAllTextures() const
{
	ResourceCounters::Entry entry;
	ResourceCounters::instance().getResourceTypeMemoryUsage(
		ResourceCounters::RT_TEXTURE, 
		&entry);
	return static_cast<size_t>( entry.sum_.gpu_ );
}

size_t TextureStreamingManager::memoryUsageActiveTextures() const
{
	return memoryUsageAllTextures() - memoryUsageGPUCache();
}

void TextureStreamingManager::tick()
{
	// Check the frequency of the task and fire it off 
	// (and make sure it isn't currently running)
	double currentTime = TimeStamp::toSeconds( timestamp() );
	if (nextEvaluationTime_ == 0.0 || 
		currentTime >= nextEvaluationTime_ )
	{
		if (pEvaluationTask_->queue())
		{
			nextEvaluationTime_ = currentTime + evaluationPeriod_;
		}
	}
	
	pStreamingManager_->tick();
}

void TextureStreamingManager::evaluateStreaming()
{
	if (enabled())
	{
		PROFILER_SCOPED( TextureStreaming_Eval );

		SimpleMutexHolder smh( textureLock_ );

		calculateIdealMips();
		calculateTargetMips();
		generateStreamingRequests();
	}

	updatePerformanceMetrics();
}

void TextureStreamingManager::drawDebug()
{
	if (debugMode_ == DebugTextures::DEBUG_TS_OBJECT_BOUNDS)
	{
		// Iterate over all views and draw debug information
		SimpleMutexHolder smh_scenes( sceneLock_ );
		for (auto iter = sceneViews_.begin(); 
			iter != sceneViews_.end(); ++iter)
		{
			// Iterate through all usages of the textures and update idealMap entry
			TextureStreamingSceneView* pView = *iter;
			pView->drawDebug( debugMode_ );
		}
	}
}

void TextureStreamingManager::updatePerformanceMetrics()
{
	idealSetError_ = calculateIdealSetErrorMetric();
	targetSetError_ = calculateTargetSetErrorMetric();
	calculateTotalTextureMemoryUsage( &textureDataInMemory_, 
		&textureDataOnDisk_ );
	textureDataPercentInMemory_ = 
		uint32(float(textureDataInMemory_) / float(textureDataOnDisk_) * 
			100.0f);
}

void TextureStreamingManager::calculateTotalTextureMemoryUsage( 
	size_t * pDataInMemory, size_t * pDataOnDisk ) const
{
	SimpleMutexHolder smh( textureLock_ );
	size_t inMemory = 0;
	size_t onDisk = 0;

	for (StreamingTextureEntryVector::const_iterator iter = 
		streamingTextures_.begin(), end = streamingTextures_.end();
		iter != end; ++iter)
	{
		const StreamingTextureEntry & entry = *iter;

		inMemory += calculateTextureMemoryUsage( entry );
		onDisk += calculateTextureMemoryUsage( entry.width, entry.height, 
			0, entry.maxAvailableMip - entry.minAvailableMip,
			entry.format );
	}
	*pDataInMemory = inMemory;
	*pDataOnDisk = onDisk;
}

size_t TextureStreamingManager::calculateTextureMemoryUsage( 
	const StreamingTextureEntry & entry ) const
{
	return calculateTextureMemoryUsage( 
		entry.width, entry.height, 
		entry.maxAvailableMip - entry.currentMip, 
		entry.currentMip - entry.minAvailableMip,
		entry.format );
}

size_t TextureStreamingManager::calculateTextureMemoryUsage( 
	uint32 width, uint32 height, uint32 skipMips, 
	uint32 mipCount, TextureFormat format ) 
{
	// Naive implementation, could be expressed as a direct equation. 
	// Also doesn't take into account compression
	width = std::max< uint32 >( width >> skipMips, 1 );
	height = std::max< uint32 >( height >> skipMips, 1 );

	// Using more accurate estimation of memory usage
	float topMipMemoryUsage = static_cast< float >(
		DX::surfaceSize( width, height, format ));
	
	// (Approximate using Sum of a geometric series)
	// usage = a ( 1-r^m) / (1 - r) r = ratio, a = first term, m num terms
	uint32 usage = static_cast< uint32 >( topMipMemoryUsage * 
		( 1.0f - pow(0.25f, static_cast< float >(mipCount))) / (0.75f) );

	return usage;
}

float TextureStreamingManager::calculateIdealSetErrorMetric()
{
	SimpleMutexHolder smh( textureLock_ );
	float errorSum = 0;

	for (StreamingTextureEntryVector::iterator iter = 
		streamingTextures_.begin(), end = streamingTextures_.end();
		iter != end; ++iter)
	{
		StreamingTextureEntry & entry = *iter;

		// No error if we're above the required mip
		if (entry.idealAvailableMip <= entry.requestedMip)
			continue;

		uint32 error = entry.idealAvailableMip - entry.requestedMip;

		errorSum += powf( static_cast< float >( error ), 2.0 );
	}
	return errorSum;
}

float TextureStreamingManager::calculateTargetSetErrorMetric()
{
	SimpleMutexHolder smh( textureLock_ );
	float errorSum = 0;

	for (StreamingTextureEntryVector::iterator iter = 
		streamingTextures_.begin(), end = streamingTextures_.end();
		iter != end; ++iter)
	{
		StreamingTextureEntry & entry = *iter;

		// No error if we're above the required mip
		if (entry.idealAvailableMip <= entry.currentMip)
			continue;

		uint32 error = entry.idealAvailableMip - entry.currentMip;

		errorSum += powf( static_cast< float >( error ), 2.0 );
	}
	return errorSum;
}

void TextureStreamingManager::requestTextureUpdate( 
	StreamingTextureEntry & entry, uint32 targetMipLevel, bool immediate )
{
	uint32 newRequestedMip = targetMipLevel;

	// Clamp the new request to the maximum number of mips away from the current
	if (newRequestedMip > entry.currentMip || 
		entry.currentMip == INVALID_MIP)
	{
		uint32 effectiveCurrentMip = entry.currentMip;
		if (effectiveCurrentMip == INVALID_MIP)
		{
			effectiveCurrentMip = max( entry.minAvailableStreamingMip, 
				minMipLevel_ );
		}

		newRequestedMip = min( newRequestedMip, 
			effectiveCurrentMip + maxMipsPerRequest_ );
	}

	// Check if we've already requested this update?
	if (newRequestedMip == entry.requestedMip)
	{
		return;
	}

	// Set the requested mip level to the given target.
	// With any luck any existing streaming tasks will use this new value if it 
	// hasn't started already.
	entry.requestedMip = newRequestedMip;
	if (entry.currentMip == entry.requestedMip)
	{
		// Nothing more to be done
		return;
	}

	if (!entry.streamingActive || immediate)
	{
		size_t estimate = 0;
		if (entry.requestedMip > entry.currentMip)
		{
			estimate = calculateTextureMemoryUsage(
				entry.width, entry.height, 
				entry.maxAvailableMip - entry.requestedMip, 
				entry.requestedMip - entry.currentMip,
				entry.format );
		}

		// Queue the task with streaming manager
		SmartPointer<StreamingTextureLoadTask> pTask = 
			new StreamingTextureLoadTask( this, entry.pTexture, estimate );

		if (immediate)
		{
			pTask->forceExecute();
			handleStreamingCompleted( entry, 
				entry.pTexture, 
				pTask->targetTexture(), 
				pTask->requestEntry() );
		}
		else
		{
			outstandingStreamingRequests_++;
			entry.streamingActive = true;
			pStreamingManager_->registerTask( pTask.get() );
		}
	}
}

void TextureStreamingManager::reportStreamingCompleted( 
	StreamingTexture * pTex, DX::BaseTexture * pNewTexture, 
	const StreamingTextureRequest & requestEntry)
{
	SimpleMutexHolder smh( textureLock_ );

	MF_ASSERT( outstandingStreamingRequests_ >= 1 );
	outstandingStreamingRequests_--;

	// Fetch the entry for this texture:
	StreamingTextureEntry * pEntry = fetchEntry( pTex );
	if (!pEntry)
	{
		return;
	}
	pEntry->streamingActive = false;

	handleStreamingCompleted( *pEntry, pTex, pNewTexture, requestEntry );
}

void TextureStreamingManager::handleStreamingCompleted( 
	StreamingTextureEntry & entry,
	StreamingTexture * pTex, DeviceTexture * pNewTexture,
	const StreamingTextureRequest & requestEntry )
{
	// TODO: MF_ASSERT( textureLock_.isHeldByCurrentThread() );

	// Check if the new texture was successfully loaded
	// If there is no new texture then the streaming operation
	// wasn't completed, and the data it has passed
	// may no longer be valid, so it should be completely ignored.
	if (!pNewTexture)
	{
		return;
	}
	
	// Update the information about the texture 
	// (if the texture's data was invalidated)
	if (requestEntry.currentMip == INVALID_MIP)
	{
		// The entry will have new information about the static
		// constraints of the texture, so copy it out
		entry.width = requestEntry.width;
		entry.height = requestEntry.height;
		entry.maxAvailableMip = requestEntry.maxAvailableMip;
		entry.minAvailableMip = requestEntry.minAvailableMip;
		entry.format = requestEntry.format;
		entry.minAvailableStreamingMip = 
			requestEntry.minAvailableStreamingMip;
		entry.maxAvailableStreamingMip = 
			requestEntry.maxAvailableStreamingMip;
	}
	else
	{
		// Make sure we have the correct entry that we're
		// updating!
		MF_ASSERT( entry.width == requestEntry.width );
		MF_ASSERT( entry.height == requestEntry.height );
		MF_ASSERT( entry.maxAvailableMip == requestEntry.maxAvailableMip );
		MF_ASSERT( entry.minAvailableMip == requestEntry.minAvailableMip );
		MF_ASSERT( entry.format == requestEntry.format );
		MF_ASSERT( entry.minAvailableStreamingMip == 
			requestEntry.minAvailableStreamingMip );

		// Max available streaming mip may have changed due to a texture detail
		// setting change whilst streaming. An example of this being the user
		// changing texture detail level in WorldEditor.
		//MF_ASSERT( entry.maxAvailableStreamingMip == 
		//	requestEntry.maxAvailableStreamingMip );
	}

	// Push the new texture:
	entry.pTexture->assignTexture( pNewTexture );
	entry.currentMip = requestEntry.requestedMip;

	// Update the requested mip for this texture such that 
	// new streaming requests will be submitted now
	entry.requestedMip = requestEntry.requestedMip;

	// Update the debug texture on this object
	assignDebugTexture( entry );
}

void TextureStreamingManager::generateReport( 
	const BW::StringRef & filenamePrefix )
{
	SimpleMutexHolder smh( textureLock_ );

	BW::string filename = filenamePrefix.to_string() + "_streamingReport.csv";
	FILE* f = fopen( filename.c_str(), "wb+" );

	if (f)
	{

		fprintf( f, "TotalTextures memory (MB): %" PRIzu "\n",
			memoryUsageAllTextures() / 1024/ 1024 );
		fprintf( f, "ManagedTextures memory (MB): %" PRIzu "\n", 
			memoryUsageManagedTextures() / 
			1024 / 1024 );
		fprintf( f, "StreamingTextures memory (MB): %" PRIzu "\n", 
			memoryUsageStreaming() / 1024/ 1024 );
		fprintf( f, "CachedTextures memory (MB): %" PRIzu "\n",
			memoryUsageGPUCache() / 1024/ 1024 );

		fprintf( f, "Num Textures in cache: %" PRIzu "\n", numTexturesInCache() );
		fprintf( f, "\n\n" );

		fprintf( f, "Name;Type;Width;Height;CurrentMemUsage;\n" );

		const TextureManager::TextureMap & textures = 
			TextureManager::instance()->textureMap();
		
		for (TextureManager::TextureMap::const_iterator it = textures.begin();
			it != textures.end(); ++it)
		{
			BaseTexture* pTexture = it->second ;
			fprintf( f, "%s;%d;%d;%d;%d\n",
				pTexture->resourceID().c_str(),
				pTexture->type(),
				pTexture->width(), pTexture->height(),
				pTexture->textureMemoryUsed()
				);
		}
		
		// Now print out a bunch of data about the streaming textures
		fprintf( f, "\nStreaming Textures:\n" );
		fprintf( f, "Name;Width;Height;MaxAvMip;MinAvMip;CurrentMip;TargetMip;"
			"IdealMip;CurrentMemUsage;\n" );

		for (StreamingTextureEntryVector::iterator iter = 
			streamingTextures_.begin(), end = streamingTextures_.end();
			iter != end; ++iter)
		{
			StreamingTextureEntry & entry = *iter;

			fprintf( f, "%s;%d;%d;%d;%d;%d;%d;%d;%d\n",
				entry.pTexture->resourceID().c_str(),
				entry.width,
				entry.height,
				entry.maxAvailableMip,
				entry.minAvailableMip,
				entry.currentMip,
				entry.targetMip,
				entry.idealMip,
				entry.pTexture->textureMemoryUsed()
				);
		}

		// Now print out info about all textures in the reuse list
		Moo::rc().textureReuseCache().generateReport( f );

		fclose( f );
	}
}


// ============================
// TextureStreamingUsageCollector section
// ============================
TextureStreamingManager::TextureStreamingUsageCollector::TextureStreamingUsageCollector( 
	TextureStreamingManager* pManager )
	: pManager_( pManager )
{

}


void TextureStreamingManager::TextureStreamingUsageCollector::submitTextureRequirements( 
	BaseTexture* pTexture, 
	uint32 requiredMip,
	uint32 numPixelsAssignedOnScreen,
	float angleToFrustumRadians )
{
	// Cast the texture to a streaming texture. This can be assumed, due to
	// the nature of the usage registration process.
	StreamingTexture* pStreamingTexture = 
		reinterpret_cast< StreamingTexture * >( pTexture );
	MF_ASSERT( pTexture->type() == BaseTexture::STREAMING );
	MF_ASSERT( dynamic_cast< StreamingTexture * >( pTexture ) != NULL );

	StreamingTextureHandle texHandle = 
		pStreamingTexture->streamingTextureHandle();
	MF_ASSERT( texHandle.isValid() );
	StreamingTextureEntry& entry = pManager_->streamingTextures_[texHandle];

	// Update the ideal mip
	if (entry.idealMip == INVALID_MIP)
	{
		entry.idealMip = requiredMip;
	}
	else
	{
		entry.idealMip = max( entry.idealMip, requiredMip );
	}

	entry.pixelsAssignedOnScreen += numPixelsAssignedOnScreen;

	entry.minAngleToFrustum = min(entry.minAngleToFrustum, 
		angleToFrustumRadians);
}


// ============================
// UvDensityTriGenFunctor section
// ============================

UvDensityTriGenFunctor::UvDensityTriGenFunctor(
	const PositionDataAccessor & positionDataAccessor,
	const TexcoordDataAccessor & texcoordDataAccessor )
{
	initialise();

	posData_ = positionDataAccessor;
	uvData_ = texcoordDataAccessor;
}

UvDensityTriGenFunctor::UvDensityTriGenFunctor( 
	const VertexFormat & format, const void * buffer, 
	size_t bufferSize,  
	uint32 positionSemanticIndex, 
	uint32 texcoordSemanticIndex,
	uint32 streamIndex)
{
	initialise();

	posData_ = format.createProxyElementAccessor<
		Moo::VertexElement::SemanticType::POSITION>( 
			buffer, bufferSize, streamIndex, positionSemanticIndex );
	uvData_ = format.createProxyElementAccessor<
		Moo::VertexElement::SemanticType::TEXCOORD>( 
			buffer, bufferSize, streamIndex, texcoordSemanticIndex );
}

void UvDensityTriGenFunctor::initialise()
{
	trianglesProcessed_ = 0;
	trianglesSkipped_ = 0;
	minUvDensity_ = FLT_MAX,
	minWorldAreaThreshold_ = DEFAULT_MIN_WORLD_AREA_THRESHOLD;
	minUvAreaThreshold_ = DEFAULT_MIN_TEXEL_AREA_THRESHOLD;
}

bool UvDensityTriGenFunctor::operator()( 
	uint32 index1, 
	uint32 index2, 
	uint32 index3 )
{
	// index range check
	if (index1 >= posData_.size() ||
		index2 >= posData_.size() ||
		index3 >= posData_.size() ||
		index1 >= uvData_.size() ||
		index2 >= uvData_.size() ||
		index3 >= uvData_.size())
	{
		return false;
	}

	// Skip degenerate triangles by world area
	Vector3 v1 = posData_[index1];
	Vector3 v2 = posData_[index2];
	Vector3 v3 = posData_[index3];
	Moo::Triangle< Vector3 > worldTri( v1, v2, v3 );

	float worldArea = worldTri.area();
	if (worldArea < minWorldAreaThreshold_)
	{
		++trianglesSkipped_;
		return true;
	}

	// Skip degenerate triangles by texel area
	Vector2 uv1 = uvData_[index1];
	Vector2 uv2 = uvData_[index2];
	Vector2 uv3 = uvData_[index3];
	Moo::Triangle<Vector2> uvTri(uv1, uv2, uv3);

	float uvArea = uvTri.area();
	if (uvArea < minUvAreaThreshold_)
	{
		++trianglesSkipped_;
		return true;
	}

	float uvDensity = uvArea / worldArea;
	minUvDensity_ = min(minUvDensity_, uvDensity);
	++trianglesProcessed_;

	return true;
}

float UvDensityTriGenFunctor::getMinUvDensity() const 
{ 
	return minUvDensity_; 
}

uint32 UvDensityTriGenFunctor::getTrianglesProcessed() const 
{ 
	return trianglesProcessed_; 
}

uint32 UvDensityTriGenFunctor::getTrianglesSkipped() const 
{ 
	return trianglesSkipped_; 
}

bool UvDensityTriGenFunctor::isValid() const
{
	return (posData_.isValid() && uvData_.isValid());
}

float UvDensityTriGenFunctor::getMinWorldAreaThreshold() const
{
	return minWorldAreaThreshold_;
}

float UvDensityTriGenFunctor::getMinUvAreaThreshold() const
{
	return minUvAreaThreshold_;
}

void UvDensityTriGenFunctor::setMinWorldAreaThreshold( 
	float minWorldAreaThreshold )
{
	MF_ASSERT( minWorldAreaThreshold >= 0.0f );
	minWorldAreaThreshold_ = minWorldAreaThreshold;
}

void UvDensityTriGenFunctor::setMinUvAreaThreshold( 
	float minTexelAreaThreshold )
{
	MF_ASSERT( minTexelAreaThreshold >= 0.0f );
	minUvAreaThreshold_ = minTexelAreaThreshold;
}

const float UvDensityTriGenFunctor::DEFAULT_MIN_WORLD_AREA_THRESHOLD = 
	0.0001f;
const float UvDensityTriGenFunctor::DEFAULT_MIN_TEXEL_AREA_THRESHOLD = 
	0.0001f;


// ============================
// Viewpoint section
// ============================

TextureStreamingManager::ViewpointData::ViewpointData() : 
	priorityBoost( 1.0f ), 
	requirementBoost( 1.0f ), 
	fov( 90.0f ), 
	resolution( 100.0f )
{	
}

TextureStreamingManager::Viewpoint::Viewpoint() : 
	manager_( NULL )
{

}

TextureStreamingManager::Viewpoint::~Viewpoint()
{
	// Viewpoint must be unregistered before destruction
	MF_ASSERT( manager_ == NULL );
}

void TextureStreamingManager::Viewpoint::manager( 
	TextureStreamingManager * val )
{
	manager_ = val;
}

TextureStreamingManager * TextureStreamingManager::Viewpoint::manager() const
{
	return manager_;
}

void TextureStreamingManager::Viewpoint::unregister()
{
	if (manager_)
	{
		manager_->unregisterViewpoint( this );
	}
	// Texture streaming manager should have assigned NULL to the manager
	// upon unregistration
	MF_ASSERT( manager_ == NULL );
}

// ============================
// StreamingTextureLoadTask section
// ============================
TextureStreamingManager::StreamingTextureLoadTask::StreamingTextureLoadTask( 
	TextureStreamingManager * pManager, StreamingTexture * pTexture, 
	size_t bandwidthEstimate ) : 
		pTexture_( pTexture ), 
		pManager_( pManager ),
		state_( INITIALIZE ), 
		pTargetTexture_( NULL )
{
	bandwidthEstimate_ = bandwidthEstimate;
}

TextureStreamingManager::StreamingTextureLoadTask::~StreamingTextureLoadTask()
{
	// Incase the task is destroyed before it gets to be completed:
	// Happens if streaming when application closed.
	if (state_ > LOCK && state_ <= UNLOCK)
	{
		unlockSurfaces();
	}
	if ( state_ < COMPLETE )
	{
		// Force notifying the manager that we've finished.
		doComplete();
	}
}

const TextureStreamingManager::StreamingTextureRequest & 
	TextureStreamingManager::StreamingTextureLoadTask::requestEntry() const
{
	return requestEntry_;
}

DX::Texture * 
	TextureStreamingManager::StreamingTextureLoadTask::targetTexture() const
{
	return pTargetTexture_.pComObject();
}

void TextureStreamingManager::StreamingTextureLoadTask::initialize()
{
	MF_ASSERT( state_ == INITIALIZE );
	
	// Determine what we need to achieve:
	// Upgrade = full process
	// Downgrade = no streaming or locks, just jump to copying shared mips 
	// (in the unlock phase).
		
	if (!pManager_->getStreamingRequest( pTexture_.get(), requestEntry_ ))
	{
		// This texture request is invalid
		state_ = COMPLETE;
		return;
	}
	MF_ASSERT( requestEntry_.pTexture == pTexture_.get() );

	// Determine from the entry what needs to be done:
	if (requestEntry_.currentMip == requestEntry_.requestedMip)
	{
		// The streaming request has been cancelled, so don't do anything
		state_ = COMPLETE;
		return;
	}

	if (requestEntry_.currentMip < requestEntry_.requestedMip ||
		requestEntry_.currentMip == INVALID_MIP)
	{
		// An upgrade, go through the whole process
		state_ = LOCK;
	}
	else
	{
		// A downgrade, so just copy existing mips
		state_ = UNLOCK;
	}
}

TextureStreamingManager::StreamingTextureLoadTask::QueueType
	TextureStreamingManager::StreamingTextureLoadTask::doQueue()
{
	if (state_ == INITIALIZE)
	{
		// Work out what state to jump to
		initialize();
		MF_ASSERT( state_ != INITIALIZE );
	}

	switch (state_)
	{
	case STREAM:
		return BACKGROUND_THREAD;

	case LOCK:
	case UNLOCK:
	default:
		return MAIN_THREAD;
	}
}

bool TextureStreamingManager::StreamingTextureLoadTask::doStreaming( 
	QueueType currentQueue )
{
	MF_ASSERT( state_ != INITIALIZE );

	switch (state_)
	{
	case LOCK:
		{
			MF_ASSERT( currentQueue == MAIN_THREAD );

			createTargetTexture();

			// Detect failure to create the texture.
			// May be because the format of the texture
			// has now changed since first loading and streaming
			// is no longer supported.
			if (!pTargetTexture_)
			{
				ERROR_MSG( "Failed to update streaming texture: %s. "
					"Format may have changed on disk and no longer "
					"support streaming.\n", 
					pTexture_->resourceID().c_str() );
				state_ = COMPLETE;
				return true;
			}

			if (!lockSurfaces())
			{
				ERROR_MSG( "Failed to lock streaming texture: %s.\n",
					pTexture_->resourceID().c_str() );
				state_ = COMPLETE;
				return true;
			}
		}
		break;

	case STREAM:
		{
			// We should only be here if we're upgrading the texture
			MF_ASSERT( requestEntry_.currentMip < requestEntry_.requestedMip ||
				requestEntry_.currentMip == INVALID_MIP );
			
			// Perform load from disk into locked surfaces
			uint32 numBytesLoaded = loadMipsFromDisk();
			reportBandwidthUsed( numBytesLoaded );
		}
		break;

	case UNLOCK:
		{
			MF_ASSERT( currentQueue == MAIN_THREAD );	

			if (!pTargetTexture_)
			{
				// Create texture for downgrade
				createTargetTexture();
				MF_ASSERT( pTargetTexture_ );
			}
			else
			{
				// Unlock surfaces from upgrade streaming
				unlockSurfaces();
			}

			// Copy across any mips that were in the original texture
			copySharedMips();
		}
		break;

	default:
		// Report completion
		state_ = COMPLETE;
		return true;
	}

	// Move onto the next state
	state_ = static_cast< State >( state_ + 1 );
	return (state_ == COMPLETE);
}

void TextureStreamingManager::StreamingTextureLoadTask::doComplete()
{
	bool isImmediateTask = streamingManager_ == NULL;
	
	if (!isImmediateTask)
	{
		// Tell the streaming manager the task has been completed
		pManager_->reportStreamingCompleted(
			pTexture_.get(),
			pTargetTexture_.pComObject(),
			requestEntry_ );
	}
}

size_t TextureStreamingManager::StreamingTextureLoadTask::doEstimateBandwidth()
{
	// The estimation of bandwidth is defined in the constructor
	return bandwidthEstimate_;
}

bool TextureStreamingManager::StreamingTextureLoadTask::createTargetTexture()
{
	StreamingTextureLoader::HeaderInfo header;

	if (requestEntry_.currentMip == INVALID_MIP)
	{
		// We need to load all the relevant header information out of the file.
		FileStreamerPtr pStream = 
			BWResource::instance().fileSystem()->streamFile( 
			requestEntry_.pTexture->resourceID() );
		if (!pStream)
		{
			return false;
		}

		// Load the header from the stream
		if (!StreamingTextureLoader::loadHeaderFromStream( pStream.get(), 
			header ))
		{
			return false;
		}

		MF_ASSERT( header.isFlatTexture_ );
		
		requestEntry_.width = header.width_;
		requestEntry_.height = header.height_;
		requestEntry_.maxAvailableMip = 
			calculateMaxMips( requestEntry_.width, requestEntry_.height );
		
		MF_ASSERT( (header.numMipLevels_ - 1) <= 
			requestEntry_.maxAvailableMip);
		
		requestEntry_.minAvailableMip = 
			requestEntry_.maxAvailableMip - (header.numMipLevels_ - 1);
		requestEntry_.format = header.format_;
		requestEntry_.minAvailableStreamingMip = requestEntry_.minAvailableMip;
		requestEntry_.maxAvailableStreamingMip = 
			requestEntry_.maxAvailableMip - requestEntry_.pTexture->mipSkip();

		if (DDSHelper::isDXTFormat( requestEntry_.format ))
		{
			// Our minimum mip to ever drop back to will be the lowest multiple
			// of 4. But the other mips are allowed to back it up, just we'll 
			// never strip away these due to limitations inside copyMipChain
			while ((requestEntry_.width >> (requestEntry_.maxAvailableMip - requestEntry_.minAvailableStreamingMip) < 4 ||
				requestEntry_.height >> (requestEntry_.maxAvailableMip - requestEntry_.minAvailableStreamingMip) < 4) &&
				requestEntry_.maxAvailableMip > requestEntry_.minAvailableStreamingMip)
			{
				requestEntry_.minAvailableStreamingMip++;
			}
		}
		requestEntry_.maxAvailableStreamingMip = 
			max(requestEntry_.maxAvailableStreamingMip, 
				requestEntry_.minAvailableStreamingMip);

		uint32 mipsToInitiallySkip = pManager_->initialLoadMipSkip();
		if (mipsToInitiallySkip <= requestEntry_.maxAvailableStreamingMip)
		{
			requestEntry_.requestedMip = 
				requestEntry_.maxAvailableStreamingMip - mipsToInitiallySkip;
		}
		else
		{
			requestEntry_.requestedMip = requestEntry_.maxAvailableStreamingMip;
		}
	}
	else
	{
		header.width_ = requestEntry_.width;
		header.height_ = requestEntry_.height;
		header.valid_ = true;
		header.numMipLevels_ = requestEntry_.maxAvailableMip - 
			requestEntry_.minAvailableMip + 1;
		header.format_ = requestEntry_.format;
		header.isFlatTexture_ = false;
	}

	uint32 targetMipLevel = requestEntry_.requestedMip;
	targetMipLevel = Math::clamp( requestEntry_.minAvailableStreamingMip, 
		targetMipLevel, requestEntry_.maxAvailableStreamingMip );
	requestEntry_.requestedMip = targetMipLevel;

	uint32 totalNumMips = targetMipLevel - requestEntry_.minAvailableMip + 1;

	StreamingTextureLoader::LoadOptions options;
	options.mipSkip_ = requestEntry_.maxAvailableMip - targetMipLevel;
	options.numMips_ = totalNumMips;

	StreamingTextureLoader::LoadResult result;
	if (!StreamingTextureLoader::createTexture( header, options, result ))
	{
		return false;
	}

	pTargetTexture_ = result.texture2D_;
	return true;
}

uint32 TextureStreamingManager::StreamingTextureLoadTask::copySharedMips()
{
	ComObjectWrap< DeviceTexture > origTexture = pTexture_->assignedTexture();
	if (!origTexture ||
		requestEntry_.currentMip == INVALID_MIP)
	{
		return 0;
	}

	// Work out where to copy from/to and how much to copy
	uint32 srcMipStart = 0;
	uint32 dstMipStart = 0;
	uint32 numToCopy = 0;

	// Detect if upgrading or downgrading:
	if (requestEntry_.requestedMip > requestEntry_.currentMip ||
		requestEntry_.currentMip == INVALID_MIP)
	{
		// Upgrade
		dstMipStart = numMipsToLoad();
		srcMipStart = 0;
		numToCopy = origTexture->GetLevelCount();
	}
	else
	{
		// Downgrade
		dstMipStart = 0;
		srcMipStart = requestEntry_.currentMip - requestEntry_.requestedMip;
		numToCopy = requestEntry_.requestedMip - 
			requestEntry_.minAvailableMip + 1;
	}

	// Attempt to copy mips from original texture to new one
	if (!StreamingTextureLoader::copyMipChain( 
		pTargetTexture_.pComObject(),
		dstMipStart,
		static_cast< DX::Texture * >( origTexture.pComObject() ),
		srcMipStart,
		numToCopy))
	{
		return 0;
	}

	//TODO: Work out how much data has just been transfered 
	return 0;
}

uint32 TextureStreamingManager::StreamingTextureLoadTask::numMipsToLoad() const
{
	uint32 newLoadedMips = requestEntry_.requestedMip - 
		requestEntry_.minAvailableMip + 1;
	if (requestEntry_.currentMip != INVALID_MIP)
	{
		newLoadedMips = requestEntry_.requestedMip - 
			requestEntry_.currentMip;
	}
	return newLoadedMips;
}

bool TextureStreamingManager::StreamingTextureLoadTask::lockSurfaces()
{
	// We are only updating the surfaces on the top of the mip chain.
	// But we need to calculate how many of them we're updating.
	uint32 numMipsToLock = numMipsToLoad();

	textureLock_.init( pTargetTexture_ );
	for (uint32 index = 0; index < numMipsToLock; ++index)
	{
		D3DLOCKED_RECT lockedRect;
		HRESULT hr = textureLock_.lockRect( index, &lockedRect, NULL, 0 );
		if (SUCCEEDED( hr ))
		{
			lockedRects_.push_back( lockedRect );
		}
		else
		{
			// We couldn't lock all the surfaces, so clean up the incomplete operation.
			unlockSurfaces();
			WARNING_MSG( 
				"TextureStreamingManager::StreamingTextureLoadTask::lockSurfaces: %s\n",
				DX::errorAsString( hr ).c_str() );
			return false;
		}
		
	}
	return true;
}

void TextureStreamingManager::StreamingTextureLoadTask::unlockSurfaces()
{
	for (uint32 index = 0; index < lockedRects_.size(); ++index)
	{
		textureLock_.unlockRect( index );
	}

	lockedRects_.clear();
	textureLock_.flush();
	textureLock_.fini();
}

uint32 TextureStreamingManager::StreamingTextureLoadTask::loadMipsFromDisk()
{
	uint32 targetMipLevel = requestEntry_.requestedMip;

	StreamingTextureLoader::LoadResult result;
	result.texture2D_ = pTargetTexture_;

	StreamingTextureLoader::HeaderInfo header;
	header.width_ = requestEntry_.width;
	header.height_ = requestEntry_.height;
	header.valid_ = true;
	header.numMipLevels_ = requestEntry_.maxAvailableMip - 
		requestEntry_.minAvailableMip + 1;
	header.format_ = requestEntry_.format;
	header.isFlatTexture_ = false;

	StreamingTextureLoader::LoadOptions options;
	options.mipSkip_ = requestEntry_.maxAvailableMip - targetMipLevel;
	options.numMips_ = numMipsToLoad();

	// We need to load all the relevant header information out of the file.
	MF_ASSERT( pTexture_.get() == requestEntry_.pTexture );
	MF_ASSERT( requestEntry_.pTexture );
	FileStreamerPtr pStream = 
		BWResource::instance().fileSystem()->streamFile( 
		requestEntry_.pTexture->resourceID() );
	if (!pStream)
	{
		return 0;
	}

	MF_ASSERT( header.valid_ );
	MF_ASSERT( result.texture2D_.hasComObject() );

	// Calculate the amount of data to skip in the file depending on the 
	// number of mipmaps we want to skip
	uint32 w = header.width_;
	uint32 h = header.height_;
	// Magic number + DDS header
	uint32 fileSkip = sizeof(DWORD) + sizeof(DDS_HEADER); 
	for (uint32 i = 0; i < options.mipSkip_ && i < header.numMipLevels_; i++)
	{
		UINT skipBytes = 0;
		DDSHelper::getSurfaceInfo( w, h, header.format_, &skipBytes, NULL, 
			NULL );
		w = std::max<uint32>( 1, w >> 1 );
		h = std::max<uint32>( 1, h >> 1 );

		fileSkip += skipBytes;
	}

	if (fileSkip > 0)
	{
		pStream->skip( fileSkip );
	}

	// Calculate the number of mips to load
	uint32 numMipsToLoad = options.numMips_;

	// Load up the mipmaps
	size_t numBytesLoaded = 0;
	for (uint32 index = 0; index < numMipsToLoad; index++)
	{
		UINT mipBytes = 0;
		UINT rowSize = 0;
		UINT rowCount = 0;
	
		D3DLOCKED_RECT& lockedRect = lockedRects_[index];;

		DDSHelper::getSurfaceInfo( w, h, header.format_, 
			&mipBytes, &rowSize, &rowCount );

		bool bSuccessRead = true;

		if (lockedRect.Pitch * rowCount == mipBytes)
		{
			size_t bytesRead = pStream->read( mipBytes, lockedRect.pBits );
			bSuccessRead = bSuccessRead && (mipBytes == bytesRead);
			numBytesLoaded += bytesRead;
		}
		else
		{
			uint8 * dest = static_cast< uint8 * >( lockedRect.pBits );
			for (uint32 j = 0; j < rowCount && bSuccessRead; j++)
			{
				size_t bytesRead = pStream->read( rowSize, dest );
				bSuccessRead = bSuccessRead && (rowSize == bytesRead);
				dest += lockedRect.Pitch;
			}
			numBytesLoaded += rowSize * rowCount;
		}

		w = std::max<uint32>( 1, w >> 1 );
		h = std::max<uint32>( 1, h >> 1 );

		if (!bSuccessRead)
		{
			// dds file is corrupted or just ended too early ?
			break;
		}
	}

	return static_cast< uint32 >( numBytesLoaded );
}

// ============================
// StreamingTextureEntry section
// ============================

TextureStreamingManager::StreamingTextureEntry::StreamingTextureEntry() : 
	pTexture( NULL ), 
	currentMip( INVALID_MIP ), 
	targetMip( 0 ), 
	requestedMip( INVALID_MIP ),
	streamingActive( false ),
	decayingBiasActive( false ),
	sacrificedDetail( 0 ),
	width( 0 ),
	height( 0 ),
	minAvailableMip( 0 ),
	minAvailableStreamingMip( 0 ),
	maxAvailableMip( 0 ),
	maxAvailableStreamingMip( 0 ),
	format( D3DFMT_UNKNOWN ), 
	streamingPriority( 0 ),
	idealAvailableMip( 0 ),
	idealMip( 0 ), 
	forcedMip( INVALID_MIP ),
	pixelsAssignedOnScreen( 0 ),
	minAngleToFrustum( 0.0f )
{

}

// ============================
// StreamingTextureEvaluationTask section
// ============================

TextureStreamingManager::StreamingTextureEvaluationTask::
	StreamingTextureEvaluationTask( TextureStreamingManager * pManager ) : 
	BackgroundTask( "StreamingTextureEvaluationTask" ), 
	pManager_( pManager ), 
	queued_( false )
{

}

bool TextureStreamingManager::StreamingTextureEvaluationTask::queue()
{
	SimpleMutexHolder smh( queuedLock_ );
	if (queued_)
	{
		return false;
	}
	else
	{
		queued_ = true;
		BgTaskManager::instance().addBackgroundTask( this );
		return true;
	}
}

void TextureStreamingManager::StreamingTextureEvaluationTask::doBackgroundTask( 
	TaskManager & mgr )
{
	SimpleMutexHolder smh( unregisterManagerLock_ );

	if (pManager_)
	{
		pManager_->evaluateStreaming();
	}
	
	{
		SimpleMutexHolder smh( queuedLock_ );
		queued_ = false;
	}
}

void TextureStreamingManager::StreamingTextureEvaluationTask::manager(
	TextureStreamingManager * pManager )
{
	SimpleMutexHolder smh( unregisterManagerLock_ );

	pManager_ = pManager;
}

// ============================
// Debug textures section
// ============================
bool DebugTextures::DebugTextureSet_Key::operator <( 
	const DebugTextureSet_Key & other )const
{
	if (debugSet != other.debugSet)
	{
		return debugSet < other.debugSet;
	}

	if (width != other.width)
	{
		return width < other.width;
	}

	if (height != other.height)
	{
		return height < other.height;
	}

	if (skipLevels != other.skipLevels)
	{
		return skipLevels < other.skipLevels;
	}
	return false;
}

BaseTexture * DebugTextures::getDebugTexture( 
	DebugTextures::TextureSet debugSet, BaseTexture * pTex )
{
	return getDebugTexture( 
		debugSet, 
		pTex->width(), 
		pTex->height(), 
		pTex->mipSkip() );
}

BaseTexture * DebugTextures::getDebugTexture( 
	DebugTextures::TextureSet debugSet, 
	uint32 width, uint32 height, uint32 skipLevels )
{
	// Find this texture:
	DebugTextureSet_Key debugRequest;
	debugRequest.debugSet = debugSet;
	debugRequest.width = width;
	debugRequest.height = height;
	debugRequest.skipLevels = skipLevels;

	DebugTextureMap::iterator findResult = 
		debugTextureSet_.find( debugRequest );
	if (findResult == debugTextureSet_.end())
	{
		return createDebugTexture( debugRequest );
	}
	return findResult->second.get();
}

BaseTexture * DebugTextures::createDebugTexture( 
	const DebugTextureSet_Key & debugRequest )
{
	static const uint32 default_colorArray[] = {
		0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFF808080};
	static const uint32 reverse_colorArray[] = {
		0xFF808080, 0xFF0000FF, 0xFF00FF00, 0xFFFF0000};
	static const uint32 size_colorArray[] = {
		0xFF808080, 0xFF000000, 0xFFFFFFFF, 0xFF0000FF,
		0xFF00FFFF, 0xFF00FF00, 0xFFFFFF00, 0xFFFF0000, 0xFFFF00FF};

	static const uint32 default_colorArrayLen = 
		ARRAY_SIZE( default_colorArray );
	static const uint32 reverse_colorArrayLen = 
		ARRAY_SIZE( reverse_colorArray );
	static const uint32 size_colorArrayLen = 
		ARRAY_SIZE( size_colorArray );

	const uint32 * pColorArray = &default_colorArray[0];
	uint32 colorArrayLen = default_colorArrayLen;
	int width = debugRequest.width;
	int height = debugRequest.height;
	switch (debugRequest.debugSet)
	{
	case DEBUG_TS_CURRENTWORKINGSET:
		{
			width = std::max( 
				debugRequest.width >> debugRequest.skipLevels, 1u );
			height = std::max( 
				debugRequest.height >> debugRequest.skipLevels, 1u );
		}
		break;
	case DEBUG_TS_MISSINGDETAIL:
		{
			width = std::max( 
				(debugRequest.width << 1) >> debugRequest.skipLevels, 1u );
			height = std::max( 
				(debugRequest.height << 1) >> debugRequest.skipLevels, 1u );
		}
		break;
	case DEBUG_TS_MISSINGAVAILABLEDETAIL:
		{
			// Only show red if there is more detail available on disk
			if (debugRequest.skipLevels != 0)
			{
				width = std::max( 
					(debugRequest.width << 1) >> debugRequest.skipLevels, 1u );
				height = std::max( 
					(debugRequest.height << 1) >> debugRequest.skipLevels, 1u );
			}
			else
			{
				width = debugRequest.width;
				height = debugRequest.height;

				pColorArray = &default_colorArray[1];
				colorArrayLen = default_colorArrayLen - 1;
			}
		}
		break;
	case DEBUG_TS_NUM_MIPS_SKIPPED:
		{
			width = 1;
			height = 1;

			uint32 skipColorIndex = 
				min( debugRequest.skipLevels, default_colorArrayLen - 1 );

			pColorArray = &default_colorArray[skipColorIndex];
			colorArrayLen = 1;
		}
		break;
	case DEBUG_TS_SACRIFICED_DETAIL:
		{
			width = 1;
			height = 1;

			uint32 skipColorIndex = 
				min( debugRequest.skipLevels, reverse_colorArrayLen - 1 );

			pColorArray = &reverse_colorArray[skipColorIndex];
			colorArrayLen = 1;
		}
		break;
	case DEBUG_TS_TEXTURE_SIZE:
		{
			width = 1;
			height = 1;

			const uint32 maxValue = 12;
			const uint32 minValue = 4;

			uint32 maxMipLevel = BW::log2ceil( debugRequest.width );
			maxMipLevel = Math::clamp( minValue, maxMipLevel, maxValue ) - 
				minValue;
			uint32 skipColorIndex = min( maxMipLevel, size_colorArrayLen - 1 );

			pColorArray = &size_colorArray[skipColorIndex];
			colorArrayLen = 1;
		}
		break;
	case DEBUG_TS_CURRENT_TEXTURE_SIZE:
		{
			width = 1;
			height = 1;

			const uint32 maxValue = 12;
			const uint32 minValue = 4;

			uint32 maxMipLevel = BW::log2ceil( debugRequest.width ) - 
				debugRequest.skipLevels;
			maxMipLevel = Math::clamp( minValue, maxMipLevel, maxValue ) - 
				minValue;
			uint32 skipColorIndex = min( maxMipLevel, size_colorArrayLen - 1 );

			pColorArray = &size_colorArray[skipColorIndex];
			colorArrayLen = 1;
		}
		break;
	default:
		return NULL;
	}

	// Create a texture
	ImageTextureARGB * pTexture = new ImageTextureARGB( width, height );
	ImageTextureARGB::ImageType & image = pTexture->image();

	// Color it's levels according to the array
	for (uint32 level = 0; level < pTexture->levels(); ++level)
	{
		uint32 color = pColorArray[min( level, colorArrayLen - 1)];

		pTexture->lock( level );
		image.fill( color );
		pTexture->unlock();
	}

	debugTextureSet_.insert( std::make_pair( debugRequest, pTexture ) );
	return pTexture;
}

void DebugTextures::clear()
{
	debugTextureSet_.clear();
}

} // namespace Moo

BW_END_NAMESPACE

// texture_manager.cpp
