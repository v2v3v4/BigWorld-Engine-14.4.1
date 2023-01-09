#include "pch.hpp"
#include "worldeditor/world/items/editor_chunk_marker_cluster.hpp"
#include "worldeditor/world/items/editor_chunk_substance.ipp"
#include "worldeditor/world/items/editor_chunk_item_tree_node.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/editor/item_editor.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "appmgr/options.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_marker.hpp"
#include "chunk/chunk_marker_cluster.hpp"
#include "chunk/geometry_mapping.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/resource_cache.hpp"
#include "moo/geometrics.hpp"
#include "model/super_model.hpp"
#include "gizmo/undoredo.hpp"


DECLARE_DEBUG_COMPONENT2( "EditorChunk", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EditorChunkMarkerClusterOperation
// -----------------------------------------------------------------------------


class EditorChunkMarkerClusterOperation : public UndoRedo::Operation
{
public:
	EditorChunkMarkerClusterOperation( EditorChunkMarkerClusterPtr n, bool removed )
		: UndoRedo::Operation( size_t(typeid(EditorChunkMarkerClusterOperation).name()) )
		, node_( n )
		, removed_( removed )
	{
		BW_GUARD;

		// store the markers the cluster references
		 node_->getCopyOfChildren(children_);
		 parent_ = node_->getParent();

		 availableMarkers_ = node_->getAvailableMarkers();

		 addChunk( node_->chunk() );
	}

	virtual void undo()
	{
		BW_GUARD;

		// First add a redo of this undo operation
		UndoRedo::instance().add( new EditorChunkMarkerClusterOperation(node_, !removed_, children_, parent_, availableMarkers_) );

		if (removed_)
		{
			// add links to clusters
			for (BW::list<ChunkItemTreeNodePtr>::iterator it = children_.begin();
				it != children_.end();
				it++)
			{
				(*it)->setParent(node_);
			}

			// tell parent about us
			node_->setParent(parent_);
			node_->setAvailableMarkers( availableMarkers_ );
		}
		else
		{
			// removed links to clusters
			for (BW::list<ChunkItemTreeNodePtr>::iterator it = children_.begin();
				it != children_.end();
				it++)
			{
				MF_ASSERT((*it)->getParent());
				(*it)->setParent(NULL);
			}

			// tell parent about us
			node_->setParent(NULL);
			node_->setAvailableMarkers( 0 );
		}
	}

	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		BW_GUARD;

		const EditorChunkMarkerClusterOperation& o = static_cast<const EditorChunkMarkerClusterOperation&>( oth );

		if (node_ != o.node_)
			return false;

		if (removed_ != o.removed_)
			return false;

		if (parent_ != o.parent_) 
			return false;

		if (availableMarkers_ != o.availableMarkers_)
			return false;

		for (BW::list<ChunkItemTreeNodePtr>::const_iterator it1 = children_.begin();
            it1 != children_.end();
			it1++)
		{
			bool found = false;
			for (BW::list<ChunkItemTreeNodePtr>::const_iterator it2 = o.children_.begin();
				it2 != o.children_.end();
				it2++)
			{
				if (*it1 == *it2)
					found = true;
			}	
			if (!found)
				return false;
		}

		return true;
	}

private:
	EditorChunkMarkerClusterOperation( EditorChunkMarkerClusterPtr n, bool removed, 
		const BW::list<ChunkItemTreeNodePtr>& c, ChunkItemTreeNodePtr p, int availMarkers )
		: UndoRedo::Operation( size_t(typeid(EditorChunkMarkerClusterOperation).name()) )
		, removed_( removed )
		, node_( n )
		, parent_( p )
		, availableMarkers_( availMarkers )
	{
		BW_GUARD;

		children_.clear();
		for (BW::list<ChunkItemTreeNodePtr>::const_iterator it = c.begin();
            it != c.end();
			it++)
		{
			children_.push_back(*it);
		}
	}

	EditorChunkMarkerClusterPtr node_;
	bool removed_;
	BW::list<ChunkItemTreeNodePtr> children_;
	ChunkItemTreeNodePtr parent_;
	int availableMarkers_;
};



// -----------------------------------------------------------------------------
// Section: EditorChunkMarkerCluster
// -----------------------------------------------------------------------------


/**
 *	Constructor.
 */
EditorChunkMarkerCluster::EditorChunkMarkerCluster()
{
	BW_GUARD;

	transform_.setIdentity();
	this->wantFlags_ = WantFlags( this->wantFlags_ | WANTS_DRAW );

	model_ = Model::get( "helpers/markers/marker_cluster.model" );
	ResourceCache::instance().addResource( model_ );
}

/**
 *	Destructor.
 */
EditorChunkMarkerCluster::~EditorChunkMarkerCluster()
{
}


void EditorChunkMarkerCluster::toss( Chunk * pChunk )
{
	BW_GUARD;

	EditorChunkSubstance<ChunkMarkerCluster>::toss( pChunk );

	MF_ASSERT(id_ != UniqueID::zero())
}


