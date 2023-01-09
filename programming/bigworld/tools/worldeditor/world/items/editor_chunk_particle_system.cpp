#include "pch.hpp"
#include "worldeditor/world/items/editor_chunk_particle_system.hpp"
#include "worldeditor/world/items/editor_chunk_substance.ipp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/editor/item_editor.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "appmgr/options.hpp"
#include "model/super_model.hpp"
#include "moo/geometrics.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/geometry_mapping.hpp"
#include "chunk/umbra_chunk_item.hpp"
#include "gizmo/undoredo.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/resource_cache.hpp"
#include "particle/meta_particle_system.hpp"
#include "particle/particle_system_manager.hpp"
#include <algorithm>


DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

static AutoConfigString s_chunkParticleSystemNotFoundModel( "system/notFoundModel" );	


BW::map<BW::string, BW::set<EditorChunkParticleSystem*> > EditorChunkParticleSystem::editorChunkParticleSystem_;

void EditorChunkParticleSystem::add( EditorChunkParticleSystem* system, const BW::string& filename )
{
	BW_GUARD;

	editorChunkParticleSystem_[ filename ].insert( system );
}

void EditorChunkParticleSystem::remove( EditorChunkParticleSystem* system )
{
	BW_GUARD;

	for( BW::map<BW::string, BW::set<EditorChunkParticleSystem*> >::iterator iter =
		editorChunkParticleSystem_.begin(); iter != editorChunkParticleSystem_.end(); ++iter )
	{
		BW::set<EditorChunkParticleSystem*>& particleSet = iter->second;
		for( BW::set<EditorChunkParticleSystem*>::iterator siter = particleSet.begin();
			siter != particleSet.end(); ++siter )
		{
			if( *siter == system )
			{
				particleSet.erase( siter );
				if( particleSet.size() )
					editorChunkParticleSystem_.erase( iter );
				return;
			}
		}
	}
}

void EditorChunkParticleSystem::reload( const BW::string& filename )
{
	BW_GUARD;

	BWResource::instance().purgeAll();
	ParticleSystemManager::instance().loader().clear();

	BW::set<EditorChunkParticleSystem*> particleSet =
		editorChunkParticleSystem_[ BWResource::dissolveFilename( filename ) ];
	for( BW::set<EditorChunkParticleSystem*>::iterator iter = particleSet.begin();
		iter != particleSet.end(); ++iter )
	{
		Chunk* myChunk = (*iter)->chunk();
		(*iter)->toss( NULL );
		(*iter)->load( BWResource::dissolveFilename( filename ) );
		(*iter)->toss( myChunk );
	}
}
// -----------------------------------------------------------------------------
// Section: EditorChunkParticleSystem
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EditorChunkParticleSystem::EditorChunkParticleSystem() :
	needsSyncInit_( false )
{
	BW_GUARD;

	psModel_ = Model::get( "resources/models/particle.model" );
	psModelSmall_ = Model::get( "resources/models/particle_small.model" );
	psBadModel_ = Model::get( s_chunkParticleSystemNotFoundModel.value() );
	ResourceCache::instance().addResource( psModel_ );
	ResourceCache::instance().addResource( psModelSmall_ );
	ResourceCache::instance().addResource( psBadModel_ );
}


/**
 *	Destructor.
 */
EditorChunkParticleSystem::~EditorChunkParticleSystem()
{
	BW_GUARD;

	remove( this );
}

ModelPtr EditorChunkParticleSystem::largeProxy()
{
	BW_GUARD;

	if( system_ == NULL )
	{
		return psBadModel_;
	}

	if ( overrideModelName_ == system_->helperModelName() )
	{
		return overrideModel_ != NULL ? overrideModel_ : psModel_;
	}
	overrideModelName_ = system_->helperModelName();
	overrideModel_ = Model::get( overrideModelName_ );
 	helperModelHardPointTransforms_.clear();
	system_->helperModelCenterOnHardPoint( (uint)(-1) );

	if( overrideModel_ != NULL )
	{
		BW::vector< BW::string > helperModelHardPointNames;
		MetaParticleSystem::getHardPointTransforms( overrideModel_, helperModelHardPointNames, helperModelHardPointTransforms_ );
	}
	return overrideModel_ != NULL ? overrideModel_ : psModel_;
}

