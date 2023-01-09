#include "pch.hpp"

#include "super_model.hpp"

#ifdef EDITOR_ENABLED
#include "../../tools/common/material_properties.hpp"
#endif

#include "cstdmf/bw_set.hpp"

#include "moo/draw_context.hpp"

#include "moo/lod_settings.hpp"

#include "model_actions_iterator.hpp"
#include "super_model_dye.hpp"
#include "super_model_animation.hpp"
#include "super_model_action.hpp"
#include "tint.hpp"


DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Init the material and its default tint and return how many we found of it.
 */
int initMatter_NewVisual( Matter & m, Moo::Visual & bulk )
{
	BW_GUARD;
	Moo::ComplexEffectMaterialPtr pOriginal;
	Matter::PrimitiveGroups primGroups;

	int num = bulk.gatherMaterials( m.replaces_, primGroups, &pOriginal );
	// it's possible that we didn't find the material from visual that's match the tint
	// in which case m.primitiveGroups_ will be empty
	std::swap<>( primGroups, m.primitiveGroups_ );
	MF_ASSERT_DEV( !m.tints_.empty() );
	for (uint i=0; i < m.tints_.size(); i++)
	{
		if( pOriginal.exists() )
			m.tints_[i]->effectMaterial_ = new Moo::ComplexEffectMaterial( *pOriginal );
		else
			m.tints_[i]->effectMaterial_ = new Moo::ComplexEffectMaterial;
	}

	return num;
}

/**
 *	Init the tint from the given material section, return true if ok.
 *	Should always leave the Tint in a good state.
 */
bool initTint_NewVisual( Tint & t, DataSectionPtr matSect )
{
	BW_GUARD;
	t.effectMaterial_ = NULL;
	t.effectMaterial_ = new Moo::ComplexEffectMaterial();

	if (!matSect)
		return false;

	return t.effectMaterial_->load( matSect );
}




Moo::EffectPropertyPtr findTextureProperty( Moo::EffectMaterialPtr m )
{
	BW_GUARD;
	if ( m && m->pEffect() )
	{
		ComObjectWrap<ID3DXEffect> pEffect = m->pEffect()->pEffect();

		Moo::EffectMaterial::Properties& properties = m->properties();
		Moo::EffectMaterial::Properties::iterator it = properties.begin();
		Moo::EffectMaterial::Properties::iterator end = properties.end();

		while ( it != end )
		{
			IF_NOT_MF_ASSERT_DEV( it->second )
			{
				continue;
			}
			D3DXHANDLE hParameter = it->first;
			Moo::EffectPropertyPtr& pProperty = it->second;

			D3DXPARAMETER_DESC desc;
			HRESULT hr = pEffect->GetParameterDesc( hParameter, &desc );
			if ( SUCCEEDED(hr) )
			{
				if (desc.Class == D3DXPC_OBJECT &&
					desc.Type == D3DXPT_TEXTURE)
				{
					return pProperty;
				}
			}
			it++;
		}
	}

	return NULL;
}




// ----------------------------------------------------------------------------
// Section: SuperModel
// ----------------------------------------------------------------------------

/**
 *  This Method tell if the pModel is one of my Models.
 */
bool SuperModel::hasModel( const Model* pModel, bool checkParent/*= false*/ ) const
{
	for (Models::size_type i = 0; i < nModels_; ++i)
	{
		Model* pTempModel = models_[i].topModel_.get();
		while( pTempModel != NULL)
		{
			if (pTempModel == pModel)
			{
				return true;
			}
			pTempModel = (checkParent)? pTempModel->parent(): NULL;
		}
	}
	return false;
}


/**
 *  This Method tell if the pVisual is one of my Models' visual.
 */
bool SuperModel::hasVisual( const Moo::Visual* pVisual, 
									bool checkParent/*= false*/  ) const
{
	for (Models::size_type i = 0; i < nModels_; ++i)
	{
		Model* pTempModel = models_[i].topModel_.get();
		while( pTempModel != NULL)
		{
			if (pTempModel && pTempModel->getVisual().get() == pVisual)
			{
				return true;
			}
			pTempModel = (checkParent)? pTempModel->parent(): NULL;
		}
	}
	return false;
}


/**
 *  @return Tree giving the merged node tree for the top LOD for all
 *		constituent models, or NULL if their graph is incompatible with
 *		SuperModelNodeTree (@see SuperModelNodeTree).
 */
const SuperModelNodeTree * SuperModel::getInstanceNodeTree() const
{
	BW_GUARD;

	if (instanceNodeTreeDirty_)
	{
		//This does not modify externally visible state, we only call it
		//here to potentially defer the calculation. Therefore, const_cast
		//use is reasonable.
		const_cast<SuperModel *>(this)->regenerateInstanceNodeTree();
	}
	return instanceNodeTree_.get();
}


/**
 *	When the SuperModelNodeTree is constructed it is assumed that the models of
 *	the SuperModel won't change, and neither will Moo::Node identifiers or
 *	relationships. If they do, the tree will need to be regenerated.
 *	
 *	@return True if the tree was able to be generated successfully, false if
 *		the node structure is incompatible.
 */
