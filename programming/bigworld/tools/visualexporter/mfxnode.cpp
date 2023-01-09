#include "mfxexp.hpp"

#include "mfxnode.hpp"
#include "utility.hpp"
#include "math/blend_transform.hpp"

#ifndef CODE_INLINE
#include "mfxnode.ipp"
#endif

BW_BEGIN_NAMESPACE

MFXNode::MFXNode()
: node_( NULL ),
  parent_( NULL ),
  contentFlag_( false )

{
	transform_.IdentityMatrix();
}

MFXNode::MFXNode( INode *node )
: node_( node ),
  parent_( NULL ),
  contentFlag_( false ),
  include_( false )
{
	transform_.IdentityMatrix();
}

Matrix3 MFXNode::getTransform( TimeValue t, bool normalise )
{
	// Is there a max node attached to this mfxnode, if there is, return its world transform
	if( node_ )
	{
		if( normalise )
			return normaliseMatrix( node_->GetNodeTM( t ) );
		else
			return node_->GetNodeTM( t );
	}

	// do we have a parent, if so return the parents worldtransform
	if( parent_ )
	{
		if( normalise )
			return  normaliseMatrix( parent_->getTransform( t ) * transform_ );
		else
			return  parent_->getTransform( t ) * transform_;
	}

	// return the stored transform
	if( normalise )
		return normaliseMatrix( transform_ );

	return transform_;
}

void MFXNode::includeAncestors()
{
	MFXNode* parent = parent_;
	while (parent)
	{
		parent->include(true);
		parent = parent->getParent();
	}
}

Matrix3 MFXNode::getRelativeTransform( TimeValue t, bool normalise,
	MFXNode * idealParent )
{
	// Does this mfx node have a max node?
	if( node_ )
	{
		// Do we want to override the parent?
		if (parent_ && !idealParent) idealParent = parent_;

		// If there is a parent get the relative transform, from the parent, else
		// return this nodes transform.
		if( idealParent )
		{
			return getTransform( t, normalise ) *
				Inverse( idealParent->getTransform( t, normalise ) );
		}
		else
		{
			return getTransform( t, normalise );
		}
	}

	if( normalise )
		return normaliseMatrix( transform_ );

	return transform_;
}

int MFXNode::treeSize( void )
{
	int nNodes = 1;

	for( int i = 0; i < children_.size(); i++ )
	{
		nNodes += children_[ i ]->treeSize();
	}

	return nNodes;	
}

void MFXNode::removeChild( int n )
{
	if( n < children_.size() )
	{
		removeChild( children_[ n ] );
	}
}

void MFXNode::removeChild( MFXNode *n )
{
	BW::vector< MFXNode * >::iterator it = std::find( children_.begin(), children_.end(), n ); ;

	if( it != children_.end() )
	{
		n->parent_ = NULL;
		children_.erase( it );
	}
}

/**
 * Exports this node and any children as xml sections.
 */
void MFXNode::exportTree( DataSectionPtr pParentSection, MFXNode* idealParent, bool* hasInvalidTransforms )
{
	DataSectionPtr pThisSection = pParentSection;
	MFXNode* parent = idealParent;
	if (this->include_)
	{
		pThisSection = pParentSection->newSection( "node" );
		if (pThisSection)
		{
			pThisSection->writeString( "identifier", this->getIdentifier() );

			Matrix3 m = rearrangeMatrix(getRelativeTransform( 0, !ExportSettings::instance().allowScale(), idealParent ));
			m.SetRow( 3, m.GetRow(3) * ExportSettings::instance().unitScale() );

			pThisSection->writeVector3( "transform/row0", reinterpret_cast<Vector3&>(m.GetRow(0)) );
			pThisSection->writeVector3( "transform/row1", reinterpret_cast<Vector3&>(m.GetRow(1)) );
			pThisSection->writeVector3( "transform/row2", reinterpret_cast<Vector3&>(m.GetRow(2)) );
			pThisSection->writeVector3( "transform/row3", reinterpret_cast<Vector3&>(m.GetRow(3)) );

			// Check if the node transformation is valid
			Matrix testMatrix = pThisSection->readMatrix34( "transform", Matrix::identity );
			BlendTransform BTTest = BlendTransform( testMatrix );
			Quaternion quat = BTTest.rotation();
			if ( !almostEqual( quat.length(), 1.0f ) )
				*hasInvalidTransforms = true;
		}
		parent = this;
	}

	for (int i = 0; i < children_.size(); i++)
	{
		children_[i]->exportTree( pThisSection, parent, hasInvalidTransforms );
	}
}

/*
 * Removes a child node and adds all it's children to this node.
 */
void MFXNode::removeChildAddChildren( MFXNode *n )
{
	BW::vector< MFXNode * >::iterator it = std::find( children_.begin(), children_.end(), n ); ;

	if( it != children_.end() )
	{
		n->parent_ = NULL;
		children_.erase( it );

		while( n->getNChildren() )
		{
			MFXNode *child = n->getChild( 0 );
			n->removeChild( child );
			addChild( child );
		}
	}
}

void MFXNode::removeChildAddChildren( int n )
{
//	MF_ASSERT( children_.size() > n );
	removeChildAddChildren( children_[ n ] );
}

void MFXNode::delChildren()
{
	children_.clear();
}


MFXNode::~MFXNode()
{
}

std::ostream& operator<<(std::ostream& o, const MFXNode& t)
{
	o << "MFXNode\n";
	return o;
}

BW_END_NAMESPACE

// mfxnode.cpp
