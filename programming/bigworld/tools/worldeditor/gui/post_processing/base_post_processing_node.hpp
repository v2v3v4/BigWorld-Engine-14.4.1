#ifndef BASE_POST_PROCESSING_NODE_HPP
#define BASE_POST_PROCESSING_NODE_HPP


#include "graph/node.hpp"

BW_BEGIN_NAMESPACE

// Forward declarations
class NodeCallback;
class EffectNode;
class PhaseNode;


class BasePostProcessingNode;
typedef SmartPointer< BasePostProcessingNode > BasePostProcessingNodePtr;


/**
 *	This abstract class implements the main interface for nodes of the post
 *	processing panel.
 */
class BasePostProcessingNode : public Graph::Node
{
public:
	BasePostProcessingNode( NodeCallback * callback );
	virtual ~BasePostProcessingNode();

	static bool hasChanged() { return s_changed_; }
	static void changed( bool changed ) { s_changed_ = changed; }

	static bool previewMode() { return s_previewMode_; }
	static void previewMode( bool mode ) { s_previewMode_ = mode; }

	static void resetChainPosCounter() { s_chainPosCounter_ = 0; }
	static int chainPosCounter() { return s_chainPosCounter_; }

	virtual const BW::string & name() const = 0;

	virtual EffectNode * effectNode() { return NULL; }
	virtual PhaseNode * phaseNode() { return NULL; }

	virtual PyObject * pyObject() const = 0;

	virtual int chainPos() const { return chainPos_; }
	virtual void chainPos( int pos ) { chainPos_ = pos; }

	virtual bool active() const;
	virtual void active( bool active );

	virtual NodeCallback * callback() const { return callback_; }

	virtual void edEdit( GeneralEditor * editor ) = 0;
	virtual bool edLoad( DataSectionPtr pDS ) = 0;
	virtual bool edSave( DataSectionPtr pDS ) = 0;

	bool dragging() const { return dragging_; }
	void dragging( bool dragging ) { dragging_ = dragging; }

protected:
	virtual void activeNoCallback( bool active );

private:
	NodeCallback * callback_;
	bool active_;
	bool dragging_;
	int chainPos_;

	static bool s_changed_;
	static bool s_previewMode_;
	static int s_chainPosCounter_;
};

BW_END_NAMESPACE

#endif // BASE_POST_PROCESSING_NODE_HPP