bool SuperModel::regenerateInstanceNodeTree()
{
	BW_GUARD;

	++numInstanceNodeTreeRegens_;
	instanceNodeTreeDirty_ = false;

	instanceNodeTree_.reset( new SuperModelNodeTree() );

	//Need to iterate through all models, getting the
	//base node from their visual and adding to the mesh.
	MF_ASSERT( nModels_ == models_.size() );
	for (Models::size_type i = 0; i < nModels_; ++i)
	{
		//We only use the top level LOD, lower LODs should be using the
		//same skeleton (or a compatible subset) for the SuperModelNodeTree
		//to be utilised.

		Moo::ConstNodePtr visualRoot =
			models_[i].topModel_->getVisual()->rootNode();

		if (visualRoot.hasObject() &&
			!instanceNodeTree_->addAllNodes( *visualRoot ))
		{
			instanceNodeTree_.reset();
			return false;
		}
	}

	//if ( instanceNodeTree_.get() != NULL )
	//{
	//	DEBUG_MSG( "SMTree %s.\n", instanceNodeTree_->desc().c_str() );
	//}

	return (instanceNodeTree_.get() != NULL);
}

/**
 * This function is used to generate a list of texture usages that
 * this super model may attempt to draw with.
 */
void SuperModel::generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup )
{
	for (Models::size_type i = 0; i < nModels_; ++i)
	{
		ModelPartChain& mpc = models_[i];
		for (BW::vector<LodLevelData>::size_type lodIndex = 0; 
			lodIndex < mpc.lodChain_.size(); ++lodIndex)
		{
			LodLevelData& lodData = mpc.lodChain_[lodIndex];

			lodData.pModel->generateTextureUsage( usageGroup );
		}
	}
}

/**
 *	Users can remember the value returned by this function and rely on indices
 *	in the SuperModelNodeTree (returned by getInstanceNodeTree()) remaining
 *	valid so long as this function keeps returning the same value.
 *	
 *	@return The number of times that regenerateInstanceNodeTree will have been
 *		called if getInstanceNodeTree() is invoked (since calling that function
 *		might generate or regenerate the tree if required). 
 */
uint32 SuperModel::getNumInstanceNodeTreeRegens() const
{
	return numInstanceNodeTreeRegens_ + (instanceNodeTreeDirty_ ? 1 : 0);
}


/**
 *	@pre getInstanceNodeTree() is non-NULL and consistent with the current
 *		node state (node graphs/identifiers haven't been changed).
 *	@pre @a instanceNodeTreeIndex is a valid node index in
 *		getInstanceNodeTree().
 *	@post Returned The world transform for the node @a instanceNodeTreeIndex.
 */
const Matrix & SuperModel::getNodeWorldTransform(
	int instanceNodeTreeIndex ) const
{
	BW_GUARD;

	const SuperModelNodeTree * tree = getInstanceNodeTree();
	MF_ASSERT( tree != NULL );
	MF_ASSERT( 0 <= instanceNodeTreeIndex );
	MF_ASSERT( instanceNodeTreeIndex < tree->getNumNodes() );

	return
		tree->getNode( instanceNodeTreeIndex ).getMooNode()->worldTransform();
}


/**
 *	@pre getInstanceNodeTree() is non-NULL and consistent with the current
 *		node state (node graphs/identifiers haven't been changed).
 *	@pre @a instanceNodeTreeIndex is a valid node index in
 *		getInstanceNodeTree().
 *	@post Set the world transform for the node @a instanceNodeTreeIndex to
 *		@a transform.
 */
void SuperModel::setNodeWorldTransform(
	int instanceNodeTreeIndex, const Matrix & transform )
{
	BW_GUARD;

	const SuperModelNodeTree * tree = getInstanceNodeTree();
	MF_ASSERT( tree != NULL );
	MF_ASSERT( 0 <= instanceNodeTreeIndex );
	MF_ASSERT( instanceNodeTreeIndex < tree->getNumNodes() );

	tree->getNode( instanceNodeTreeIndex
		).getMooNode()->worldTransform( transform );
}


/**
 *	@pre getInstanceNodeTree() is non-NULL and consistent with the current
 *		node state (node graphs/identifiers haven't been changed).
 *	@pre @a instanceNodeTreeIndex is a valid node index in
 *		getInstanceNodeTree().
 *	@post Returned The transform for the node @a instanceNodeTreeIndex
 *		relative to its parent.
 */
const Matrix & SuperModel::getNodeRelativeTransform(
	int instanceNodeTreeIndex ) const
{
	BW_GUARD;

	const SuperModelNodeTree * tree = getInstanceNodeTree();
	MF_ASSERT( tree != NULL );
	MF_ASSERT( 0 <= instanceNodeTreeIndex );
	MF_ASSERT( instanceNodeTreeIndex < tree->getNumNodes() );

	return
		tree->getNode( instanceNodeTreeIndex ).getMooNode()->transform();
}


/**
 *  @pre getInstanceNodeTree() is non-NULL and consistent with the current
 *		node state (node graphs/identifiers haven't been changed).
 *	@pre @a instanceNodeTreeIndex is a valid node index in
 *		getInstanceNodeTree().
 *	@post Set the transform for the node @a instanceNodeTreeIndex
 *		relative to its parent to @a transform.
 */
