#include "pch.hpp"

#include "nodeless_model.hpp"

#include "moo/animation_manager.hpp"
#include "moo/node_catalogue.hpp"
#include "moo/visual_manager.hpp"

#include "super_model.hpp"


DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

Moo::NodePtr NodelessModel::s_sceneRootNode_ = NULL;


/**
*	This function load NodefullModel data the from data section.
*/
void NodelessModel::load()
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( NodelessModel_load );
//	DEBUG_MSG("loading model %s\n",resourceID_.c_str());
//	int64 tm = timestamp();

	Model::load();

	SmartPointer<Moo::Visual> oldBulk = bulk_;

	// load standard switched model stuff
	DataSectionPtr pFile = BWResource::openSection( resourceID_ );
	MF_ASSERT( pFile );
	if (!this->wireSwitch( pFile, &loadVisual, "nodelessVisual", "visual" ))
		return;

	// Mutually exclusive of bulk_ reloading, start
	bulk_->beginRead();

	// load dyes
	this->readDyes( pFile, true );

	if (oldBulk != bulk_)
	{
		if (oldBulk)
		{
			//dismiss the reload listen relation between me and the old bulk_
			oldBulk->deregisterListener( this, true );
		}
		bulk_->registerListener( this );
	}

	// Mutually exclusive of parent_ reloading, end
	if (parent_)
	{
		// parent_->beginRead() is called inside Model::load()
		parent_->endRead();
	}
	// Mutually exclusive of bulk_ reloading, end
	bulk_->endRead();
//	DEBUG_MSG( "Done (%fms)\n", stampsToSeconds(timestamp() - tm) * 1000.0f );
}



/**
 *	Constructor
 */
NodelessModel::NodelessModel( const BW::StringRef & resourceID, DataSectionPtr pFile ) :
	SwitchedModel<Moo::VisualPtr>( resourceID, pFile )
{
	BW_GUARD;

	this->load();
}


#if ENABLE_RELOAD_MODEL
/**
 *	This function check if the load file is valid
 *	We are currently doing a fully load and check,
 *	but we might want to optimize it in the future 
 *	if it's needed.
 *	@param resourceID	the model file path
 */
bool NodelessModel::validateFile( const BW::string& resourceID )
{
	BW_GUARD;
	DataSectionPtr pFile = BWResource::openSection( resourceID );
	if (!pFile)
	{
		ASSET_MSG( "NodelessModel::reload: "
			"%s is not a valid model file\n",
			resourceID.c_str() );
		return false;
	}

	NodelessModel tempModel( resourceID, pFile );
	PostLoadCallBack::run();


	tempModel.isInModelMap( false );
	if (!tempModel.valid())
	{
		return false;
	}
	return true;
}


/**
 *	This function reload the current model from the disk.
 *	This function isn't thread safe. It will only be called
 *	For reload an externally-modified model
 *	@param	doValidateCheck	do we check if the file is valid first?
 *	@return if succeed
*/
bool NodelessModel::reload( bool doValidateCheck )
{
	BW_GUARD;
	if (doValidateCheck && 
		!NodelessModel::validateFile( resourceID_ ))
	{
		return false;
	}

	this->onPreReload();
	reloadRWLock_.beginWrite();

	this->load();
	PostLoadCallBack::run();

	reloadRWLock_.endWrite();
	this->onReloaded();

	return true;
}
#endif //ENABLE_RELOAD_MODEL


/**
 * if our visual has been reloaded, 
 * update the related data.
 */
void NodelessModel::onReloaderReloaded( Reloader* pReloader )
{
	BW_GUARD;
	if ( bulk_ && pReloader == bulk_.get() )
	{
		this->pullInfoFromVisual( bulk_ );
	}

	Model::onReloaderReloaded( pReloader );

}


/**
 *	This method dresses this nodeless model.
 */
void NodelessModel::dress( const Matrix& world,
	TransformState* pTransformState )
{
	BW_GUARD;
	this->SwitchedModel<Moo::VisualPtr>::dressSwitchedModel();

	if (s_sceneRootNode_ != NULL)
	{
		s_sceneRootNode_->blendClobber(
			Model::blendCookie(), Matrix::identity );
		s_sceneRootNode_->visitSelf( world );

		if (pTransformState)
		{
			MF_ASSERT( pTransformState->size() > 0 );
			pTransformState->front() = s_sceneRootNode_->worldTransform();
		}
	}
}


