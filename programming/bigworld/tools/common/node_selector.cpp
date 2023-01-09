//---------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop
//---------------------------------------------------------------------------
#include <algorithm>
#include "node_selector.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
NodeSelector::NodeSelector(Scene* scene )
: scene_( scene ),
  locked_( false )
{
}
//---------------------------------------------------------------------------
NodeSelector::~NodeSelector()
{
}
//---------------------------------------------------------------------------
SceneNode* NodeSelector::nodePresent(  float fdx, float fdy )
{
    //TODO : put back in
	//return scene_->nextPickable( );//renderContext_, fdx, fdy, true );
    return NULL;
}
//---------------------------------------------------------------------------
bool NodeSelector::picked(  float fdx, float fdy )
{
	//TODO : put back in
	bool result = false;

	/*SceneNodeVector::iterator it  = selectionList_.begin();
	SceneNodeVector::iterator end = selectionList_.end();

    while ( !result && it != end )
    {
		//if ( scene_->picked( renderContext_, fdx, fdy, *it ) )
        if ( (*it)->intersects(  fdx, fdy ) )
        	result = true;

        it++;
    }             */

    return result;
}
//---------------------------------------------------------------------------
void NodeSelector::highlight( float fdx, float fdy )
{
	//TODO : put back in
	/*highlightList_.clear( );

	SceneNode* node = scene_->nextPickable( );//renderContext_, fdx, fdy );

	if ( node )
		highlightList_.push_back( node );*/
}
//---------------------------------------------------------------------------
void NodeSelector::highlight( const TRect& rect )
{
	highlightList_.clear( );
}
//---------------------------------------------------------------------------
void NodeSelector::deselect( SceneNode* node )
{
	//node->treeDeselect( );
    SceneNodeVector::iterator it = std::find( selectionList_.begin(), selectionList_.end(), node );

    if ( it  != selectionList_.end() )
    	selectionList_.erase( it );
}
//---------------------------------------------------------------------------
void NodeSelector::deselect( void )
{
	if ( !locked_ )
    {
    	//for ( int i = 0; i < selectionList_.size(); i++ )
        //	selectionList_[i]->treeDeselect( );

		highlightList_.clear( );
        selectionList_.clear( );
    }
}
//---------------------------------------------------------------------------
void NodeSelector::select( SceneNode* node )
{
	if ( !locked_ )
    {
    	//for ( int i = 0; i < selectionList_.size(); i++ )
        //	selectionList_[i]->treeDeselect( );

		deselect( );

        //node->treeSelect( );
    	selectionList_.push_back( node );
    }
}
//---------------------------------------------------------------------------
void NodeSelector::select( float fdx, float fdy )
{
	if ( !locked_ )
    {
		deselect( );

		selectAdd( fdx, fdy );
    }
}
//---------------------------------------------------------------------------
bool NodeSelector::selected( SceneNode* node )
{
    if ( std::find( selectionList_.begin(), selectionList_.end(), node ) == selectionList_.end() )
    	return false;

    return true;
}
//---------------------------------------------------------------------------
bool NodeSelector::highlighted( SceneNode* node )
{
    if ( std::find( highlightList_.begin(), highlightList_.end(), node ) == highlightList_.end() )
    	return false;

    return true;
}
//---------------------------------------------------------------------------
void NodeSelector::selectAdd( SceneNode* node )
{
	if ( !locked_ )
    {
	    if ( node && !selected( node ) )
        {
        	//node->treeSelect( );

			selectionList_.push_back( node );
        }
    }
}
//---------------------------------------------------------------------------
void NodeSelector::selectAdd( float fdx, float fdy )
{
	//TODO : put back in
	/*if ( !locked_ )
    {
		//SceneNode* node = scene_->pick( renderContext_, fdx, fdy, true );
		SceneNode* node = scene_->nextPickable( );

		if ( node && !selected( node ) )
        {
        	//node->treeSelect( );

			selectionList_.push_back( node );
        }
    }*/
}
//---------------------------------------------------------------------------
void NodeSelector::selectAll( const Scene* parent )
{
	if ( !locked_ )
    {
    	deselect( );

    	for ( int i = 0; i < parent->sceneList( ).size( ); i++ )
        {
			selectAdd( parent->sceneList( )[i] );
        }
    }
}
//---------------------------------------------------------------------------
void NodeSelector::selectCycle( float fdx, float fdy )
{
	//TODO : put back in
	/*if ( !locked_ )
    {
		SceneNode* node = scene_->pick( );//renderContext_, fdx, fdy, false );

		if ( node && !selected( node ) )
        {
			selectionList_.push_back( node );
        }
    }*/
}
//---------------------------------------------------------------------------
SceneNodeVector& NodeSelector::selectionList( void )
{
	return selectionList_;
}
//---------------------------------------------------------------------------
SceneNodeVector& NodeSelector::highlightList( void )
{
	return highlightList_;
}
//---------------------------------------------------------------------------
float NodeSelector::getFloorHeight( void )
{
	SceneNodeVector::iterator it  = selectionList_.begin();
	SceneNodeVector::iterator end = selectionList_.end();

    float minFloor = 1000000.f;

    if ( selectionList_.size( ) )
    {
        while ( it != end )
        {
            float floor = (*it)->location()[1];

            minFloor = min( minFloor, floor );

            it++;
        }
    }
    else
    	minFloor = 0.0f;

    return minFloor;
}
//---------------------------------------------------------------------------
BoundingBox& NodeSelector::worldBoundingBox( void )
{
	BoundingBox bb;
    boundingBox_ = bb;

    if ( selectionList_.size( ) )
    {
        SceneNodeVector::iterator it  = selectionList_.begin();
        SceneNodeVector::iterator end = selectionList_.end();
        bool initialized = false;

        while ( it != end )
        {
        	(*it)->calculateWorldBoundingBox( bb );

			if ( initialized )
            	boundingBox_.addBounds( bb );
            else
            {
            	boundingBox_ = bb;
                initialized = true;
            }

            it++;
        }

        // clamp the world bounding box to 1m x 1m x 1m minimum
        Vector3 max  = boundingBox_.maxBounds();
        Vector3 size = boundingBox_.maxBounds() - boundingBox_.minBounds();

        if ( size[0] < 1 )
        	max[0] = boundingBox_.minBounds()[0] + 1;
        if ( size[1] < 1 )
        	max[1] = boundingBox_.minBounds()[1] + 1;
        if ( size[2] < 1 )
        	max[2] = boundingBox_.minBounds()[2] + 1;

        boundingBox_.setBounds( boundingBox_.minBounds(), max );
    }

    return boundingBox_;
}
//---------------------------------------------------------------------------
void NodeSelector::centerOfSelection( Vector3& centerOfSelection )
{
	if (selectionList_.size( ) == 0)
		return;

    centerOfSelection.setZero( );

    SceneNodeVector::iterator it  = selectionList_.begin( );
	SceneNodeVector::iterator end = selectionList_.end( );

    while ( it != end )
    {
        SceneNode *node = (*it);


        //accumulate location
        /*BoundingBox bb = node->boundingBox( );

        //transfrom bounding box to world coords
        bb.transformBy( node->worldTransform( ) );

        Vector3 centerOfNode = ( bb.minBounds() + bb.maxBounds() ) / 2.f;

        centerOfSelection += centerOfNode;*/

        centerOfSelection += node->worldTransform( ).applyToOrigin();

        it++;
    }

    // the height for center of selection is the height of the origin of the
    // last node in world space or what is in effect the current floor height
    //if ( selectionList_.size( ) )
	//    centerOfSelection[1] = (*(end-1))->worldTransform()[3][1];

	//size of vector is guaranteed not to be zero
	centerOfSelection /= selectionList_.size( );
}
//---------------------------------------------------------------------------
void NodeSelector::locked( bool state )
{
	locked_ = state;
}
//---------------------------------------------------------------------------
bool NodeSelector::isLocked( void )
{
	return locked_;
}
//---------------------------------------------------------------------------
NodeSelector& NodeSelector::operator=(const NodeSelector& other)
{
    this->scene_ = other.scene_;
    this->selectionList_.clear( );
    this->highlightList_.clear( );

    for ( int i = 0; i < other.selectionList_.size( ); i++ )
    {
		this->selectionList_.push_back( other.selectionList_[i] );
    }

    for ( int i = 0; i < other.highlightList_.size( ); i++ )
    {
		this->highlightList_.push_back( other.highlightList_[i] );
    }

    return *this;
}
//---------------------------------------------------------------------------
/*std::ostream& operator<<(std::ostream& o, const NodeSelector& t)
{
	o << "NodeSelector\n";
	return o;
}*/
//---------------------------------------------------------------------------
/*node_selector.cpp*/
