#include "pch.hpp"

#include "nodefull_model.hpp"

#include "moo/animation.hpp"
#include "moo/animation_manager.hpp"
#include "moo/streamed_animation_channel.hpp"
#include "moo/streamed_data_cache.hpp"
#include "moo/visual_manager.hpp"

#include "nodefull_model_animation.hpp"
#include "nodefull_model_default_animation.hpp"

#include "super_model.hpp"


DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

void NodefullModel::release()
{
	BW_GUARD;
	if (pAnimCache_)
	{
		delete pAnimCache_;
		pAnimCache_ = NULL;
	}
	nodeTree_.clear();
}


/**
*	This function load NodefullModel data the from data section.
*/
void NodefullModel::load()
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( NodefullModel_load );
	Model::load();

	this->release();

	static BW::vector < BW::string > s_animWarned;

	// load our bulk
	DataSectionPtr pFile = BWResource::openSection( resourceID_ );
	MF_ASSERT( pFile );
	BW::string bulkName = pFile->readString( "nodefullVisual" ) + ".visual";

	SmartPointer<Moo::Visual> oldBulk = bulk_;

	bulk_ = Moo::VisualManager::instance()->get( bulkName );

	if (!bulk_)
	{
		ASSET_MSG( "NodefullModel::NodefullModel: "
			"Could not load visual resource '%s' as main bulk of model %s\n",
			bulkName.c_str(), resourceID_.c_str() );
		return;		// maybe we should load a placeholder
	}

	// Mutually exclusive of bulk_ reloading, start
	bulk_->beginRead();

	// Get the visibility box for the model if it exists,
	// if it doesn't then use the visual's bounding box instead.
	DataSectionPtr bb = pFile->openSection("visibilityBox");
	if (bb)
	{
		Vector3 minBB = bb->readVector3("min", Vector3(0.f,0.f,0.f));
		Vector3 maxBB = bb->readVector3("max", Vector3(0.f,0.f,0.f));
		visibilityBox_ = BoundingBox( minBB, maxBB );
	}

	this->pullInfoFromVisual();

	// load dyes
	this->readDyes( pFile, true );



	// TODO: Figure out what the itinerant node index really is instead
	//  of just assuming that it's 1 (maybe get from an anim?). It is also
	//  possible that some visuals may not have an itinerant node at all.

	BW::vector<LoadedAnimationInfo> animationInfos;
	Model::loadAnimations( resourceID_, &pAnimCache_, &animationInfos );
	
	for (size_t i = 0; i < animationInfos.size(); ++i)
	{
		LoadedAnimationInfo& info = animationInfos[i];

		NodefullModelAnimation * pNewAnim = new NodefullModelAnimation( *this, info.pAnim_, info.pDS_ );
		int existingIndex = this->getAnimation( info.name_ );
		if (existingIndex == -1)
		{
			animationsIndex_.insert(
				std::make_pair( info.name_, static_cast<int>(animations_.size()) ) );
			animations_.push_back( pNewAnim );
		}
		else
		{
			animations_[ existingIndex ] = pNewAnim;
		}
	}

	postLoad( pFile );


	if (oldBulk != bulk_)
	{
		if (oldBulk)
		{
			//dismiss the reload listen relation between me and the old bulk_
			oldBulk->deregisterListener( this, true );
		}
		if (bulk_)
		{
			bulk_->registerListener( this );
		}
	}

	// Mutually exclusive of parent_ reloading, end
	if (parent_)
	{
		// parent_->beginRead() is called inside Model::load()
		parent_->endRead();
	}
	// Mutually exclusive of bulk_ reloading, end
	bulk_->endRead();

}

/**
 *	Constructor
 */
NodefullModel::NodefullModel( const BW::StringRef & resourceID, DataSectionPtr pFile ) :
	Model( resourceID, pFile ),
	pAnimCache_( NULL )
{
	BW_GUARD;
	this->load();
}

/**
 *	Destructor
 */
NodefullModel::~NodefullModel()
{
	BW_GUARD;
	bw_safe_delete(pAnimCache_);
}

/**
 *  This function pull data out from our bulk_
 */
