#include "pch.hpp"

#include "model.hpp"

#include "model_actions_iterator.hpp"
#include "model_animation.hpp"
#include "model_common.hpp"
#include "model_factory.hpp"
#include "model_map.hpp"
#include "nodefull_model.hpp"
#include "nodeless_model.hpp"
#include "tint.hpp"
#include "moo/resource_load_context.hpp"
#include "moo/effect_property.hpp"
#include "moo/texture_usage.hpp"
#include "moo/streamed_data_cache.hpp"
#include "moo/animation_manager.hpp"
#include "moo/streamed_animation_channel.hpp"
#include "resmgr/auto_config.hpp"

DECLARE_DEBUG_COMPONENT2( "Model", 0 )

BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "model.ipp"
#endif

Model::PropCatalogue Model::s_propCatalogue_;
SimpleMutex Model::s_propCatalogueLock_;

/*static*/ BW::vector< BW::string > Model::s_warned_;

Model::ModelStatics Model::s_ModelStatics;

Model::ModelStatics & Model::ModelStatics::instance()
{
	SINGLETON_MANAGER_WRAPPER( ModelStatics )
	return s_ModelStatics;
}

#ifdef EDITOR_ENABLED
/*static*/ SmartPointer< AnimLoadCallback > Model::s_pAnimLoadCallback_ = NULL;
static AutoConfigString s_modelMetaDataConfig( "editor/metaDataConfig/model", "helpers/meta_data/model.xml" );
#endif


template <class TYPE>
uint32 contentSize( const BW::vector< TYPE >& vector )
{
	return vector.size() * sizeof( TYPE );
}

template <class TYPE>
uint32 contentSize( const OrderedStringMap< TYPE >& vector )
{
	return vector.size() * sizeof( TYPE );
}


/**
 *	This class lets us use a singleton of another class in a safe way
 */
template <class C> class OneClassPtr
{
public:
	C * operator->()
		{ if (pC == NULL) pC = new C(); return pC; }
	C & operator*()
		{ return *this->operator->(); }
	~OneClassPtr()
		{ if (pC != NULL) { bw_safe_delete(pC); } }
private:
	static C	* pC;
};

typedef OneClassPtr<ModelMap> OneModelMapPtr;

template <> ModelMap * OneClassPtr<ModelMap>::pC = NULL;
//ModelMap * OneModelMapPtr::pC = NULL;

static OneModelMapPtr s_pModelMap;





PostLoadCallBack::PLCBmap PostLoadCallBack::threads_;
SimpleMutex PostLoadCallBack::threadsLock_;

/**
 *	This method adds a callback object to either the global or local list
 */
void PostLoadCallBack::add( PostLoadCallBack * plcb, bool global )
{
	BW_GUARD;
	SimpleMutexHolder smh( threadsLock_ );

	PLCBs & plcbs = threads_[OurThreadID()];
	if (!global)	plcbs.locals.push_back ( plcb );
	else			plcbs.globals.push_back( plcb );
}

/**
 *	This method runs all callbacks from one of the lists
 */
void PostLoadCallBack::run( bool global )
{
	BW_GUARD;
	SimpleMutexHolder smh( threadsLock_ );
	BW::vector< PostLoadCallBack * > * cbs;

	PLCBs & plcbs = threads_[OurThreadID()];
	cbs = global ? &plcbs.globals : &plcbs.locals;

	while (!cbs->empty())
	{
		PostLoadCallBack * lf = cbs->front();
		cbs->erase( cbs->begin() );

		threadsLock_.give();
		(*lf)();
		bw_safe_delete(lf);
		threadsLock_.grab();

		PLCBs & plcbs = threads_[OurThreadID()];
		cbs = global ? &plcbs.globals : &plcbs.locals;
	}
}


/**
 *	Enter a level of model loads in the current thread.
 */
void PostLoadCallBack::enter()
{
	BW_GUARD;
	SimpleMutexHolder smh( threadsLock_ );
	threads_[OurThreadID()].enterCount++;
}

/**
 *	Leave a level of model loads in the current thread.
 *	If it was the last level, call the global callbacks.
 */
void PostLoadCallBack::leave()
{
	BW_GUARD;
	threadsLock_.grab();
	bool wasLastLevel = (--threads_[OurThreadID()].enterCount == 0);
	threadsLock_.give();

	if (wasLastLevel) run( true );
}


/**
 *	This class is an object for calling a model's postLoad function
 */
class ModelPLCB : public PostLoadCallBack
{
public:
	ModelPLCB( Model & m, DataSectionPtr p ) : m_( m ), p_( p ) {};

private:
	virtual void operator()() { m_.postLoad( p_ ); }

	Model & m_;
	DataSectionPtr p_;
};


/**
 *	Special number given to models to tell them to calculate their own LOD
 *	based on their distance from the camera.
 */
/*static*/ const float Model::LOD_AUTO_CALCULATE = -1.0f;

/**
 *	Constant that the extent is set to when the model has been hidden.
 */
/*static*/ const float Model::LOD_HIDDEN = -1.0f;


/**
 *	Constant that the extent is set to when the model did not load properly.
 */
/*static*/ const float Model::LOD_MISSING = -2.0f;


/**
 *	Minimum possible extent.
 */
/*static*/ const float Model::LOD_MIN = 0.0f;


/**
 *	Maximum possible extent.
 */
/*static*/ const float Model::LOD_MAX = FLT_MAX;


void Model::release()
{
	BW_GUARD;
	extent_ = Model::LOD_MIN;
	pDefaultAnimation_ = NULL;

	Matters::iterator miter = matters_.begin();
	for (; miter != matters_.end(); ++miter)
	{ delete (*miter);
	}
	matters_.clear();

	animations_.clear();
	animationsIndex_.clear();
	actions_.clear();
	actionsIndex_.clear();

#ifdef EDITOR_ENABLED
	metaData_ = this;
#endif

}


