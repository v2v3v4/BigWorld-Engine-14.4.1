#include "pch.hpp"
#include "worldeditor/world/items/editor_chunk_model.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/item_info_db.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/editor/item_editor.hpp"
#include "worldeditor/editor/chunk_item_placer.hpp"
#include "worldeditor/project/project_module.hpp"
#include "worldeditor/misc/cvswrapper.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "worldeditor/misc/fences_tool_view.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_model_obstacle.hpp"
#include "chunk/chunk_light.hpp"
#include "chunk/geometry_mapping.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/string_provider.hpp"
#include "common/material_utility.hpp"
#include "common/material_properties.hpp"
#include "common/material_editor.hpp"
#include "common/dxenum.hpp"
#include "common/tools_common.hpp"
#include "common/space_editor.hpp"
#include "model/matter.hpp"
#include "model/super_model.hpp"
#include "model/super_model_animation.hpp"
#include "model/super_model_dye.hpp"
#include "model/tint.hpp"
#include "physics2/material_kinds.hpp"
#include "physics2/bsp.hpp"
#include "moo/visual_manager.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/resource_load_context.hpp"
#include "appmgr/options.hpp"
#include "appmgr/module_manager.hpp"
#include "math/colour.hpp"
#include "cstdmf/debug.hpp"
#include "gizmo/item_view.hpp"
#include "cstdmf/bw_set.hpp"


DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )

BW_BEGIN_NAMESPACE

namespace
{
	static bool s_AllowBigModels = false;
}

static AutoConfigString s_chunkModelNotFoundModel( "system/notFoundModel" );	
StringHashMap<int> EditorChunkModel::s_materialKinds_;


BW::map<uint, BW::vector<EditorChunkModel*> > EditorChunkModel::s_fences;
bool EditorChunkModel::s_isAutoSelectingFence = false;
bool EditorChunkModel::s_autoSelectFenceSections = true;

void EditorChunkModel::clean()
{
	BW_GUARD;

	bw_safe_delete(pSuperModel_);
	calculateIsShellModel_ = true;
	cachedIsShellModel_ = false;
	firstToss_ = true;
	primGroupCount_ = 0;
	customBsp_ = false;
	standinModel_ = false;
	originalSect_ = NULL;
	outsideOnly_ = false;
	castsShadow_ = true;
	desc_.clear();
	animationNames_.clear();
	dyeTints_.clear();
	tintName_.clear();

	pAnimation_ = NULL;
	tintMap_.clear();
	materialFashions_.clear();
	label_.clear();

	deleteFenceInfo();
}

// -----------------------------------------------------------------------------
// Section: EditorChunkModel
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EditorChunkModel::EditorChunkModel()
	: primGroupCount_( 0 )
	, customBsp_( false )
	, standinModel_( false )
	, originalSect_( NULL )
	, outsideOnly_( false )
	, pEditorModel_( NULL )
	, pFenceInfo_(NULL)
	, isInScene_(false)
	, lastDrawEditorProxy_( false )
{
	wantFlags_ = WantFlags( wantFlags_ | WANTS_TICK );
}


/**
 *	Destructor.
 */
EditorChunkModel::~EditorChunkModel()
{
	BW_GUARD;

	bw_safe_delete(pEditorModel_);

	deleteFenceInfo();
}

/**
 *	overridden edShouldDraw method
 */
bool EditorChunkModel::edShouldDraw() const
{
	BW_GUARD;

	if( !ChunkModel::edShouldDraw() )
		return false;
	if( isShellModel() )
		return !Chunk::hideIndoorChunks_;

	return OptionsScenery::visible();
}


/**
 *	The method checks if we are drawing the chunk models or some sort of editor
 *	visualisation instead.
 *	@return true if we are drawing the actual model.
 */
bool EditorChunkModel::isDrawingModels() const
{
	BW_GUARD;
	if (!edShouldDraw())
	{
		return false;
	}

	if (pSuperModel_ == NULL)
	{
		return false;
	}

	bool drawRed = OptionsMisc::readOnlyVisible() &&
		!EditorChunkCache::instance( *chunk() ).edIsWriteable();
	drawRed &= !!OptionsMisc::visible();

	if (drawRed && WorldManager::instance().drawSelection())
	{
		return false;
	}

	const bool projectModule = (ProjectModule::currentInstance() ==
		ModuleManager::instance().currentModule());
	const bool drawBsp = !projectModule &&
		WorldManager::instance().drawBSP() != 0 &&
		WorldManager::instance().drawBSPOtherModels() != 0;

	if (drawBsp)
	{
		return false;
	}

	return true;
}


/**
 *	Editor check for editor proxies being enabled recently.
 *	Updates the editor proxy by adding it to the chunk scene 
 *	when they become enabled.
 */
void EditorChunkModel::tick( float /*dTime*/ )
{
	const bool drawEditorProxy = OptionsEditorProxies::visible();
	if ((drawEditorProxy != lastDrawEditorProxy_) && (pEditorModel_))
	{
		this->ChunkModel::tossWithExtra( pChunk_, 
			drawEditorProxy ? pEditorModel_ : NULL );
		lastDrawEditorProxy_ = drawEditorProxy;
	}
}


/**
 *	Re-calculate LOD and apply animations to this chunk model for this frame.
 *	Should be called once per frame, unless the ChunkModel is static and does
 *	not require its animations updated every frame (and so has been marked
 *	without WANTS_UPDATE). In this case this function is not called and
 *	the model's LOD will be updated before it is drawn.
 */
void EditorChunkModel::updateAnimations()
{
	BW_GUARD;

	if (!this->isDrawingModels())
	{
		return;
	}

	TRANSFORM_ASSERT( this->wantsUpdate() );

	EditorChunkModelBase::updateAnimations();

	if (lastDrawEditorProxy_ && pEditorModel_)
	{
		TmpTransforms preTransformFashions;
		if (pAnimation_.hasObject())
		{
			preTransformFashions.push_back( pAnimation_.get() );
		}

		pEditorModel_->updateAnimations( transform_,
			&preTransformFashions, // pPreTransformFashions
			NULL, // pPostFashions
			Model::LOD_AUTO_CALCULATE ); // atLod
	}
}


/**
 *	Overridden draw method.
 *	Draws this ChunkModel's SuperModel. If this ChunkModel is not animated then
 *	this method will re-calcualte the LOD before drawing. If it is animated
 *	then the LOD calculation should have been done in updateAnimations().
 */
