
#include "pch.hpp"

#include "chunk_model.hpp"
#include "chunk_model_obstacle.hpp"
#include "chunk_manager.hpp"
#include "geometry_mapping.hpp"

#include "math/blend_transform.hpp"

#include "model/model_animation.hpp"

#include "model/fashion.hpp"
#include "model/super_model.hpp"
#include "model/super_model_animation.hpp"
#include "model/super_model_dye.hpp"

#include "moo/resource_load_context.hpp"

#if UMBRA_ENABLE
#include "umbra_chunk_item.hpp"
#include "chunk_umbra.hpp"
#endif

#include "fmodsound/sound_manager.hpp"

#include "physics2/bsp.hpp"

#ifndef CODE_INLINE
#include "chunk_model.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )


BW_BEGIN_NAMESPACE

int ChunkModel_token;

// ----------------------------------------------------------------------------
// Section: ChunkModel
// ----------------------------------------------------------------------------

#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk, &errorString)
IMPLEMENT_CHUNK_ITEM( ChunkModel, model, 0 )
IMPLEMENT_CHUNK_ITEM_ALIAS( ChunkModel, shell, 0 )


ChunkModel::ChunkModel() :
	ChunkItem( (WantFlags)(WANTS_DRAW) ),
	pSuperModel_( NULL ),
	pAnimation_( NULL ),
	tickMark_( ChunkManager::instance().tickMark() ),
	lastTickTimeInMS_( ChunkManager::instance().totalTickTimeInMS() ),
	castsShadow_( true ),
	animRateMultiplier_( 1.f ),
	transform_( Matrix::identity ),
	worldTransform_( Matrix::identity ),
	reflectionVisible_( false ),
	calculateIsShellModel_( true ),
	cachedIsShellModel_( false ),
	pChunkBeforeReload_( NULL )
{
}


ChunkModel::~ChunkModel()
{
	BW_GUARD;
	materialFashions_.clear();
	pAnimation_ = NULL;

	bw_safe_delete(pSuperModel_);
}


// in chunk.cpp
extern void readMooMatrix( DataSectionPtr pSection, const BW::string & tag,
	Matrix &result );


 /**
 * if pSuperModel_ has been reloaded, 
 * update the related data.
 */
void ChunkModel::onReloaderReloaded( Reloader* pReloader )
{
	BW_GUARD;

	if (pReloader && pSuperModel_->hasModel( (Model*) pReloader, true ))
	{
		this->pullInfoFromSuperModel( pChunkBeforeReload_, true );
		// Re-toss, so data like bsp get re-add into space
		if (pChunkBeforeReload_ != NULL)
		{
			this->ChunkModel::toss( pChunkBeforeReload_ );
			pChunkBeforeReload_ = NULL;

			// If the wants update flag is not set
			// we cache the default transforms in the SuperModel
			if (!this->wantsUpdate())
			{
				pSuperModel_->cacheDefaultTransforms( this->worldTransform() );
			}
		}
#if ENABLE_BSP_MODEL_RENDERING
		// Remove the old bsp from infos_ so it will be recreated again if bsp need be drawn. 
		this->delBsp();
#endif // ENABLE_BSP_MODEL_RENDERING
	}
}


 /**
 * if pSuperModel_ is about to be reloaded, 
 */
void ChunkModel::onReloaderPreReload( Reloader* pReloader )
{
	BW_GUARD;

	if (pReloader && pSuperModel_->hasModel( (Model*) pReloader, true ))
	{
		pChunkBeforeReload_ = pChunk_;
		// remove from pChunk_ so later it can be re-tossed.
		if (pChunk_ != NULL)
		{
			this->ChunkModel::toss( NULL );
		}
	}
}

/**
 *  This function pull data out from our visual
 *	@param pChunk pass in the chunk the model lives in, it does not use the
 *					pChunk_ property since this can be called before the model
 *					has officially been put in a chunk
 *	@return			true if update is needed when
 *					 pSuperModel_ is reloaded.
 */
bool ChunkModel::pullInfoFromSuperModel( Chunk* pChunk, bool reload /*= false*/ )
{
	BW_GUARD;
	SuperModel::ReadGuard guard( pSuperModel_ );
	bool modelDependent = false;

#ifndef EDITOR_ENABLED
	for (int i = 0; i < pSuperModel_->nModels(); i++)
	{
		ModelPtr pModel = pSuperModel_->topModel( i );
		if (pModel
#ifndef ENABLE_RELOAD_MODEL
			//Only check this if reload not defined, otherwise we want
			// to always reload if a model is present
			&& pModel->decompose()
#endif
			)
		{
			modelDependent = true;
			break;
		}
	}
#else
	// since for tools we always create a bsp model if it doesn't have one
	// so we need update it.
	modelDependent = true;
#endif//EDITOR_ENABLED

	return modelDependent;
}