void NodefullModel::pullInfoFromVisual()
{
	BW_GUARD;
	if (!bulk_)
		return;

	Moo::Visual::ReadGuard guard( bulk_.get() );

	// Get the visibilty box for the model if it exists,
	// if it doesn't then use the visual's bounding box instead.
	DataSectionPtr pFile = BWResource::openSection( resourceID_ );
	DataSectionPtr bb = pFile->openSection("visibilityBox");
	if (!bb)
	{
		visibilityBox_ = bulk_->boundingBox();
	}

	this->rebuildNodeHierarchy();
}


/**
 *	This method rebuilds the locally cached node hierarchy and
 *	recalculates the initial pose animation
 */
void NodefullModel::rebuildNodeHierarchy()
{
	//Note: the node in nodeTree_ doesn't have to be 
	//same instance as the one (of same name) in visual,  
	//refer to the Note in NodeCatalogue::findOrAdd.
	BW::vector<Matrix>	initialPose;
	nodeTree_.clear();

	// add any nodes found in our visual to the node table,
	// and build the node index tree at the same time
	BW::vector<Moo::NodePtr>	stack;
	stack.push_back( bulk_->rootNode() );
	while (!stack.empty())
	{
		Moo::NodePtr pNode = stack.back();
		stack.pop_back();

		// save this node's initial transform
		initialPose.push_back( pNode->transform() );

		// add this node to our table, if it's not already there
		Moo::Node * pStorageNode =
			Moo::NodeCatalogue::instance().findOrAdd( pNode );

		// do the same for the children
		for (int i = pNode->nChildren() - 1; i >= 0 ; i--)
		{
			stack.push_back( pNode->child( i ) );
		}

		// and fill in the tree
		nodeTree_.push_back( NodeTreeData( pStorageNode, pNode->nChildren() ) );
	}	

	// add the default 'initial pose' animation
	pDefaultAnimation_ = new NodefullModelDefaultAnimation( nodeTree_, 1, initialPose );
	if (!pDefaultAnimation_->valid())
	{
		ASSET_MSG(	"Unable to create the default pose for model '%s'\n",
			this->resourceID_.c_str() );
		pDefaultAnimation_ = NULL;
		return;
	}
}

/**
 * if our visual has been reloaded, 
 * update the related data.
 */
void NodefullModel::onReloaderReloaded( Reloader* pReloader )
{
	BW_GUARD;
	if ( bulk_ && pReloader == bulk_.get() )
	{
		this->pullInfoFromVisual();
	}

	Model::onReloaderReloaded( pReloader );
}


#if ENABLE_RELOAD_MODEL
/**
 *	This function check if the load file is valid
 *	We are currently doing a fully load and check,
 *	but we might want to optimize it in the future 
 *	if it's needed.
 *	@param resourceID	the model file path
 */
