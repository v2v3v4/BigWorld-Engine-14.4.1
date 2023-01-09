/**
 *	@file
 */

#ifndef BSP_HPP
#define BSP_HPP

// This file implements a BSP tree. It does not own (and thus delete)
// the triangles it contains.

#include "math/planeeq.hpp"
#include "math/boundbox.hpp"
#include "cstdmf/smartpointer.hpp"
#include "worldpoly.hpp"
#include "worldtri.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This interface is used by intersects.
 */
class CollisionVisitor
{
public:
	virtual ~CollisionVisitor() {};
	virtual bool visit( const WorldTriangle & hitTriangle, float dist ) = 0;
};


class BinaryBlock;
typedef SmartPointer<BinaryBlock> BinaryPtr;

typedef BW::vector< WorldTriangle::Flags > BSPFlagsMap;

/**
 *	This class is used to store a BSP tree. It is responsible for the triangles
 *	that are in its member nodes.
 */
class BSPNode;

class BSPTree
{
public:
	~BSPTree();

	bool intersects( const WorldTriangle & triangle,
		const WorldTriangle ** ppHitTriangle = NULL ) const;

	bool intersects( const Vector3 & start,
		const Vector3 & end,
		float & dist,
		const WorldTriangle ** ppHitTriangle = NULL,
		CollisionVisitor * pVisitor = NULL ) const;

	bool intersects( const WorldTriangle & triangle,
		const Vector3 & translation,
		CollisionVisitor * pVisitor = NULL ) const;

	void remapFlags( BSPFlagsMap& flagsMap );

	void				generateBoundingBox();
	const BoundingBox& boundingBox() const;

	bool			canCollide() const;
	
	WorldTriangle*  triangles() const;
	uint			numTriangles() const;
	bool			empty() const;

	uint			numNodes() const;

	size_t			usedMemory() const;

	void			validate() const;

	typedef uint32 UserDataKey;

	static const uint32	MD5_DIGEST = 0;
	static const uint32	TIME_STAMP = 1;
	static const uint32	USER_DEFINED = 1 << 16;

	BinaryPtr		getUserData(UserDataKey type);
	void			setUserData(UserDataKey type, const BinaryPtr& data);

	friend class BSPTreeTool;
private:
	BSPTree();

	// pointer to nodes memory
	BSPNode*		pNodes_;
	uint32			numNodes_;
	// points to the bsp root node
	BSPNode*		pRoot_;
	// array of triangles
	WorldTriangle*	pTriangles_;
	uint32			numTriangles_;
	// array of pointers to triangles which are shared between different nodes
	WorldTriangle**	 pSharedTrianglePtrs_;
	uint32			numSharedTrianglePtrs_;
	// memory block in which we keep bsp nodes,
	// triangles and shared pointers to triangles
	char*			pMemory_;
	// pre-generated bounding box
	BoundingBox		boundingBox_;
	// user data, usually NULL
	typedef BW::map<UserDataKey, BinaryPtr> UserDataMap;
	UserDataMap*	pUserData_;
};

/**
 *	This helper class is used to build, load and save a BSP tree.
 */

class BSPTreeTool
{
public:
	static uint8		getCurrentBSPVersion();
	static uint8		getBSPVersion( const BinaryPtr& bp );

	static BSPTree*		buildBSP( RealWTriangleSet & triangles );
	static BSPTree*		loadBSP( const BinaryPtr& bp );
	static BinaryPtr	saveBSPInMemory( BSPTree* pBSP );
	static bool			saveBSPInFile( BSPTree* pBSP, const char* fileName );
private:
	class BSPConstructor;
	static BSPTree*		loadBSPLegacy( const BinaryPtr& bp );
	static BSPTree*		loadBSPCurrent( const BinaryPtr& bp );
	static void			loadUserData( BSPTree* pBSP, char* pData, uint dataLen );
};

#ifdef CODE_INLINE
    #include "bsp.ipp"
#endif

BW_END_NAMESPACE

#endif