/**
 *	Load yourself from this section
 */
bool ChunkModel::load( DataSectionPtr pSection,
					  Chunk * pChunk, BW::string* pErrorString )
{
	BW_GUARD;
	pAnimation_ = NULL;
	materialFashions_.clear();
	label_.clear();
	calculateIsShellModel_ = true;
	cachedIsShellModel_ = false;

	label_ = pSection->asString();

	BW::vector<BW::string> models;
	pSection->readStrings( "resource", models );

	castsShadow_ = pSection->readBool( "castsShadow", castsShadow_ );
	castsShadow_ = pSection->readBool( "editorOnly/castsShadow", castsShadow_ ); // check legacy

	//uint64 ltime = timestamp();
	BW::string mname;
	if (!models.empty()) mname = models[0].substr( models[0].find_last_of( '/' )+1 );
	Moo::ScopedResourceLoadContext resLoadCtx( mname );

	MF_ASSERT( pSuperModel_ == NULL );
	pSuperModel_ = new SuperModel( models );

    // Mutually exclusive of pSuperModel_ reloading, start
	pSuperModel_->beginRead();

	// Only attempt to load the model if there are child models of this super model.
	//	if there are no children then we want to clean up the supermodel cleanly, 
	//	& fill out any errors
	const bool bLoadSuccess = pSuperModel_->nModels() > 0;

	if (bLoadSuccess)
	{
#ifndef MF_SERVER
		sceneObject_.flags().isCastingStaticShadow( castsShadow_ );
		sceneObject_.flags().isCastingDynamicShadow( castsShadow_ );
#endif // MF_SERVER

		// load the specified animation
		DataSectionPtr pAnimSec = pSection->openSection( "animation" );
		if (pAnimSec)
		{
			pAnimation_ = pSuperModel_->getAnimation(
				pAnimSec->readString( "name" ) );
			pAnimation_->time = 0.f;
			pAnimation_->blendRatio = 1.0f;
			animRateMultiplier_ = pAnimSec->readFloat( "frameRateMultiplier", 1.f );

			if (pAnimation_->pSource( *pSuperModel_ ) == NULL)
			{
				ASSET_MSG( "SuperModel can't find its animation %s\n",
					pAnimSec->readString( "name" ).c_str() );
				pAnimation_ = NULL;
			}
			else
			{
				// Since we have an animation we need to update
				// the wants update flag
				this->refreshWantsUpdate( true );
			}
		}

		// Load up dyes
		BW::vector<DataSectionPtr> dyeSecs;
		pSection->openSections( "dye", dyeSecs );

		for( BW::vector<DataSectionPtr>::iterator iter = dyeSecs.begin(); iter != dyeSecs.end(); ++iter )
		{
			BW::string dye = ( *iter )->readString( "name" );
			BW::string tint = ( *iter )->readString( "tint" );
			SuperModelDyePtr dyePtr = pSuperModel_->getDye( dye, tint );

			if( dyePtr )
			{
				materialFashions_.push_back( dyePtr );
			}
		}

		// load our transform
		readMooMatrix( pSection, "transform", transform_ );
		this->updateWorldTransform( pChunk );

		reflectionVisible_ = pSection->readBool( "reflectionVisible", reflectionVisible_ );
		//reflectionVisible_ |= this->resourceIsOutsideOnly();

		if (this->pullInfoFromSuperModel( pChunk ))
		{
			pSuperModel_->registerModelListener( this );
		}
	}
	// Mutually exclusive of pSuperModel_ reloading, end
	pSuperModel_->endRead();

	// super model doesn't have any children, clean supermodel up, & report errors
	if (!bLoadSuccess)
	{
		WARNING_MSG( "No models loaded into SuperModel\n" );
		bw_safe_delete(pSuperModel_);
		// report an error if we have an output error string
		if (pErrorString)
		{
			*pErrorString = "Could not load ChunkModel ";
			for (uint i = 0; i < models.size(); i++)
			{
				if ( i > 0 )
				{
					*pErrorString = *pErrorString + ",";
				}
				*pErrorString = *pErrorString + " '" + models[i] + "'";
			}
		}
	}

	return bLoadSuccess;
}