/**
 *	This function is a concurrency lock between
 *	me reloading myself and our client pull info from
 *	myself, client begin to read.
*/
void Model::beginRead()
{
#if ENABLE_RELOAD_MODEL
	reloadRWLock_.beginRead();

	if (parent_)
	{
		parent_->beginRead();
	}

	Moo::VisualPtr pBulk = this->getVisual();
	if (pBulk)
	{
		pBulk->beginRead();
	}
#endif//ENABLE_RELOAD_MODEL
}


/**
 *	This function is a concurrency lock between
 *	me reloading myself and our client pull info from
 *	myself, client end read.
*/
void Model::endRead()
{
#if ENABLE_RELOAD_MODEL
	reloadRWLock_.endRead();

	if (parent_)
	{
		parent_->endRead();
	}

	Moo::VisualPtr pBulk = this->getVisual();
	if (pBulk)
	{
		pBulk->endRead();
	}

#endif//ENABLE_RELOAD_MODEL
}



/**
*	This function load myself's Model data.
*	Note, whoever called this function need
*	call parent_->endRead() outside, cause until 
*	NodefulModel/NodelessModel::load() finishes
*	we need lock the parent_ from reloading in other thread,
*	it's a bit embarrassing to do this way, but
*	it might be the easiest way.
*/
void Model::load()
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( Model_load );
	this->release();

	ModelPtr oldParent = parent_;

	DataSectionPtr pFile = BWResource::openSection( resourceID_ );

	// read the parent
	BW::string parentResID = pFile->readString( "parent" );
	if (!parentResID.empty()) 
	{
		parent_ = Model::get( parentResID + ".model" );
	}
	else
	{
		parent_ = NULL;
	}

	// Read the extent, if there is no extent section then it does not hide
	extent_ = pFile->readFloat( "extent", Model::LOD_MAX );

	// copy our parent's stuff
	if (parent_)
	{
		PROFILE_FILE_SCOPED( parent_beginRead );
		// Mutually exclusive of parent_ reloading, start
		parent_->beginRead();

		animations_ = parent_->animations_;

		matters_.reserve( parent_->matters_.size() );
		for (uint i=0; i < parent_->matters_.size(); i++)
		{
			Matter & pm = *parent_->matters_[i];

			matters_.push_back( new Matter( pm.name_, pm.replaces_, pm.tints_ ) );
		}
	}

	// schedule postLoad to be called
	PostLoadCallBack::add( new ModelPLCB( *this, pFile ) );

#ifdef EDITOR_ENABLED
	static MetaData::Desc dataDesc( s_modelMetaDataConfig.value() );
	metaData_.load( pFile, dataDesc );
#endif

	if (oldParent != parent_)
	{
		if (oldParent)
		{
			// Dismiss the reload listen relation between me and the old parent_
			oldParent->deregisterListener( this, true );
		}
		if (parent_)
		{
			// Register a listener if the parent_ is different
			parent_->registerListener( this );
		}
	}
}
/**
 *	Constructor
 */
Model::Model( const BW::StringRef & resourceID, DataSectionPtr pFile ) :
	resourceID_( resourceID.data(), resourceID.size() ),
	parent_( NULL ),
	extent_( Model::LOD_MIN ),
	isInModelMap_( true ),
	pDefaultAnimation_( NULL )
#ifdef EDITOR_ENABLED
	,metaData_( this )
#endif
{
	BW_GUARD;
}


/**
 *	Destructor
 */
Model::~Model()
{
	BW_GUARD;
	// subtract the memory we use from the counter
	for (uint i = 0; i < matters_.size(); i++)
	{
		bw_safe_delete(matters_[i]);
	}
	matters_.clear();
}


/**
 *	destroy is called when the ref count reaches 0, the model map is 
 *	responsible for deleting the object if it is deemed safe to do so.
 */
void Model::destroy() const
{
	bool isInMap = isInModelMap_;
	s_pModelMap->tryDestroy( this, isInMap );
}

/**
 * if our visual or parent_ is about to be reloaded, 
 * remove the related data from the old visual or parent_
 */
void Model::onReloaderPreReload( Reloader* pReloader)
{
	BW_GUARD;
	// bulk_ Visual  is about to be reloaded
	if ( this->getVisual() && pReloader == this->getVisual().get() )
	{
		// Since datas like bsp tree, visibilityBox_ will be updated,
		// while our listeners might be dependent on it, so
		// we update our listeners that the visual has changed.
		this->onPreReload( pReloader );
	}
	// parent_('s parent_..) Model is about to reload.
	else if ( this->isAncestor( (Model*)pReloader ))
	{
		// remove matters from the old parent_
		Matters::iterator miter = matters_.begin();
		while (miter != matters_.end())
		{ 
			bool removed = false;
			Matters::iterator mpiter =  parent_->matters_.begin();
			for (; mpiter !=  parent_->matters_.end(); ++mpiter)
			{
				Matter* pMatter = (*miter);
				Matter* pParentMatter = (*mpiter);
				if (pMatter->name_ == pParentMatter->name_ &&
					pMatter->replaces_ == pParentMatter->replaces_ )
				{
					delete pMatter;
					miter = matters_.erase( miter );
					removed = true;
					break;
				}
			}

			if (!removed)
			{
				++miter;
			}
		}
		//parent_'s parent_..
		this->onPreReload( pReloader );
	}
}


/**
 * This method return if pModel is in
 * my parent tree.
 */
bool Model::isAncestor( const Model* pModel ) const
{
	const Model* pTempModel = parent_.get();
	while (pTempModel != NULL)
	{
		if (pTempModel == pModel)
		{
			return true;
		}
		pTempModel = pTempModel->parent();
	}
	return false;
}