void EditorChunkParticleSystem::tick( float dTime )
{
	BW_GUARD;

	int renderParticleProxy = OptionsParticleProxies::visible();
	int renderLargeProxy = OptionsParticleProxies::particlesLargeVisible();

	ModelPtr newModel;

	if (renderLargeProxy && renderParticleProxy) 
	{
		newModel = largeProxy();
	}
	else if (renderParticleProxy)
	{
		newModel = (system_ ? psModelSmall_ : psBadModel_);
	}

	if (currentModel_ != newModel)
	{
		needsSyncInit_ = true;
		if (pChunk_)
		{
			ChunkModelObstacle::instance( *pChunk_ ).delObstacles( this );
		}
		currentModel_ = newModel;
		if (pChunk_)
		{
			this->addAsObstacle();
		}
	}

	EditorChunkSubstance<ChunkParticles>::tick( dTime );

	if (needsSyncInit_)
	{
		#if UMBRA_ENABLE
		bw_safe_delete(pUmbraDrawItem_);

		if (!system_ && currentModel_)
		{
			// Grab the visibility bounding box
			BoundingBox bb = currentModel_->boundingBox();

			// Set up object transforms
			Matrix m = pChunk_->transform();
			m.preMultiply( localTransform_ );

			// Create the umbra chunk item
			UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem();
			pUmbraChunkItem->init( this, bb, m, pChunk_->getUmbraCell());
			pUmbraDrawItem_ = pUmbraChunkItem;

			this->updateUmbraLenders();
		}
		#endif	

		needsSyncInit_ = false;
	}
}


bool EditorChunkParticleSystem::edShouldDraw()
{
	BW_GUARD;

	if( !EditorChunkSubstance<ChunkParticles>::edShouldDraw() )
		return false;

	return OptionsScenery::particlesVisible();
}

void EditorChunkParticleSystem::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if (!edShouldDraw())
		return;
	
	ModelPtr model = reprModel();

	if (edIsTooDistant() && !WorldManager::instance().drawSelection())
	{
		// Set the proxy model to NULL if we are too far away.
		model = NULL;
	}

	if (WorldManager::instance().drawSelection() && model)
	{
		WorldManager::instance().registerDrawSelectionItem( this );

		// draw a some points near the centre of the reprModel, so the system
		// can be selected from the distance where the repr model might be
		// smaller than a pixel and fail to draw.
		Moo::rc().push();
		Moo::rc().world( chunk()->transform() );
		Moo::rc().preMultiply( edTransform() );
		// bias of half the size of the representation model's bounding box in
		// the vertical axis, because the object might be snapped to terrain
		// or another object, so the centre might be below something else.
		float bias = model->boundingBox().width() / 2.0f;
		Vector3 points[3];
		points[0] = Vector3( 0.0f, -bias, 0.0f );
		points[1] = Vector3( 0.0f, 0.0f, 0.0f );
		points[2] = Vector3( 0.0f, bias, 0.0f );
		union
		{
			DWORD argb;
			void * ptr;
		} cast;
		cast.ptr = this;
		Geometrics::drawPoints( points, 3, 3.0f, cast.argb );
		Moo::rc().pop();
	}

	if (model)
	{
		Moo::rc().push();
		Moo::rc().preMultiply( this->edTransform() );
		
		if ( model == overrideModel_&&
			system_->helperModelCenterOnHardPoint() < helperModelHardPointTransforms_.size() )
		{
			Moo::rc().postMultiply( helperModelHardPointTransforms_[ system_->helperModelCenterOnHardPoint() ] );
		}

		model->dress( Moo::rc().world() );	// should really be using a supermodel...
		model->draw( drawContext, true );
		
		Moo::rc().pop();
	}
	
	if ( system_ == NULL )
		return;
	
	if ( !WorldManager::instance().drawSelection() )
	{
		ChunkParticles::draw( drawContext );

		ChunkParticles::drawBoundingBoxes( 
			BoundingBox::s_insideOut_, 
			BoundingBox::s_insideOut_, 
			Matrix::identity ); 
	}
}