void NodelessModel::dressFromState( const TransformState& transformState )
{
	BW_GUARD;
	this->SwitchedModel<Moo::VisualPtr>::dressSwitchedModel();

	
	if (s_sceneRootNode_ != NULL)
	{
		MF_ASSERT( transformState.size() > 0 );
		s_sceneRootNode_->worldTransform( transformState.front() );
		s_sceneRootNode_->blend( Model::blendCookie(), 1.f );
	}
}


/**
 *	This method draws this nodeless model.
 */
void NodelessModel::draw( Moo::DrawContext& drawContext, bool checkBB )
{
	BW_GUARD_PROFILER( NodelessModel_draw );
	PROFILER_SCOPED_DYNAMIC_STRING( resourceID().c_str() );
	MF_ASSERT( frameDraw_ );

	frameDraw_->draw( drawContext, !checkBB, false );
}


/**
 *	This method returns a BSPTree of this model when decomposed
 */
const BSPTree * NodelessModel::decompose() const
{
	BW_GUARD;
	return bulk_ ? bulk_->pBSPTree() : NULL;
}


/**
 *	This method returns the bounding box of this model
 */
const BoundingBox & NodelessModel::boundingBox() const
{
	BW_GUARD;
	if (!bulk_)
		return BoundingBox::s_insideOut_;;
	return bulk_->boundingBox();
}

/**
 *	This method returns the bounding box of this model
 */
const BoundingBox & NodelessModel::visibilityBox() const
{
	BW_GUARD;
	return boundingBox();
}


int NodelessModel::gatherMaterials(	const BW::string & materialIdentifier,
									BW::vector< Moo::Visual::PrimitiveGroup * > & primGroups,
									Moo::ConstComplexEffectMaterialPtr * ppOriginal )
{
	BW_GUARD;
	return bulk_->gatherMaterials( materialIdentifier, primGroups, ppOriginal );
}


/*static*/ Moo::VisualPtr NodelessModel::loadVisual( Model & m, const BW::string & resourceID )
{
	BW_GUARD;
	BW::string visualName = resourceID + ".static.visual";
	Moo::VisualPtr vis = Moo::VisualManager::instance()->get( visualName );
	if (!vis)
	{
		visualName = resourceID + ".visual";
		vis = Moo::VisualManager::instance()->get( visualName );
	}
	pullInfoFromVisual( vis );
	return vis;
}


/**
 *  This function pull data out from our visual
 */
void NodelessModel::pullInfoFromVisual( Moo::VisualPtr pVisual )
{
	if (pVisual)
	{
		Moo::Visual::ReadGuard guard( pVisual.get() );

		// complain if this visual has nodes
		if (pVisual->rootNode()->nChildren() != 0)
		{
			WARNING_MSG( "NodelessModel::loadVisual: "
				"Visual %s has multiple nodes! (attachments broken)\n",
				pVisual->resourceID().c_str() );
		}
		else
		{
			// but all have a root node
			Moo::Node * pCatNode =
				Moo::NodeCatalogue::instance().findOrAdd( pVisual->rootNode() );
			if (!s_sceneRootNode_)
			{
				if (pVisual->rootNode()->isSceneRoot())
				{
					s_sceneRootNode_ = pCatNode;
				}
			}

			if (!s_sceneRootNode_ || s_sceneRootNode_ != pCatNode)
			{
				WARNING_MSG( "NodelessModel::loadVisual: "
					"Visual %s has an invalid root node: %s\n",
					pVisual->resourceID().c_str(),
					pVisual->rootNode()->identifier().c_str());
			}
		}
	}
}

int NodelessModel::initMatter( Matter & m )
{
	BW_GUARD;
	return initMatter_NewVisual( m, *bulk_ );
}


bool NodelessModel::initTint( Tint & t, DataSectionPtr matSect )
{
	BW_GUARD;
	return initTint_NewVisual( t, matSect );
}

Model::TransformState* NodelessModel::newTransformStateCache() const
{
	return new TransformState( 1, Matrix::identity );
}


BW_END_NAMESPACE

// nodeless_model.cpp
