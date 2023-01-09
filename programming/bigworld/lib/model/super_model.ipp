// super_model.ipp

#ifdef CODE_INLINE
	#define INLINE inline
#else
	#define INLINE
#endif

INLINE bool SuperModel::nodeless() const
{
	return nodeless_;
}

INLINE int SuperModel::nModels() const
{
	return static_cast<int>( nModels_ );
}

INLINE const Model * SuperModel::curModel( int i ) const
{
	return models_[i].curModel();
}

INLINE Model * SuperModel::curModel( int i )
{
	return models_[i].curModel();
}

INLINE const ModelPtr SuperModel::topModel( int i ) const
{
	return models_[i].topModel_;
}

INLINE ModelPtr SuperModel::topModel( int i )
{
	return models_[i].topModel_;
}

INLINE float SuperModel::lastLod() const
{
	return lod_;
}

INLINE void SuperModel::checkBB( bool checkit )
{
	checkBB_ = checkit;
}

INLINE bool SuperModel::isOK() const
{
	return isOK_;
}

INLINE const Model* SuperModel::ModelPartChain::curModel() const
{
	return lodChainIndex_ < lodChain_.size() ?
		lodChain_[lodChainIndex_].pModel : NULL;
}

INLINE Model* SuperModel::ModelPartChain::curModel()
{
	return lodChainIndex_ < lodChain_.size() ?
		lodChain_[lodChainIndex_].pModel : NULL;
}

INLINE const Model::TransformState*
	SuperModel::ModelPartChain::curTransformState() const
{
	return lodChainIndex_ < lodChain_.size()
		? lodChain_[lodChainIndex_].transformState : NULL;
}

INLINE Model::TransformState* SuperModel::ModelPartChain::curTransformState()
{
	return lodChainIndex_ < lodChain_.size()
		? lodChain_[lodChainIndex_].transformState : NULL;
}

/**
 *	Get the the currently calculated lod of the SuperModel.
 *	@return the lod.
 */
INLINE float SuperModel::getLod() const
{
	return lod_;
}


/**
 *	Get the next point a model within the SuperModel will change LODs.
 *	@return the next lod if you get further.
 */
INLINE float SuperModel::getLodNextDown() const
{
	return lodNextDown_;
}


/**
 *	Get the next point a model within the SuperModel will change LODs.
 *	@return the next lod if you get closer.
 */
INLINE float SuperModel::getLodNextUp() const
{
	return lodNextUp_;
}

// super_model.ipp