/**
 *	This method saves the data section pointer before calling its
 *	base class's load method
 */
bool EditorChunkParticleSystem::load( DataSectionPtr pSection, Chunk* chunk, BW::string* errorString )
{
	BW_GUARD;

	remove( this );
	bool ok = this->EditorChunkSubstance<ChunkParticles>::load( pSection );
	resourceName_ = pSection->readString( "resource" );
	add( this, resourceName_ );
	if ( !ok )
	{
		pOriginalSect_ = pSection;
		WorldManager::instance().addError(
			chunk, this, "Couldn't load particle system: %s",
			pSection->readString( "resource" ).c_str() );
	}

	return true;
}


/**
 *	Save any property changes to this data section
 */
bool EditorChunkParticleSystem::edSave( DataSectionPtr pSection )
{
	BW_GUARD;

	if (!edCommonSave( pSection ))
		return false;

	if ( system_ == NULL )
	{
		// the particle system didn't load, so just save the original section
		// but set the appropriate transform.
		pSection->copy( pOriginalSect_ );
		pSection->writeMatrix34( "transform", localTransform_ );
		return true;
	}

	pSection->writeString( "resource", resourceName_ );
	pSection->writeMatrix34( "transform", localTransform_ );
	pSection->writeBool( "reflectionVisible", getReflectionVis() );

	return true;
}


/**
 *	Get the current transform
 */
const Matrix & EditorChunkParticleSystem::edTransform()
{
	return localTransform_;
}

/**
*	Get bounding box of this item in world space
*/
void EditorChunkParticleSystem::edWorldBounds( BoundingBox& bbRet )
{
	BW_GUARD;
	this->worldVisibilityBoundingBox( bbRet, false );
}

/**
 *	Change our transform, temporarily or permanently
 */
bool EditorChunkParticleSystem::edTransform( const Matrix & m, bool transient )
{
	BW_GUARD;

	// it's permanent, so find out where we belong now
	Chunk * pOldChunk = pChunk_;
	Chunk * pNewChunk = this->edDropChunk( m.applyToOrigin() );
	if (pNewChunk == NULL) return false;

	// if this is only a temporary change, keep it in the same chunk
	if (transient)
	{
		// move the system
		localTransform_ = m;
		Matrix world;
		world.multiply( m, chunk()->transform() );
		setMatrix( world );

		// move the bounding boxes too
		if ( system_ != NULL )
			system_->clear();
		this->syncInit();
		return true;
	}

	// make sure the chunks aren't readonly
	if (!EditorChunkCache::instance( *pOldChunk ).edIsWriteable() 
		|| !EditorChunkCache::instance( *pNewChunk ).edIsWriteable())
		return false;
	
	// clear to reset bounding boxes (otherwise will be stretched during undo/redo)
	if ( system_ != NULL )
		system_->clear();
	
	// ok, accept the transform change then
	localTransform_.multiply( m, pOldChunk->transform() );
	localTransform_.postMultiply( pNewChunk->transformInverse() );


	edMove( pOldChunk, pNewChunk );

	// check to see if we are undoing or redoing so we will reset the source action
	if ( system_ != NULL && UndoRedo::instance().isUndoing() )
		system_->setFirstUpdate();
	this->syncInit();
	return true;
}

/**
 *	Add the properties of this flare to the given editor
 */
