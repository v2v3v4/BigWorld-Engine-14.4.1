#ifndef SUPER_MODEL_HPP
#define SUPER_MODEL_HPP


#include <memory>
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/bw_noncopyable.hpp"
#include "cstdmf/bw_string.hpp"
#include "moo/forward_declarations.hpp"
#include "moo/moo_math.hpp"
#include "physics2/worldtri.hpp"
#include "resmgr/forward_declarations.hpp"

#include "forward_declarations.hpp"
#include "model.hpp"
#include "super_model_node_tree.hpp"
#include "model/fashion.hpp"
#include "moo/reload.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
class ModelTextureUsageGroup;
class Visual;
class DrawContext;
}

class Action;
class Animation;
class BoundingBox;
class BSPTree;
class LodSettings;
class StaticLightValues;

typedef BW::vector< Moo::NodePtr > MooNodeChain;

int initMatter_NewVisual( Matter & m, Moo::Visual & bulk );
bool initTint_NewVisual( Tint & t, DataSectionPtr matSect );



/**
 *	This class defines a super model. It's not just a model,
 *	it's a whole lot more - discrete level of detail, billboards,
 *	continuous level of detail, static mesh animations, multi-part
 *	animated models - you name it, this baby's got it.
 */
class SuperModel final:
/*
*	To support model reloading, if class A is pulling info out from a Model
*	through a SuperModel, A needs call
*	pSuperModel_->registerModelListener( pModel ),
*	and A::onReloaderReloaded(..) needs be implemented, which re-pulls info
*	out from pModel. A::onReloaderPreReload() might need be implemented
*	which detaches info from the old pModel before it's reloaded.
*	Also when A is initialising from Model, you need call
*	pSuperModel_->beginRead()
*	and pSuperModel_->endRead() to avoid concurrency issues.
*	Refer to SuperModelAction for an example.
*/
	public ReloadListener //detects model reload for regenerating node tree.
	, private BW::NonCopyable
{
public:
	explicit SuperModel( const BW::vector< BW::string > & modelIDs );
	virtual ~SuperModel();

	bool isLodVisible() const;
#if ENABLE_TRANSFORM_VALIDATION
	bool isTransformFrameDirty( uint32 frameTimestamp ) const;
	bool isVisibleTransformFrameDirty( uint32 frameTimestamp ) const;
#endif // ENABLE_TRANSFORM_VALIDATION

	bool needsUpdate() const;
	virtual void updateAnimations( const Matrix& world,
		const TmpTransforms * pPreFashions,
		const TmpTransforms * pPostFashions,
		float atLod );

	virtual void updateLodsOnly( const Matrix& world, float atLod );

	//void* draw( const Matrix& world,
	//	const MaterialFashionVector * pMaterialFashions ) {}

	void draw( Moo::DrawContext& drawContext, const Matrix& world,
		const MaterialFashionVector * pMaterialFashions );

	void dressInDefault();

	bool nodeless() const;
	Moo::NodePtr findNode( const BW::string & nodeName,
		MooNodeChain * pParentChain = NULL, bool addToLods = false );

	SuperModelAnimationPtr	getAnimation( const BW::string & name );
	SuperModelActionPtr		getAction( const BW::string & name );
	void getMatchableActions( BW::vector<SuperModelActionPtr> & actions );
	SuperModelDyePtr		getDye( const BW::string & matter, const BW::string & tint );

	int nModels() const;
	const Model * curModel( int i ) const;
	Model * curModel( int i );
	const ModelPtr topModel( int i ) const;
	ModelPtr topModel( int i );

	float lastLod() const;

	void localBoundingBox( BoundingBox & bbRet ) const;
	void localVisibilityBox( BoundingBox & vbRet ) const;

	void checkBB( bool checkit = true );

	bool isOK() const;

	int numTris() const;
	int numPrims() const;

	void	beginRead();
	void	endRead();
	void	deregisterModelListener( ReloadListener* pListener );
	void	registerModelListener( ReloadListener* pListener );
	bool	hasModel( const Model* pModel, bool checkParent = false ) const;
	bool	hasVisual( const Moo::Visual* pVisual, bool checkParent = false ) const;
	
	void	cacheDefaultTransforms( const Matrix& world );
	
	const SuperModelNodeTree * getInstanceNodeTree() const;
	bool regenerateInstanceNodeTree();
	uint32 getNumInstanceNodeTreeRegens() const;

	void generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup );

	const Matrix & getNodeWorldTransform(
		int instanceNodeTreeIndex ) const;
	void setNodeWorldTransform(
		int instanceNodeTreeIndex, const Matrix & transform );
	const Matrix & getNodeRelativeTransform(
		int instanceNodeTreeIndex ) const;
	void setNodeRelativeTransform(
		int instanceNodeTreeIndex, const Matrix & transform );

	virtual void onReloaderReloaded( Reloader * pReloader );

	float getLod() const;
	float getLodNextDown() const;
	float getLodNextUp() const;

	struct ReadGuard
	{
		ReadGuard( SuperModel* super ): super( super ) { super->beginRead(); }
		~ReadGuard() { super->endRead(); }
	private:
		SuperModel* super;
	};

