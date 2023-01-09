#ifdef _MSC_VER 
#pragma once
#endif

#ifndef MODEL_HPP
#define MODEL_HPP

#include "forward_declarations.hpp"
#include "math/forward_declarations.hpp"
#include "moo/forward_declarations.hpp"
#include "physics2/forward_declarations.hpp"
#include "resmgr/forward_declarations.hpp"
#include "moo/visual.hpp"

#include "dye_property.hpp"
#include "dye_selection.hpp"
#include "matter.hpp"
#include "model_action.hpp"
#include "model_dye.hpp"
#include "node_tree.hpp"
#include "moo/reload.hpp"


#ifdef EDITOR_ENABLED
#include "gizmo/meta_data.hpp"
#include "gizmo/meta_data_helper.hpp"
#endif


BW_BEGIN_NAMESPACE

namespace Moo
{
class ModelTextureUsageGroup;
class DrawContext;
}

typedef BW::vector< Moo::NodePtr > MooNodeChain;


/**
 *	Helper class for sorting pointers to strings by their string value
 */
class StringOrderProxy
{
public:
	StringOrderProxy( const BW::string * pString ) : pString_( pString ) { }

	bool operator <( const StringOrderProxy & other ) const
		{ return (*pString_) < (*other.pString_); }
	bool operator ==( const StringOrderProxy & other ) const
		{ return (*pString_) == (*other.pString_); }
private:
	const BW::string * pString_;
};



/**
 * TODO: to be documented.
 */
class AnimLoadCallback : public ReferenceCount
{
public:
	AnimLoadCallback() {}

	virtual void execute() = 0;
};


/**
 *	This class defines and manages callbacks which occur at various
 *	times in the model loading process.
 */
class PostLoadCallBack
{
public:
	virtual ~PostLoadCallBack() {};

	virtual void operator()() = 0;

	static void add( PostLoadCallBack * plcb, bool global = false );
	static void run( bool global = false );

	static void enter();
	static void leave();

private:

	/**
	 *	Helper struct to store the callbacks for one thread
	 */
	struct PLCBs
	{
		PLCBs() : enterCount( 0 ) { }

		int									enterCount;
		BW::vector< PostLoadCallBack * >	globals;
		BW::vector< PostLoadCallBack * >	locals;
	};

	typedef BW::map< uint32, PLCBs > PLCBmap;
	static PLCBmap		threads_;
	static SimpleMutex	threadsLock_;
};

/**
 *	This class manages a model. There is only one instance
 *	of this class for each model file, as it represents only
 *	the static data of a model. Dynamic data is stored in the
 *	referencing SuperModel file.
 */
class Model : 
	public SafeReferenceCount,
/*
*	Note: Model is always listening if it's bulk_(visual) has 
*	been reloaded, if you are pulling info out from the visual
*	then please update it again in onReloaderReloaded which is 
*	called when the visual is reloaded so that related data
*	will need be update again.and you might want to do something
*	in onReloaderPriorReloaded which happen right before the visual reloaded.
*
*	Model is also always listening if it's parent_ has 
*	been reloaded, if you are pulling info out from the parent_
*	then please update it again in onReloaderReloaded which is 
*	called when the parent model is reloaded. and you might want to do something
*	in onReloaderPriorReloaded which happen before the parent_ reloaded.
*/
	public ReloadListener,