bool NodefullModel::validateFile( const BW::string& resourceID )
{
	BW_GUARD;
	DataSectionPtr pFile = BWResource::openSection( resourceID );
	if (!pFile)
	{
		ASSET_MSG( "NodefullModel::reload: "
			"%s is not a valid model file\n",
			resourceID.c_str() );
		return false;
	}
	NodefullModel tempModel( resourceID, pFile );
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
bool NodefullModel::reload( bool doValidateCheck )
{
	BW_GUARD;

	if (doValidateCheck && 
		!NodefullModel::validateFile( resourceID_ ))
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
 *	This function returns true if the NodefullModel is in a valid state.
 *	This function will not raise a critical error or assert if called but may
 *	commit ERROR_MSGs to the log.
 *
 *	@return		Returns true if the model is in a valid state, otherwise false.
 */
bool NodefullModel::valid() const
{
	BW_GUARD;
	if (!this->Model::valid())
		return false;

	if (!(bulk_ && bulk_->isOK()))
		return false;

	return true;
}


/**
 *	This method traverses this nodefull model, and soaks its dyes
 */
void NodefullModel::dress( const Matrix& world, TransformState* pTransformState )
{
	BW_GUARD;
	const BW::vector<Matrix> & initialPose =
		static_cast<NodefullModelDefaultAnimation&>
		(*pDefaultAnimation_).cTransforms();

	world_ = world;

	static struct TravInfo
	{
		const Matrix	* trans;
		int					pop;
	} stack [NODE_STACK_SIZE ];
	int			sat = 0;

	if (pTransformState && pTransformState->size() != nodeTree_.size())
	{
		pTransformState->resize( nodeTree_.size(), Matrix::identity );
	}

	
	// start with the root node
	stack[sat].trans = &world_;
	stack[sat].pop = static_cast<int>(nodeTree_.size());
	for (NodeTree::size_type i = 0; i < nodeTree_.size(); i++)
	{
		// pop off any nodes we're through with
		while (stack[sat].pop-- <= 0) --sat;

		NodeTreeData & ntd = nodeTree_[i];

		// make sure it's valid for this supermodel
		if (isZero( ntd.pNode->blend( Model::blendCookie() ) ))
			ntd.pNode->blendClobber( Model::blendCookie(), initialPose[i] );

		// calculate this one
		ntd.pNode->visitSelf( *stack[sat].trans );

		if (pTransformState)
		{
			MF_ASSERT( pTransformState->size() > i );

			(*pTransformState)[i] = ntd.pNode->worldTransform();
		}

		// and visit its children if it has any
		if (ntd.nChildren > 0)
		{
			++sat;

			if (sat >= NODE_STACK_SIZE)
			{
				--sat;
				ERROR_MSG( "Node Hierarchy too deep, unable to dress nodes\n" );
				/* Set this equal to the end to prevent infinite loops 
				*/
				stack[sat].trans = &ntd.pNode->worldTransform();
				stack[sat].pop = ntd.nChildren;
				return;
			}

			stack[sat].trans = &ntd.pNode->worldTransform();
			stack[sat].pop = ntd.nChildren;
		}
	}
}


void NodefullModel::dressFromState( const TransformState& transformState )
{
		BW_GUARD;
	for (NodeTree::size_type i = 0; i < nodeTree_.size(); ++i)
	{
		NodeTreeData & ntd = nodeTree_[i];

		ntd.pNode->worldTransform( transformState[i] );
		ntd.pNode->blend( Model::blendCookie(), 1.f );
	}

	// TODO: Factor out the world_ member, it should
	// not be needed
	world_ = transformState[0];
}


/**
 *	This method draws this nodefull model.
 */
void NodefullModel::draw( Moo::DrawContext& drawContext, bool checkBB )
{
	BW_GUARD_PROFILER( NodefullModel_draw );
	PROFILER_SCOPED_DYNAMIC_STRING( resourceID().c_str() );

	Moo::rc().push();
	Moo::rc().world( world_ );
	bulk_->draw( drawContext, !checkBB, false );
	Moo::rc().pop();
}


/**
 *	This method returns a BSPTree of this model when decomposed
 */
const BSPTree * NodefullModel::decompose() const
{
	BW_GUARD;
	return bulk_ ? bulk_->pBSPTree() : NULL;
}


/**
 *	This method returns the bounding box of this model
 */
const BoundingBox & NodefullModel::boundingBox() const
{
	BW_GUARD;
	if (!bulk_)
		return BoundingBox::s_insideOut_;
	return bulk_->boundingBox();
}

/**
 *	This method returns the bounding box of this model
 */
const BoundingBox & NodefullModel::visibilityBox() const
{
	BW_GUARD;
	return visibilityBox_;
}

/**
 *  Returns note tree iterator pointing to first node in tree.
 */
NodeTreeIterator NodefullModel::nodeTreeBegin() const
{
	BW_GUARD;
	const size_t size = nodeTree_.size();
	return size > 0
		? NodeTreeIterator( &nodeTree_[0], (&nodeTree_[size-1])+1, &world_ )
		: NodeTreeIterator( 0, 0, 0 );
}

/**
 *  Returns note tree iterator pointing to one past last node in tree.
 */
NodeTreeIterator NodefullModel::nodeTreeEnd() const
{
	BW_GUARD;
	const size_t size = nodeTree_.size();
	return size > 0
		? NodeTreeIterator( &nodeTree_[size-1]+1, (&nodeTree_[size-1])+1, &world_ )
		: NodeTreeIterator( 0, 0, 0 );
}

/**
 *	This method returns whether or not the model has this node
 */
bool NodefullModel::hasNode( Moo::Node * pNode,
	MooNodeChain * pParentChain ) const
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pNode != NULL )
	{
		return false;
	}

	if (pParentChain == NULL || nodeTree_.empty())
	{
		for (NodeTree::const_iterator it = nodeTree_.begin();
			it != nodeTree_.end();
			it++)
		{
			if (it->pNode == pNode) return true;
		}
	}
	else
	{
		static VectorNoDestructor<NodeTreeData> stack;
		stack.clear();

		stack.push_back( NodeTreeData( NULL, 1 ) );	// dummy parent for root
		for (uint i = 0; i < nodeTree_.size(); i++)
		{
			// pop off any nodes we're through with
			while (stack.back().nChildren-- <= 0) stack.pop_back();

			// see if this is the one
			const NodeTreeData & ntd = nodeTree_[i];
			if (ntd.pNode == pNode)
			{
				pParentChain->clear();
				for (uint j = 0; j < stack.size(); j++)
					pParentChain->push_back( stack[i].pNode );
				return true;
			}

			// and visit its children if it has any
			if (ntd.nChildren > 0) stack.push_back( ntd );
		}
	}

	return false;
}