/**
 * if our visual or parent_ has been reloaded, 
 * update the related data.
 */
void Model::onReloaderReloaded( Reloader* pReloader )
{
	BW_GUARD;
	// bulk_ Visual reloaded
	if ( this->getVisual() && pReloader == this->getVisual().get() )
	{
		Matters::iterator miter = matters_.begin();
		for (; miter != matters_.end(); ++miter)
		{
			Matter* pMatter = (*miter);
			if (this->initMatter( *pMatter ) == 0)
			{
				ASSET_MSG( "Model::readDyes: "
					"In model \"%s\", for dye matter \"%s\", "
					"the material id \"%s\" isn't in the visual\n",
					resourceID_.c_str(), pMatter->name_.c_str(), pMatter->replaces_.c_str() );
			}
		}

		// Since datas like bsp tree, visibilityBox_ will be updated,
		// while our listeners might be dependent on it, so
		// we update our listeners that the visual has changed.
		this->onReloaded( pReloader );
	}
	// parent_('s parent_..) Model reloaded.
	else if ( this->isAncestor( (Model*)pReloader ))
	{
		{
#if ENABLE_RELOAD_MODEL
			ReadWriteLock::ReadGuard readGuard( parent_->reloadRWLock_ );
			ReadWriteLock::WriteGuard writeGuard( reloadRWLock_ );
#endif
			//recalculate the animationsIndex_
			AnimationsIndex	animationsIndex;
			Animations animations;
			// add animations from the new parent_ first.
			animations = parent_->animations_;
			//add animations from current model
			AnimationsIndex::iterator iiter = animationsIndex_.begin();
			for (; iiter != animationsIndex_.end(); ++iiter)
			{
				animationsIndex.insert(
					std::make_pair( iiter->first, (int)animations.size() ) );
				animations.push_back( animations_[ iiter->second ] );
			}
			animations_ = animations;
			animationsIndex_ = animationsIndex;

			// add matters from the new parent_
			for (uint i=0; i < parent_->matters_.size(); i++)
			{
				Matter & pm = *parent_->matters_[i];

				matters_.push_back( new Matter( pm.name_, pm.replaces_, pm.tints_ ) );
			}
		}

		// Since animations_ have been changed, so the PyModel's SuperModelAction, SuperModelDye
		// must be updated, and parent_ as well.
		this->onReloaded( pReloader );
	}
}

/**
 *	This method loads a model's dyes, if it has them. If multipleMaterials is
 *	true, then it calls the derived model to search for searches for material
 *	names in its bulk. Otherwise the dyes are read as if for a billboard model,
 *	which can have only one material (and thus dyes must each be composited).
 */