CollisionSceneProviderPtr ChunkModel::getCollisionSceneProvider( Chunk* chunkToCalc ) const
{
	const BSPTree* tree = pSuperModel_->topModel(0)->decompose();

	if (tree)
	{
		RealWTriangleSet portalTriangles;

		// If the current chunk is an outside chunk then it will already have a
		// convex hull which will be used to cull outside areas
		if (isShellModel() && chunkToCalc->isOutsideChunk())
		{
			const float OFFSET = ( chunkToCalc == pChunk_ ? -1.f : 1.f );

			for (Chunk::piterator iter = pChunk_->pbegin();
				iter != pChunk_->pend(); iter++)
			{
				// Only create portal walls for chunk we are working with,
				// otherwise small portals could cross into the bounds of this chunk
				if (iter->hasChunk() && (iter->pChunk == chunkToCalc || pChunk_ == chunkToCalc))
				{
					Vector3 offset = iter->plane.normal() * OFFSET;
					Vector3 p0 = iter->objectSpacePoint( 0 ) + offset;
					Vector3 p1 = iter->objectSpacePoint( 1 ) + offset;

					for (int i = 2; i < (int)iter->points.size(); ++i)
					{
						Vector3 p2 = iter->objectSpacePoint( i ) + offset;

						portalTriangles.push_back( WorldTriangle( p0, p1, p2 ) );
						p1 = p2;
					}
				}
			}
		}

		return new BSPTreeCollisionSceneProvider( tree,
			this->worldTransform(),
			portalTriangles );
	}

	return NULL;
}


/**
 *	Re-calculate LOD and apply animations to this chunk model for this frame.
 *	Should be called once per frame, unless the ChunkModel is static and does
 *	not require its animations updated every frame (and so has been marked
 *	without WANTS_UPDATE). In this case this function is not called and
 *	the model's LOD will be updated before it is drawn.
 */