/**
 *	This method adds a copy of the node pNode to the models node
 *	hierarchy. It requires pNode to be part of a proper hierarchy 
 *	and for it and it's parents transforms  to be valid and sensible
 *	as it uses the parents to determine where to insert the new node
 *	in this Model's  hierarchy
 */
bool NodefullModel::addNode( Moo::Node* pNode )
{
	BW_GUARD;

	MF_ASSERT_DEBUG( bulk_.exists() );

	Moo::NodePtr pVisualRoot = bulk_->rootNode();
	if (pVisualRoot->find( pNode->identifier() ))
	{
		return true;
	}

	Matrix transform = pNode->transform();

	Moo::NodePtr pSrcParent = pNode->parent();

	while (pSrcParent.exists() &&
		!pVisualRoot->find( pSrcParent->identifier() ).exists())
	{
		transform.postMultiply( pSrcParent->transform() );
		pSrcParent = pSrcParent->parent();
	}

	Moo::NodePtr pVisualParent = pVisualRoot;

	if (pSrcParent.exists())
	{
		pVisualParent = pVisualRoot->find( pSrcParent->identifier() );
	}

	Moo::NodePtr pVisualNode = new Moo::Node;
	pVisualNode->identifier( pNode->identifier() );
	Moo::NodeCatalogue::instance().findOrAdd( pVisualNode );
	pVisualNode->transform( transform );

	pVisualParent->addChild( pVisualNode );

	// Ensure our initial transforms are correct
	pDefaultAnimation_->play( NULL );

	this->rebuildNodeHierarchy();

	return true;
}


/**
 *	This method adds a named node to the lods of this model.
 *	It only adds the node if it can be found in the current
 *	model.
 */
void NodefullModel::addNodeToLods( const BW::string& nodeName )
{
	MF_ASSERT_DEBUG( bulk_.exists() );

	if (bulk_.exists() && parent_.exists())
	{
		Moo::NodePtr pNode = bulk_->rootNode()->find( nodeName );
		
		if (pNode.exists())
		{
			pDefaultAnimation_->play( NULL );
			if (parent_->addNode( pNode.get() ) )
			{
				parent_->addNodeToLods( nodeName );
			}
		}
		else
		{
			INFO_MSG( "NodefullModel::addNodeToLods - couldn't find Node %s\n",
				nodeName.c_str() );
		}
	}
}


int NodefullModel::initMatter( Matter & m )
{
	BW_GUARD;
	return initMatter_NewVisual( m, *bulk_ );
}


bool NodefullModel::initTint( Tint & t, DataSectionPtr matSect )
{
	BW_GUARD;
	return initTint_NewVisual( t, matSect );
}

Model::TransformState* NodefullModel::newTransformStateCache() const
{
	return new TransformState( this->nodeTree_.size(), Matrix::identity );
}

BW_END_NAMESPACE

// nodefull_model.cpp