int Model::readDyes( DataSectionPtr pFile, bool multipleMaterials )
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( readDyes );
	int dyeProduct = 1;

	// read our own dyes
	BW::vector<DataSectionPtr> vDyeSects;
	pFile->openSections( "dye", vDyeSects );
	for (uint i = 0; i < vDyeSects.size(); i++)
	{
		DataSectionPtr pDyeSect = vDyeSects[i];

		BW::string matterName = pDyeSect->readString( "matter", "" );
		BW::string replaces = pDyeSect->readString( "replaces", "" );

		//Ignore any empty "dye" sections
		if ((matterName == "") && (replaces == ""))
			continue;

		// see if we already have a matter of that name


		Matters::iterator miter = matters_.begin();
		for (; miter != matters_.end(); ++miter)
		{
			if ((*miter)->name_ == matterName)
				break;
		}

		if (miter == matters_.end())
		{
			matters_.push_back( new Matter( matterName, replaces ) );
			miter = matters_.end() - 1;
		}

		// set it up
		Matter * m = *miter;
		m->replaces_ = replaces;

		// get all instances of replaces from the visual,
		// and set the default tint from the original material
		if (multipleMaterials)	// not for billboards
		{
			if (this->initMatter( *m ) == 0)
			{
				ASSET_MSG( "Model::readDyes: "
					"In model \"%s\", for dye matter \"%s\", "
					"the material id \"%s\" isn't in the visual\n",
					resourceID_.c_str(), matterName.c_str(), replaces.c_str() );

				continue;
			}
		}
		else					// for billboards
		{
			//miter->second.tints_["Default"]->oldMaterial_->textureFactor( 0 );
		}

		// add the other tints
		BW::vector<DataSectionPtr> vTintSects;
		pDyeSect->openSections( "tint", vTintSects );
		for (uint j = 0; j < vTintSects.size(); j++)
		{
			DataSectionPtr pTintSect = vTintSects[j];

			BW::string tintName = pTintSect->readString( "name", "" );
			if (tintName.empty())
			{
				ASSET_MSG( "Model::readDyes: "
					"In model %s, for dye matter %s, "
					"tint section index %d has no name.\n",
					resourceID_.c_str(), matterName.c_str(), j );
				continue;
			}

			// if it's already there then use that one
			Matter::Tints::iterator titer = m->tints_.begin();
			for (; titer != m->tints_.end(); ++titer)
			{
				if ((*titer)->name_ == tintName)
					break;
			}

			// Create if missing
			if (titer == m->tints_.end())
			{

				m->tints_.push_back( new Tint( tintName ) );
				titer = m->tints_.end() - 1;
			}

			TintPtr t = *titer;

			// if it's not a billboard...
			if (multipleMaterials)
			{
				// clear source dyes that we don't use
				t->sourceDyes_.clear();

				// read the material
				DataSectionPtr matSect = pTintSect->openSection( "material" );
				if (!this->initTint( *t, matSect ))
				{
					WARNING_MSG( "Model::readDyes: "
						"In model %s, for dye matter %s tint %s, "
						"there %s <material> section.\n",
						resourceID_.c_str(), matterName.c_str(), tintName.c_str(),
						(!!matSect) ? "was a problem loading the" : "is no" );
				}

				// read the properties
				BW::vector<DataSectionPtr> vPropSects;
				pTintSect->openSections( "property", vPropSects );
				for (uint k = 0; k < vPropSects.size(); k++)
				{
					DataSectionPtr pPropSect = vPropSects[k];

					BW::string propName = pPropSect->readString( "name", "" );
					if (propName.empty())
					{
						ASSET_MSG( "Model::readDyes: "
							"In model %s, for dye matter %s tint %s, "
							"property section index %d has no name.\n",
							resourceID_.c_str(), matterName.c_str(),
							tintName.c_str(), k );
						continue;
					}

					s_propCatalogueLock_.grab();
					BW::stringstream catalogueName;
					catalogueName << matterName << "." << tintName << "." << propName;
					PropCatalogue::iterator pit =
						s_propCatalogue_.find( catalogueName.str().c_str() );
					if (pit == s_propCatalogue_.end())
					{
						pit = s_propCatalogue_.insert(
							std::make_pair( catalogueName.str().c_str(), Vector4() ) );
					}

					DyeProperty dp;
					size_t index = pit - s_propCatalogue_.begin();
					MF_ASSERT( index <= INT_MAX );
					dp.index_ = ( int ) index;
					s_propCatalogueLock_.give();
					dp.controls_ = pPropSect->readInt( "controls", 0 );
					dp.mask_ = pPropSect->readInt( "mask", 0 );
					dp.future_ = pPropSect->readInt( "future", 0 );
					dp.default_ = pPropSect->readVector4( "default",
						Vector4(0,0,0,0) );

					if (dp.mask_ == -1) dp.mask_ = 0;
					// look for a corresponding D3DXHANDLE
					// (if newMaterial is NULL, won't have any props anyway)

					if ( t->effectMaterial_->baseMaterial()->pEffect() )
					{
						ID3DXEffect * effect = t->effectMaterial_->baseMaterial()->pEffect()->pEffect(); 
						D3DXHANDLE propH = effect->GetParameterByName( NULL, propName.c_str() ); 

						if (propH != NULL)
						{
							const Moo::EffectPropertyFactoryPtr& factory = 
								Moo::EffectPropertyFactory::findFactory( "Vector4" );
							MF_ASSERT( factory != NULL );

							Moo::EffectPropertyPtr modifiableProperty = 
								factory->create( propName, propH, effect );

							modifiableProperty->be( dp.default_ ); 
							t->effectMaterial_->baseMaterial()->replaceProperty( propName,
								modifiableProperty ); 

							dp.controls_ = (intptr)propH;
							dp.mask_ = -1;
						}
					}

					uint l;
					for (l = 0; l < t->properties_.size(); l++)
					{
						if (t->properties_[l].index_ == dp.index_) break;
					}
					if (l < t->properties_.size())
						t->properties_[l] = dp;
					else
						t->properties_.push_back( dp );

					// end of properties
				}
			}
			// if it's a billboard...
			else
			{
				t->properties_.clear();
				t->sourceDyes_.clear();


				// read the source dye selections
				BW::vector<DataSectionPtr> vSourceDyes;
				pTintSect->openSections( "dye", vSourceDyes );
				for (uint k = 0; k < vSourceDyes.size(); k++)
				{
					DataSectionPtr pSourceDye = vSourceDyes[k];
					DyeSelection dsel;

					if (!this->readSourceDye( pSourceDye, dsel ))
					{
						ASSET_MSG( "Model::readDyes: "
							"In model %s, for dye matter %s tint %s, "
							"source dye selection section index %d is missing "
							"either a matter ('%s') or tint ('%s') name.\n",
							resourceID_.c_str(), matterName.c_str(),
							tintName.c_str(), k,
							dsel.matterName_.c_str(), dsel.tintName_.c_str() );
					}

					t->sourceDyes_.push_back( dsel );
				}


			}

			// end of tints
		}

		// keep a running total of the dye product
		dyeProduct *= static_cast<int>(m->tints_.size());

		MF_ASSERT( dyeProduct );

		// end of matters (dyes)
	}

	// end of method
	return dyeProduct;
}



/*static*/ void Model::clearReportedErrors()
{
	BW_GUARD;
	s_warned_.clear();
}

/**
 *	Static function to get the model with the given resource ID
 */
ModelPtr Model::get( const BW::StringRef & resourceID, bool loadIfMissing )
{
	BW_GUARD_PROFILER( Model_get );
	ModelPtr pFound = s_pModelMap->find( resourceID );
	if (pFound)
		return pFound;

	if (!loadIfMissing)
		return NULL;

	DataSectionPtr pFile = BWResource::openSection( resourceID );
	if (!pFile)
	{
		BW::vector< BW::string >::iterator entry = std::find( s_warned_.begin(), s_warned_.end(), resourceID );
		if (entry == s_warned_.end())
		{

			ASSET_MSG( "Model::get: Could not open model resource %s\n",
				resourceID.to_string().c_str() );
			s_warned_.push_back( resourceID.to_string() );
		}
		return NULL;
	}

	PostLoadCallBack::enter();

	ModelPtr mp = Model::load( resourceID, pFile );

	if (mp)
	{
		// must take reference to model before adding to map, or else
		// another thread looking for it could think it's been deleted
		s_pModelMap->add( &*mp, resourceID );
	}

	PostLoadCallBack::leave();

	return mp;
}

/**
 *	This method returns the size of this Model in bytes.
 */