bool EditorChunkParticleSystem::edEdit( class GeneralEditor & editor )
{
	BW_GUARD;

	if (this->edFrozen())
		return false;

	if (!edCommonEdit( editor ))
	{
		return false;
	}

	STATIC_LOCALISE_NAME( s_position, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_PARTICLE/POSITION" );
	STATIC_LOCALISE_NAME( s_rotation, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_PARTICLE/ROTATION" );
	STATIC_LOCALISE_NAME( s_particleName, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_PARTICLE/PARTICLE_NAME" );
	STATIC_LOCALISE_NAME( s_reflectionVisible, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_PARTICLE/REFLECTION_VISIBLE" );
	STATIC_LOCALISE_NAME( s_assetMetadataGroup, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_PARTICLE/ASSET_META_DATA" );

	MatrixProxy * pMP = new ChunkItemMatrix( this );
	editor.addProperty( new ChunkItemPositionProperty( s_position, pMP, this ) );
	editor.addProperty( new GenRotationProperty( s_rotation, pMP ) );
	editor.addProperty( new StaticTextProperty( s_particleName,
		new ConstantDataProxy<StringProxy>(
		resourceName_ ) ) );
	editor.addProperty( new GenBoolProperty( s_reflectionVisible,
			new AccessorDataProxy< EditorChunkParticleSystem, BoolProxy >(
				this, "reflectionVisible",
				&EditorChunkParticleSystem::getReflectionVis,
				&EditorChunkParticleSystem::setReflectionVis ) ) );

	if (system_ != NULL)
	{
		editor.addingAssetMetadata( true );
		system_->metaData().edit( editor, s_assetMetadataGroup, true );
		editor.addingAssetMetadata( false );
	}

	return true;
}

BW::vector<BW::string> EditorChunkParticleSystem::edCommand( const BW::string& path ) const
{
	BW_GUARD;

	if ( system_ == NULL )
		return BW::vector<BW::string>();

	BW::vector<BW::string> commands;
	commands.push_back( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_PARTICLE/EDIT_IN_PARTICLE_EDITOR") );
	return commands;
}

bool EditorChunkParticleSystem::edExecuteCommand( const BW::string& path, BW::vector<BW::string>::size_type index )
{
	BW_GUARD;

	if ( system_ == NULL )
		return true;

	if( path.empty() && index == 0 )
	{
		BW::string commandLine = "-o ";
		commandLine += '\"' + BWResource::resolveFilename( resourceName_ ) + '\"';
		std::replace( commandLine.begin(), commandLine.end(), '/', '\\' );

		PyObject * function = NULL;
		PyObject* pModule = PyImport_ImportModule( "WorldEditor" );
		if (pModule != NULL)
		{
			function = PyObject_GetAttrString( pModule, "launchTool" );
		}

		return Script::call( function,
			Py_BuildValue( "(ss)", 
#ifdef _WIN64
			"..\\..\\particleeditor\\win64\\particleeditor.exe",
#else
			"..\\..\\particleeditor\\win32\\particleeditor.exe",
#endif
			commandLine.c_str() ), 
			"WorldEditor" );
	}
	return false;
}

void EditorChunkParticleSystem::drawBoundingBoxes( const BoundingBox &bb, const BoundingBox &vbb, const Matrix &spaceTrans ) const
{
	BW_GUARD;

	if ( system_ == NULL )
		return;

	if (!WorldManager::instance().drawSelection())
		EditorChunkSubstance<ChunkParticles>::drawBoundingBoxes( bb, vbb, spaceTrans );
}


bool EditorChunkParticleSystem::addYBounds( BoundingBox& bb ) const
{
	BW_GUARD;
	ChunkParticles::addYBounds( bb );
	return true;
}


/**
 *	Return a modelptr that is the representation of this chunk item
 */
ModelPtr EditorChunkParticleSystem::reprModel() const
{
	return currentModel_;
}


/// Write the factory statics stuff
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk, &errorString)
IMPLEMENT_CHUNK_ITEM( EditorChunkParticleSystem, particles, 1 )
BW_END_NAMESPACE