void SuperModel::setNodeRelativeTransform(
	int instanceNodeTreeIndex, const Matrix & transform )
{
	BW_GUARD;

	const SuperModelNodeTree * tree = getInstanceNodeTree();
	MF_ASSERT( tree != NULL );
	MF_ASSERT( 0 <= instanceNodeTreeIndex );
	MF_ASSERT( instanceNodeTreeIndex < tree->getNumNodes() );

	tree->getNode( instanceNodeTreeIndex
		).getMooNode()->transform( transform );
}


///Required to implement ReloadListener.
void SuperModel::onReloaderReloaded( Reloader * pReloader )
{
	//Will be called when a model is reloaded. In this case the node tree
	//will need to be recalculated.
	if (this->hasModel( (Model*)pReloader, true ))
	{
		instanceNodeTreeDirty_ = true;
	}

	for (Models::iterator it = models_.begin();
		it != models_.end();
		++it)
	{
		ModelPartChain& modelChain = (*it);
		if (modelChain.topModel_ == (Model*)pReloader  ||
			modelChain.topModel_->isAncestor( (Model*)pReloader ))
		{
			//re-generate lods.
			modelChain.pullInfoFromModel();
		}
	}
	lodNextUp_ = Model::LOD_MAX;
	lodNextDown_ = Model::LOD_MAX;
}


/**
 *	This method deregister a listener to my models_
 */
void SuperModel::deregisterModelListener( ReloadListener* pListener )
{
#if ENABLE_RELOAD_MODEL
	for (Models::size_type i = 0; i < nModels_; ++i)
	{
		if (models_[i].curModel())
		{
			models_[i].curModel()->deregisterListener( pListener, true );
		}
	}
#endif//ENABLE_RELOAD_MODEL
}


/**
 *	This method register a listener to my models_
 */
void SuperModel::registerModelListener( ReloadListener* pListener )
{
#if ENABLE_RELOAD_MODEL
	for (Models::size_type i = 0; i < nModels_; ++i)
	{
		if (models_[i].curModel())
		{
			models_[i].curModel()->registerListener( pListener );
		}
	}
#endif//ENABLE_RELOAD_MODEL
}


/**
 *	This function is a concurrency lock between
 *	me reloading myself and our client pull info from
 *	myself, client begin to read.
*/
void SuperModel::beginRead()
{
#if ENABLE_RELOAD_MODEL
	for (Models::size_type i = 0; i < nModels_; ++i)
	{
		if (models_[i].curModel())
		{
			models_[i].curModel()->beginRead();
		}
	}
#endif//ENABLE_RELOAD_MODEL
}


/**
 *	This function is a concurrency lock between
 *	me reloading myself and our client pull info from
 *	myself, client end read.
*/
void SuperModel::endRead()
{
#if ENABLE_RELOAD_MODEL
	for (Models::size_type i = 0; i < nModels_; ++i)
	{
		if (models_[i].curModel())
		{
			models_[i].curModel()->endRead();
		}
	}
#endif//ENABLE_RELOAD_MODEL
}


/**
 *	SuperModel constructor
 */
SuperModel::SuperModel( const BW::vector< BW::string > & modelIDs ) :
	lod_( 0.f ),
	nModels_( 0 ),
	lodNextUp_( Model::LOD_MAX ),
	lodNextDown_( Model::LOD_MAX ),
	checkBB_( true ),
	nodeless_( true ),
	isOK_( true ),
	instanceNodeTreeDirty_( true ),
	numInstanceNodeTreeRegens_( 0 )
{
	BW_GUARD;
	models_.reserve( modelIDs.size() );
	for (BW::vector< BW::string >::size_type i = 0; i < modelIDs.size(); ++i)
	{
		ModelPtr pModel = Model::get( modelIDs[i] );
		if (pModel == NULL)
		{
			isOK_ = false;
			continue;	// Model::get will already have reported an error
		}

		models_.push_back( ModelPartChain( pModel ) );

		pModel->registerListener( this );

		nodeless_ &= pModel->nodeless();

		lodNextDown_ = models_.back().topModel_->extent();
	}
	nModels_ = models_.size();
}


/**
 *	SuperModel destructor
 */
SuperModel::~SuperModel()
{
	for (Models::iterator it = models_.begin(); it != models_.end(); ++it)
	{
		it->fini();
	}
}


#if ENABLE_TRANSFORM_VALIDATION
/**
 *	Check if this super model's animations have been updated this frame.
 *	@param frameTimestamp the timestamp for this frame.
 *	@return true if this super model needs animating this frame,
 *		false if it has already been marked as animated.
 */
bool SuperModel::isTransformFrameDirty( uint32 frameTimestamp ) const
{
	BW_GUARD;
	return this->isAnyDirtyTransforms( frameTimestamp );
}


/**
 *	Check if this super model's animations have been updated this frame.
 *
 *	Use this check when checking for if we need to update.
 *	ie. want to update lods.
 *
 *	@param frameTimestamp the timestamp for this frame.
 *	@return true if this super model needs animating this frame,
 *		false if it has already been marked as animated.
 */
bool SuperModel::isVisibleTransformFrameDirty( uint32 frameTimestamp ) const
{
	BW_GUARD;
	return this->isAnyVisibleDirtyTransforms( frameTimestamp );
}
#endif // ENABLE_TRANSFORM_VALIDATION