uint32 Model::sizeInBytes() const
{
	BW_GUARD;
	size_t size = sizeof(*this) + resourceID_.length();
	uint parentSize;

	AnimationsIndex::const_iterator it;

	size += animations_.capacity() * sizeof(animations_[0]);
	parentSize = parent_ ? static_cast<uint>(parent_->animations_.size()) : 0;
	for (it = animationsIndex_.begin(); it != animationsIndex_.end(); it++)
	{
		size += 16;	// about the size of a tree node, excluding value_type
		size += sizeof(it) + it->first.length();
	}
	for (uint i = parentSize; i < animations_.size(); i++)
		size += animations_[i]->sizeInBytes();

	size += actions_.capacity() * sizeof(actions_[0]);
	Actions::const_iterator it2;
	for (it2 = actions_.begin(); it2 != actions_.end(); it2++)
	{
		size += 16;										// map node overhead
		size += sizeof(ActionsIndex::const_iterator);	// map node size

		size += (*it2)->sizeInBytes();
	}

	MF_ASSERT( size <= UINT_MAX );
	return ( uint32 ) size;
}


/**
 *	This private static method performs operations common to
 *	both the 'get' and 'reload' methods.
 */
ModelPtr Model::load( const BW::StringRef & resourceID, DataSectionPtr pFile )
{
	BW_GUARD;
	Moo::ScopedResourceLoadContext resLoadCtx( BWResource::getFilename( resourceID ) );

	ModelFactory factory( resourceID, pFile );
	ModelPtr model = Moo::createModelFromFile( pFile, factory );
	
	if (!model)
	{
		ASSET_MSG( "Could not load model: '%s'\n", resourceID.to_string().c_str() );
		return NULL;
	}

	PostLoadCallBack::run();

	if (!(model && model->valid()))
	{
		ASSET_MSG( "Error in post load of model: '%s'\n", resourceID.to_string().c_str() );
		return NULL;
	}

	return model;
}



/**
 *	This method reads the source dye described in the given data section
 *	into a DyeSelection object.
 *
 *	@return true for success
 */
bool Model::readSourceDye( DataSectionPtr pSourceDye, DyeSelection & dsel )
{
	BW_GUARD;
	for (DataSectionIterator il = pSourceDye->begin();
		il != pSourceDye->end();
		++il)
	{
		const BW::string & sName = (*il)->sectionName();
		if (sName == "matter")
			dsel.matterName_ = sName;
		else if (sName == "tint")
			dsel.tintName_ = sName;
		else	// general property
		{
			DyePropSetting	dps;

			s_propCatalogueLock_.grab();
			PropCatalogue::iterator pit =
				s_propCatalogue_.find( sName.c_str() );
			if (pit == s_propCatalogue_.end())
			{
				pit = s_propCatalogue_.insert(
					std::make_pair( sName.c_str(), Vector4() ) );
			}

			size_t index = pit - s_propCatalogue_.begin();
			MF_ASSERT( index <= INT_MAX );
			dps.index_ = ( int ) index;
			s_propCatalogueLock_.give();
			dps.value_ = (*il)->asVector4();
			dsel.properties_.push_back( dps );
		}
	}

	return !(dsel.matterName_.empty() || dsel.tintName_.empty());
}


/**
 *	This method loads stuff that is common to all model types but must
 *	be done after the rest of the model has been loaded - i.e. actions.
 */
void Model::postLoad( DataSectionPtr pFile )
{
	BW_GUARD;
	PROFILER_SCOPED( postLoad );
	// load the actions
	BW::vector<DataSectionPtr> vActionSects;
	pFile->openSections( "action", vActionSects );
	for (uint i = 0; i < vActionSects.size(); i++)
	{
		DataSectionPtr pActionSect = vActionSects[i];

		ModelAction * pAction = new ModelAction( pActionSect );

		if (!pAction->valid(*this))
		{
			ASSET_MSG( "Invalid Action: '%s' in model '%s'\n",
						pActionSect->readString("name", "<no name>").c_str(),
						this->resourceID().c_str());
			bw_safe_delete(pAction);
			continue;
		}

		actionsIndex_.insert(
			std::make_pair( &pAction->name_, (int)actions_.size() ) );
		actions_.push_back( pAction );
	}

	for (uint i = 0; i < matters_.size(); i++)
	{
		Matter & m = *matters_[i];
		for (uint j = 0; j < m.tints_.size(); j++)
		{
			TintPtr t = m.tints_[j];
			for (uint k = 0; k < t->sourceDyes_.size(); k++)
			{
				DyeSelection & d = t->sourceDyes_[k];
			}
		}
	}
}


/**
 *	@todo
 */
const int Model::blendCookie()
{
	return Model::ModelStatics::instance().s_blendCookie_;
}


/**
 *	@todo
 */
int Model::getUnusedBlendCookie()
{
	return ((Model::ModelStatics::instance().s_blendCookie_ - 16) & 0x0fffffff);
}


/**
 *	@todo
 */
int Model::incrementBlendCookie()
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV(MainThreadTracker::isCurrentThreadMain())
	{
		MF_EXIT( "incrementBlendCookie called, but not from the main thread" );
	}

	ModelStatics & statics = Model::ModelStatics::instance();
	statics.s_blendCookie_ = ((statics.s_blendCookie_ + 1) & 0x0fffffff);
	return statics.s_blendCookie_;
}


/**
 *	This method soaks the Model's materials in the currently emulsified dyes,
 *	or the default for each matter if it has no emulsion. It is a utility
 *	method called by models that use multiple materials to implement their dyes.
 */
void Model::soakDyes()
{
	BW_GUARD;
	for (Matters::iterator it = matters_.begin(); it != matters_.end(); it++)
	{
		(*it)->soak();
	}
}

namespace
{
	bool isModelExtentValid( const float modelExtent )
	{
		return ! ( isEqual( modelExtent, Model::LOD_MISSING )
		        || isEqual( modelExtent, Model::LOD_HIDDEN )
		        || isZero( modelExtent ) );
	}

