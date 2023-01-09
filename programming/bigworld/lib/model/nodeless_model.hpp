#ifdef _MSC_VER 
#pragma once
#endif

#ifndef NODELESS_MODEL_HPP
#define NODELESS_MODEL_HPP


#include "switched_model.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Class for models whose bulk is a visual without nodes
 */
class NodelessModel : public SwitchedModel< Moo::VisualPtr >
{
public:
	NodelessModel( const BW::StringRef & resourceID, DataSectionPtr pFile );

	virtual void load();

	virtual bool nodeless() const { return true; }

	virtual void draw( Moo::DrawContext& drawContext, bool checkBB );
	virtual void dress( const Matrix& world,
		TransformState* pTransformState = NULL );
	virtual void dressFromState( const TransformState& transformState );

	virtual const BSPTree * decompose() const;

	virtual const BoundingBox & boundingBox() const;
	virtual const BoundingBox & visibilityBox() const;

	Moo::VisualPtr getVisual();

	virtual void onReloaderReloaded( Reloader* pReloader );

	TransformState* newTransformStateCache() const;

#if ENABLE_RELOAD_MODEL
	virtual bool reload( bool doValidateCheck );
#endif // ENABLE_RELOAD_MODEL



private:
#if ENABLE_RELOAD_MODEL
	static bool validateFile( const BW::string& resourceID );
#endif // ENABLE_RELOAD_MODEL

	virtual int gatherMaterials(	
							const BW::string & materialIdentifier,
							BW::vector< Moo::Visual::PrimitiveGroup * > & primGroups,
							Moo::ConstComplexEffectMaterialPtr * ppOriginal = NULL );

	static Moo::VisualPtr loadVisual(	Model & m,
										const BW::string & resourceID );



	virtual int initMatter( Matter & m );
	virtual bool initTint( Tint & t, DataSectionPtr matSect );

	static void pullInfoFromVisual( Moo::VisualPtr pVisual );

	static Moo::NodePtr s_sceneRootNode_;

};


inline Moo::VisualPtr NodelessModel::getVisual()
{
	return bulk_;
}


BW_END_NAMESPACE


#endif // NODELESS_MODEL_HPP
