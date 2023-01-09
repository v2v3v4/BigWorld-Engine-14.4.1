#include "pch.hpp"

#include "compiled_space_mapping.hpp"
#include "compiled_space.hpp"
#include "loader.hpp"
#include "entity_udo_factory.hpp"
#include "entity_list_types.hpp"

#include "cstdmf/string_builder.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/profiler.hpp"

#include "scene/scene.hpp"

#include "resmgr/packed_section.hpp"
#include "moo/texture_streaming_scene_view.hpp"

#if ENABLE_ASSET_PIPE
#include "asset_pipeline/asset_client.hpp"
#endif


namespace BW {
namespace CompiledSpace {

class CompiledSpaceMapping::LoaderTask : public BackgroundTask
{
public:
	LoaderTask( CompiledSpaceMapping* pLoader, TaskManager & mgr ) : 
		BackgroundTask( "CompiledSpaceLoader" ),
		pLoader_( pLoader ),
		manager_( mgr ),
		completed_( false )
	{
		MF_ASSERT( pLoader_ != NULL );
	}

	virtual ~LoaderTask()
	{

	}

	virtual void doBackgroundTask( TaskManager & mgr )
	{
		MF_ASSERT( &manager_ == &mgr );
		pLoader_->loadBG();
		mgr.addMainThreadTask( this );

		semaphore_.push();
	}

	virtual void doMainThreadTask( TaskManager & mgr )
	{
		MF_ASSERT( &manager_ == &mgr );
		pLoader_->loadBGFinished();
		completed_ = true;
	}

	void wait()
	{
		MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );
		semaphore_.pull();
		manager_.tick();