	// A recursive function that will check that the Model chain defined by
	// a model and its parent_ (and its parent_'s parent_ etc.), which are all
	// ordered by their extents.  The 'parent' parameter is not expected
	// to be the immediate parent of 'model', this function may re-enter with
	// subsequent ancestors until one is found with a visible extent or is
	// otherwise NULL.
	bool checkModelExtentsAreOrdered( const Model & model, const Model * parent )
	{
		const float modelExtent = model.extent();
		if (!parent || !isModelExtentValid( modelExtent ))
		{
			return true;
		}
		
		const float parentExtent = parent->extent();
		if (!isModelExtentValid( parentExtent ))
		{
			return checkModelExtentsAreOrdered( model, parent->parent() );
		}

		if (modelExtent >= parentExtent)
		{
			ASSET_MSG( "Cannot load model '%s' "
				"with a higher LOD %f than its visible ancestor '%s' LOD %f\n",
				model.resourceID().c_str(),
				modelExtent,
				parent->resourceID().c_str(),
				parentExtent );
			return false;
		}
		return true;
	}
} // end unnamed namespace

/**
 *	This function returns true if the Model is in a valid
 *	state.
 *	This function will not raise a critical error or asserts if called but may
 *	commit ERROR_MSGs to the log.
 *
 *	@return		Returns true if the animation is in a valid state, otherwise
 *				false.
 */
bool Model::valid() const
{
	BW_GUARD;
	if (!pDefaultAnimation_)
	{
		ASSET_MSG("Model '%s' has no default pose.\n", resourceID().c_str());
		return false;
	}

	for (	AnimationsIndex::const_iterator i = animationsIndex_.begin();
			i != animationsIndex_.end();
			++i)
	{
		if (i->second >= animations_.size())
		{
			return false;
		}

		if (i->first.empty())
		{
			return false;
		}
	}

	for (uint i=0; i < animations_.size(); i++)
	{
		if (!animations_[i]->valid())
		{
			BW::string animationName = "<unknown animation>";

			for (	AnimationsIndex::const_iterator j = animationsIndex_.begin();
					j != animationsIndex_.end();
					++j)
			{
				if (i == j->second)
				{
					animationName = j->first;
					break;
				}
			}

			ASSET_MSG(	"Invalid animation '%s' in model '%s'\n",
						animationName.c_str(),
						resourceID().c_str());
			return false;
		}
	}

	for (uint i=0; i < actions_.size(); i++)
	{
		if (!actions_[i]->valid(*this))
		{
			ASSET_MSG(	"Invalid action '%s' in model '%s'\n",
						actions_[i]->name_.c_str(),
						resourceID().c_str());
			return false;
		}
	}

	// Check that the models are sorted by their LODs
	return checkModelExtentsAreOrdered( *this, parent() );
}


/**
 *	This method gets the index of the input animation name in this model's
 *	table. The index is also valid for all the ancestors and descendants
 *	of this model.
 *
 *	@return the index of the animation, or -1 if not found
 */
int Model::getAnimation( const BW::string & name ) const
{
	BW_GUARD;
	for (const Model * pModel = this; pModel != NULL; pModel = &*pModel->parent_)
	{
		AnimationsIndex::const_iterator found =
			pModel->animationsIndex_.find( name );
		if (found != pModel->animationsIndex_.end())
			return static_cast<int>(found->second);
	}
	return -1;
}

/**
 *	This method ticks the animation with the given index if it exists.
 */
void Model::tickAnimation( int index, float dtime, float otime, float ntime )
{
	BW_GUARD;
	if (uint(index) >= animations_.size())
		return;
	animations_[ index ]->tick( dtime, otime, ntime );
}

/**
 *	This method plays the animation with the given index. If the animation
 *	doesn't exist for this model, then the default animation is played instead.
*/
void Model::playAnimation( int index, float time, float blendRatio, int flags )
{
	BW_GUARD;
	ModelAnimationPtr & anim = (uint(index) < animations_.size()) ?
			animations_[ index ] : pDefaultAnimation_;

	if (time > anim->duration_) time = anim->looped_ ?
		fmodf( time, anim->duration_ ) : anim->duration_;
	// only not doing this in Animation::play because its virtual.
	anim->play( time, blendRatio, flags );
}


/**
 *	This method gets the animation that is most local to this model
 *	 for the given index. It does not return the default animation
 *	 if the index cannot be found - instead it returns NULL.
 */
ModelAnimation * Model::lookupLocalAnimation( int index )
{
	BW_GUARD;
	if (index >= 0 && (uint)index < animations_.size())
	{
		return animations_[ index ].getObject();
	}

	return NULL;
}


/**
 *	This method gets the pointer to the input action name in this model's
 *	table. Actions only make sense in the context of the most derived model
 *	in a SuperModel (i.e. the top lod one), so we don't have to do any
 *	tricks with indices here. Of course, the animations referenced by these
 *	actions still do this.
 *
 *	@return the action, or NULL if not found
 */
const ModelAction * Model::getAction( const BW::string & name ) const
{
	BW_GUARD;
	for (const Model * pModel = this;
		pModel != NULL;
		pModel = &*pModel->parent_)
	{
		ActionsIndex::const_iterator found =
			pModel->actionsIndex_.find( &name );
		if (found != pModel->actionsIndex_.end())
			return pModel->actions_[ found->second ].getObject();
	}
	return NULL;
}





/**
 *	This method looks up the local action of the given index
 */
ModelActionsIterator Model::lookupLocalActionsBegin() const
{
	BW_GUARD;
	return ModelActionsIterator( this );
}

ModelActionsIterator Model::lookupLocalActionsEnd() const
{
	BW_GUARD;
	return ModelActionsIterator( NULL );
}




/**
 *	This static method returns the index of the property with the given
 *	name in the global property catalogue. It returns -1 if a property
 *	of that name does not exist.
 */