/**
 *	Check if this super model needs to be updated this frame.
 *	@return true if it needs an update, false if it does not.
 */
bool SuperModel::needsUpdate() const
{
	BW_GUARD;
	// SuperModel always needs an update to calculate its lods
#if ENABLE_TRANSFORM_VALIDATION
	return this->isTransformFrameDirty( Moo::rc().frameTimestamp() );
#else // ENABLE_TRANSFORM_VALIDATION
	return true;
#endif // ENABLE_TRANSFORM_VALIDATION
}


/**
 *	This method updates the LODs and transforms for this frame.
 *	@param world the world transform.
 *	@param pPreFashions any transforms to apply (eg animations etc)
 *		or NULL if there are none.
 *	@param pPostFashions any transforms to apply after animation (eg trackers)
 *		or NULL if there are none.
 *	@param atLod specify this to use a specific LOD, use -1 to calculate
 *		automatically.
 */
void SuperModel::updateAnimations( const Matrix& world,
	const TmpTransforms * pPreFashions,
	const TmpTransforms * pPostFashions,
	float atLod )
{
	BW_GUARD_PROFILER( SuperModel_update );
	static DogWatch modelUpdateTimer( "Super model update" );
	ScopedDogWatch sdw( modelUpdateTimer );

	// Check that the model is within the world
 	if (!this->isTransformSane( world ))
 	{
 		return;
 	}

	// Update LOD
	LodSettings& lodSettings = LodSettings::instance();
	this->updateLods( atLod, lodSettings, world );

	this->updateTransformFashions( world, pPreFashions, pPostFashions );

	return;
}


/**
 *	This method updates the lod levels for the model only.
 *  This is used for static models that have their transforms
 *	already cached.
 *	@param world the world transform for the SuperModel.
 *	@param atLod the lod to update to (-1 means the lod will be
 *  recalculated from the world transform).
 */
void SuperModel::updateLodsOnly( const Matrix& world, float atLod )
{
	BW_GUARD;
	this->updateLods( atLod, LodSettings::instance(), world );
#if ENABLE_TRANSFORM_VALIDATION
	for (Models::iterator it = models_.begin();
		it != models_.end();
		++it)
	{
		it->markTransform( Moo::rc().frameTimestamp() );
	}
#endif // ENABLE_TRANSFORM_VALIDATION
}


/**
 *	Draw this SuperModel at the current Moo::rc().world() transform.
 *	@param world the world transform.
 *	@param pMaterialFashions a list of materials to be applied to the models
 *		or NULL if there are none.
 */
void SuperModel::draw( Moo::DrawContext& drawContext, const Matrix& world,
	const MaterialFashionVector * pMaterialFashions )
{
	BW_GUARD_PROFILER( SuperModel_draw );
	static DogWatch modelDrawTimer( "Super model draw" );
	ScopedDogWatch sdw( modelDrawTimer );

	// Check that the model is within the world
	if (!this->isTransformSane( world ))
	{
		return;
	}

	// Check that we have visible lods
	if (!this->isLodVisible())
	{
		return;
	}

	// Check that updateTransforms has been called this frame
	TRANSFORM_ASSERT( !this->isAnyVisibleDirtyTransforms(
		Moo::rc().frameTimestamp() ) );

	// The blend cookie is used to reset nodes during animation updates
	// so should not need a reset during draw, except
	// that it has a dual purpose to be used by Matter::soak
	Model::incrementBlendCookie();

	// Draw
	this->applyMaterialsAndTransforms( pMaterialFashions );

	// draw context rendering does not support material fashions, so
	// make draw code to fallback to legacy codepath
	// TODO_DRAW_CONTEXT: we need to reduce amount of global state mutation
	// and wrap the rest of it into GlobalStateBlocks
	// code base in its current state has only 3 material fashions
	// where super model dye is the most complex one
	// if we can drop support for super model dye we can make the other one to
	// use GlobalStateBlock we remove this immediate mode switch.
	const bool useImmediateMode = pMaterialFashions && pMaterialFashions->size() > 0;
	if (useImmediateMode)
	{
		drawContext.pushImmediateMode();
	}
	
	this->drawModels( drawContext, world );

	if (useImmediateMode)
	{
		drawContext.popImmediateMode();
	}
	this->removeMaterials( pMaterialFashions );

	return;
}


/**
 *	This method determines what LODs to apply transforms to and draw.
 *	@param atLod specify this to use a specific LOD, use -1 to calculate
 *		automatically.
 *	@param lodSettings the LOD settings instance to use for calculating LODs.
 *	@param world the world transform.
 */