/**
 *	Draw: draw the visual representation.
 *	also draw the lines out to each marker
 */
void EditorChunkMarkerCluster::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if (!edShouldDraw())
		return;

	if (WorldManager::instance().drawSelection())
	{
		WorldManager::instance().registerDrawSelectionItem( this );
	}

	ModelPtr model = this->reprModel();
	if (model)
	{
		Moo::rc().push();

		Matrix transform = this->edTransform();
		Moo::rc().preMultiply( transform );

		model->dress( Moo::rc().world() );	// should really be using a supermodel...
		model->draw( drawContext, true );

		Moo::rc().pop();
	}

	if( WorldManager::instance().drawSelection() )
		return;

	// draw a line back to the parent (if applicable)
	Moo::rc().push();
	Moo::rc().world( Matrix::identity );

	if (getParent())
	{
		const Moo::Colour lineColour = 0xff0000ff;

		Vector3 from = chunk()->transform().applyPoint( edTransform().applyToOrigin() )
			+ Vector3(0.f, 0.1f, 0.f);
		Vector3 to = getParent()->chunk()->transform().applyPoint( getParent()->edTransform().applyToOrigin() )
			+ Vector3(0.f, 0.1f, 0.f);

		Geometrics::drawLine( from, to, lineColour, false );	// false = z-buffer the lines

		// draw a little arrow to indicate which is parent
		Vector3 direction = to - from;
		const float distance = direction.length();
		direction.normalise();

		Vector3 up(0.f, 1.f, 0.f);
		if (direction.dotProduct(up) > 0.9f)
			up = Vector3(1.f, 0.f, 0.f);

		const Vector3 rightAngleVector = direction.crossProduct(up);

		float arrowDisp = 0.2f;
		const float crossOver = 5.f;
		if (distance < crossOver)
			arrowDisp = 0.5f;

		float arrowDistanceAway = arrowDisp * distance;
		if (arrowDistanceAway > 5.f)
			arrowDistanceAway = 5.f;

		const float arrowWidth = 0.3f;
		const float arrowLength = 0.5f;
		Vector3 arrowHead = arrowDistanceAway * direction + from;
		Vector3 arrowBase1 = arrowHead - arrowLength * direction;
		Vector3 arrowBase2 = arrowBase1 - rightAngleVector * arrowWidth;
		arrowBase1 += rightAngleVector * arrowWidth;

		Geometrics::drawLine( arrowHead, arrowBase1, lineColour, false );
		Geometrics::drawLine( arrowBase1, arrowBase2, lineColour, false );
		Geometrics::drawLine( arrowBase2, arrowHead, lineColour, false );

		arrowHead = -arrowDistanceAway * direction + to;
		arrowBase1 = arrowHead - arrowLength * direction;
		arrowBase2 = arrowBase1 - rightAngleVector * arrowWidth;
		arrowBase1 += rightAngleVector * arrowWidth;

		Geometrics::drawLine( arrowHead, arrowBase1, lineColour, false );
		Geometrics::drawLine( arrowBase1, arrowBase2, lineColour, false );
		Geometrics::drawLine( arrowBase2, arrowHead, lineColour, false );
	}

	Moo::rc().pop();
}

/*
 *	Load: call the base load and then perform editor initialisations
 */
bool EditorChunkMarkerCluster::load( DataSectionPtr pSection, Chunk * pChunk, BW::string* errorString )
{
	BW_GUARD;

	bool ok = this->EditorChunkSubstance<ChunkMarkerCluster>::load( pSection, pChunk );
	if (ok)
	{
		transform_.translation( pSection->readVector3( "position" ) );
	}
	else if ( errorString )
	{
		*errorString = "Marker Cluster load failed";
	}

	return ok;
}


/**
 *	Save any property changes to this data section
 */
bool EditorChunkMarkerCluster::edSave( DataSectionPtr pSection )
{
	BW_GUARD;

	if (!edCommonSave( pSection ))
		return false;

	ChunkItemTreeNode::save( pSection );

	pSection->writeVector3( "position", transform_.applyToOrigin() );
	pSection->writeInt( "available_markers", availableMarkers_ );
	return true;
}


/**
 *	Get the current transform
 */
const Matrix & EditorChunkMarkerCluster::edTransform()
{
	return transform_;
}


/**
 *	Change our transform, temporarily or permanently
 */
bool EditorChunkMarkerCluster::edTransform( const Matrix & m, bool transient )
{
	BW_GUARD;

	// it's permanent, so find out where we belong now
	Chunk * pOldChunk = pChunk_;
	Chunk * pNewChunk = this->edDropChunk( m.applyToOrigin() );
	if (pNewChunk == NULL) return false;

	// if this is only a temporary change, keep it in the same chunk
	if (transient)
	{
		transform_ = m;
		return true;
	}

	// make sure the chunks aren't readonly
	if (!EditorChunkCache::instance( *pOldChunk ).edIsWriteable() 
		|| !EditorChunkCache::instance( *pNewChunk ).edIsWriteable())
		return false;

	// ok, accept the transform change then
	transform_.multiply( m, pOldChunk->transform() );
	transform_.postMultiply( pNewChunk->transformInverse() );

	// note that both affected chunks have seen changes
	WorldManager::instance().changedChunk( pOldChunk );
	if (pNewChunk != pOldChunk )
	{
		WorldManager::instance().changedChunk( pNewChunk );
	}

	edMove( pOldChunk, pNewChunk );

	return true;
}

