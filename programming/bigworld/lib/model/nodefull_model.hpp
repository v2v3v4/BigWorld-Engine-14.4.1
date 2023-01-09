#ifdef _MSC_VER 
#pragma once
#endif

#ifndef NODEFULL_MODEL_HPP
#define NODEFULL_MODEL_HPP

#include "model.hpp"

#include "model_animation.hpp"


BW_BEGIN_NAMESPACE

class NodefullModelAnimation;


/**
 *	Class for models whose bulk is a visual with nodes
 */
class NodefullModel : public Model
{
	//friend class ModelAnimation;
	friend class DefaultAnimation;
	friend class NodefullModelAnimation;
public:
	NodefullModel( const BW::StringRef & resourceID, DataSectionPtr pFile );
	~NodefullModel();

	virtual void load();

	virtual bool valid() const;
	virtual bool nodeless() const { return false; }

	virtual void dress( const Matrix& world, 
		TransformState* pTransformState = NULL );
	virtual void dressFromState( const TransformState& transformState );
		
	virtual void draw( Moo::DrawContext& drawContext, bool checkBB );

	virtual const BSPTree * decompose() const;

	virtual const BoundingBox & boundingBox() const;
	virtual const BoundingBox & visibilityBox() const;
	virtual bool hasNode( Moo::Node * pNode,
		MooNodeChain * pParentChain ) const;

	virtual bool addNode( Moo::Node * pNode );
	virtual void addNodeToLods( const BW::string & nodeName );

	virtual NodeTreeIterator nodeTreeBegin() const;
	virtual NodeTreeIterator nodeTreeEnd() const;

	virtual int gatherMaterials( const BW::string & materialIdentifier, BW::vector< Moo::Visual::PrimitiveGroup * > & primGroups, Moo::ConstComplexEffectMaterialPtr * ppOriginal = NULL )
	{
		return bulk_->gatherMaterials( materialIdentifier, primGroups, ppOriginal );
	}

	Moo::VisualPtr getVisual() { return bulk_; }

	virtual void onReloaderReloaded( Reloader* pReloader );

	TransformState* newTransformStateCache() const;

#if ENABLE_RELOAD_MODEL
	virtual bool reload( bool doValidateCheck );
#endif // ENABLE_RELOAD_MODEL

private:

#if ENABLE_RELOAD_MODEL
	static bool validateFile( const BW::string& resourceID );
#endif // ENABLE_RELOAD_MODEL

	Matrix						world_;	// set when model is dressed (traversed)
	BoundingBox					visibilityBox_;
	SmartPointer<Moo::Visual>	bulk_;

	NodeTree					nodeTree_;

	Moo::StreamedDataCache	* pAnimCache_;

	virtual int initMatter( Matter & m );
	virtual bool initTint( Tint & t, DataSectionPtr matSect );
	void release();
	void pullInfoFromVisual();
	void rebuildNodeHierarchy();
};

BW_END_NAMESPACE


#endif // NODEFULL_MODEL_HPP