void SuperModel::updateLods( float atLod,
	LodSettings& lodSettings,
	const Matrix & world )
{
	BW_GUARD_PROFILER( SuperModel_updateLods );

	static DogWatch updateLodsTimer( "Super model update LODs" );
	ScopedDogWatch sdw( updateLodsTimer );

	// Calculate LOD
	if (atLod < Model::LOD_MIN)
	{
		lod_ = lodSettings.calculateLod( world );
	}
	else
	{
		lod_ = atLod;
	}

	// Set which model to use on the model chain
	if ( lod_ > lodNextDown_ || lod_ <= lodNextUp_)
	{

		// Reset all of the stored LOD data for this frame
		float modelLodNextUp = Model::LOD_HIDDEN;
		float modelLodNextDown = models_.back().topModel_->extent();

		lodNextUp_ = Model::LOD_HIDDEN;
		lodNextDown_ = Model::LOD_MAX;

		for (Models::iterator it1 = models_.begin(); it1 != models_.end(); ++it1)
		{
			// Reset
			ModelPartChain& modelChain = (*it1);

			modelLodNextUp = Model::LOD_HIDDEN;
			modelLodNextDown = modelChain.topModel_->extent();

			modelChain.lodChainIndex_ = 0;

			// Get correct LOD index
			// The list should be sorted
			for (BW::vector<LodLevelData>::const_iterator it2 =
				modelChain.lodChain_.begin();
				it2 < modelChain.lodChain_.end();
				++it2)
			{
				const LodLevelData& lodChain = (*it2);
				const Model* pModel = lodChain.pModel;
				MF_ASSERT( pModel != NULL );
				const float extent = pModel->extent();

				// 1 - This LOD is hidden, ignore it
				if (isEqual( extent, Model::LOD_HIDDEN ) || 
					(extent <= modelLodNextDown &&
					modelLodNextUp != Model::LOD_HIDDEN))
				{
					++(modelChain.lodChainIndex_);
				}

				// 2 - Go down to next lod in the chain
				else if (extent < lod_)
				{
					modelLodNextUp = modelLodNextDown;
					modelLodNextDown = extent;
					// NextDown can be moved when we know the next extent
					++(modelChain.lodChainIndex_);
				}

				// 3 - Have found the correct lod
				// NextUp == LOD_HIDDEN, NextDown == extent.
				// Stop searching.
				else if (isEqual( modelLodNextDown, extent ))
				{
 					break;
				}

				// 4 - Have found the correct lod
				// Move NextUp down
				// Move NextDown to be the extent
				// Stop searching.
				else
				{
					modelLodNextUp = modelLodNextDown;
					modelLodNextDown = extent;
					break;
				}
			} // Get correct LOD index

			// Went through condition #2 and was the last model in the
			// model chain so it never hit #4
			// Want NextUp == last model's extent, NextDown = infinite
			if (lod_ > modelLodNextDown)
			{
				modelLodNextUp = modelLodNextDown;
				modelLodNextDown = Model::LOD_MAX;
			}

			// Only set extents if this model is making the range more narrow
			lodNextUp_ = std::max( lodNextUp_, modelLodNextUp );
			lodNextDown_ = std::min( lodNextDown_, modelLodNextDown );
		}

		MF_ASSERT( (lod_ <= lodNextDown_) && (lod_ >= lodNextUp_) );

	} // Set which model to use on the model chain

	return;
}


#if ENABLE_TRANSFORM_VALIDATION
/**
 *	Check if any of their models need their transforms updated.
 *	
 *	Use this check when checking for if we need to update.
 *	ie. want to update lods.
 *	
 *	@param frameTimestamp the timestamp for this frame.
 *	@return true if there is a model without its transforms updated.
 */
bool SuperModel::isAnyDirtyTransforms( uint32 frameTimestamp ) const
{
	BW_GUARD;
	for (Models::const_iterator it = models_.begin(); it != models_.end(); ++it)
	{
		if (it->transformDirty( frameTimestamp ))
		{
			return true;
		}
	}
	return false;
}


/**
 *	Check if any visible (un-lodded) models need their transforms updated.
 *	
 *	Use this check when checking if we need to draw.
 *	ie. only care about visible objects.
 *	
 *	@param frameTimestamp the timestamp for this frame.
 *	@return true if there is a model visible that has not been updated.
 */
bool SuperModel::isAnyVisibleDirtyTransforms( uint32 frameTimestamp ) const
{
	BW_GUARD;
	for (Models::const_iterator it = models_.begin(); it != models_.end(); ++it)
	{
		if (it->isLodVisible() && it->transformDirty( frameTimestamp ))
		{
			return true;
		}
	}
	return false;
}
#endif // ENABLE_TRANSFORM_VALIDATION


bool SuperModel::isLodVisible() const
{
	BW_GUARD;
	for (Models::const_iterator it = models_.begin(); it != models_.end(); ++it)
	{
		if (it->isLodVisible())
		{
			return true;
		}
	}
	return false;
}


/**
 *	Update the models transforms for this frame.
 *	@param world the world transform.
 *	@param pPreFashions any transforms to apply (eg animations etc)
 *		or NULL if there are none.
 *	@param pPostFashions any transforms to apply after animation (eg trackers)
 *		or NULL if there are none.
 */