int Model::getPropInCatalogue( const char * name )
{
	BW_GUARD;
	SimpleMutexHolder smh( s_propCatalogueLock_ );
	PropCatalogue::iterator found = s_propCatalogue_.find( name );
	if (found == s_propCatalogue_.end()) return -1;
	MF_ASSERT( s_propCatalogue_.size() <= INT_MAX );
	return ( int ) ( found - s_propCatalogue_.begin() );
}

/**
 *	This static method looks up the name of the given property in the
 *	global property catalogue. There is no method to look up the value
 *	and return a pointer to it as that would not be thread safe.
 *	Returns NULL if the index is out of range.
 */
const char * Model::lookupPropName( int index )
{
	BW_GUARD;
	SimpleMutexHolder smh( s_propCatalogueLock_ );
	if (uint(index) >= s_propCatalogue_.size()) return NULL;
	return (s_propCatalogue_.begin() + index)->first;
	// safe to return this pointer as props are never deleted from
	// the catalogue and even if they move around in the tables or
	// vector the string memory stays put.
}


/**
 *	This method gets a (packed) index of the dye formed by the
 *	given matter and tint names. If ppTint is not NULL, and the
 *	dye is found, then the pointer to the tint in the local model
 *	is written into ppTint.
 */
ModelDye Model::getDye(	const BW::string & matterName,
						const BW::string & tintName,
						Tint ** ppTint )
{
	BW_GUARD;
	int matterIndex = -1;
	int tintIndex = -1;

	Matters::iterator miter = matters_.begin();
	for (; miter != matters_.end(); ++miter)
	{
		if ((*miter)->name_ == matterName)
			break;
	}

	if (miter != matters_.end())
	{
		MF_ASSERT( matters_.size() <= INT_MAX );
		matterIndex = ( int ) ( miter - matters_.begin() );

		Matter::Tints & tints = (*miter)->tints_;
		Matter::Tints::iterator titer = tints.begin();
		for (; titer != tints.end(); ++titer)
		{
			if ((*titer)->name_ == tintName)
				break;
		}

		if (titer != tints.end())
		{
			MF_ASSERT( tints.size() <= INT_MAX );
			tintIndex = ( int ) ( titer - tints.begin() );
			if (ppTint != NULL)
				*ppTint = &*(*titer);
		}
	}

	return ModelDye( *this, matterIndex, int16(tintIndex) );
}


/**
 *	This method soaks this model in the given (packed) dye index.
 *	It is a default method for model classes that use multiple materials.
 */
void Model::soak( const ModelDye & dye )
{
	BW_GUARD;
	// look up the matter
	if (uint(dye.matterIndex()) >= matters_.size())
		return;

	// tell it to tint
	matters_[ dye.matterIndex() ]->emulsify( dye.tintIndex() );
}


/**
 *	This method looks up the local matter of the given index
 */
const Matter * Model::lookupLocalMatter( int index ) const
{
	BW_GUARD;
	if (index >= 0 && (uint)index < matters_.size())
		return matters_[ index ];
	else
		return NULL;
}


/**
 *	@todo
 */
int Model::gatherMaterials(	const BW::string & materialIdentifier,
							BW::vector< Moo::Visual::PrimitiveGroup * > & primGroups,
							Moo::ConstComplexEffectMaterialPtr * ppOriginal )
{
	BW_GUARD;
	return 0;
}

/**
 *	This method updates the usageGroup with the set of textures
 * used by this model.
 */
void Model::generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup )
{
	float density = uvSpaceDensity();

	// Gather all materials that are used by the base visual
	Moo::VisualPtr pVisual = getVisual();
	if (pVisual)
	{
		pVisual->generateTextureUsage( usageGroup );
	}
	
	// Now gather the data from the matters
	for (Matters::iterator iter = matters_.begin(), end = matters_.end(); 
		iter != end; ++iter)
	{
		Matter* pMatter = *iter;
		
		for (Matter::Tints::iterator tintIter = pMatter->tints_.begin(),
			tintEnd = pMatter->tints_.end(); tintIter != tintEnd;
			++tintIter)
		{
			Tint * pTint = (*tintIter).get();
			Moo::ComplexEffectMaterial * pMaterial = pTint->effectMaterial_.get();
			if (pMaterial)
			{
				pMaterial->baseMaterial()->generateTextureUsage( usageGroup, density );
			}
		}
	}
}

/**
 * This method fetches the cached maxiumum uvSpaceDensity 
 * of the model.
 */
float Model::uvSpaceDensity()
{
	Moo::VisualPtr pVisual = getVisual();
	if (pVisual)
	{
		return getVisual()->uvSpaceDensity();
	}
	else
	{
		return Moo::ModelTextureUsage::INVALID_DENSITY;
	}
}

/**
 *	Default implementation. Returns empty node tree iterator.
 */
NodeTreeIterator Model::nodeTreeBegin() const
{
	return NodeTreeIterator( NULL, NULL, NULL );
}

/**
 *	Default implementation. Returns empty node tree iterator.
 */
NodeTreeIterator Model::nodeTreeEnd() const
{
	BW_GUARD;
	return NodeTreeIterator( NULL, NULL, NULL );
}


/**
 *	Helper for loading animation sections out of the given model resource.
 */
