#ifndef MOO_NODE_HPP
#define MOO_NODE_HPP

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"

#include "moo_math.hpp"

#ifdef _WIN32
#include "math/blend_transform.hpp"
#endif

#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

class Node;

typedef SmartPointer< Node > NodePtr;
typedef ConstSmartPointer< Node > ConstNodePtr;
typedef BW::vector< NodePtr > NodePtrVector;

/**
 *	This class maintains the transform at a node.
 *	It is used by visuals to make their skeletons of nodes.
 */
class Node : public SafeReferenceCount
{
public:
	static const char* SCENE_ROOT_NAME;

	Node();
	~Node();

#ifndef MF_SERVER
	void				traverse( );
	void				loadIntoCatalogue( );
#endif
	void				visitSelf( const Matrix& parent );

	void				addChild( NodePtr node );
	void				removeFromParent( );
	void				removeChild( NodePtr node );

	Matrix&				transform( );
	const Matrix&		transform( ) const;
	void				transform( const Matrix& m );

	const Matrix&		worldTransform( ) const;
	void				worldTransform( const Matrix& );

	NodePtr				parent( ) const;

	uint32				nChildren( ) const;
	NodePtr				child( uint32 i );
	ConstNodePtr		child( uint32 i ) const;

	const BW::string&	identifier( ) const;
	void				identifier( const BW::string& identifier );

	bool				isSceneRoot() const;
	
	NodePtr				find( const BW::string& identifier );
	uint32				countDescendants( ) const;

	void				loadRecursive( DataSectionPtr nodeSection );

	bool				needsReset( int blendCookie ) const;
	float				blend( int blendCookie ) const;
	void				blend( int blendCookie, float blendRatio );
	void				blendClobber( int blendCookie, const Matrix & transform );

#ifdef _WIN32
	BlendTransform &	blendTransform();
#endif

#ifdef _WIN32
	bool compareNodeData( const Node & other ) const;
#endif

private:

	Matrix				transform_;
	Matrix				worldTransform_;

	int					blendCookie_;
	float				blendRatio_;

#ifdef _WIN32
	BlendTransform		blendTransform_;
#endif
	bool				transformInBlended_;

	// parent is not a smart pointer, this is to ensure that the whole tree
	// will be destructed if the parent's reference count reaches 0
	Node*				parent_;
	NodePtrVector		children_;

	BW::string			identifier_;
	bool				isSceneRoot_;


	Node(const Node&);
	Node& operator=(const Node&);

public:
	static int			s_blendCookie_;
};

} // namespace Moo

#ifdef CODE_INLINE
#include "node.ipp"
#endif

BW_END_NAMESPACE


#endif // MOO_NODE_HPP