void SuperModel::updateTransformFashions( const Matrix& world,
	const TmpTransforms * pPreFashions,
	const TmpTransforms * pPostFashions )
{
	BW_GUARD_PROFILER( SuperModel_updateTransformFashions );

	if (!this->isLodVisible())
	{
		return;
	}

	// Check if this needs updating this frame
	TRANSFORM_ASSERT( this->isTransformFrameDirty(
		Moo::rc().frameTimestamp() ) );

	static DogWatch updateTransformsTimer( "Super model update transforms" );
	ScopedDogWatch sdw( updateTransformsTimer );

	// Step 3: Dress up in the latest fashion
	Model::incrementBlendCookie();

	// World can be modified by ModelAligner::dress()
	Matrix transformWorld( world );

	// Apply animation transforms
	if (pPreFashions != NULL)
	{
		for (TmpTransforms::const_iterator fashIt =
			pPreFashions->begin();
			fashIt != pPreFashions->end();
			++fashIt)
		{
			TransformFashion* pFashion = (*fashIt);
			MF_ASSERT( pFashion != NULL );
			MF_ASSERT( !pFashion->needsRedress() );
			pFashion->dress( *this, transformWorld );
		}
	}

	for (Models::iterator it = models_.begin();
		it != models_.end();
		++it)
	{
		ModelPartChain& modelChain = (*it);
		Model * pModel = modelChain.curModel();
		
		if (pModel != NULL)
		{
			TRANSFORM_ASSERT( modelChain.transformDirty(
				Moo::rc().frameTimestamp() ) );
			pModel->dress( transformWorld, modelChain.curTransformState() );
		}
	}


	// Apply tracker transforms
	if (pPostFashions != NULL)
	{
		bool needRedress = false;
		for (TmpTransforms::const_iterator fashIt =
			pPostFashions->begin();
			fashIt != pPostFashions->end();
			++fashIt)
		{
			TransformFashion* pFashion = (*fashIt);
			MF_ASSERT( pFashion != NULL );
			pFashion->dress( *this, transformWorld );
			needRedress = ( needRedress | pFashion->needsRedress() );
		}

		if (needRedress)
		{
			for (Models::iterator it = models_.begin();
				it != models_.end();
				++it)
			{
				ModelPartChain& modelChain = (*it);
				Model * pModel = modelChain.curModel();
				if (pModel != NULL)
				{
					TRANSFORM_ASSERT( modelChain.transformDirty(
						Moo::rc().frameTimestamp() ) );
					pModel->dress( transformWorld,
						modelChain.curTransformState() );
				}
			}
		}
	}

#if ENABLE_TRANSFORM_VALIDATION
	// Mark transforms with this frame's timestamp
	for (Models::iterator it = models_.begin();
		it != models_.end();
		++it)
	{
		ModelPartChain& modelChain = (*it);
		modelChain.markTransform( Moo::rc().frameTimestamp() );
	}
#endif // ENABLE_TRANSFORM_VALIDATION
}


/**
 *	This method sets up any material settings before drawing and applies
 *	the cached transforms generated in updateTransforms().
 *	Should be matched by removeMaterials().
 *	@param pMaterialFashions any materials to apply or NULL if there are none.
 */
void SuperModel::applyMaterialsAndTransforms(
	const MaterialFashionVector * pMaterialFashions )
{
	BW_GUARD;

	static DogWatch applyMaterialsTimer( "Super model apply materials" );
	ScopedDogWatch sdw( applyMaterialsTimer );

	if (pMaterialFashions != NULL)
	{
		for (MaterialFashionVector::const_iterator fashIt =
			pMaterialFashions->begin();
			fashIt != pMaterialFashions->end();
			++fashIt)
		{
			MaterialFashionPtr pFashion = (*fashIt);
			MF_ASSERT( pFashion.exists() );
			pFashion->dress( *this );
		}
	}

	for (Models::iterator it = models_.begin();
		it != models_.end();
		++it)
	{
		ModelPartChain& modelChain = (*it);
		Model * pModel = modelChain.curModel();
		if (pModel != NULL)
		{
			pModel->dressFromState( *(modelChain.curTransformState()) );
		}
	}
}


/**
 *	Draw all the models.
 */
void SuperModel::drawModels( Moo::DrawContext& drawContext, const Matrix& world )
{
	BW_GUARD;
	static DogWatch modelDrawTimer( "Super model draw models" );
	ScopedDogWatch sdw( modelDrawTimer );

	Moo::rc().push();
	Moo::rc().world( world );

	for (Models::iterator it = models_.begin();
		it != models_.end();
		++it)
	{
		ModelPartChain& modelChain = (*it);
		Model * pModel = modelChain.curModel();
		if (pModel != NULL)
		{
			// commit the dye setup per model.
			pModel->soakDyes();
			pModel->draw( drawContext, checkBB_ );
		}
	}

	Moo::rc().pop();
}


/**
 *	Remove any material state after drawing.
 *	@param pMaterialFashions any materials to apply or NULL if there are none.
 */
void SuperModel::removeMaterials(
	const MaterialFashionVector * pMaterialFashions )
{
	BW_GUARD;
	static DogWatch removeMaterialsTimer( "Super model remove materials" );
	ScopedDogWatch sdw( removeMaterialsTimer );

	if (pMaterialFashions == NULL)
	{
		return;
	}

	for (MaterialFashionVector::const_reverse_iterator fashRIt =
		pMaterialFashions->rbegin();
		fashRIt != pMaterialFashions->rend();
		++fashRIt)
	{
		MaterialFashionPtr pFashion = (*fashRIt);
		MF_ASSERT( pFashion.exists() );
		pFashion->undress( *this );
	}
}


/**
 *	This method checks if the model is within the world before drawing.
 *	@param world the world transform.
 */
