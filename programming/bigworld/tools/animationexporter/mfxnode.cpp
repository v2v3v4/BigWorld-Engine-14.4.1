
#pragma warning (disable:4530)
#pragma warning (disable:4786)

#include <ostream>
#include <fstream>
#include <strstream>
#include "mfxexp.hpp"

#include "mfxnode.hpp"
#include "utility.hpp"
#include "cstdmf/binaryfile.hpp"
//#include "moo/moo_math_helper.hpp"
#include "math/blend_transform.hpp"

#ifndef CODE_INLINE
#include "mfxnode.ipp"
#endif

#include "cuetrack.hpp"
#include "expsets.hpp"
#include <Max.h>
#include <NoteTrck.h>

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

	// Extract any cues from the note track if we need to
	if( node && ExportSettings::instance().exportCueTrack() )
	{
		for( int i = 0; i < node->NumNoteTracks(); ++i )
		{
			DefNoteTrack* note = (DefNoteTrack*) node->GetNoteTrack( i );

			for( int j = 0; j < note->keys.Count(); ++j )
			{
#ifdef _UNICODE
				CueTrack::addCue( note->keys[j]->time, bw_wtoutf8( note->keys[j]->note.data() ).c_str() );
#else
				CueTrack::addCue( note->keys[j]->time, note->keys[j]->note );
#endif
				
			}
		}
	}
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

	for( uint i = 0; i < children_.size(); i++ )
	{
		nNodes += children_[ i ]->treeSize();
	}

	return nNodes;
}

void MFXNode::removeChild( int n )
{
	if( uint(n) < children_.size() )
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
void MFXNode::exportTree( DataSectionPtr pParentSection, MFXNode* idealParent )
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

		}
		parent = this;
	}

	for (uint i = 0; i < children_.size(); i++)
	{
		children_[i]->exportTree( pThisSection, parent );
	}
}

void MFXNode::exportAnimation( BinaryFile& animFile, const CompressionInfo& ci, MFXNode* idealParent )
{
	MFXNode* nextParent = idealParent;

	typedef std::pair<float, Vector3> ScaleKey;
	typedef std::pair<float, Vector3> PositionKey;
	typedef std::pair<float, Quaternion> RotationKey;

	BW::vector< int > boundTable;
	BW::vector< ScaleKey > scales;
	BW::vector< PositionKey > positions;
	BW::vector< RotationKey > rotations;

	if (include_)
	{
		if (trailingLeadingWhitespaces(this->getIdentifier()))
		{
			char s[1024];
			bw_snprintf( s, sizeof(s), "Node \"%s\" has an illegal name. Remove any spaces at the beginning and end of the name.", this->getIdentifier().c_str() );
#ifdef _UNICODE
			MessageBox( NULL, bw_utf8tow( s ).c_str(), L"MFExporter Error", MB_OK | MB_ICONWARNING );
#else
			MessageBox( NULL, s, "MFExporter Error", MB_OK | MB_ICONWARNING );
#endif
			
		}

		nextParent = this;
		int firstFrame = ExportSettings::instance().firstFrame();
		int lastFrame = ExportSettings::instance().lastFrame();
		bool normalise = !ExportSettings::instance().allowScale();

//		std::ofstream debugText( "d:\\debug.txt", std::ios::app );
//		debugText << this->getIdentifier() << '\n';

		float time = 0;
		for (int i = firstFrame; i <= lastFrame; i++)
		{
			Matrix3 theMatrix = this->getRelativeTransform( i * GetTicksPerFrame(), normalise, idealParent );
			theMatrix = rearrangeMatrix( theMatrix );
			Matrix m;
			m.setIdentity();
			m[0] = reinterpret_cast<Vector3&>( theMatrix.GetRow(0) );
			m[1] = reinterpret_cast<Vector3&>( theMatrix.GetRow(1) );
			m[2] = reinterpret_cast<Vector3&>( theMatrix.GetRow(2) );
			m[3] = reinterpret_cast<Vector3&>( theMatrix.GetRow(3) );
			BlendTransform bt( m );

			// Normalise the rotation quaternion
			bt.normaliseRotation();

/*			debugText << "\tRotation: " << bt.rotation() << " Scale: " << bt.scaling() << " Translation: " << bt.translation() << '\n';

			bt.output( m );

			debugText << "\tRow0: " << m[0] << '\n';
			debugText << "\tRow1: " << m[1] << '\n';
			debugText << "\tRow2: " << m[2] << '\n';
			debugText << "\tRow3: " << m[3] << '\n';*/

			boundTable.push_back( i - firstFrame + 1 );
			scales.push_back( ScaleKey( time, bt.scaling() ) );
			positions.push_back( PositionKey( time, bt.translation() * ExportSettings::instance().unitScale() ) );
			rotations.push_back( RotationKey( time, bt.rotation() ) );
			time = time + 1;
		}
//		debugText << '\n';


		if (ci.specifyAmounts_)
		{
			animFile << int(4);
			animFile << this->getIdentifier();
			animFile << ci.scaleCompressionError_;
			animFile << ci.positionCompressionError_;
			animFile << ci.rotationCompressionError_;
		}
		else
		{
			animFile << int(1);
			animFile << this->getIdentifier();
		}

		animFile.writeSequence( scales );
		animFile.writeSequence( positions );
		animFile.writeSequence( rotations );
		animFile.writeSequence( boundTable );
		animFile.writeSequence( boundTable );
		animFile.writeSequence( boundTable );
	}

	for (uint i = 0; i < children_.size(); i++)
	{
		children_[ i ]->exportAnimation( animFile, ci, nextParent );
	}
}

int MFXNode::nIncludedNodes()
{
	int included = include_ ? 1 : 0;

	for (uint i = 0; i < children_.size(); i++)
	{
		included += children_[i]->nIncludedNodes();
	}

	return included;
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


MFXNode::~MFXNode()
{
}

std::ostream& operator<<(std::ostream& o, const MFXNode& t)
{
	o << "MFXNode\n";
	return o;
}

/**
 *	Used to order a sequence of nodes based on the number of children each has.
 *
 *	@param	lhs	The Node to the left hand side of the operation.
 *	@param	rhs	The Node to the right hand side of the operation.
 *	@return	True is lhs has more children than rhs, false otherwise.
 */
bool numChildDescending( MFXNode* lhs, MFXNode* rhs )
{
	if (lhs->getNChildren() > rhs->getNChildren())
		return true;
	else
		return false;
}

BW_END_NAMESPACE

/*mfxnode.cpp*/
