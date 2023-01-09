#ifdef CODE_INLINE
	#define INLINE inline
#else
	#define INLINE
#endif


#ifdef EDITOR_ENABLED
INLINE /*static*/ void Model::setAnimLoadCallback(
	SmartPointer< AnimLoadCallback > pAnimLoadCallback )
{
	s_pAnimLoadCallback_ = pAnimLoadCallback;
}


INLINE MetaData::MetaData& Model::metaData()
{
	return metaData_;
}
#endif


INLINE Moo::VisualPtr Model::getVisual()
{
	return NULL;
}


INLINE const BSPTree * Model::decompose() const
{
	return NULL;
}


INLINE bool Model::hasNode( Moo::Node * pNode,
	MooNodeChain * pParentChain ) const
{
	return false;
}


INLINE bool Model::addNode( Moo::Node * pNode )
{
	return false;
}


INLINE void Model::addNodeToLods( const BW::string & nodeName )
{
}


INLINE const BW::string & Model::resourceID() const
{
	return resourceID_;
}


/**
 *	Intentionally not returning a smart pointer here because we're
 *	the only ones that should be keeping references to our parent
 *	(when not keeping references to us).
 */
INLINE Model * Model::parent()
{
	return parent_.getObject();
}


/**
 *	Intentionally not returning a smart pointer here because we're
 *	the only ones that should be keeping references to our parent
 *	(when not keeping references to us).
 */
INLINE const Model * Model::parent() const
{
	return parent_.getObject();
}


/**
 *	This method gets the model's "extent".
 *	"Extent" means the maximum distance you can be from the object before it
 *	gets lodded out.
 *	A model's extent must be lower than that of it's parent model.
 *	@return the max distance at which you can view the model.
 *		If the model is hidden, then the extent is LOD_HIDDEN.
 *		If the model is missing, then the extent is LOD_MISSING.
 */
INLINE float Model::extent() const
{
	return extent_;
}


INLINE ModelAnimation * Model::lookupLocalDefaultAnimation()
{
	return pDefaultAnimation_.getObject();
}


/**
 *	Use with care... lock all accesses to the catalogue
 */
INLINE /*static*/ Model::PropCatalogue & Model::propCatalogueRaw()
{
	return s_propCatalogue_;
}


/**
 *	Use with care... lock all accesses to the catalogue
 */
INLINE /*static*/ SimpleMutex & Model::propCatalogueLock()
{
	return s_propCatalogueLock_;
}


INLINE int Model::initMatter( Matter & m )
{
	return 0;
}


INLINE bool Model::initTint( Tint & t, DataSectionPtr matSect )
{
	return false;
}


INLINE void Model::isInModelMap( bool newVal )
{
	isInModelMap_ = newVal;
}