void ChunkModel::updateAnimations()
{
	BW_GUARD_PROFILER( ChunkModel_updateAnimations );
	static DogWatch drawWatch( "ChunkModel updateAnimations" );
	ScopedDogWatch watcher( drawWatch );

	TRANSFORM_ASSERT( this->wantsUpdate() );

	if (pSuperModel_ != NULL)
	{
		this->tickAnimation();

		TmpTransforms preTransformFashions;
		if (pAnimation_)
		{
			preTransformFashions.push_back( pAnimation_.get() );
		}

		pSuperModel_->updateAnimations( this->worldTransform(),
			&preTransformFashions, // preTransformFashions
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
void ChunkModel::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD_PROFILER( ChunkModel_draw );
	static DogWatch drawWatch( "ChunkModel draw" );
	ScopedDogWatch watcher( drawWatch );

	//TODO: optimise
	//TODO: sort out the overriding of reflection (should be able to set the
	// reflection
	// per model but be able to override the setting in the editor.
	//TODO: distance check from water position?
	if (Moo::rc().reflectionScene() && !reflectionVisible_)
	{
		return;
	}

	if ( Moo::rc().dynamicShadowsScene() && !castsShadow_ )
	{
		return;
	}

#if ENABLE_BSP_MODEL_RENDERING
	if (this->drawBspInternal())
	{
		// If a BSP was drawn, don't draw the object
		return;
	}
#endif // ENABLE_BSP_MODEL_RENDERING

	if (pSuperModel_ != NULL) 
	{
		// Update lod levels if the super model has not been updated
		// as part of the update loop
		if (!this->wantsUpdate())
		{
			pSuperModel_->updateLodsOnly( this->worldTransform(),
				Model::LOD_AUTO_CALCULATE );
		}

		pSuperModel_->draw( drawContext, this->worldTransform(), &materialFashions_ );
	}
}


#if ENABLE_BSP_MODEL_RENDERING
/**
 *	Draw the BSP of the chunk model.
 *	@return true if it was drawn, false if the BSP is not visible.
 */
bool ChunkModel::drawBspInternal()
{
	BW_GUARD;

	// Check if drawing BSPs
	if (!ChunkBspHolder::getDrawBsp() || !ChunkBspHolder::getDrawBspOtherModels())
	{
		return false;
	}

	// Don't draw reflections for BSPs
	if (Moo::rc().reflectionScene())
	{
		return true;
	}

	// Load up the BSP tree if needed
	if (!this->isBspCreated())
	{
		// No vertices loaded yet, create some
		const BSPTree * tree = pSuperModel_->topModel(0)->decompose();

		if ((tree != NULL) && !tree->empty())
		{
			BW::vector< Moo::VertexXYZL > verts;
			Moo::Colour colour;
			this->getRandomBspColour( colour );
			Moo::BSPTreeHelper::createVertexList( *tree, verts, colour );

			// If BSP has collideable vertices
			if (!verts.empty())
			{
				this->addBsp( verts, pSuperModel_->topModel( 0 )->resourceID() );
				MF_ASSERT( this->isBspCreated() );
			}
		}
	}

	// Draw the BSP
	if (this->isBspCreated())
	{
		this->batchBsp( this->worldTransform() );
		return true;
	}

	return false;
}
#endif // ENABLE_BSP_MODEL_RENDERING


void ChunkModel::syncInit()
{
	BW_GUARD;

	// If the wants update flag is not set
	// we cache the default transforms in the SuperModel
	if (!this->wantsUpdate())
	{
		pSuperModel_->cacheDefaultTransforms( this->worldTransform() );
	}

#if UMBRA_ENABLE
	this->syncUmbra();
#endif

#if FMOD_SUPPORT
	if (SoundManager::instance().modelOcclusionEnabled())
	{
		soundOccluder_.construct( pSuperModel_ );
		soundOccluder_.update( this->worldTransform() );
	}
#endif
}


#if UMBRA_ENABLE
void ChunkModel::syncUmbra()
{
	if (!pChunk_)
	{
		// This can happen if we are representing a VLO model.
		return;
	}

	// Delete any old umbra draw item
	bw_safe_delete(pUmbraDrawItem_);

	// Grab the visibility bounding box
	BoundingBox bb = BoundingBox::s_insideOut_;
	pSuperModel_->localVisibilityBox(bb);

	// Create a umbra model based on the bounding box of the model.

	UmbraModelProxyPtr pUmbraModel = UmbraModelProxy::getObbModel( bb.minBounds(), bb.maxBounds() );
	UmbraObjectProxyPtr pUmbraObject = UmbraObjectProxy::get( pUmbraModel );

	UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem;
	pUmbraChunkItem->init( this,
		pUmbraObject,
		this->worldTransform(),
		pChunk_->getUmbraCell() );

	pUmbraDrawItem_ = pUmbraChunkItem;
	this->updateUmbraLenders();
	this->syncShadowCaster();
}
#endif


void ChunkModel::syncShadowCaster()
{
	BW_GUARD;

#if UMBRA_ENABLE
	Matrix m = pChunk_->transform();
	m.preMultiply( transform_ );

	// Delete any old umbra shadow caster item.
	delete pUmbraShadowCasterTestItem_;

	// Grab the visibility bounding box
	BoundingBox bb = BoundingBox::s_insideOut_;
	pSuperModel_->localVisibilityBox( bb );

	UmbraChunkItemShadowCaster* pUmbraChunkItem = new UmbraChunkItemShadowCaster;
	pUmbraChunkItem->init( this, bb, m, pChunk_->getUmbraCell() );
	pUmbraShadowCasterTestItem_ = pUmbraChunkItem;
#endif
}


/**
 *	This method refreshes the wants update flag for this
 *	ChunkModel, adding or removing it from the space
 *	update list as needed
 *	@param wantsUpdate value to set wants update to
 */
void ChunkModel::refreshWantsUpdate( bool wantsUpdate )
{
	if (this->wantsUpdate() != wantsUpdate)
	{
		if (wantsUpdate)
		{
			wantFlags_  = WantFlags(wantFlags_ | WANTS_UPDATE);

			if (pChunk_ && pChunk_->space())
			{
				pChunk_->space()->addUpdateItem( this );
			}
		}
		else
		{
			if (pChunk_ && pChunk_->space())
			{
				pChunk_->space()->delUpdateItem( this );
		}
			wantFlags_ = WantFlags(wantFlags_ & ~WANTS_UPDATE);
		}
	}
}


/**
 *	overridden lend method
 */
void ChunkModel::lend( Chunk * pLender )
{
	BW_GUARD;
	if (pSuperModel_ != NULL && pChunk_ != NULL)
	{
		BoundingBox bb;
		pSuperModel_->localVisibilityBox( bb );
		bb.transformBy( this->worldTransform() );

		this->lendByBoundingBox( pLender, bb );
	}
}


/**
 *	label accessor
 */
const char * ChunkModel::label() const
{
	return label_.c_str();
}


/**
 *	This method returns whether or not this model should cast a shadow.
 *
 *  @return		Returns whether or not this model should cast a shadow
 */
bool ChunkModel::affectShadow() const
{
	return this->hasBSP() && castsShadow_;
}

/**
 *	This method returns whether or not this model has bsp.
 *
 *  @return		Returns whether or not this model has bsp.
 */
bool ChunkModel::hasBSP() const
{
	bool hasBSP = false;

	if (this->pSuperModel_ && this->pSuperModel_->nModels() > 0)
	{
		const BSPTree* bsp = this->pSuperModel_->topModel( 0 )->decompose();

		if (bsp && bsp->numTriangles())
		{
			hasBSP = true;
		}
	}

	return hasBSP;
}


/**
 *	Add this model to (or remove it from) this chunk
 */
void ChunkModel::toss( Chunk * pChunk )
{
	BW_GUARD;
	// remove it from old chunk
	if (pChunk_ != NULL)
	{
		ChunkModelObstacle::instance( *pChunk_ ).delObstacles( this );
	}

	// call base class method
	this->ChunkItem::toss( pChunk );
	this->updateWorldTransform( this->chunk() );

	// add it to new chunk
	if (pChunk_ != NULL)
	{
		for (int i = 0; i < this->pSuperModel_->nModels(); i++)
		{
			ChunkModelObstacle::instance( *pChunk_ ).addModel(
				this->pSuperModel_->topModel( i ),
				this->worldTransform(),
				this );
		}
	}
}


void ChunkModel::generateTextureUsage( Moo::ModelTextureUsageGroup& usageGroup )
{
	if (pSuperModel_)
	{
		pSuperModel_->generateTextureUsage( usageGroup );
	}
}


void ChunkModel::tossWithExtra( Chunk * pChunk, SuperModel* extraModel )
{
	BW_GUARD;
	this->ChunkModel::toss(pChunk);

	if (pChunk_ != NULL && extraModel != NULL)
	{
		ChunkModelObstacle::instance( *pChunk_ ).addModel(
			extraModel->topModel( 0 ), this->worldTransform(), this, true );
	}
}


bool ChunkModel::addYBounds( BoundingBox& bb ) const
{
	BW_GUARD;
	if (pSuperModel_)
	{
		BoundingBox me;
		pSuperModel_->localVisibilityBox( me );
		me.transformBy( transform_ );
		bb.addYBounds( me.minBounds().y );
		bb.addYBounds( me.maxBounds().y );
	}
	return true;
}


/**
 * Are we the interior mesh for the chunk?
 *
 * We check by seeing if the model is in the shells directory
 */
bool ChunkModel::isShellModel() const
{
	BW_GUARD;

	if (chunk() && chunk()->isOutsideChunk())
		return false;

	if( !pSuperModel_ || !pSuperModel_->topModel( 0 ) )
		return false;

	if (calculateIsShellModel_)
	{
		calculateIsShellModel_ = false;
		BW::string itemRes = pSuperModel_->topModel( 0 )->resourceID();

		cachedIsShellModel_ = itemRes.substr( 0, 7 ) == "shells/" ||
							itemRes.find( "/shells/" ) != BW::string::npos;
	}
	return cachedIsShellModel_;
}


/**
 *	This method ticks the animation.
 */
void ChunkModel::tickAnimation()
{
	BW_GUARD_PROFILER( ChunkModel_tickAnimation );
	// If we have an animation and we have not been ticked this frame,
	// update the animation time
	ModelAnimation* pModelAnim = (pAnimation_) ? pAnimation_->pSource( *pSuperModel_ ) : NULL;
	if (pModelAnim && tickMark_ != ChunkManager::instance().tickMark())
	{
		// Get the delta time since last time the animation time was updated
		float dTime = float(ChunkManager::instance().totalTickTimeInMS() - lastTickTimeInMS_) /
			1000.f;

		// Update our stored tick mark and tick time
		tickMark_ = ChunkManager::instance().tickMark();
		lastTickTimeInMS_ = ChunkManager::instance().totalTickTimeInMS();

		// Update animation time
		pAnimation_->time += dTime * animRateMultiplier_;
		float duration = pModelAnim->duration_;
		pAnimation_->time -= int64( pAnimation_->time / duration ) * duration;
	}
}


/*static */const BW::string & ChunkModel::getSectionName()
{
	static const BW::string s_chunkModelSectionName( "model" );
	return s_chunkModelSectionName;
}

BW_END_NAMESPACE

// chunk_model.cpp