private:

	struct LodLevelData
	{
		LodLevelData() :
			pModel( NULL ), transformState( NULL )
		{}
		Model* pModel;
		Model::TransformState* transformState;
	};

	class ModelPartChain
	{
	public:
		ModelPartChain( const ModelPtr& pModel );
		void pullInfoFromModel();

		const Model* curModel() const;
		Model* curModel();
		const Model::TransformState* curTransformState() const;
		Model::TransformState* curTransformState();
		
		bool isLodVisible() const;
#if ENABLE_TRANSFORM_VALIDATION
		bool transformDirty( uint32 frameTimestamp ) const;
		void markTransform( uint32 frameTimestamp );
#endif // ENABLE_TRANSFORM_VALIDATION

		void fini();

		ModelPtr	  topModel_;
		BW::vector<LodLevelData> lodChain_;
		BW::vector<LodLevelData>::size_type lodChainIndex_;

#if ENABLE_TRANSFORM_VALIDATION
		uint32 frameTimestamp_;
#endif // ENABLE_TRANSFORM_VALIDATION
	};

	void updateLods( float atLod,
		LodSettings& lodSettings,
		const Matrix & mooWorld );
	void updateTransformFashions( const Matrix& world,
		const TmpTransforms * pPreFashions,
		const TmpTransforms * pPostFashions );

#if ENABLE_TRANSFORM_VALIDATION
	bool isAnyDirtyTransforms( uint32 frameTimestamp ) const;
	bool isAnyVisibleDirtyTransforms( uint32 frameTimestamp ) const;
#endif // ENABLE_TRANSFORM_VALIDATION

	void applyMaterialsAndTransforms(
		const MaterialFashionVector * pMaterialFashions );
	void removeMaterials( const MaterialFashionVector * pMaterialFashions );
	void drawModels( Moo::DrawContext& drawContext, const Matrix& world );

	bool isTransformSane( const Matrix& mooWorld ) const;

	typedef BW::vector< ModelPartChain > Models;
	Models						models_;

	Models::size_type			nModels_;
	bool						nodeless_;

	float						lod_;
	float						lodNextUp_;
	float						lodNextDown_;

	bool	checkBB_;
	bool	isOK_;

	///This flag indicates that instanceNodeTree_ is dirty and needs to be
	///regenerated on the next access.
	bool instanceNodeTreeDirty_;
	///Stores the node graph for this SuperModel, or NULL if the combined node
	///graph of the constituent visuals doesn't meet the SuperModelNodeTree
	///requirements.
	std::auto_ptr<SuperModelNodeTree> instanceNodeTree_;
	///Used to store the number of times regenerateInstanceNodeTree invoked.
	uint32 numInstanceNodeTreeRegens_;
};


#ifdef CODE_INLINE
	#include "super_model.ipp"
#endif

BW_END_NAMESPACE


#endif // SUPER_MODEL_HPP