		MF_ASSERT( completed_ );
	}

private:
	TaskManager& manager_;
	CompiledSpaceMapping* pLoader_;
	SimpleSemaphore semaphore_;
	bool completed_;
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
CompiledSpaceMapping::CompiledSpaceMapping( const BW::string& path,
		const Matrix& transform,
		const DataSectionPtr& pSettings,
		CompiledSpace& space,
		AssetClient* pAssetClient ) :
	path_( path ),
	transform_( transform ),
	pSettings_( pSettings ),
	state_( STATE_UNLOADED ),
	pAssetClient_( pAssetClient ),
	space_( space ),
	pTask_( NULL ),
	numFailures_( 0 ),
	loadStartTimestamp_( 0 )
{
	MF_ASSERT( pSettings != NULL );
}


// ----------------------------------------------------------------------------
CompiledSpaceMapping::~CompiledSpaceMapping()
{
	unloadSync();
}


// ----------------------------------------------------------------------------
void CompiledSpaceMapping::addLoader( ILoader* pLoader )
{
	MF_ASSERT( pLoader != NULL );
	MF_ASSERT( std::find( loaders_.begin(), loaders_.end(), pLoader ) == 
		loaders_.end() );

	loaders_.push_back( pLoader );
}


// ----------------------------------------------------------------------------
void CompiledSpaceMapping::removeLoader( ILoader* pLoader )
{
	MF_ASSERT( pLoader != NULL );
	std::remove( loaders_.begin(), loaders_.end(), pLoader );
}


// ----------------------------------------------------------------------------
bool CompiledSpaceMapping::loadAsync( TaskManager& taskMgr )
{
	MF_ASSERT( state_ == STATE_UNLOADED );
	if (state_ != STATE_UNLOADED)
	{
		return false;
	}

	state_ = STATE_LOADING;

	MF_ASSERT( pTask_ == NULL );
	pTask_ = new LoaderTask( this, taskMgr );
	taskMgr.addBackgroundTask( pTask_ );

	loadStartTimestamp_ = timestamp();

	return true;
}


// ----------------------------------------------------------------------------
void CompiledSpaceMapping::loadBG()
{
	PROFILER_SCOPED( CompiledSpaceMapping_loadBG );

	MF_ASSERT( state_ == STATE_LOADING );
	this->ensureSpaceCompiled();

	char buffer[BW_MAX_PATH];
	StringBuilder spaceBinaryID( buffer, sizeof(buffer) );
	spaceBinaryID.append( path_ );
	spaceBinaryID.append( "/" );
	spaceBinaryID.append( "space.bin" );

	if (!reader_.open( spaceBinaryID.string() ))
	{
		ASSET_MSG( "Failed to open BinaryFormat '%s'\n",
			spaceBinaryID.string() );
		++numFailures_;
		return;
	}

	//
	// Absolutely required
	if (!settings_.read( reader_ ))
	{
		ASSET_MSG( "Failed to load settings for compiled space '%s'.\n",
			path_.c_str() );

		++numFailures_;
		return;
	}

	if (!stringTable_.read( reader_ ))
	{
		ASSET_MSG( "Failed to load string table for compiled space '%s'.\n",
			path_.c_str() );

		++numFailures_;
		return;
	}

	if (!staticGeometry_.read( reader_ ))
	{
		ASSET_MSG( "Failed to load static geometry for compiled space '%s'.\n",
			path_.c_str() );

		++numFailures_;
		return;
	}

	if (!assetList_.read( reader_ ))
	{
		ASSET_MSG( "Failed to load asset list for compiled space '%s'.\n",
			path_.c_str() );

		++numFailures_;
		return;
	}
	
	assetList_.preloadAssets( stringTable_ );


	//
	// Go off to our registered loaders
	for (Loaders::iterator it = loaders_.begin();
		it != loaders_.end(); ++it )
	{
		ILoader* pLoader = *it;
		if (!pLoader->loadFromSpace( &space_,
			reader_, pSettings_,
			transform_, stringTable_ ) && pLoader->loadMustSucceed())
		{
			++numFailures_;
		}
	}


	//
	// Entities/UDOs
	entities_.read( reader_, EntityListTypes::CLIENT_ENTITIES_MAGIC );
	userDataObjects_.read( reader_, EntityListTypes::UDOS_MAGIC );


	//
	// Tell asset list to free strong references. In theory, we could check
	// here if anything subsequently loaded is even using what's in the asset 
	// list as a sanity check (make sure we're not loading too much).
	assetList_.releasePreloadedAssets();

	space_.mappingLoadedBG( this );
}


// ----------------------------------------------------------------------------
void CompiledSpaceMapping::loadBGFinished()
{
	PROFILER_SCOPED( CompiledSpaceMapping_loadBGFinished );

	MF_ASSERT( state_ == STATE_LOADING );
	MF_ASSERT( pTask_ != NULL );
	pTask_ = NULL; // TaskManager owns and will delete

	if (numFailures_ == 0)
	{
		// Execute bind on all our loaders
		for (Loaders::iterator it = loaders_.begin();
			it != loaders_.end(); ++it )
		{
			ILoader* pLoader = *it;
			if (!pLoader->bind() && pLoader->loadMustSucceed())
			{
				++numFailures_;
			}
		}
	}

	// Deliberately update publicly exposed state on main thread as opposed
	// to loadBG in the background thread.
	if (numFailures_ == 0)
	{
		state_ = STATE_LOADED;

		space_.mappingLoaded( this );

		this->spawnEntities();
		this->spawnUserDataObjects();

		TRACE_MSG( "Loaded compiled space '%s' in %.4f sec.\n",
			path_.c_str(),
			double(timestamp() - loadStartTimestamp_) / stampsPerSecondD() );
	}
	else
	{
		state_ = STATE_FAILED_TO_LOAD;

		ASSET_MSG( "Failed to load compiled space '%s'.\n",
			path_.c_str() );
	}
}


// ----------------------------------------------------------------------------
void CompiledSpaceMapping::unloadSync()
{
	if (state_ == STATE_UNLOADED)
	{
		return;
	}

	if (pTask_)
	{
		pTask_->wait();
	}

	this->removeEntities();
	this->removeUserDataObjects();

	for (Loaders::iterator it = loaders_.begin();
		it != loaders_.end(); ++it )
	{
		ILoader* pLoader = *it;
		if (pLoader->loadStatus() != 0.0f)
		{
			pLoader->unload();
		}
	}

	staticGeometry_.close();
	assetList_.close();
	stringTable_.close();
	settings_.close();
	
	entities_.close();
	userDataObjects_.close();

	reader_.close();

	state_ = STATE_UNLOADED;
}


// ----------------------------------------------------------------------------
void CompiledSpaceMapping::spawnEntities()
{
	MF_ASSERT( entityInstances_.empty() );

	IEntityUDOFactory* pFactory = CompiledSpace::entityUDOFactory();
	if (!entities_.isValid() || !pFactory)
	{
		return;
	}

	DataSectionPtr pEntitiesDS = entities_.dataSection();

	entityInstances_.reserve( pEntitiesDS->countChildren() );
	for (int i = 0; i < pEntitiesDS->countChildren(); ++i)
	{
		DataSectionPtr pDS = pEntitiesDS->openChild( i );
		if (pDS)
		{
			entityInstances_.push_back(
				pFactory->createEntity( space_.id(), pDS ) );
		}
	}

	// Don't need to keep this mapped into memory anymore.
	entities_.close();
}


// ----------------------------------------------------------------------------
void CompiledSpaceMapping::removeEntities()
{
	if (entityInstances_.empty())
	{
		return;
	}

	IEntityUDOFactory* pFactory = CompiledSpace::entityUDOFactory();
	MF_ASSERT( pFactory != NULL );

	for (size_t i = 0; i < entityInstances_.size(); ++i)
	{
		if (entityInstances_[i])
		{
			pFactory->destroyEntity( entityInstances_[i] );
		}
	}

	entityInstances_.clear();
}


// ----------------------------------------------------------------------------
void CompiledSpaceMapping::spawnUserDataObjects()
{
	MF_ASSERT( userDataObjectInstances_.empty() );

	IEntityUDOFactory* pFactory = CompiledSpace::entityUDOFactory();
	if (!userDataObjects_.isValid() || !pFactory)
	{
		return;
	}

	DataSectionPtr pUserDataObjectsDS = userDataObjects_.dataSection();

	userDataObjectInstances_.reserve( pUserDataObjectsDS->countChildren() );
	for (int i = 0; i < pUserDataObjectsDS->countChildren(); ++i)
	{
		DataSectionPtr pDS = pUserDataObjectsDS->openChild( i );
		if (pDS)
		{
			userDataObjectInstances_.push_back( pFactory->createUDO( pDS ) );
		}
	}

	// Don't need to keep this mapped into memory anymore.
	userDataObjects_.close();
}


// ----------------------------------------------------------------------------
void CompiledSpaceMapping::removeUserDataObjects()
{
	if (userDataObjectInstances_.empty())
	{
		return;
	}

	IEntityUDOFactory* pFactory = CompiledSpace::entityUDOFactory();
	MF_ASSERT( pFactory != NULL );

	for (size_t i = 0; i < userDataObjectInstances_.size(); ++i)
	{
		if (userDataObjectInstances_[i])
		{
			pFactory->destroyUDO( userDataObjectInstances_[i] );
		}
	}

	userDataObjectInstances_.clear();
}


// ----------------------------------------------------------------------------
CompiledSpaceMapping::State CompiledSpaceMapping::state() const
{
	return state_;
}


// ----------------------------------------------------------------------------
float CompiledSpaceMapping::loadStatus() const
{
	if (state_ == STATE_FAILED_TO_LOAD ||
	    state_ == STATE_LOADED )
	{
		return 1.0;
	}
	else if (state_ == STATE_UNLOADED)
	{
		return 0.0f;
	}

	float sum = 0.f;
	int numSubsystems = 0;

	sum += staticGeometry_.percentLoaded();
	++numSubsystems;

	sum += assetList_.percentLoaded();
	++numSubsystems;

	for (Loaders::const_iterator it = loaders_.begin();
		it != loaders_.end(); ++it)
	{
		ILoader* pLoader = *it;
		sum += pLoader->loadStatus();
		++numSubsystems;
	}

	if (numSubsystems > 0)
	{
		return sum / (float)numSubsystems;
	}
	else
	{
		return 0.f;
	}
}


// ----------------------------------------------------------------------------
const AABB& CompiledSpaceMapping::boundingBox() const
{
	return settings_.boundingBox();
}


// ----------------------------------------------------------------------------
void CompiledSpaceMapping::ensureSpaceCompiled()
{
#if ENABLE_ASSET_PIPE
	if (pAssetClient_)
	{
		// Requesting space.settings will implicitly trigger a 
		// space compilation.
		pAssetClient_->requestAsset( path_ + "/space.bin", true );
	}
#endif // ENABLE_ASSET_PIPE
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
DefaultCompiledSpaceMapping::DefaultCompiledSpaceMapping( const BW::string& path,
	const Matrix& transform,
	const DataSectionPtr& pSettings,
	CompiledSpace& space,
	AssetClient* pAssetClient ) 
	: CompiledSpaceMapping( path, transform, pSettings, space, pAssetClient )
	, staticScene_( StaticSceneTypes::FORMAT_MAGIC[StaticSceneTypes::eDEFAULT] )
	, vloScene_( StaticSceneTypes::FORMAT_MAGIC[StaticSceneTypes::eVLO] )
	, particleLoader_( space.dynamicScene() )

{
	Moo::TextureStreamingSceneView* pTextureStreamingView = 
		space.scene().getView<Moo::TextureStreamingSceneView>();
	pTextureStreamingView->ignoreObjectsFrom( &staticScene_ );

	staticScene_.addTypeHandler( &staticModels_ );
#if SPEEDTREE_SUPPORT
	staticScene_.addTypeHandler( &speedTrees_ );
#endif
	staticScene_.addTypeHandler( &flares_ );
	staticScene_.addTypeHandler( &decals_ );

	vloScene_.addTypeHandler( &water_ );

	this->addLoader( &staticScene_ );
	this->addLoader( &vloScene_ );
	this->addLoader( &terrain2_ );
	this->addLoader( &particleLoader_ );
	this->addLoader( &lightScene_ );
	this->addLoader( &textureStreamingScene_ );
}


// ----------------------------------------------------------------------------
DefaultCompiledSpaceMapping::~DefaultCompiledSpaceMapping()
{
	unloadSync();

	Moo::TextureStreamingSceneView* pTextureStreamingView = 
		space_.scene().getView<Moo::TextureStreamingSceneView>();
	pTextureStreamingView->unignoreObjectsFrom( &staticScene_ );

	this->removeLoader( &staticScene_ );
	this->removeLoader( &vloScene_ );
	this->removeLoader( &terrain2_ );
	this->removeLoader( &particleLoader_ );
	this->removeLoader( &lightScene_ );
	this->removeLoader( &textureStreamingScene_ );
}


// ----------------------------------------------------------------------------
CompiledSpaceMapping* DefaultCompiledSpaceMapping::create( 
	const BW::string& path,
	const Matrix& transform,
	const DataSectionPtr& pSettings,
	CompiledSpace& space,
	AssetClient* pAssetClient  )
{
	return new DefaultCompiledSpaceMapping( path,
		transform,
		pSettings,
		space,
		pAssetClient );
}

} // namespace CompiledSpace
} // namespace BW
