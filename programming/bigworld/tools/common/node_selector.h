#ifndef NODE_SELECTOR_HPP
#define NODE_SELECTOR_HPP

//#include <Classes.hpp>

//#include <iostream>
#include "scene2/scene_node.hpp"
#include "scene2/scene.hpp"

/**
 * NodeSelector class
 */
class NodeSelector
{
public:
	NodeSelector( Scene* scene );
	~NodeSelector();

    // allows changing of the scene
    void scene( Scene* scene ) { deselect( ); scene_ = scene; };
	// highlight selection
	void highlight( float fdx, float fdy );
	// highlight a rect
	void highlight( const TRect& rect );
    // deselect the highlighted node
    void deselectHighlight( void ) { highlightList_.clear( ); };
    // detect a node present at the point
	SceneNode* nodePresent(  float fdx, float fdy );
    // checks to see if a node is in the pick list
	bool picked(  float fdx, float fdy );
    // is the given node currently selected?
    bool selected( SceneNode* node );
    // is the given node currently highlighted?
	bool highlighted( SceneNode* node );
	// deselect everyhing
	void deselect   ( void );
    // deselect a specific node
    void deselect( SceneNode* node );
    // select the node
	void select( SceneNode* node );
	// select anything at this point
	void select   ( float fdx, float fdy );
    // select and add a single node to the select list, if it is not already there
    void selectAdd( SceneNode* node );
	// select and add to the select list, anything at this point
	void selectAdd( float fdx, float fdy );
	// selects all the nodes of a parent
	void selectAll( const Scene* parent );
    // cycle through the rendercontext selection list
	void selectCycle( float fdx, float fdy );
	// retreive the select list, into your own nodeList
	SceneNodeVector& selectionList( void );
	// retreive the highlight list.
	SceneNodeVector& highlightList( void );
    // retrieve the lowest bounding box (floor height) of the selection
    float getFloorHeight( void );
    // get the selections world bounding box
	BoundingBox& worldBoundingBox( void );
    // center of selection
	void centerOfSelection( Vector3& centerOfSelection );
    // number of selections
    int selections( void ) { return selectionList_.size(); };
    // lock selection
	void locked( bool state );
	bool isLocked( void );
    //void resetPick( void ) { scene_->resetLastPick( ); };

    NodeSelector& operator=(const NodeSelector&);

private:

	Scene*			scene_;			// scene we can select from
	SceneNodeVector	selectionList_;	// list of selected nodes
	SceneNodeVector	highlightList_;	// list of highlighted nodes
    BoundingBox		boundingBox_;	// bounding box of selection
    bool			locked_;		// flag: selection is locked

	NodeSelector(const NodeSelector&);
	//NodeSelector& operator=(const NodeSelector&);

	//friend std::ostream& operator<<(std::ostream&, const NodeSelector&);
};

#endif
/*node_selector.hpp*/
