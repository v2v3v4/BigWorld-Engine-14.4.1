
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE

// INLINE void MFXNode::inlineFunction()
// {
// }


INLINE 
void MFXNode::setMaxNode( INode *node )
{
	node_ = node;
}

INLINE
INode* MFXNode::getMaxNode( void ) const
{
	return node_;
}

INLINE
void MFXNode::setTransform( const Matrix3 &m )
{
	transform_ = m;
}

INLINE
const Matrix3 &MFXNode::getTransform( void ) const
{
	return transform_;
}

INLINE
int MFXNode::getNChildren( void ) const
{
	return static_cast<int>(children_.size());
}

INLINE
MFXNode* MFXNode::getChild( int n ) const
{
	if( uint(n) < children_.size() )
		return children_[ n ];

	return NULL;
}

INLINE
MFXNode* MFXNode::getParent( void ) const
{
	return parent_;
}

INLINE
void MFXNode::addChild( MFXNode *node )
{
	if( node )
	{
		children_.push_back( node );
		node->parent_ = this;
	}

}

INLINE
const BW::string &MFXNode::getIdentifier( void )
{
	if( node_ )
	{
#ifdef UNICODE
	 bw_wtoutf8(node_->GetName(), identifier_);	
#else
	identifier_ = node_->GetName();
#endif
	}
	return identifier_;
}

INLINE
void MFXNode::setIdentifier( const BW::string &s )
{
	identifier_ = s;
}

INLINE
MFXNode *MFXNode::find( INode *node )
{
	if( node == node_ )
		return this;
	uint32 i = 0;
	MFXNode *ret = NULL;
	while( children_.size() > i && ret == NULL )
	{
		ret = children_[ i++ ]->find( node );
	}
	return ret;
}

INLINE
MFXNode *MFXNode::find( const BW::string &identifier )
{
	if( identifier == getIdentifier() )
		return this;
	uint32 i = 0;
	MFXNode *ret = NULL;
	while( children_.size() > i && ret == NULL )
	{
		ret = children_[ i++ ]->find( identifier );
	}
	return ret;

}

INLINE
bool MFXNode::contentFlag( ) const
{
	return contentFlag_;
}

INLINE
void MFXNode::contentFlag( bool state )
{
	contentFlag_ = state;
}

BW_END_NAMESPACE

/*mfxnode.ipp*/
