#include "pch.hpp"

#include "compiled_space.hpp"
#include "compiled_space_factory.hpp"
#include "compiled_space_mapping.hpp"
#include "terrain2_scene_provider.hpp"
#include "light_scene_provider.hpp"

#include "py_attachment_entity_embodiment.hpp"
#include "py_model_obstacle_entity_embodiment.hpp"

#include "resmgr/bwresource.hpp"

#include "cstdmf/command_line.hpp"
#include "math/vector3.hpp"
#include "math/convex_hull.hpp"
#include "romp/enviro_minder.hpp"
#include "romp/time_of_day.hpp"

#include "scene/scene.hpp"
#include "scene/intersection_set.hpp"
#include "scene/draw_helpers.hpp"
#include "scene/light_scene_view.hpp"

#include "space/space_manager.hpp"
#include "space/deprecated_space_helpers.hpp"
#include "chunk/chunk_water.hpp"
#include "chunk/chunk_flare.hpp"
#include "duplo/py_splodge.hpp"
#include "moo/render_target.hpp"
#include "speedtree/speedtree_renderer.hpp"
#include "terrain/terrain_scene_view.hpp"
#include "space/space_romp_collider.hpp"
#include "scene/change_scene_view.hpp"

namespace BW {
namespace CompiledSpace {