void EditorChunkModel::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if( !edShouldDraw() || (Moo::rc().reflectionScene() && !reflectionVisible_) )
		return;

	if (!hasPostLoaded_)
	{
		edPostLoad();
		hasPostLoaded_ = true;
	}

	if (Moo::rc().dynamicShadowsScene() && !castsShadow_)
	{
		return;
	}

	if (pSuperModel_ != NULL )
	{
		bool projectModule = ProjectModule::currentInstance() == ModuleManager::instance().currentModule();
		
		if (!projectModule)
		{
			BoundingBox bbox;
			Matrix boxTransform;
			if (this->edFrozen() && OptionsMisc::frozenVisible())
			{
				// display model as frozen
				pSuperModel_->localBoundingBox( bbox );
				boxTransform = pChunk_ ? pChunk_->transform() : Matrix::identity;
				boxTransform.preMultiply( transform_ );

				WorldManager::instance().showFrozenRegion( bbox, boxTransform );
			}
		}

		// Notify the chunk cache of how many render sets we're going to draw
		WorldManager::instance().addPrimGroupCount( chunk(), primGroupCount_ );

		bool ignoreStaticLighting = false;
		bool drawBsp = !projectModule &&
			WorldManager::instance().drawBSP() != 0 &&
			WorldManager::instance().drawBSPOtherModels() != 0;

		if (WorldManager::instance().drawSelection() &&
			this->edSelectable())
		{
			WorldManager::instance().registerDrawSelectionItem( this );
		}
		// Load up the bsp tree if needed
		if (drawBsp && !this->isBspCreated())
		{
			// no vertices loaded yet, create some
			const BSPTree * tree = pSuperModel_->topModel(0)->decompose();

			if (tree)
			{
				BW::vector<Moo::VertexXYZL> verts;
				Moo::Colour colour;
				this->getRandomBspColour( colour );
				Moo::BSPTreeHelper::createVertexList( *tree, verts, colour);

				// If BSP has collideable vertices
				if (!verts.empty())
				{
					this->addBsp( verts,
						pSuperModel_->topModel( 0 )->resourceID() );
					MF_ASSERT( this->isBspCreated() );
				}
			}
		}

		if (drawBsp)
		{
			if (this->isBspCreated())
			{
				//set the transforms
				Matrix transform;
				transform.multiply(edTransform(), chunk()->transform());

				if (WorldManager::instance().drawSelection())
				{
					this->drawBsp( Moo::rc(), transform, true );
				}
				else
				{
					this->batchBsp( transform );
				}
			}
		}
		else
		{
			// Update lod levels if the super model has not been updated
			// as part of the update loop
			if (!this->wantsUpdate())
			{
				pSuperModel_->updateLodsOnly( this->worldTransform(),
					Model::LOD_AUTO_CALCULATE );
			}

			if (lastDrawEditorProxy_ && pEditorModel_)
			{
				// Update lod levels if the super model has not been updated
				// as part of the update loop
				if (!this->wantsUpdate())
				{
					pEditorModel_->updateLodsOnly( this->worldTransform(),
						Model::LOD_AUTO_CALCULATE );
				}
				pEditorModel_->draw( drawContext, this->worldTransform(), &materialFashions_ );
			}
			else
			{
				pSuperModel_->draw( drawContext, this->worldTransform(), &materialFashions_ );
			}
		}
	}
}


/**
 *	Used by load to get the names of the animations in a model file.
 */
static void addNames(BW::vector<BW::string>& sections, DataSectionPtr ds,
	const BW::string& name)
{
	BW_GUARD;

	BW::vector<DataSectionPtr> children;
	ds->openSections( name, children );

	BW::set<BW::string> names;

	BW::vector<BW::string>::iterator ni;
	// Build a list of names we already have
	for (ni = sections.begin(); ni != sections.end(); ++ni)
		names.insert( *ni );

	BW::vector<DataSectionPtr>::iterator i;
	for (i = children.begin(); i != children.end(); ++i)
	{
		// Only add it if we don't already have a section with the same
		// name
		if (!names.count( (*i)->readString( "name" ) ))
		{
			sections.push_back( (*i)->readString( "name" ) );
		}
	}
}

/**
 *	Used by load to get the names of the dyes and tints in a model file.
 */