bool SuperModel::isTransformSane( const Matrix& world ) const
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( -100000.f < world.applyToOrigin().x &&
		world.applyToOrigin().x < 100000.f &&
		-100000.f < world.applyToOrigin().z &&
		world.applyToOrigin().z < 100000.f )
	{
		ERROR_MSG(
			"SuperModel::isTransformSane: Tried to draw a SuperModel at bad "
			"translation: ( %f, %f, %f )\n",
			world.applyToOrigin().x,
			world.applyToOrigin().y,
			world.applyToOrigin().z );
		return false;
	}
	return true;
}


/**
 *	Dress this model up in its default fashions, but don't draw it.
 */
void SuperModel::dressInDefault()
{
	BW_GUARD;
	SuperModelAnimationPtr smap = this->getAnimation( "" );
	smap->blendRatio = 1.f;

	Model::incrementBlendCookie();

	Matrix transformWorld( Matrix::identity );
	smap->dress( *this, transformWorld );

	for (Models::iterator it = models_.begin();
		it != models_.end();
		++it)
	{
		ModelPartChain& modelChain = (*it);
		modelChain.topModel_->dress( Matrix::identity,
			modelChain.lodChain_[0].transformState );
	}
}


/**
 *	This method caches the default world transforms in the transform
 *	cache of the models so that the model can be rendered as static
 *	@param world the world transform of the root of the model
 */
void SuperModel::cacheDefaultTransforms( const Matrix& world )
{
	Model::incrementBlendCookie();
	for (Models::iterator it = models_.begin();
		it != models_.end();
		++it)
	{
		ModelPartChain& modelChain = (*it);
		for (BW::vector<LodLevelData>::iterator lit = 
				modelChain.lodChain_.begin();
			lit != modelChain.lodChain_.end();
			++lit)
		{
			lit->pModel->dress( world,
				lit->transformState );
		}
	}
}


/**
 *	This method gets an animation from the SuperModel. The animation
 *	is only good for this supermodel - don't try to use it with other
 *	supermodels.
 */
SuperModelAnimationPtr SuperModel::getAnimation( const BW::string & name )
{
	BW_GUARD;
	if (nModels_ == 0) return NULL;

	char * pBytes = new char[sizeof(SuperModelAnimation)+(nModels_-1)*sizeof(int)];
	// since this is a simple char array, I believe that we don't
	//  have to delete [] it.

	SuperModelAnimation * pAnim = new ( pBytes ) SuperModelAnimation( *this, name );

	return pAnim;
}



/**
 *	This method gets an action from the SuperModel. The action
 *	is only good for this supermodel - don't try to use it with other
 *	supermodels.
 */
SuperModelActionPtr SuperModel::getAction( const BW::string & name )
{
	BW_GUARD;
	if (nModels_ == 0) return NULL;

	char * pBytes = new char[sizeof(SuperModelAction)+(nModels_-1)*sizeof(int)];

	SuperModelActionPtr pAction = new ( pBytes ) SuperModelAction( *this, name );

	return pAction->pSource_ != NULL ? pAction : SuperModelActionPtr(NULL);
}


/**
 *	This method gets all the matchable actions in any of the Models in
 *	this SuperModel.
 */
void SuperModel::getMatchableActions(
	BW::vector< SuperModelActionPtr > & actions )
{
	BW_GUARD;
	actions.clear();

	// make a set of strings
	BW::vector<const BW::string*>	matchableNames;

	// add the names of each model's matchable actions to it
	for (int i = (int)nModels_ - 1; i >= 0; i--)
	{
		Model * tM = models_[i].topModel_.getObject();
		const ModelAction * pAct;

		for (ModelActionsIterator it = tM->lookupLocalActionsBegin();
			it != tM->lookupLocalActionsEnd() && (pAct = &*it) != NULL;
			it++)
		{
			if (pAct->isMatchable_)
			{
				matchableNames.push_back( &pAct->name_ );
			}
		}
	}

	// iterate over our own actions first then our parent's
	BW::set<BW::string> seenNames;
	for (int i = 0; i <= int(matchableNames.size())-1; i++)
	{
		// only add this one if we haven't seen it before
		if (seenNames.insert( *matchableNames[i] ).second == true)
		{
			actions.push_back( this->getAction( *matchableNames[i] ) );
		}
	}
}



/**
 *	This method gets a dye from the SuperModel. The dye is only good
 *	for this supermodel - don't try to use it with other supermodels.
 */
SuperModelDyePtr SuperModel::getDye( const BW::string & matter,
	const BW::string & tint)
{
	BW_GUARD;
	if (nModels_ == 0)
		return NULL;

	SuperModelDyePtr pDye = new SuperModelDye( *this, matter, tint );

	return pDye->effective( *this ) ? pDye : SuperModelDyePtr(NULL);
}


/**
 *	This returns the number of triangles in the supermodel.
 */
int SuperModel::numTris() const
{
	BW_GUARD;
	int tris = 0;

	for (Models::size_type i = 0; i < nModels_; ++i)
	{
		Moo::VisualPtr pVisual = this->models_[ i ].topModel_->getVisual();

		if (pVisual)
		{
			tris += pVisual->nTriangles();
		}
	}

	return tris;
}


/**
 *	This returns the number of primitive groups in the supermodel.
 */