/**
 *	Add the properties of this item to the given editor
 */
bool EditorChunkMarkerCluster::edEdit( class GeneralEditor & editor )
{
	BW_GUARD;

	if (this->edFrozen())
		return false;

	if (!allChildrenLoaded())
		return false;

	if (!edCommonEdit( editor ))
	{
		return false;
	}

	STATIC_LOCALISE_NAME( s_position, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MARK_CLUSTER/POSITION" );
	STATIC_LOCALISE_NAME( s_numberSpawned, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MARK_CLUSTER/NUMBER_SPAWNED" );
	STATIC_LOCALISE_NAME( s_numberChildren, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MARK_CLUSTER/NUMBER_CHILDREN" );

	MatrixProxy * pMP = new ChunkItemMatrix( this );
	editor.addProperty( new GenPositionProperty( s_position, pMP ) );
	editor.addProperty( new GenIntProperty( s_numberSpawned,
		new AccessorDataProxy<EditorChunkMarkerCluster, IntProxy>(
			this, "number spawned", 
			&EditorChunkMarkerCluster::getAvailableMarkers, 
			&EditorChunkMarkerCluster::setAvailableMarkers ) ) );

	editor.addProperty( new StaticTextProperty( s_numberChildren,
		new ConstantDataProxy<StringProxy>(
			numberChildrenAsString() ) ) );

	return true;
}


/**
 * Return false if any of the markers are not yet loaded or parent not fully loaded
 */
bool EditorChunkMarkerCluster::edCanDelete()
{
	BW_GUARD;

	if (allChildrenLoaded())
		if ((getParent() && getParent()->allChildrenLoaded()) || (!getParent() && (parentID_ == UniqueID::zero())))
			return EditorChunkSubstance<ChunkMarkerCluster>::edCanDelete();

	return false;
}

/**
 * Tell the markers they are no longer part of a cluster
 */
void EditorChunkMarkerCluster::edPreDelete()
{
	BW_GUARD;

	if (isNodeConnected())
	{
		UndoRedo::instance().add( new EditorChunkMarkerClusterOperation( this, true ) );
		removeThisNode();
	}
	EditorChunkItem::edPreDelete();
}

void EditorChunkMarkerCluster::edPostClone( EditorChunkItem* srcItem )
{
	BW_GUARD;

	// copy nothing, set as empty
	availableMarkers_ = 0;

	// extract from any existing links, assign new ID
	removeThisNode();
	setNewNode();

	EditorChunkItem::edPostClone( srcItem );
}

/**
 *	Return a modelptr that is the representation of this chunk item
 */
ModelPtr EditorChunkMarkerCluster::reprModel() const
{
	return model_;
}


/// Write the factory statics stuff
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk, &errorString)
IMPLEMENT_CHUNK_ITEM( EditorChunkMarkerCluster, marker_cluster, 1 )


bool EditorChunkMarkerCluster::setAvailableMarkers(const int& number)
{
	BW_GUARD;

	// check don't exceed the number of markers in the cluster
	if (number > numberChildren())
		availableMarkers_ = numberChildren();
	else
		availableMarkers_ = number; 

	return true;
}

BW::string EditorChunkMarkerCluster::numberChildrenAsString()
{
	BW_GUARD;

	BW::stringstream out;
	out << numberChildren();
	return out.str();
}


void EditorChunkMarkerCluster::setParent(ChunkItemTreeNodePtr parent)
{
	BW_GUARD;

	if (!edCanDelete())
		return;

	// Save the changes
	ChunkItemTreeNodePtr oldParent = getParent();
	ChunkItemTreeNode::setParent(parent);
	WorldManager::instance().changedChunk( chunk() );

	if (oldParent)
	{
		oldParent->edSave( oldParent->pOwnSect() );
		WorldManager::instance().changedChunk( oldParent->chunk() );
	}

	if (parent)
	{
		parent->edSave( parent->pOwnSect() );
		WorldManager::instance().changedChunk( parent->chunk() );
	}

	edSave( pOwnSect() );
}


void EditorChunkMarkerCluster::onRemoveChild()
{
	BW_GUARD;

	// check available is <= total
	if (availableMarkers_ > numberChildren())
	{
		INFO_MSG( "Resetting parent cluster available markers\n" );
		WorldManager::instance().addCommentaryMsg( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MARK_CLUSTER/RESETTING_PARENT") );

		availableMarkers_ = 0;
	}
}

BW_END_NAMESPACE