static void addDyeTints( BW::map<BW::string, BW::vector<BW::string> >& sections, DataSectionPtr ds )
{
	BW_GUARD;

	BW::vector<DataSectionPtr> dyes;
	ds->openSections( "dye", dyes );

	BW::vector<DataSectionPtr>::iterator i;
	for (i = dyes.begin(); i != dyes.end(); ++i)
	{
		if( sections.find( (*i)->readString( "matter" ) ) == sections.end() )
		{
			BW::vector<BW::string> names;
			names.push_back( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/DEFAULT_TINT_NAME") );
			addNames( names, (*i), "tint" );
			if( names.size() > 1 )
				sections[ (*i)->readString( "matter" ) ] = names;
		}
	}
}

/**
 *	This method saves the data section pointer before calling its
 *	base class's load method
 */
bool EditorChunkModel::load( DataSectionPtr pSection, Chunk * pChunk /*= NULL*/,
							BW::string* pErrorString /*= NULL */)
{
	BW_GUARD;

	primGroupCount_ = 0;
	customBsp_ = false;
	standinModel_ = false;
	originalSect_ = NULL;
	outsideOnly_ = false;
	desc_.clear();
	animationNames_.clear();
	dyeTints_.clear();
	tintName_.clear();
	isInScene_ = false;

	edCommonLoad( pSection );

	pOwnSect_ = pSection;
	BW::string mname = pSection->readString( "resource" );

	bool ok = this->EditorChunkModelBase::load( pSection, pChunk, pErrorString );
	if (!ok)
	{
		originalSect_ = new XMLSection( sectionName() );
		originalSect_->copy( pSection );

		// load in a replacement model
		DataSectionPtr pTemp = new XMLSection( sectionName() );
		pTemp->writeString( "resource", s_chunkModelNotFoundModel );
		pTemp->writeMatrix34( "transform", pSection->readMatrix34( "transform" ) );
		ok = this->ChunkModel::load( pTemp, pChunk );

		standinModel_ = true;

		// tell the user
		SpaceEditor::instance().addError(pChunk, this, "Model not loaded: %s", mname.c_str());

		// don't look for visuals
		hasPostLoaded_ = true;
	}
	else
	{
		Moo::ScopedResourceLoadContext resLoadCtx(
						"chunk model " + BWResource::getFilename( mname ) );

		if (pAnimation_)
		{
			animName_ = pSection->readString( "animation/name" );
		}

		tintName_.clear();
		BW::vector<DataSectionPtr> dyes;
		pSection->openSections( "dye", dyes );
		for( BW::vector<DataSectionPtr>::iterator iter = dyes.begin(); iter != dyes.end(); ++iter )
		{
			BW::string dye = (*iter)->readString( "name" );
			BW::string tint = (*iter)->readString( "tint" );
			if( tintMap_.find( dye ) != tintMap_.end() )
				tintName_[ dye ] = tint;
		}

		outsideOnly_ = pOwnSect()->readBool( "editorOnly/outsideOnly", outsideOnly_ );
		outsideOnly_ |= this->resourceIsOutsideOnly();

		// read fence section data (if present)
		int fenceId = pOwnSect()->readInt("editorOnly/fenceId", -1);
		if (fenceId != -1)
		{
			int sectionId = pOwnSect()->readInt("editorOnly/fenceSectionId", -1);
			pFenceInfo_ = getFenceInfo(true, fenceId, sectionId);
		}
		else
			deleteFenceInfo();

		// check we're not > gridSize metres in x or z
		BoundingBox bbox;
		edBounds(bbox);
		bbox.transformBy(edTransform());
		Vector3 boxVolume = bbox.maxBounds() - bbox.minBounds();
		const float lengthLimit = pChunk->space()->gridSize();
		if (s_AllowBigModels == false &&
			(boxVolume.x > lengthLimit || boxVolume.z > lengthLimit))
		{
			SpaceEditor::instance().addError(pChunk, this,
				LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/MODEL_TOO_BIG", mname ).c_str() );
		}

		// Reset the amount of primitive groups in all the visuals
		primGroupCount_ = 0;
		hasPostLoaded_ = false;


		this->pullInfoFromSuperModel();
		if (pSuperModel_)
		{
			pSuperModel_->registerModelListener( this );
		}

		// After loading,count the amount of primitive groups in all the
		// visuals.
		BW::vector<BW::string> visuals = extractVisualNames();
		BW::vector<BW::string>::iterator i = visuals.begin();
		for (; i != visuals.end(); ++i)
		{
			DataSectionPtr visualSection = BWResource::openSection( *i );
			if (!visualSection)
				continue;

			// Check for a custom bsp while we're here
			if (visualSection->readBool( "customBsp", false ))
				customBsp_ = true;

			BW::vector<DataSectionPtr> renderSets;
			visualSection->openSections( "renderSet", renderSets );

			BW::vector<DataSectionPtr>::iterator j = renderSets.begin();
			for (; j != renderSets.end(); ++j)
			{
				BW::vector<DataSectionPtr> geoms;
				(*j)->openSections( "geometry", geoms );

				BW::vector<DataSectionPtr>::iterator k = geoms.begin();
				for (; k != geoms.end(); ++k)
				{
					DataSectionPtr geom = *k;

					BW::vector<DataSectionPtr> primGroups;
					(*k)->openSections( "primitiveGroup", primGroups );

					primGroupCount_ += static_cast<uint>(primGroups.size());
				}
			}
		}
	}

	isInScene_ = true;
	BW::string desc = pSection->readString( "resource" );
	BW::string::size_type pos = desc.find_last_of( "/" );
	if (pos != BW::string::npos)
		desc = desc.substr( pos + 1 );
	pos = desc.find_last_of( "." );
	if (pos != BW::string::npos)
		desc = desc.substr( 0, pos );

	desc_ = Name( desc );

	return ok;
}



 /**
 * if pSuperModel_ has been reloaded, 
 * update the related data.
 */
void EditorChunkModel::onReloaderReloaded( Reloader* pReloader )
{
	BW_GUARD;
	if (!pReloader)
	{
		return;
	}

	ChunkModel::onReloaderReloaded( pReloader );

	// either model reloaded or visual reloaded.
	bool modelReloaded = pSuperModel_->hasModel( (Model*) pReloader, true );
	bool visualReloaded = pSuperModel_->hasVisual( (Moo::Visual*) pReloader );
	bool proxyModelReloaded = (pEditorModel_) ? 
			pEditorModel_->hasModel( (Model*) pReloader, true ): false;

	if (modelReloaded)
	{
		this->pullInfoFromSuperModel();
		if (pEditorModel_)
		{
			proxyModelReloaded = true;
		}
	}
	if (proxyModelReloaded)
	{
		// we cache the default transforms in the pEditorModel_
		pEditorModel_->cacheDefaultTransforms( this->worldTransform() );

		if (pChunk_ != NULL)
		{
			// Update the obstacle data.
			ChunkModelObstacle::instance( *pChunk_ ).delObstacles( this );
			ChunkModelObstacle::instance( *pChunk_ ).addModel(
				pEditorModel_->topModel( 0 ), this->worldTransform(), 
				this, true );
		}
	}

	if (modelReloaded || visualReloaded || proxyModelReloaded)
	{
		// Dirty the occupied chunks
		if( pChunk_ )
		{
			BW::set<Chunk*> chunks;
			BoundingBox bb;
			this->edWorldBounds( bb );
			WorldManager::instance().collectOccupiedChunks( bb, pChunk_, chunks );
			WorldManager::instance().changedChunks( chunks, *this );
		}

		// Clear the selection if I am selected.
		if (WorldManager::instance().isItemSelected( this ))
		{
			WorldManager::instance().setSelection( BW::vector<ChunkItemPtr>() );
		}


		WorldManager::instance().addCommentaryMsg( 
			LocaliseUTF8( L"COMMON/MODEL_RELOAD/RELOADED_MODEL", 
			(modelReloaded || proxyModelReloaded) ? ((Model*) pReloader)->resourceID():
			((Moo::Visual*) pReloader)->resourceID() ) );
	}
}



/**
 * if our pSuperModel_ is about to be reloaded, 
 */
void EditorChunkModel::onReloaderPreReload( Reloader* pReloader)
{
	BW_GUARD;

	// either model reloaded or visual reloaded.
	if (pReloader && 
		(pSuperModel_->hasModel( (Model*) pReloader, true ) || 
		pSuperModel_->hasVisual( (Moo::Visual*) pReloader )))
	{
		// Dirty the occupied chunks
		if( pChunk_ )
		{
			BW::set<Chunk*> chunks;
			BoundingBox bb;
			this->edWorldBounds( bb );
			WorldManager::instance().collectOccupiedChunks( bb, pChunk_, chunks );
			WorldManager::instance().changedChunks( chunks, *this );
		}
	}

	ChunkModel::onReloaderPreReload( pReloader );
}


/**
 *  This function pull data out from our models.
 */
void EditorChunkModel::pullInfoFromSuperModel()
{
	SuperModel::ReadGuard guard( pSuperModel_ );
	// Build a list of the animations in the model file
	animationNames_.clear();
	animationNamesW_.clear();

	// Build a list of the animations in the model file
	animationNames_.push_back( BW::string() );

	BW::string modelName;
	if (pOwnSect())
	{
		modelName = pOwnSect()->readString( "resource" );
	}
	else if (pSuperModel_)
	{
		// Cause if this model is toss( NULL ) then pOwnSet() will be NULL.
		modelName = pSuperModel_->topModel( 0 )->resourceID();
	}
	else
	{
		ASSERT( false );
	}
	DataSectionPtr modelSect = BWResource::openSection( modelName );
	DataSectionPtr current = modelSect;
	while (current)
	{
		addNames( animationNames_, current, "animation" );

		BW::string parent = current->readString( "parent" );

		if (parent.empty())
			break;

		current = BWResource::openSection( parent + ".model" );
	}
	std::sort( animationNames_.begin(), animationNames_.end() );
	// cache wide string version for performance
	bw_containerConversion( animationNames_, animationNamesW_, bw_utf8towSW );


	// Build a list of dyes in the model file
	{
		dyeTints_.clear();
		dyeTintsW_.clear();

		tintMap_.clear();
		DataSectionPtr current = modelSect;
		if( current )
		{
			addDyeTints( dyeTints_, current );

			// cache wide string version for performance
			for (BW::map<BW::string, BW::vector<BW::string> >::iterator
				iter = dyeTints_.begin(); iter != dyeTints_.end(); ++iter)
			{
				BW::vector<BW::wstring> wsecond;
				bw_containerConversion( iter->second, wsecond, bw_utf8towSW );
				dyeTintsW_[ iter->first ] = wsecond;
			}
		}
	}

	if (modelSect) 
	{
		BW::string editorModel = modelSect->readString( "editorModel", "" );
		if (pEditorModel_)
		{
			pEditorModel_->deregisterModelListener( this );
		}
		bw_safe_delete(pEditorModel_);
		if (editorModel != "")
		{
			BW::vector<BW::string> editorModels;
			editorModels.push_back( editorModel );
			pEditorModel_ = new SuperModel( editorModels );
			pEditorModel_->registerModelListener( this );
		}
	}
}


void EditorChunkModel::loadModels( Chunk* chunk )
{
	BW::vector<BW::string> models;
	pOwnSect_->readStrings( "resource", models );
	if( models.size() )
	{
		SmartPointer<Model> model = Model::get( models[0] );
		if( model )
		{
			model->reload( false );
			load( pOwnSect_, chunk );
		}
	}
}

void EditorChunkModel::edPostLoad()
{
	// Don't put here code that might cause frame rate spikes (for example,
	// code that reads from or writes to disk).
}


/**
 * We just loaded up with srcItems lighting data, create some new stuff of our
 * own
 */
void EditorChunkModel::edPostClone( EditorChunkItem* srcItem )
{
	BW_GUARD;

	SpaceEditor::instance().changedChunk( pChunk_ );

	EditorChunkItem::edPostClone( srcItem );
	ChunkBspHolder::postClone();


	// after cloning one/many fence sections, make them separate fence sequence
	// to not link it with source fence sequence (e.g. prevent simultaneous movement/selection)
	if (pFenceInfo_ != NULL)
	{
		// advance sequence id to guarantee no intersections with existing sections,
		// this will be applied to all cloned fence sections at once, linking them together
		static int s_lastFenceCloneOpId = -1;
		static int s_clonedFenceId = -1;
		if (s_lastFenceCloneOpId != ChunkItemPlacer::s_currFenceCloneOpId)
		{
			s_lastFenceCloneOpId = ChunkItemPlacer::s_currFenceCloneOpId;
			s_clonedFenceId = static_cast<int>(s_fences.size() + 1);
		}
		uint fenceId = s_clonedFenceId;
		uint sectionId = pFenceInfo_->sectionId;

		// since clone uses load() and creates new FenceInfo with old sectionId, recreate FenceInfo with different params
		deleteFenceInfo();
		getFenceInfo(true, fenceId, sectionId);

		FencesToolView *pToolView = WorldManager::instance().getFenceToolView();
		if (pToolView != NULL)
			pToolView->continueSequenceFrom(this);
	}
}


/**
*	This method return the invalidateFlag 
*	that should notify the chunk to update 
*	when manupulating this item
*
*/
InvalidateFlags EditorChunkModel::edInvalidateFlags()
{
	InvalidateFlags flags( InvalidateFlags::FLAG_THUMBNAIL );
	if (this->hasBSP())
	{
		flags |= InvalidateFlags::FLAG_NAV_MESH;
	}
	if (this->affectShadow())
	{
		flags |= InvalidateFlags::FLAG_SHADOW_MAP;
	}
	return flags;
}

void EditorChunkModel::edPostCreate()
{
	BW_GUARD;
	EditorChunkItem::edPostCreate();

	isInScene_ = true;
	if (pFenceInfo_ != NULL)
	{
		FencesToolView *pToolView = WorldManager::instance().getFenceToolView();
		if (pToolView != NULL)
			pToolView->continueSequenceFrom(this);
	}	
}


namespace
{
	BW::string toString( int i )
	{
		BW_GUARD;

		char buf[128];
		itoa( i, buf, 10 );
		return buf;
	}
}


/**
 *	This method does extra stuff when this item is tossed between chunks.
 *
 *	It updates its datasection in that chunk.
 */
void EditorChunkModel::toss( Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk_ != NULL && pOwnSect_)
	{
		EditorChunkCache::instance( *pChunk_ ).
			pChunkSection()->delChild( pOwnSect_ );
		pOwnSect_ = NULL;
	}

	this->EditorChunkModelBase::tossWithExtra( pChunk,
		OptionsEditorProxies::visible() ? pEditorModel_ : NULL  );

	if (pChunk_ != NULL && !pOwnSect_)
	{
		pOwnSect_ = EditorChunkCache::instance( *pChunk_ ).
			pChunkSection()->newSection( sectionName() );
		this->edSave( pOwnSect_ );
	}

	// Update the database
	ItemInfoDB::instance().toss( this, pChunk != NULL );
}


/**
 *	Object finished loading, add it to the scene. 
 *	Push the most recent set of transforms to the editor proxy.
 */
void EditorChunkModel::syncInit()
{
	BW_GUARD;

	EditorChunkModelBase::syncInit();
	if (pEditorModel_)
	{
		pEditorModel_->cacheDefaultTransforms( this->worldTransform() );
	}
}


/**
 *	Save to the given section
 */
bool EditorChunkModel::edSave( DataSectionPtr pSection )
{
	BW_GUARD;

	if (!edCommonSave( pSection ))
		return false;

	if (standinModel_)
	{
		// only change its transform (want to retain knowledge of what it was)
		pSection->copy( originalSect_ );
		pSection->writeMatrix34( "transform", transform_ );
		return true;
	}

	if (pSuperModel_)
	{
		for (int i = 0; i < pSuperModel_->nModels(); i++)
		{
			pSection->writeString( "resource",
				pSuperModel_->topModel(i)->resourceID() );
		}

		if (pAnimation_)
		{
			DataSectionPtr pAnimSec = pSection->openSection( "animation", true );
			pAnimSec->writeString( "name", animName_ );
			pAnimSec->writeFloat( "frameRateMultiplier", animRateMultiplier_ );
		}
		else
		{
			pSection->delChild( "animation" );
		}

		while( pSection->findChild( "dye" ) )
			pSection->delChild( "dye" );
		for( BW::map<BW::string,BW::string>::iterator iter = tintName_.begin(); iter != tintName_.end(); ++iter )
		{
			if( iter->second != LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/DEFAULT_TINT_NAME") )
			{
				DataSectionPtr pDyeSec = pSection->newSection( "dye" );
				pDyeSec->writeString( "name", iter->first );
				pDyeSec->writeString( "tint", iter->second );
			}
		}

		pSection->writeMatrix34( "transform", transform_ );
	}

	//Editor only data.
	if (outsideOnly_ && !resourceIsOutsideOnly())
		pSection->writeBool( "editorOnly/outsideOnly", true );
	else
		pSection->delChild( "editorOnly/outsideOnly" );

	pSection->delChild( "editorOnly/castsShadow" );

	pSection->writeBool( "castsShadow", castsShadow_ );
		if (pFenceInfo_ != NULL)
	{
		pSection->writeInt("editorOnly/fenceId", pFenceInfo_->fenceId);
		pSection->writeInt("editorOnly/fenceSectionId", pFenceInfo_->sectionId);
	}
	else
	{
		pSection->delChild("editorOnly/fenceId");
		pSection->delChild("editorOnly/fenceSectionId");
	}

	pSection->setString( label_ );

	pSection->writeBool( "reflectionVisible", reflectionVisible_ );

	return this->EditorChunkModelBase::edSave( pSection );
}

/**
 *	Called when our containing chunk is saved
 */
void EditorChunkModel::edChunkSave()
{
}


/**
 *	This method sets this item's transform for the editor
 *	It takes care of moving it into the right chunk and recreating the
 *	collision scene and all that
 */
bool EditorChunkModel::edTransform( const Matrix & m, bool transient )
{
	BW_GUARD;

	// find out where we belong now
	BoundingBox lbb( Vector3(0.f,0.f,0.f), Vector3(1.f,1.f,1.f) );
	if (pSuperModel_)
	{
		pSuperModel_->localBoundingBox( lbb );
	}
	Chunk * pOldChunk = pChunk_;
	Chunk * pNewChunk = this->edDropChunk( m.applyPoint(
		(lbb.minBounds() + lbb.maxBounds()) * 0.5f ) );
	if (pNewChunk == NULL) return false; // failure, outside the space!

	// for transient transforms there are no more, checks, update the transform
    // and return
	if (transient)
	{
		transform_ = m;

		this->updateWorldTransform( pOldChunk );

		this->syncInit();
		return true;
	}

	// make sure the chunks aren't readonly
	if (!EditorChunkCache::instance( *pOldChunk ).edIsWriteable() 
		|| !EditorChunkCache::instance( *pNewChunk ).edIsWriteable())
		return false;


	transform_.multiply( m, pOldChunk->transform() );
	transform_.postMultiply( pNewChunk->transformInverse() );


	edMove( pOldChunk, pNewChunk );

	this->syncInit();
	return true;
}


void EditorChunkModel::edPreDelete()
{
	BW_GUARD;

	EditorChunkItem::edPreDelete();


	isInScene_ = false;
	FencesToolView *pToolView = WorldManager::instance().getFenceToolView();
	if (pFenceInfo_ != NULL && pToolView != NULL)
	{
		EditorChunkModel *pPrevSection = getPrevFenceSection();
		if (pPrevSection != NULL)
			pToolView->continueSequenceFrom(pPrevSection);
		else
			pToolView->breakSequence();
	}

	deleteFenceInfo( true );
}


/**
 *	Get the bounding box
 */
void EditorChunkModel::edBounds( BoundingBox& bbRet ) const
{
	BW_GUARD;

	if (pSuperModel_)
		pSuperModel_->localBoundingBox( bbRet );

	BoundingBox ebb;
	if (OptionsEditorProxies::visible() && pEditorModel_)
	{
		pEditorModel_->localBoundingBox( ebb );
		bbRet.addBounds( ebb );
	}
}


/**
 *	Helper struct for gathering matter names
 */
struct MatterDesc
{
	BW::set<BW::string>	tintNames;
};
typedef BW::map<BW::string,MatterDesc> MatterDescs;

/// I can't believe there's not an algorithm for this...
template <class It, class S> void seq_append( It F, It L, S & s)
	{ for(; F != L; ++F) s.push_back( *F ); }


extern "C"
{
/**
 *	This property makes a dye from a matter name to one of a number of tints
 */
class ModelDyeProperty : public GeneralProperty
{
public:
	ModelDyeProperty( const Name& name,
			const BW::string & current, const MatterDesc & tints,
			EditorChunkModel * pModel ) :
		GeneralProperty( name ),
		curval_( current ),
		pModel_( pModel )
	{
		BW_GUARD;

		tints_.push_back( "Default" );
		seq_append( tints.tintNames.begin(), tints.tintNames.end(), tints_ );

		GENPROPERTY_MAKE_VIEWS()
	}

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( STRING ); }

	virtual PyObject * pyGet()
	{
		BW_GUARD;

		return PyString_FromString( curval_.c_str() );
	}

	virtual int pySet( PyObject * value )
	{
		BW_GUARD;

		// try it as a string
		if (PyString_Check( value ))
		{
			const char * valStr = PyString_AsString( value );

			BW::vector<BW::string>::iterator found =
				std::find( tints_.begin(), tints_.end(), valStr );
			if (found == tints_.end())
			{
				BW::string errStr = "GeneralEditor.";
				errStr += name_.c_str();
				errStr += " must be set to a valid tint string or an index.";
				errStr += " Valid tints are: ";
				for (uint i = 0; i < tints_.size(); i++)
				{
					if (i)
					{
						if (i+1 != tints_.size())
							errStr += ", ";
						else
							errStr += ", or ";
					}
					errStr += tints_[i];
				}

				PyErr_Format( PyExc_ValueError, errStr.c_str() );
				return -1;
			}

			curval_ = valStr;
			// TODO: Set this in the editor chunk item!
			return 0;
		}

		// try it as a number
		int idx = 0;
		if (Script::setData( value, idx ) == 0)
		{
			if (idx < 0 || idx >= (int) tints_.size())
			{
				PyErr_Format( PyExc_ValueError, "GeneralEditor.%s "
					"must be set to a string or an index under %d",
					name_, tints_.size() );
				return -1;
			}

			curval_ = tints_[ idx ];
			// TODO: Set this in the editor chunk item!
			return 0;
		}

		// give up
		PyErr_Format( PyExc_TypeError, "GeneralEditor.%s, "
			"being a dye property, must be set to a string or an index",
			name_ );
		return NULL;
	}

private:
	BW::string					curval_;
	BW::vector<BW::string>	tints_;
	EditorChunkModel *			pModel_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( ModelDyeProperty )
};
}

GENPROPERTY_VIEW_FACTORY( ModelDyeProperty )

static Name matUIName( EditorEffectProperty* pProperty )
{
	BW_GUARD;

	BW::string uiName = MaterialUtility::UIName( pProperty );
	if( uiName.size() == 0 )
		uiName = pProperty->name();
	return Name( uiName );
}

bool EditorChunkModel::edEdit( GeneralEditor & editor )
{
	return edEdit( editor, this );
}


/**
 *	This method adds this item's properties to the given editor
 */
bool EditorChunkModel::edEdit(
	GeneralEditor & editor, ChunkItemPtr chunkItem )
{
	BW_GUARD;

	if (this->edFrozen())
		return false;

	if (!edCommonEdit( editor ))
	{
		return false;
	}

	// can only move this model if it's not in the shells directory
	if (!isShellModel())
	{
		STATIC_LOCALISE_NAME( s_position, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/POSITION" );
		STATIC_LOCALISE_NAME( s_rotation, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/ROTATION" );
		STATIC_LOCALISE_NAME( s_scale, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/SCALE" );
		STATIC_LOCALISE_NAME( s_castsShadow, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/CASTS_SHADOW" );
		STATIC_LOCALISE_NAME( s_outsideOnly, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/OUTSIDE_ONLY" );

		MatrixProxy * pMP = new ChunkItemMatrix( chunkItem );
		editor.addProperty( new ChunkItemPositionProperty( s_position, pMP, this ) );
		editor.addProperty( new GenRotationProperty( s_rotation, pMP ) );
		editor.addProperty( new GenScaleProperty( s_scale, pMP ) );

		// can affect shadow?
		editor.addProperty( new GenBoolProperty( s_castsShadow,
			new AccessorDataProxy< EditorChunkModel, BoolProxy >(
				this, "castsShadow", 
				&EditorChunkModel::getCastsShadow, 
				&EditorChunkModel::setCastsShadow ) ) );

		// can flag models as outside-only
		editor.addProperty( new GenBoolProperty( s_outsideOnly,
			new AccessorDataProxy< EditorChunkModel, BoolProxy >(
				this, "outsideOnly", 
				&EditorChunkModel::getOutsideOnly, 
				&EditorChunkModel::setOutsideOnly ) ) );
	}

	STATIC_LOCALISE_NAME( s_reflectionVisible, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/REFLECTION_VISIBLE" );
	STATIC_LOCALISE_NAME( s_animation, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/ANIMATION" );
	STATIC_LOCALISE_NAME( s_animationSpeed, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/ANIMATION_SPEED" );
	STATIC_LOCALISE_NAME( s_modelName, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/MODEL_NAME" );
	STATIC_LOCALISE_NAME( s_modelNames, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/MODEL_NAMES" );
	STATIC_LOCALISE_NAME( s_assetMetadataGroup, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/ASSET_META_DATA" );

	editor.addProperty( new GenBoolProperty( s_reflectionVisible,
		new AccessorDataProxy< EditorChunkModel, BoolProxy >(
			this, "reflectionVisible",
			&EditorChunkModel::getReflectionVis,
			&EditorChunkModel::setReflectionVis ) ) );

	editor.addProperty( new ListTextProperty( s_animation,
		new AccessorDataProxy< EditorChunkModel, StringProxy >(
			this, "animation", 
			&EditorChunkModel::getAnimation, 
			&EditorChunkModel::setAnimation ), animationNamesW_ ) );

	for (BW::map<BW::string, BW::vector<BW::wstring> >::iterator
			iter = dyeTintsW_.begin(); iter != dyeTintsW_.end(); ++iter)
	{
		ListTextProperty* ltProperty = 
			new ListTextProperty( Name( iter->first ),
				new AccessorDataProxyWithName< EditorChunkModel, StringProxy >(
					this, BW::string( iter->first ), 
					&EditorChunkModel::getDyeTints, 
					&EditorChunkModel::setDyeTints ), 
					iter->second );
		ltProperty->setGroup( Name( "dye" ) );
		editor.addProperty( ltProperty );
	}

	editor.addProperty( new GenFloatProperty( s_animationSpeed,
			new AccessorDataProxy< EditorChunkModel, FloatProxy >(
				this, "animation speed", 
				&EditorChunkModel::getAnimRateMultiplier, 
				&EditorChunkModel::setAnimRateMultiplier ) ) );


	editor.addProperty( new StaticTextProperty( pSuperModel_->nModels() == 1 ?
		s_modelName : s_modelNames, new ConstantDataProxy<StringProxy>( getModelName() ) ) );

	if (pSuperModel_ != NULL)
	{
		MatterDescs	mds;

		// for each model
		for (int i = 0; i < pSuperModel_->nModels(); i++)
		{
			ModelPtr pTop = pSuperModel_->topModel(i);

			// get each matter element pointer
			for (int j = 0; ; j++)
			{
				const Matter * pmit = pTop->lookupLocalMatter( j );
				if (pmit == NULL)
					break;

				// and collect its tints it has
				const Matter::Tints & mts = pmit->tints_;
				for (Matter::Tints::const_iterator tit = mts.begin()+1;
					tit != mts.end();	// except default...
					tit++)
				{
					mds[ pmit->name_ ].tintNames.insert( (*tit)->name_ );
				}
			}
		}

		// now add them all as properties
		for (MatterDescs::iterator it = mds.begin(); it != mds.end(); it++)
		{
			editor.addProperty( new ModelDyeProperty(
				Name( it->first ), "Default", it->second, this ) );
		}

		editor.addingAssetMetadata( true );
		pSuperModel_->topModel(0)->metaData().edit( editor, s_assetMetadataGroup, true );
		editor.addingAssetMetadata( false );
	}

	return true;
}


/**
 *	Find the drop chunk for this item
 */
Chunk * EditorChunkModel::edDropChunk( const Vector3 & lpos )
{
	BW_GUARD;

	Vector3 npos = pChunk_->transform().applyPoint( lpos );

	Chunk * pNewChunk = NULL;

	if ( !this->outsideOnly_ )
		pNewChunk = pChunk_->space()->findChunkFromPointExact( npos );
	else
		pNewChunk = EditorChunk::findOutsideChunk( npos );

	if (pNewChunk == NULL)
	{
		ERROR_MSG( "Cannot move %s to (%f,%f,%f) "
			"because it is not in any loaded chunk!\n",
			this->edDescription().c_str(), npos.x, npos.y, npos.z );
		return NULL;
	}

	return pNewChunk;
}


/**
 * Which section name shall we use when saving?
 */
const char* EditorChunkModel::sectionName()
{
	BW_GUARD;

	return isShellModel() ? "shell" : "model";
}


Name EditorChunkModel::edDescription()
{
	BW_GUARD;

	if (isShellModel())
	{
		if (pChunk_)
		{
			STATIC_LOCALISE_STRING( s_edDesc, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/ED_DESCRIPTION" );
			return Name( s_edDesc + pChunk_->identifier() );
		}
		else
		{
			STATIC_LOCALISE_NAME( s_unknownChunk, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/UNKNOWN_CHUNK" );
			return s_unknownChunk;
		}
	}
	else
	{
		return desc_;
	}
}


/**
 *	This returns the number of triangles of this model.
 */
int EditorChunkModel::edNumTriangles() const
{
	BW_GUARD;

	if (pSuperModel_)
	{
		return pSuperModel_->numTris();
	}

	return 0;
}


/**
 *	This returns the number of primitive groups of this model.
 */
int EditorChunkModel::edNumPrimitives() const
{
	BW_GUARD;

	if (pSuperModel_)
	{
		return pSuperModel_->numPrims();
	}

	return 0;
}


/**
 *	This returns the file path of the asset file.
 */
BW::string EditorChunkModel::edAssetName() const
{
	BW_GUARD;

	if (pSuperModel_)
	{
		return BWResource::getFilename( pSuperModel_->topModel( 0 )->resourceID() ).to_string();
	}

	return "";
}


/**
 *	This returns the file path of the asset file.
 */
BW::string EditorChunkModel::edFilePath() const
{
	BW_GUARD;

	if (pSuperModel_)
	{
		return pSuperModel_->topModel( 0 )->resourceID();
	}

	return "";
}


BW::vector<BW::string> EditorChunkModel::edCommand( const BW::string& path ) const
{
	BW_GUARD;

	BW::vector<BW::string> commands;
	BW::vector<BW::string> models;
	pOwnSect_->readStrings( "resource", models );
	if( !models.empty() )
	{
		commands.push_back( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/EDIT_IN_MODEL_EDITOR") );
/*		if( animationNames_.size() > 1 )
		{
			commands.push_back( "animation: (default)" );
			for( BW::vector<BW::string>::const_iterator iter = animationNames_.begin() + 1;
				iter != animationNames_.end(); ++iter )
				commands.insert( commands.end(), BW::string( "animation: " ) + *iter );
		}*/
	}
	return commands;
}

bool EditorChunkModel::edExecuteCommand( const BW::string& path, BW::vector<BW::string>::size_type index )
{
	BW_GUARD;

	if( path.empty() && index == 0 )
	{
		BW::vector<BW::string> models;
		pOwnSect_->readStrings( "resource", models );

		BW::string commandLine = "-o ";
		commandLine += '\"' + BWResource::resolveFilename( models[0] ) + '\"';
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
			"..\\..\\modeleditor\\win64\\modeleditor.exe",
#else
			"..\\..\\modeleditor\\win32\\modeleditor.exe",
#endif
			commandLine.c_str() ), 
			"WorldEditor" );
	}
	else if( path.empty() && animationNames_.size() > 1 && index - 1 < animationNames_.size() )
	{
		setAnimation( animationNames_[ index - 1 ] );
	}

	return false;
}

Vector3 EditorChunkModel::edMovementDeltaSnaps()
{
	BW_GUARD;

	if (isShellModel())
	{
		return Options::getOptionVector3( "shellSnaps/movement", Vector3( 0.f, 0.f, 0.f ) );
	}
	else
	{
		return EditorChunkItem::edMovementDeltaSnaps();
	}
}

float EditorChunkModel::edAngleSnaps()
{
	BW_GUARD;

	if (isShellModel())
	{
		return Options::getOptionFloat( "shellSnaps/angle", 0.f );
	}
	else
	{
		return EditorChunkItem::edAngleSnaps();
	}
}


BW::string EditorChunkModel::getModelName()
{
	BW::string modelNames;

	if (pSuperModel_ != NULL)
	{
		for (int i = 0; i < pSuperModel_->nModels(); i++)
		{
			if (i)
			{
				modelNames += ", ";
			}

			modelNames += pSuperModel_->topModel(i)->resourceID();
		} 
	}

	return modelNames;
}


bool EditorChunkModel::setAnimation(const BW::string & newAnimationName)
{
	BW_GUARD;

	if (newAnimationName.empty())
	{
		animName_ = "";
		if (pAnimation_)
		{
			this->refreshWantsUpdate( false );
			
			Matrix world( pChunk_->transform() );
			world.preMultiply( transform_ );

			// Since the model is now static it needs to cache
			// the default transforms
			pSuperModel_->cacheDefaultTransforms( world );
		}

		pAnimation_ = 0;

		return true;
	}
	else
	{
		SuperModelAnimationPtr newAnimation = pSuperModel_->getAnimation(
			newAnimationName );

		if (!newAnimation)
			return false;

		if (newAnimation->pSource( *pSuperModel_ ) == NULL)
			return false;

		newAnimation->time = 0.f;
		newAnimation->blendRatio = 1.0f;

		pAnimation_ = newAnimation;

		animName_ = newAnimationName;

		this->refreshWantsUpdate( true );

		return true;
	}
}

BW::string EditorChunkModel::getDyeTints( const BW::string& dye ) const
{
	BW_GUARD;

	if( tintName_.find( dye ) == tintName_.end() )
		return LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/DEFAULT_TINT_NAME");
	return tintName_.find( dye )->second;
}

bool EditorChunkModel::setDyeTints( const BW::string& dye, const BW::string& tint )
{
	BW_GUARD;

	SuperModelDyePtr newDye = pSuperModel_->getDye( dye, tint );

	if( !newDye )
		return false;

	if( tintMap_[ dye ] )
	{
		const MaterialFashionPtr pFashion = tintMap_[ dye ];
		materialFashions_.erase( std::find( materialFashions_.begin(),
			materialFashions_.end(),
			pFashion ) );
	}

	tintMap_[ dye ] = newDye;
	materialFashions_.push_back( tintMap_[ dye ] );

	tintName_[ dye ] = tint;

	return true;
}

bool EditorChunkModel::setAnimRateMultiplier( const float& f )
{
	BW_GUARD;

	if ( f < 0.f )
		return false;

	// limit animation preview speed to 100x the original speed
	float mult = f;
	if ( mult > 100.0f )
		mult = 100.0f;

	animRateMultiplier_ = mult;

	return true;
}


bool EditorChunkModel::resourceIsOutsideOnly() const
{
	BW_GUARD;

	if ( !pOwnSect_ )
		return false;

	DataSectionPtr modelResource = BWResource::openSection(
		pOwnSect_->readString( "resource") ) ;
	if ( modelResource )
	{
		return modelResource->readBool( "editorOnly/outsideOnly", false );
	}

	return false;
}


bool EditorChunkModel::setOutsideOnly( const bool& outsideOnly )
{
	BW_GUARD;

	if ( outsideOnly_ != outsideOnly )
	{
		//We cannot turn off outsideOnly, if the model resource
		//specifies it to be true.
		if ( !outsideOnly && this->resourceIsOutsideOnly() )
		{
			ERROR_MSG( "Cannot turn off outsideOnly because the .model file overrides the chunk entry\n" );
			return false;
		}

		outsideOnly_ = outsideOnly;
		if (!edTransform( transform_, false ))
		{
			ERROR_MSG( "Changed outsideOnly flag, but could not change the chunk for this model\n" );
		}
		return true;
	}

	return false;
}


bool EditorChunkModel::setCastsShadow( const bool& castsShadow )
{
	BW_GUARD;

	if ( castsShadow_ != castsShadow )
	{
		castsShadow_ = castsShadow;

		MF_ASSERT( pChunk_ != NULL );

		BoundingBox bb = BoundingBox::s_insideOut_;
		this->edBounds( bb );
		bb.transformBy( this->edTransform() );
		bb.transformBy( chunk()->transform() );

		BW::set<Chunk*> chunks;
		WorldManager::instance().collectOccupiedChunks( bb, pChunk_, chunks );
		WorldManager::instance().changedChunks( chunks,
			InvalidateFlags::FLAG_SHADOW_MAP |
			InvalidateFlags::FLAG_THUMBNAIL,
			*this );

		return true;
	}

	return false;
}

Moo::LightContainerPtr EditorChunkModel::edVisualiseLightContainer()
{
	if (!chunk())
		return NULL;

	Moo::LightContainerPtr lc = new Moo::LightContainer;

	BoundingBox bb = this->localBB();
	bb.transformBy( this->transform_ );
	bb.transformBy( this->chunk()->transform() );

	lc->init( ChunkLightCache::instance( *chunk() ).pAllLights(), bb, false );

	return lc;
}

bool fenceSectionOrderPredicate(EditorChunkModel *a, EditorChunkModel *b)
{
	const FenceInfo *pA = a->getFenceInfo(false);
	const FenceInfo *pB = b->getFenceInfo(false);

	if (pA == NULL)
		return false;
	else if (pB == NULL)
		return true;
	else
		return (pA->sectionId > pB->sectionId);
}

FenceInfo *EditorChunkModel::getFenceInfo(bool createIfNotExist, int fenceId, int sectionId)
{
	if (pFenceInfo_ == NULL && createIfNotExist)
	{
		if (fenceId == -1)
		{
			fenceId = int( s_fences.size() + 1 );
		}

		if (sectionId == -1)
		{
			BW::map<uint, BW::vector<EditorChunkModel*>>::iterator it =
				s_fences.find(fenceId);

			if (it == s_fences.end())
			{
				sectionId = 1;
			}
			else
			{
				sectionId = int( it->second.size() + 1 );
			}
		}

		pFenceInfo_ = new FenceInfo();
		pFenceInfo_->fenceId = fenceId;
		pFenceInfo_->sectionId = sectionId;
		createFenceInfo();
		toss(chunk());
	}

	return pFenceInfo_;
}

void EditorChunkModel::createFenceInfo()
{
	MF_ASSERT( pFenceInfo_ != nullptr );

	BW::map<uint, BW::vector<EditorChunkModel*>>::iterator it =
		s_fences.find( pFenceInfo_->fenceId );

	if (it == s_fences.end())
	{
		BW::vector<EditorChunkModel*> sections;
		sections.push_back( this );
		s_fences[pFenceInfo_->fenceId] = sections;
	}
	else
	{
		BW::vector<EditorChunkModel*>& sections = it->second;
		
		if (std::find( sections.begin(), sections.end(), this ) == sections.end())
		{
			sections.push_back( this );
			std::sort(sections.begin(), sections.end(), fenceSectionOrderPredicate);
		}
	}
}

void EditorChunkModel::deleteFenceInfo( bool keepValues )
{
	if (pFenceInfo_ == NULL)
		return;

	BW::map<uint, BW::vector<EditorChunkModel*> >::iterator it =
		s_fences.find(pFenceInfo_->fenceId);

	if (it != s_fences.end())
	{
		BW::vector<EditorChunkModel*> &sections = it->second;
		BW::vector<EditorChunkModel*>::iterator itS = 
			std::find(sections.begin(), sections.end(), this);
		if (itS != sections.end())
			sections.erase(itS);
		if (sections.size() == 0)
			s_fences.erase(it);
	}

	if ( !keepValues )
	{
		bw_safe_delete( pFenceInfo_);
	}
}


EditorChunkModel *EditorChunkModel::getPrevFenceSection()
{
	if (pFenceInfo_ == NULL)
		return NULL;

	BW::map<uint, BW::vector<EditorChunkModel*> >::iterator it =
		s_fences.find(pFenceInfo_->fenceId);

	if (it == s_fences.end())
		return NULL;

	const BW::vector<EditorChunkModel*> &sections = it->second;
	EditorChunkModel *pPrevSection = NULL;

	for (size_t i = 0; i < sections.size(); i++)
	{
		FenceInfo *pFenceInfo = sections[i]->pFenceInfo_;
		if (sections[i]->isInScene_ 
			&& pFenceInfo != NULL
			&& pFenceInfo->sectionId < pFenceInfo_->sectionId
			&& (pPrevSection == NULL || pFenceInfo->sectionId > pPrevSection->pFenceInfo_->sectionId))
		{
			pPrevSection = sections[i];
		}
	}

	return pPrevSection;
}

void EditorChunkModel::edSelected( BW::vector<ChunkItemPtr>& selection )
{
	BW_GUARD;

	EditorChunkItem::edSelected( selection );

	if (!s_autoSelectFenceSections || s_isAutoSelectingFence 
		|| pFenceInfo_ == NULL || UndoRedo::instance().isUndoing()
		|| InputDevices::instance().isShiftDown())
		return;

	BW::map<uint, BW::vector<EditorChunkModel*> >::iterator itF =
		s_fences.find( pFenceInfo_->fenceId );
	if (itF == s_fences.end())
		return;

	s_isAutoSelectingFence = true;
	bool selectionChanged = false;

	for (size_t i = 0; i < itF->second.size(); i++)
	{
		EditorChunkModel *pModel = itF->second[i];
		auto found = std::find( selection.begin(), selection.end(), pModel );
		
		if ( !pModel->edIsSelected() && pModel->isInScene_ &&
			found == selection.end() )
		{
			pModel->edSelected( selection );
			selection.push_back( ChunkItemPtr( pModel ) );
		}
	}

	s_isAutoSelectingFence = false;
}


/*static */void EditorChunkModel::startBigModelLoad()
{
	s_AllowBigModels = true;
}


/*static */void EditorChunkModel::endBigModelLoad()
{
	s_AllowBigModels = false;
}

/// Write the factory statics stuff
/// Write the factory statics stuff
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk)
IMPLEMENT_CHUNK_ITEM( EditorChunkModel, model, 2 )
IMPLEMENT_CHUNK_ITEM_ALIAS( EditorChunkModel, shell, 2 )

BW_END_NAMESPACE
// editor_chunk_model.cpp
