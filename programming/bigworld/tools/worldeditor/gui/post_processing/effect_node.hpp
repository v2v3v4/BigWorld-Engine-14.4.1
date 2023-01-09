#ifndef EFFECT_NODE_HPP
#define EFFECT_NODE_HPP


#include "base_post_processing_node.hpp"

BW_BEGIN_NAMESPACE

// Forward declarations
namespace PostProcessing
{
	class Effect;
	typedef SmartPointer<Effect> EffectPtr;
}

class EffectNode;
typedef SmartPointer< EffectNode > EffectNodePtr;


/**
 *	This class implements an Effect node, implementing the necessary additional
 *	editor info for a Post-processing Effect.
 */
class EffectNode : public BasePostProcessingNode
{
public:
	EffectNode( PostProcessing::Effect * pyEffect, NodeCallback * callback );
	~EffectNode();

	bool active() const;
	void active( bool active );

	const BW::string & name() const { return name_; }

	virtual EffectNode * effectNode() { return this; }

	PostProcessing::EffectPtr pyEffect() const { return pyEffect_; }

	PyObject * pyObject() const { return (PyObject *)(pyEffect_.get()); }

	EffectNodePtr clone() const;

	void edEdit( GeneralEditor * editor );
	bool edLoad( DataSectionPtr pDS );
	bool edSave( DataSectionPtr pDS );

	BW::string getName() const;
	bool setName( const BW::string & name );

	Vector4 getBypass() const;
	bool setBypass( const Vector4 & bypass );

private:
	PostProcessing::EffectPtr pyEffect_;
	BW::string name_;
};

BW_END_NAMESPACE

#endif // EFFECT_NODE_HPP
