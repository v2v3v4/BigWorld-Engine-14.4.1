#ifndef SPEEDTREE_COLLISION_HPP
#define SPEEDTREE_COLLISION_HPP

// Module Interface
#include "speedtree_config.hpp"

// BW Tech Headers
#include "math/vector3.hpp"
#include "cstdmf/smartpointer.hpp"
#include <memory>
#include "cstdmf/bw_vector.hpp"

class CSpeedTreeRT;


BW_BEGIN_NAMESPACE

class BSPTree;
class BoundingBox;
typedef class BW::vector<class WorldTriangle> RealWTriangleSet;


namespace speedtree {

typedef std::auto_ptr< BSPTree > BSPTreePtr;

void computeTreeGeometry(
	CSpeedTreeRT & speedTree,
	const char   * filename, 
	int            seed);

void deleteTreeGeometry(
	CSpeedTreeRT & speedTree);

BSPTree * getBSPTree( 
	const char  * filename, 
	int           seed,
	BoundingBox & o_bbox );

BSPTreePtr createBSPTree( 
	CSpeedTreeRT * speedTree, 
	const char   * filename, 
	int            seed,
	BoundingBox  & o_bbox );

BW::string getBSPFileName( const char* filename );

BSPTreePtr loadBSP2File(
	CSpeedTreeRT* speedTree,
	const char* filename,
	const BW::string& bspName,
	int seed );

#if SPEEDTREE_SUPPORT
BSPTreePtr generateBSPFromFile(
	CSpeedTreeRT* speedTree, 
	const char* filename, 
	const BW::string& bspName,
	int seed,
	BoundingBox & o_bbox );
#endif

#if ( defined EDITOR_ENABLED ) && ( SPEEDTREE_SUPPORT )
BSPTreePtr generateBSPFromBB(
	CSpeedTreeRT* speedTree, 
	const char* filename, 
	const BW::string& bspName,
	int seed,
	BoundingBox & o_bbox );
#endif

void adjustBoundingBox(
	const BSPTreePtr& bspTree,
	BoundingBox& o_bbox );

void createBoxBSP(
	const float      * position, 
	const float      * dimension, 
	RealWTriangleSet & o_triangles);

// custom speedtree mat kinds
const int SPT_COLLMAT_SOLID  = 71;
const int SPT_COLLMAT_LEAVES = 72;

} // namespace speedtree

BW_END_NAMESPACE

#endif // SPEEDTREE_COLLISION_HPP