int SuperModel::numPrims() const
{
	BW_GUARD;
	int prims = 0;

	for (Models::size_type i = 0; i < nModels_; ++i)
	{
		Moo::VisualPtr pVisual = this->models_[ i ].topModel_->getVisual();

		if (pVisual)
		{
			for (Moo::Visual::RenderSetVector::iterator rsit =
				pVisual->renderSets().begin();
				rsit != pVisual->renderSets().end(); ++rsit)
			{
				for (Moo::Visual::GeometryVector::iterator git =
					(*rsit).geometry_.begin();
					git != (*rsit).geometry_.end(); ++git)
				{
					prims += static_cast<int>((*git).primitiveGroups_.size());
				}
			}
		}
	}

	return prims;
}


/**
 *	Get the node by name.
 */
Moo::NodePtr SuperModel::findNode( const BW::string & nodeName,
	MooNodeChain * pParentChain, bool addToLods )
{
	BW_GUARD;
	// see if any model has this node (and get its node pointer)
	Moo::NodePtr pFound =
		Moo::NodeCatalogue::instance().find( nodeName.c_str() );
	if (!pFound)
	{
		return NULL;
	}

	// see if one of our models has this node
	Moo::Node * pMaybe = pFound.getObject();
	for (Models::size_type i = 0; i < nModels_; ++i)
	{
		if (models_[i].topModel_->hasNode( pMaybe, pParentChain ))
		{
			if (addToLods)
			{
				models_[i].topModel_->addNodeToLods( nodeName );
			}

			return pFound;
		}
	}

	// see if it's the root node (bit of a hack for nodeless models)
	if (pFound->isSceneRoot())
	{
		if (pParentChain != NULL)
		{
			pParentChain->clear();
		}
		return pFound;
	}

	// ok, no such node then
	return NULL;
}


/**
 *	Calculate the bounding box of this supermodel
 */
void SuperModel::localBoundingBox( BoundingBox & bbRet ) const
{
	BW_GUARD;
	if (nModels_ != 0)
	{
		bbRet = models_[0].topModel_->boundingBox();

		for (Models::size_type i = 1; i < nModels_; ++i)
		{
			bbRet.addBounds( models_[i].topModel_->boundingBox() );
		}
	}
	else
	{
		bbRet = BoundingBox( Vector3(0,0,0), Vector3(0,0,0) );
	}
}

/**
 *	Calculate the bounding box of this supermodel
 */
void SuperModel::localVisibilityBox( BoundingBox & vbRet ) const
{
	BW_GUARD;
	if (nModels_ != 0)
	{
		vbRet = models_[0].topModel_->visibilityBox();

		for (Models::size_type i = 1; i < nModels_; ++i)
		{
			vbRet.addBounds( models_[i].topModel_->visibilityBox() );
		}
	}
	else
	{
		vbRet = BoundingBox( Vector3(0,0,0), Vector3(0,0,0) );
	}
}


SuperModel::ModelPartChain::ModelPartChain( const ModelPtr& pModel ) :
	topModel_( pModel ),
	lodChainIndex_( 0 )
#if ENABLE_TRANSFORM_VALIDATION
	, frameTimestamp_( 0 )
#endif // ENABLE_TRANSFORM_VALIDATION
{
	BW_GUARD;
	this->pullInfoFromModel();
}


void SuperModel::ModelPartChain::pullInfoFromModel()
{
	BW_GUARD;
	lodChain_.clear();

	// Mutually exclusive of topModel_ and it's parents reloading, start
	topModel_->beginRead();

	Model* curModel = topModel_.get();
	MF_ASSERT( curModel != NULL );

	while (curModel != NULL)
	{
		LodLevelData modelLod;
		modelLod.pModel = curModel;
		modelLod.transformState = curModel->newTransformStateCache();

		lodChain_.push_back( modelLod );

		curModel = curModel->parent();
	}

	// Mutually exclusive of topModel_ and it's parents reloading, end
	topModel_->endRead();
}


/**
 *	Check if this model part is visible.
 *	@return true if it is visible, false if it has lodded out.
 */
bool SuperModel::ModelPartChain::isLodVisible() const
{
	BW_GUARD;
	return (lodChainIndex_ < lodChain_.size());
}


#if ENABLE_TRANSFORM_VALIDATION
/**
 *	Check if this model part's animations have been updated this frame.
 *	@param frameTimestamp the timestamp for this frame.
 *	@return true if this model part needs animating this frame,
 *		false if it has already been marked as animated.
 */
bool SuperModel::ModelPartChain::transformDirty( uint32 frameTimestamp ) const
{
	BW_GUARD;
	return frameTimestamp != frameTimestamp_;
}


/**
 *	Mark this model part as animated for this frame.
 *	@param frameTimestamp the timestamp of the current frame to save.
 */
void SuperModel::ModelPartChain::markTransform( uint32 frameTimestamp )
{
	BW_GUARD;
	frameTimestamp_ = frameTimestamp;
}
#endif // ENABLE_TRANSFORM_VALIDATION


void SuperModel::ModelPartChain::fini()
{
	BW_GUARD;
	for (BW::vector<LodLevelData>::iterator it = lodChain_.begin(); 
		it != lodChain_.end(); ++it)
	{
		bw_safe_delete( it->transformState );
	}
}


#ifndef CODE_INLINE
	#include "super_model.ipp"
#endif

BW_END_NAMESPACE

// super_model.cpp