/*
*	Note: Model is also a reloader that other reloadListeners
*	like SuperModel will listen to and update if it's reloaded.
*	To support model reloading, if class A is pulling info out from Model,
*	A needs be registered as a listener of this pModel
*	and A::onReloaderReloaded(..) needs be implemented.
*	A::onReloaderReloaded(..) re-pulls info
*	out from pModel after pModel is reloaded. A::onReloaderPreReload()
*	might need be implemented which detaches info from the old pModel 
*	before it's reloaded.
*	Also when A is initialising from Model, you need call pModel->beginRead()
*	and pModel->endRead() to avoid concurrency issues.
*/
	public Reloader
{
	// TODO: remove use of friend
	friend class ModelAction;
	friend class ModelActionsIterator;
	friend class ModelMap;
	friend class ModelPLCB;
public:

	static const float LOD_AUTO_CALCULATE;
	static const float LOD_HIDDEN;
	static const float LOD_MISSING;
	static const float LOD_MIN;
	static const float LOD_MAX;

	static void clearReportedErrors();
	static ModelPtr get( const BW::StringRef & resourceID,
		bool loadIfMissing = true );

	void	beginRead();
	void	endRead();

#if ENABLE_RELOAD_MODEL
protected:

	ReadWriteLock				reloadRWLock_;
public:
	virtual bool reload( bool doValidateCheck ) = 0;
#endif//ENABLE_RELOAD_MODEL

public:
	struct LoadedAnimationInfo
	{
		BW::string name_;
		DataSectionPtr pDS_;
		Moo::AnimationPtr pAnim_;
	};

	static bool loadAnimations(
		const BW::string & resourceID,
		Moo::StreamedDataCache ** pOutAnimCache = NULL,
		BW::vector< LoadedAnimationInfo >* pOutInfo = NULL);

#ifdef EDITOR_ENABLED
	static void setAnimLoadCallback(
		SmartPointer< AnimLoadCallback > pAnimLoadCallback );
	MetaData::MetaData& metaData();
#endif

public:

	virtual bool valid() const;
	virtual bool nodeless() const = 0;

	static const int blendCookie();
	static int getUnusedBlendCookie();
	static int incrementBlendCookie();

	// Used for caching a models per instance transform state
	typedef BW::vector<Matrix> TransformState;

	void soakDyes();

	virtual void load();
	virtual void dress( const Matrix& world,
		TransformState* pTransformState = NULL ) = 0;
	virtual void dressFromState( const TransformState& transformState ) = 0;

	virtual void draw( Moo::DrawContext& drawContext, bool checkBB ) = 0;

	virtual Moo::VisualPtr getVisual();

	virtual const BSPTree * decompose() const;

	virtual const BoundingBox & boundingBox() const = 0;
	virtual const BoundingBox & visibilityBox() const = 0;
	virtual bool hasNode( Moo::Node * pNode,
		MooNodeChain * pParentChain ) const;

	virtual bool addNode( Moo::Node * pNode );
	virtual void addNodeToLods( const BW::string & nodeName );

	const BW::string & resourceID() const;

	Model * parent();
	const Model * parent() const;

	float extent() const;

	int getAnimation( const BW::string & name ) const;
	void tickAnimation( int index, float dtime, float otime, float ntime );
	void playAnimation( int index, float time, float blendRatio, int flags );



	// Don't try to store the pointer this returns, unless you guarantee
	//  the model will be around.
	ModelAnimation * lookupLocalAnimation( int index );
	ModelAnimation * lookupLocalDefaultAnimation();

	const ModelAction * getAction( const BW::string & name ) const;

	ModelActionsIterator lookupLocalActionsBegin() const;
	ModelActionsIterator lookupLocalActionsEnd() const;

	typedef OrderedStringMap< Vector4 > PropCatalogue;
	static int getPropInCatalogue( const char * name );
	static const char * lookupPropName( int index );

	// use with care... lock all accesses to the catalogue
	static PropCatalogue & propCatalogueRaw();
	static SimpleMutex & propCatalogueLock();


	typedef BW::vector< DyeSelection > DyeSelections;

	ModelDye getDye(	const BW::string & matterName,
						const BW::string & tintName,
						Tint ** ppTint = NULL );

	virtual void soak( const ModelDye & dye );

	const Matter * lookupLocalMatter( int index ) const;

public:
	virtual int gatherMaterials(	const BW::string & materialIdentifier,
									BW::vector< Moo::Visual::PrimitiveGroup * > & uses,
									Moo::ConstComplexEffectMaterialPtr * ppOriginal = NULL );

	void generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup );
	float uvSpaceDensity();

	bool isAncestor( const Model* pModel ) const;

	uint32 sizeInBytes() const;

	virtual NodeTreeIterator nodeTreeBegin() const;
	virtual NodeTreeIterator nodeTreeEnd() const;


	virtual void onReloaderReloaded( Reloader* pReloader );
	virtual void onReloaderPreReload( Reloader* pReloader);

	virtual TransformState* newTransformStateCache() const = 0;
	
	void isInModelMap( bool newVal );

protected:

	virtual ~Model();
	virtual void destroy() const;

	void release();
	static ModelPtr load( const BW::StringRef & resourceID, DataSectionPtr pFile );
	Model( const BW::StringRef & resourceID, DataSectionPtr pFile );

	virtual int initMatter( Matter & m );
	virtual bool initTint( Tint & t, DataSectionPtr matSect );

	int  readDyes( DataSectionPtr pFile, bool multipleMaterials );
	bool readSourceDye( DataSectionPtr pSourceDye, DyeSelection & dsel );
	void postLoad( DataSectionPtr pFile );


	BW::string				resourceID_;
	ModelPtr				parent_;
	float					extent_;

	typedef BW::vector< ModelAnimationPtr >	Animations;
	Animations				animations_;

	typedef BW::map< BW::string, std::size_t >	AnimationsIndex;
	AnimationsIndex			animationsIndex_;

	ModelAnimationPtr		pDefaultAnimation_;

#ifdef EDITOR_ENABLED
	static SmartPointer< AnimLoadCallback >		s_pAnimLoadCallback_;
	MetaData::MetaData metaData_;
#endif


	typedef BW::vector< ModelActionPtr >	Actions;
	typedef BW::map< StringOrderProxy, int >	ActionsIndex;
	Actions					actions_;
	ActionsIndex			actionsIndex_;


	typedef BW::vector< Matter * >		Matters;
	Matters					matters_;

	static PropCatalogue	s_propCatalogue_;
	static SimpleMutex		s_propCatalogueLock_;

	static BW::vector< BW::string > s_warned_;

private:
	struct ModelStatics
	{
		ModelStatics()
			: s_blendCookie_( 0x08000000 )
		{
			REGISTER_SINGLETON( ModelStatics )
		}

		// incremented first thing in each SuperModel::draw call.
		int s_blendCookie_;
		static ModelStatics & instance();
	};
	
	static ModelStatics s_ModelStatics;

	bool					isInModelMap_;
};

#ifdef CODE_INLINE
#include "model.ipp"
#endif

BW_END_NAMESPACE

#endif // MODEL_HPP
