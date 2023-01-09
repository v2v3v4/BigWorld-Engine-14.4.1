#ifndef CHUNK_MODEL_HPP
#define CHUNK_MODEL_HPP

#include "chunk_item.hpp"

#include "chunk/chunk_bsp_holder.hpp"

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/guard.hpp"

#include "model/super_model.hpp"
#include "moo/moo_math.hpp"
#include "moo/animation.hpp"
#include "moo/reload.hpp"

#include "model/fashion.hpp"

#include "fmodsound/sound_occluder.hpp"

#include "umbra_config.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	class ModelTextureUsageGroup;
}

class SuperModel;

typedef SmartPointer<class SuperModelAnimation> SuperModelAnimationPtr;

/**
 *	This class defines a chunk item inhabitant that
 *	displays a super model ( a .model file ).
 *
 *	Additionally, you can specify a single animation
 *	that is to play, by specifying an &lt;animation&gt; tag
 *	and a &lt;name&gt; sub-tag.
 */
class ChunkModel : 
	public ChunkItem, 
	/*
	*	Note: ChunkModel is always listening if pSuperModel_ has 
	*	been reloaded, if you are pulling info out from the pSuperModel_
	*	then please update it again in onReloaderReloaded which is 
	*	called when pSuperModel_ is reloaded so that related data
	*	will need be update again.and you might want to do something
	*	in onReloaderPriorReloaded which happen right before the pSuperModel_ reloaded.
	*/
	public ReloadListener
#if ENABLE_BSP_MODEL_RENDERING
	, public ChunkBspHolder
#endif // ENABLE_BSP_MODEL_RENDERING
{
	DECLARE_CHUNK_ITEM( ChunkModel )
	DECLARE_CHUNK_ITEM_ALIAS( ChunkModel, shell )
public:
	static const BW::string & getSectionName();

	ChunkModel();
	~ChunkModel();

	virtual bool load( DataSectionPtr pSection, Chunk * pChunk = NULL,
		BW::string* pErrorString = NULL  );

	virtual void toss( Chunk * pChunk );
	void tossWithExtra( Chunk * pChunk, SuperModel * extraModel );
	
	virtual CollisionSceneProviderPtr getCollisionSceneProvider( Chunk* ) const;
	virtual void updateAnimations();
	virtual void draw( Moo::DrawContext& drawContext );
	virtual void lend( Chunk * pLender );
	bool isModelNodeless() const { return pSuperModel_ ? pSuperModel_->nodeless() : false; }

	virtual const char * label() const;

	virtual bool reflectionVisible() { return reflectionVisible_; }
	virtual bool affectShadow() const;

	bool hasBSP() const;

	bool getReflectionVis() const { return reflectionVisible_; }
	bool setReflectionVis( const bool& reflVis ) { reflectionVisible_=reflVis; return true; }
	BoundingBox localBB() const
	{
		BoundingBox bb;
		if( pSuperModel_ )
			pSuperModel_->localBoundingBox( bb );
		return bb;
	}
	void generateTextureUsage( Moo::ModelTextureUsageGroup& usageGroup );

	virtual void onReloaderPreReload( Reloader* pReloader);
	virtual void onReloaderReloaded( Reloader* pReloader );

	void transform( const Matrix& newTransform );
	const Matrix& transform() const;
	const Matrix& worldTransform() const;

	SuperModel * getSuperModel() { return pSuperModel_; }

	virtual void syncInit();

protected:
	
#if ENABLE_BSP_MODEL_RENDERING
	virtual bool drawBspInternal();
#endif // ENABLE_BSP_MODEL_bsRENDERING

	SuperModel					* pSuperModel_;

	SuperModelAnimationPtr		pAnimation_;

	uint32						tickMark_;
	uint64						lastTickTimeInMS_;

	bool						castsShadow_;

	float						animRateMultiplier_;

	MaterialFashionVector		materialFashions_;

	Matrix						transform_;
	Matrix						worldTransform_;

	BW::string					label_;

	bool						reflectionVisible_;

	mutable bool				calculateIsShellModel_;
	mutable bool				cachedIsShellModel_;

	virtual void				syncShadowCaster();
	void						refreshWantsUpdate( bool wantsUpdate );

	void						updateWorldTransform( const Chunk* pChunk );


#if UMBRA_ENABLE
	void						syncUmbra();
	BW::string					umbraModelName_;
#endif

#if FMOD_SUPPORT
    SoundOccluder soundOccluder_;
#endif

	virtual bool addYBounds( BoundingBox& bb ) const;

	bool isShellModel() const;

	void tickAnimation();

private:
	ChunkModel( const ChunkModel & other );
	ChunkModel& operator=( const ChunkModel & other );

	bool pullInfoFromSuperModel( Chunk* pChunk, bool reload = false );
	Chunk* pChunkBeforeReload_;

};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "chunk_model.ipp"
#endif

#endif // CHUNK_MODEL_HPP
