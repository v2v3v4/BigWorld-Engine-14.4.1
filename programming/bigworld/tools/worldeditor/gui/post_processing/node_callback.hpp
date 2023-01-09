#ifndef NODE_CALLBACK_HPP
#define NODE_CALLBACK_HPP

BW_BEGIN_NAMESPACE

// Forward declarations
class BasePostProcessingNode;
typedef SmartPointer< BasePostProcessingNode > BasePostProcessingNodePtr;


/**
 *	This class works as an base interface class for other class that want to 
 *	receive notifications from an Effect node or a Phase node.
 */
class NodeCallback
{
public:
	virtual void nodeClicked( const BasePostProcessingNodePtr & node ) {}
	virtual void nodeActive( const BasePostProcessingNodePtr & node, bool active ) {}
	virtual void nodeDeleted( const BasePostProcessingNodePtr & node ) {}
	virtual void doDrag( const CPoint & pt, const BasePostProcessingNodePtr & node ) {}
	virtual void endDrag( const CPoint & pt, const BasePostProcessingNodePtr & node, bool canceled = false ) {}
};

BW_END_NAMESPACE

#endif // NODE_CALLBACK_HPP