	namespace {
		const char* SPACE_SETTING_FILE_NAME = "space.settings";
		IEntityUDOFactory* s_entityUDOFactory = NULL;
		const char* CMD_PARAM_SPACETYPE = "spaceType";
		const char* COMPILED_SPACE_TYPE_NAME = "COMPILED_SPACE";
	}

// ----------------------------------------------------------------------------
//static 
bool CompiledSpace::init(
		const DataSectionPtr& pConfigDS,
		IEntityUDOFactory* pEntityFactory,
		AssetClient* pAssetClient )
{
	MF_ASSERT( pConfigDS != NULL );
	MF_ASSERT( pEntityFactory != NULL );

	s_entityUDOFactory = pEntityFactory;

	// Fetch the configuration of the space from the type specified
	// in the Engine_config.xml file.
	bool isCompiledSpaceEnabled = 
		pConfigDS->readString( CMD_PARAM_SPACETYPE ) == COMPILED_SPACE_TYPE_NAME;

	// Override that setting from the command line if one was specified.
	LPSTR commandLine = ::GetCommandLineA();
	CommandLine cmd( commandLine );
	if (cmd.hasParam(CMD_PARAM_SPACETYPE))
	{
		const char * type = cmd.getParam( CMD_PARAM_SPACETYPE );
		isCompiledSpaceEnabled = 
			 strcmp( type, COMPILED_SPACE_TYPE_NAME ) == 0;
	}

	if (isCompiledSpaceEnabled)
	{
		INFO_MSG( "Using Compiled Spaces.\n" );
		SpaceManager::instance().factory(
			new CompiledSpaceFactory( pAssetClient ) );
		RompCollider::setDefaultCreationFuction(
			&SpaceRompCollider::create );
	}

	return isCompiledSpaceEnabled;
}


// ----------------------------------------------------------------------------
//static 
IEntityUDOFactory* CompiledSpace::entityUDOFactory()
{
	return s_entityUDOFactory;
}


// ----------------------------------------------------------------------------
CompiledSpace::CompiledSpace( SpaceID id, AssetClient* pAssetClient ) :
	ClientSpace( id ),
	pAssetClient_( pAssetClient ),
	enviroMinderScene_( id ),
	mappingCreator_( &DefaultCompiledSpaceMapping::create )
{
	dynamicScene_.setPartitionSizeHint( 50.0f );

	this->scene().addProvider( &dynamicScene_ );
	this->scene().addProvider( &dynamicLightScene_ );
	this->scene().addProvider( &enviroMinderScene_ );
	this->scene().addProvider( &shadowScene_ );

	PyAttachmentEntityEmbodiment::registerHandlers( this->scene() );
	PyModelObstacleEntityEmbodiment::registerHandlers( this->scene() );
	Terrain2SceneProvider::registerHandlers( this->scene() );
	StaticSceneProvider::registerHandlers( this->scene() );
	ParticleSystemLoader::registerHandlers( this->scene() );
	LightSceneProvider::registerHandlers( this->scene() );
}


// ----------------------------------------------------------------------------
CompiledSpace::~CompiledSpace()
{
	this->scene().removeProvider( &dynamicScene_ );
	this->scene().removeProvider( &dynamicLightScene_ );
	this->scene().removeProvider( &enviroMinderScene_ );
	this->scene().removeProvider( &shadowScene_ );
}


// ----------------------------------------------------------------------------
DynamicSceneProvider& CompiledSpace::dynamicScene()
{
	return dynamicScene_;
}

// ----------------------------------------------------------------------------
DynamicLightSceneProvider& CompiledSpace::dynamicLightScene()
{
	return dynamicLightScene_;
}


// ----------------------------------------------------------------------------
void CompiledSpace::mappingCreator( MappingCreator func )
{
	mappingCreator_ = func;
}


// ----------------------------------------------------------------------------
void CompiledSpace::mappingLoadedBG( CompiledSpaceMapping* pLoader )
{
	// This function is called on the background loading thread after
	// everything has been loaded but before it is passed back to the
	// main thread.
	TRACE_MSG( "CompiledSpace::mappingLoadedBG\n" );
}


// ----------------------------------------------------------------------------
void CompiledSpace::mappingLoaded( CompiledSpaceMapping* pLoader )
{
	TRACE_MSG( "CompiledSpace::mappingLoaded\n" );

	ChangeSceneView* pView = scene().getView<ChangeSceneView>();
	pView->notifyAreaLoaded( pLoader->boundingBox() );
	
	updateSpaceBounds();
}


// ----------------------------------------------------------------------------
void CompiledSpace::mappingUnloaded( CompiledSpaceMapping* pLoader )
{
	TRACE_MSG( "CompiledSpace::mappingUnloaded\n" );

	ChangeSceneView* pView = scene().getView<ChangeSceneView>();
	pView->notifyAreaUnloaded( pLoader->boundingBox() );

	updateSpaceBounds();
}


// ----------------------------------------------------------------------------
void CompiledSpace::updateSpaceBounds()
{
	// Iterate over all mappings and create a new bounding box
	AABB bb;
	for (size_t i = 0; i < mappings_.size(); ++i)
	{
		CompiledSpaceMapping* pLoader = mappings_[i].pLoader_;
		bb.addBounds( pLoader->boundingBox() );
	}

	if (bb == bounds_)
	{
		// Nothing has changed
		return;
	}
	bounds_ = bb;

	// Notify the dynamic scene and the shadow scene cache
	{
		shadowScene_.configure( bounds_, 100.0f );

		// Only update the dynamic scene if it has become larger
		// (no point wasting time if it became smaller).
		dynamicScene_.resizeScene( bounds_ );
	}
}

// ----------------------------------------------------------------------------
size_t CompiledSpace::mappingIDToIndex( GeometryMappingID id )
{
	for (size_t i = 0; i < mappings_.size(); ++i)
	{
		if (mappings_[i].mappingID_ == id)
		{
			return i;
		}
	}

	return size_t(-1);
}


// ----------------------------------------------------------------------------
bool CompiledSpace::doAddMapping( GeometryMappingID mappingID,
		Matrix& transform, const BW::string & path, 
		const SmartPointer< DataSection >& pSettings )
{
	MF_ASSERT( this->mappingIDToIndex( mappingID ) == size_t(-1) );

	DataSectionPtr pLoadedSettings;
	if (pSettings)
	{
		pLoadedSettings = pSettings;
	}
	else
	{
		pLoadedSettings = BWResource::openSection(
			path + "/" + SPACE_SETTING_FILE_NAME );
	}

	if (!pLoadedSettings)
	{
		ERROR_MSG( "CompiledSpace::addMapping: could not open '%s/%s'.\n",
			path.c_str(), SPACE_SETTING_FILE_NAME );
		return false;
	}

	CompiledSpaceMapping* pMapping = (*mappingCreator_)( 
		path, transform, pLoadedSettings, *this, pAssetClient_ );

	Mapping newMapping =
	{
		mappingID,
		pMapping
	};

	mappings_.push_back( newMapping );

	if (newMapping.pLoader_->loadAsync( FileIOTaskManager::instance() ))
	{
		// TODO: Would be good to move this on background thread, but
		// this is on par with chunky experience at least.
		enviroMinderScene_.loadEnvironment( pLoadedSettings );
		return true;
	}
	else
	{
		return false;
	}
}


// ----------------------------------------------------------------------------
float CompiledSpace::doGetLoadStatus( float /* distance */ ) const
{
	if (mappings_.empty())
	{
		return 0.f;
	}

	float sum = 0.f;
	for (Mappings::const_iterator it = mappings_.begin();
		it != mappings_.end(); ++it)
	{
		if (it->pLoader_)
		{
			sum += it->pLoader_->loadStatus();
		}
	}

	return sum / (float)mappings_.size();
}


// ----------------------------------------------------------------------------
void CompiledSpace::doDelMapping( GeometryMappingID mappingID )
{
	for (size_t i = 0; i < mappings_.size(); ++i)
	{
		if (mappings_[i].mappingID_ == mappingID)
		{
			bw_safe_delete( mappings_[i].pLoader_ );
			mappings_.erase( mappings_.begin() + i );
			break;
		}
	}
}


// ----------------------------------------------------------------------------
void CompiledSpace::doClear()
{
	// Make sure the dynamic providers are cleared out.
	dynamicLightScene_.clear();

	for (size_t i = 0; i < mappings_.size(); ++i)
	{
		bw_safe_delete( mappings_[i].pLoader_ );
	}

	mappings_.clear();
}


// ----------------------------------------------------------------------------
void CompiledSpace::doTick( float dTime )
{
	TickSceneView* pView = scene().getView<TickSceneView>();
	MF_ASSERT( pView != NULL );
	pView->tick( dTime );
}


// ----------------------------------------------------------------------------
void CompiledSpace::doUpdateAnimations( float dTime )
{
	TickSceneView* pView = scene().getView<TickSceneView>();
	MF_ASSERT( pView != NULL );
	pView->updateAnimations( dTime );
}


// ----------------------------------------------------------------------------
Vector3 CompiledSpace::doClampToBounds( const Vector3& position ) const
{
	AABB spaceBounds = doGetBounds();
	if (spaceBounds.insideOut())
	{
		return position;
	}

	Vector3 result( position );
	result.clamp( spaceBounds.minBounds(), spaceBounds.maxBounds() );
	return result;
}


// ----------------------------------------------------------------------------
AABB CompiledSpace::doGetBounds() const
{
	return bounds_;
}


// ----------------------------------------------------------------------------
float CompiledSpace::doCollide( const Vector3 & start, const Vector3 & end,
	CollisionCallback & cc ) const
{
	const CollisionSceneView* pView =
		this->scene().getView<CollisionSceneView>();
	MF_ASSERT( pView != NULL );

	return pView->collide( start, end, cc );
}


// ----------------------------------------------------------------------------
float CompiledSpace::doCollide( const WorldTriangle & triangle,
	const Vector3 & translation,
	CollisionCallback & cc ) const
{
	const CollisionSceneView* pView =
		this->scene().getView<CollisionSceneView>();
	MF_ASSERT( pView != NULL );

	return pView->collide( triangle, translation, cc );
}


// ----------------------------------------------------------------------------
float CompiledSpace::doFindTerrainHeight( const Vector3 & position ) const
{
	const Terrain::TerrainSceneView* pView =
		this->scene().getView<Terrain::TerrainSceneView>();
	MF_ASSERT( pView != NULL );

	return pView->findTerrainHeight( position );
}


// ----------------------------------------------------------------------------
const Moo::DirectionalLightPtr & CompiledSpace::doGetSunLight() const
{
	return enviroMinderScene_.sunLight();
}


// ----------------------------------------------------------------------------
const Moo::Colour & CompiledSpace::doGetAmbientLight() const
{
	return enviroMinderScene_.ambientLight();
}


// ----------------------------------------------------------------------------
const Moo::LightContainerPtr & CompiledSpace::doGetLights() const
{
	return enviroMinderScene_.lights();
}


// ----------------------------------------------------------------------------
bool CompiledSpace::doGetLightsInArea( const AABB& worldBB, 
	Moo::LightContainerPtr* allLights )
{
	Moo::LightContainerPtr & pLights = *allLights;
	if (! pLights)
	{
		pLights = new Moo::LightContainer;
	}
	const LightSceneView * lightView = this->scene().getView<LightSceneView>();
	MF_ASSERT( lightView );
	lightView->intersect( worldBB, *pLights );
	return true;
}

EnviroMinder & CompiledSpace::doEnviro()
{
	return enviroMinderScene_.enviroMinder();
}

} // namespace CompiledSpace
} // namespace BW