//static
bool Model::loadAnimations(
	const BW::string & resourceID,
	Moo::StreamedDataCache ** pOutAnimCache,
	BW::vector< LoadedAnimationInfo >* pOutInfo )
{
	static BW::vector < BW::string > s_animWarned;

	DataSectionPtr pFile = BWResource::openSection( resourceID );
	if (!pFile)
	{
		return false;
	}

	Moo::StreamedDataCache * pAnimCache = NULL;

	// read in the sections describing our animations
	BW::vector<DataSectionPtr>	vAnimSects;
	pFile->openSections( "animation", vAnimSects );

	// set up an animation cache for this model, if we have any animations
#ifndef EDITOR_ENABLED
	if (!vAnimSects.empty())
	{
		BW::string animCacheName = resourceID;
		if (animCacheName.length() > 5)
			animCacheName.replace( animCacheName.length()-5, 5, "anca" );
		else
			animCacheName.append( ".anca" );
		pAnimCache = new Moo::StreamedDataCache( animCacheName, true );

		for( uint i = 0; i < vAnimSects.size(); ++i )
		{
			DataSectionPtr pAnimData;
			if( !( pAnimData = vAnimSects[ i ]->openSection( "nodes" ) ) )
				continue;// we'll revenge later

			BW::string nodesRes = pAnimData->asString() + ".animation";
			uint64 modifiedTime = BWResource::modifiedTime( nodesRes );

			if( pAnimCache->findEntryData( nodesRes, Moo::StreamedDataCache::CACHE_VERSION, modifiedTime ) == NULL )
			{
				pAnimCache->deleteOnClose( true );
				delete pAnimCache;
				pAnimCache = new Moo::StreamedDataCache( animCacheName, true );
				break;
			}
		}

		Moo::StreamedAnimation::s_loadingAnimCache_ = pAnimCache;
		Moo::StreamedAnimation::s_savingAnimCache_ = pAnimCache;
	}
#endif

	// We keep a temporary map of the animations for this model in 
	// case multiple sub-animations using the same resource are used
	typedef BW::map< BW::string, Moo::AnimationPtr > AnimMap;
	AnimMap modelAnims;

	// load our animations in the context of the global node table.
	for (uint i = 0; i < vAnimSects.size(); i++)
	{
		DataSectionPtr pAnimSect = vAnimSects[i];

		// read the name
		BW::string animName = pAnimSect->readString( "name" );

		// now read the node stuff
		DataSectionPtr pAnimData;
		if (!(pAnimData = pAnimSect->openSection( "nodes" )))
		{
			ERROR_MSG( "Animation %s (section %d) in model %s has no 'nodes' section\n",
				animName.c_str(), i, resourceID.c_str() );
			continue;
		}

		BW::string nodesRes = pAnimData->asString();

		if (animName.empty())
		{
			const char * nameBeg = nodesRes.data() +
				nodesRes.find_last_of( '/' )+1;
			const char * nameEnd = nodesRes.data() +
				nodesRes.length();
			animName.assign( nameBeg, nameEnd );
		}

		if (animName.empty())
		{
			ERROR_MSG( "Animation section %d in model %s has no name\n",
				i, resourceID.c_str() );
			continue;
		}

		// ok, now we know where to get it and what to call it,
		//  let's load the actual animation!
		nodesRes += ".animation";

#ifdef EDITOR_ENABLED
		// We don't use the .anca files in the editor so just grab our animation
		// fromthe animation manager
		Moo::AnimationPtr pAnim = Moo::AnimationManager::instance().get( nodesRes );
#else
		// Since we are using .anca files we do not want to get animations
		// from the anim manager. 
		Moo::AnimationPtr pAnim = NULL;

		// See if we have already loaded the animation for this model
		AnimMap::iterator loadedAnim = modelAnims.find( nodesRes );
		if (loadedAnim != modelAnims.end())
		{
			pAnim = loadedAnim->second;
		}
		else
		{
			// If the animation has not already been loaded, load it up,
			// don't use the animation manager as this may caus us to use
			// an the animation from a different .anca file, which will
			// be invalid if the owning model is removed from the scene.
			pAnim = new Moo::Animation;
			if (!pAnim->load(nodesRes))
			{
				pAnim = NULL;
			}
			else
			{
				// This line makes sure the node catalogue is set up
				// TODO: Fix this so it makes a bit more sense
				pAnim = new Moo::Animation( pAnim.get() );
			}

			modelAnims.insert( std::make_pair( nodesRes, pAnim ) );
		}
#endif //EDITOR_ENABLED
		//static int omu = memUsed() - memoryAccountedFor();
		//int nmu = memUsed() - memoryAccountedFor();
		//dprintf( "Using %d more, %d total after %s\n", nmu-omu, nmu, nodesRes.c_str() );
		//omu = nmu;

#ifdef EDITOR_ENABLED
		if (s_pAnimLoadCallback_)
			s_pAnimLoadCallback_->execute();
#endif

		if (!pAnim)
		{
			if (std::find( s_animWarned.begin(), s_animWarned.end(), nodesRes + animName + resourceID ) ==  s_animWarned.end())
			{
				BW::StringRef modelName = BWResource::getFilename( resourceID );
				ERROR_MSG( "Could not load the animation:\n\n"
					"  \"%s\"\n\n"
					"for \"%s\" of \"%s\".\n\n"
					"Please make sure this file exists or set a different one using ModelEditor.",
					nodesRes.c_str(), animName.c_str(), modelName.to_string().c_str() );
				s_animWarned.push_back( nodesRes + animName + resourceID );
			}
			continue;
		}

		if (pOutInfo)
		{
			LoadedAnimationInfo info;
			info.name_ = animName;
			info.pDS_ = pAnimSect;
			info.pAnim_ = pAnim;

			pOutInfo->push_back( info );
		}
	}

	// reset the static animation cache pointers
	Moo::StreamedAnimation::s_loadingAnimCache_ = NULL;
	Moo::StreamedAnimation::s_savingAnimCache_ = NULL;

	// delete it if it's empty
	if (pAnimCache && pAnimCache->numEntries() == 0)
	{
		pAnimCache->deleteOnClose( true );
		delete pAnimCache;
		pAnimCache = NULL;
	}

	if (pOutAnimCache)
	{
		*pOutAnimCache = pAnimCache;
	}
	else
	{
		bw_safe_delete( pAnimCache );
	}

	return true;
}

BW_END_NAMESPACE



// model.cpp
