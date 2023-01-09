#pragma warning ( disable : 4530 )

#ifndef MFXNODE_HPP
#define MFXNODE_HPP

#include <iostream>
#include "cstdmf/bw_vector.hpp"
#include <string>

#include "resmgr/datasection.hpp"
#include "max.h"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/string_utils.hpp"

#include <algorithm>

BW_BEGIN_NAMESPACE

class MFXNode
{
public:
	MFXNode();
	MFXNode( INode *node );
	~MFXNode();

	void setMaxNode( INode *node );
	INode* getMaxNode( void ) const;

	void setTransform( const Matrix3 &m );
	const Matrix3 &getTransform( void ) const;

	int getNChildren( void ) const;
	MFXNode* getChild( int n ) const;
	MFXNode* getParent( void ) const;

	void addChild( MFXNode *node );

	void removeChild( int n );
	void removeChild( MFXNode *n );

	//removeChildAddChildren removes the child from this node, and adds all children
	//from the child to this node
	void removeChildAddChildren( MFXNode *n );
	void removeChildAddChildren( int n );

	
	Matrix3 getTransform( TimeValue t, bool normalise = false );
	Matrix3 getRelativeTransform( TimeValue t, bool normalise = false,
		MFXNode * idealParent = NULL );

	const BW::string &getIdentifier( void );
	void setIdentifier( const BW::string &s );

	int treeSize( void );

	MFXNode *find( INode *node );
	MFXNode *find( const BW::string &identifier );

	bool contentFlag( ) const;
	void contentFlag( bool state );

	bool include( ) const {return include_;};
	void include( bool state ) {include_ = state;};

	void includeAncestors();
	void delChildren();

	void exportTree( DataSectionPtr pParentSection, MFXNode* idealParent = NULL, bool* hasInvalidTransforms = NULL );

private:

	BW::string identifier_;
	MFXNode* parent_;
	BW::vector < MFXNode * > children_;

	INode *node_;
	Matrix3 transform_;

	bool contentFlag_;
	bool include_;

	MFXNode(const MFXNode&);
	MFXNode& operator=(const MFXNode&);

	friend std::ostream& operator<<(std::ostream&, const MFXNode&);
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "mfxnode.ipp"
#endif

#endif
/*mfxnode.hpp*/
