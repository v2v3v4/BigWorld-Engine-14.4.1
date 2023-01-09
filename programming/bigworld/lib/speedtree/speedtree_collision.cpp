/***
 *

This file declares the functions to build and cache BSP-tree (collision) 
information from a CSpeedTreeRT object. 

The BSP-tree for a speedtree is built from its highest LOD branch geometry
and the collision volumes defined in SpeedTreeCAD. The branch geometry 
triagles are added to the BSP-tree with a material-kind different from the
one used by the collision volumes triangles. 

Before creating a BSP-tree from the speedtree geometry, this module looks
for a corresponding cached ".bsp2" file. This file is used if it exists 
and is up-to-date in relation to the ".spt" file (both timestamp and md5
signature are checked). Otherwise, a new BSP-tree object is created and 
cached for future reference. Setting SKIP_BSP_LOAD to 1 will skip loading
the cached ".bsp2" file and always create the BSP-tree from the geometry.

This file can be used in two contexts: with SPEEDTREE_SUPPORT enabled or
disabled. The above description applies to when SPEEDTREE_SUPPORT is 
enabled. If SPEEDTREE_SUPPORT is disabled, loading a tree BSP-tree from
a cached ".bsp2" still works. [Re]Creating it from a ".spt" file is not
possible, though, for obvious reason. Thus, altough in general the server 
is built with SPEEDTREE_SUPPORT disabled, it can still load the trees'
collision scene if the corresponding ".bps2" files are accessible.

Listed below are the core data structures and functions defined here. Please
refer to their inlined doxigen documentation for detailed information.

>> Functions
	>>	getBSPTree()
	>>	createBSPTree()
	>>	extractCollisionVolumes()
	>>	extractBranchesMesh()

 *
 ***/
#include "pch.hpp"

// Module Interface
#include "speedtree_collision.hpp"

// BW Tech Headers
#include "math/boundbox.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/md5.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"
#include "physics2/bsp.hpp"

// STD Headers
#include "cstdmf/bw_map.hpp"
#include <stdexcept>

// Set SKIP_BSP_LOAD to 1 to prevent a cached
// bsp file to be loaded even if it's up-to-date
#define SKIP_BSP_LOAD 0

// Set SOLID_SPT_VOLUMES to 0 if you wish the 
// player not to collide against the collision 
// geometry defined inside SpeedtreeCAD
// use solid leaves
#define SOLID_LEAVES 0

#if SPEEDTREE_SUPPORT
	#include <SpeedTreeRT.h>
#endif // SPEEDTREE_SUPPORT

DECLARE_DEBUG_COMPONENT2("SpeedTree", 0)


BW_BEGIN_NAMESPACE

namespace speedtree {

namespace { // anonymous

// Named Contants
// collision objects (sphere, capsule) resolution decreased from 8 to 6 for better performance.
const int   c_CollisionObjRes      = 6; 
const float c_CoservativeBoxFactor = 0.5f;
const float c_PlaneEpsilon         = 0.1f;

#if SPEEDTREE_SUPPORT

BSPTreePtr loadBSPIfValid(
	BinaryPtr      bspData,
	CSpeedTreeRT * speedTree,
	const char   * filename, 
	int            seed);

bool saveBSPWithMD5(
	BSPTree*          pBspTree,
	const BW::string & bspName,
	const MD5::Digest & md5Dig);

CSpeedTreeRT * acquireSpeedTree(
	CSpeedTreeRT * speedTree,
	const char   * filename);

void extractCollisionVolumes(
	CSpeedTreeRT     & speedTree,
	RealWTriangleSet & o_triangles,
	bool             & o_emptyBSP );

void extractBranchesMesh(
	CSpeedTreeRT     & speedTree,
	int                seed,
	RealWTriangleSet & o_triangles,
	bool             & o_emptyBSP );

void createBoxTriagles(
	const float      * position,
	const Matrix     & rotation,
	const float      * dimensions,
	RealWTriangleSet & o_triangles);

void createSphereCapTriagles(
	bool               upper,
	const float      * position,
	const Matrix     & rotation,
	const float      * dimensions,
	RealWTriangleSet & o_triangles,
	float              height = 0);

#endif // SPEEDTREE_SUPPORT

/**
 *	Helper class. Entry into the cache of BSPs.
 */
class CachedBSPTree : public SafeReferenceCount
{
public:
	CachedBSPTree(BSPTree * tree, const BoundingBox & bbox) :
		bspTree_(tree),
		boundingBox_(bbox)
	{}

	~CachedBSPTree()
	{
		bw_safe_delete(this->bspTree_);
	}

	BSPTree * bspTree()
	{
		return this->bspTree_;
	}

	const BoundingBox & boundingBox()
	{
		return this->boundingBox_;
	}

private:
	BSPTree *   bspTree_;
	BoundingBox boundingBox_;
};

typedef SmartPointer< CachedBSPTree > CachedBSPTreePtr;
typedef BW::map< BW::string, CachedBSPTreePtr > SpeedTreeMap;
SpeedTreeMap s_treesCache;

} // namespace anonymous

/**
 *	Retrieves BSPTree for the specified SPT file. Look for it in the local 
 *	memory cache before trying to load from file. 
 *
 *	@note	this function is used only by the server. Client and tools get 
 *			the BSP-tree using the SpeedTreeRenderer::bsp() function. 
 *
 *	@param	filename	the name of the spt file used to load the tree.
 *	@param	seed		the seed to be used when computing the tree.
 *	@param	o_bbox		will hold the tree's bounding box.
 *
 *	@return				Pointer to BSPTree object (don't delete it).
 */
BSPTree * getBSPTree(
	const char *  filename, 
	int           seed,
	BoundingBox & o_bbox)
{
	BW_GUARD;
	BW::string name  = filename;
	BSPTree * bspTree = 0;
	SpeedTreeMap::iterator treeIt = s_treesCache.find(name);
	if (treeIt != s_treesCache.end())
	{
		bspTree = treeIt->second->bspTree();
		o_bbox.addBounds(treeIt->second->boundingBox());
	}
	else
	{
		BoundingBox bbox = BoundingBox::s_insideOut_;
		bspTree = createBSPTree(0, filename, seed, bbox).release();
		o_bbox.addBounds(bbox);
		s_treesCache.insert(std::make_pair(name,
			CachedBSPTreePtr(new CachedBSPTree(bspTree, bbox))));
	}

	return bspTree;
}

/**
 *	Creates (or loads from file) a BSP-tree for the specified speedtree. 
 *	If a BSP-tree is compiled from the speedtree, this function will save
 *	the corresponding ".bsp2" file. No memory caching if performed.
 *
 *	@param	speedTree	the tree object.
 *	@param	filename	the name of the spt file used to load the tree.
 *	@param	seed		the seed to be used when computing the tree.
 *	@param	o_bbox		will hold the tree's bounding box.
 *
 *	@note				will throw std::runtime_error if tree can't be computed.
 */
BSPTreePtr createBSPTree(
	CSpeedTreeRT * speedTree,
	const char   * filename,
	int            seed,
	BoundingBox  & o_bbox)
{
	BW_GUARD;

	BSPTreePtr bspTree;

	// Get ".bsp2" filename
	BW::string bspName = getBSPFileName( filename );

#if !SKIP_BSP_LOAD
	// Search for cached ".bsp2" file and load it if valid
	bspTree = loadBSP2File( speedTree, filename, bspName, seed );
#endif

	// Could not load
	// But can create
#if SPEEDTREE_SUPPORT
	if ( bspTree.get() == NULL )
	{
		bspTree = generateBSPFromFile(
			speedTree, filename, bspName, seed, o_bbox );
	}

	// Could not load
	// And no speedtree support, so can't create one
	// Throw error
#else
	if ( bspTree.get() == NULL )
	{
		BW::string errorMsg = "Could not load bsp file for tree: ";
		errorMsg += filename;
		throw std::runtime_error( errorMsg.c_str() );
	}
#endif

#if ( defined EDITOR_ENABLED ) && ( SPEEDTREE_SUPPORT )
	// If tree BSP is actually empty, create a BSP based on the
	// bounding box, so that it can be selected in the editor
	if ( bspTree->numTriangles() == 0)
	{
		bspTree = generateBSPFromBB(
			speedTree, filename, bspName, seed, o_bbox );
	}
#endif

	// Adjust bounding box
	MF_ASSERT( bspTree.get() != 0 );
	adjustBoundingBox( bspTree, o_bbox );

	return bspTree;
}

BW::string getBSPFileName( const char* filename )
{
	BW::StringRef noExtension = BWResource::removeExtension( filename );
	BW::string bspName = noExtension + ".bsp2";
	return bspName;
}

/**
 *	Attempt to load BSP tree from cached file.
 *	
 *	@param	speedTree	the tree object.
 *	@param	filename	the name of the spt file used to load the tree.
 *	@param	bspName		the name of the bsp2 file to load.
 *	@param	seed		the seed to be used when computing the tree.
 *	
 *	@return NULL on failure.
 */
#if SPEEDTREE_SUPPORT
BSPTreePtr loadBSP2File(
	CSpeedTreeRT* speedTree,
	const char* filename,
	const BW::string& bspName,
	int seed )
{
	BW_GUARD;

	MF_ASSERT( filename != NULL );

	CSpeedTreeRT* speedTreeLocal = NULL;
	BSPTreePtr bspTree;
	BinaryPtr bspData;

	bool fileExists;
	bool fileAgeValid;

	// Check file exists
	fileExists = BWResource::fileExists( bspName );

	// Check file is correct age compared to speedtree file
	fileAgeValid = !BWResource::isFileOlder( bspName, filename );

	// File not valid
	if ( !fileExists )
	{
		// Get's generated from ".spt" file later
		//DEBUG_MSG( "loadBSP2File: "
		//	".bsp2 file for %s does not exist.\n", filename );
	}
	else if ( !fileAgeValid )
	{
		// Get's generated from ".spt" file later
		//DEBUG_MSG( "loadBSP2File: "
		//	".spt file %s has changed.\n", filename );
	}
	// Attepmt to load file
	else
	{
		// -- Compare ".spt" and ".bsp2" MD5 sums
		// Bsp file exists, even if it's older than the spt file we still need
		// to check MD5 sum in case the ".bsp2" doesn't match the ".spt".
		// loadBSPIfValid will load it, but only return
		// if the MD5 hash matches the one from the spt file.
		try
		{
			//DEBUG_MSG( "Attempting to load %s\n", bspName.c_str() );

			// Load BSP data from file
			DataSectionPtr bspSect = BWResource::openSection( bspName );
			if ( !bspSect.exists() )
			{
				BW::string errorMsg = "Could not load BSP section from file: ";
				errorMsg += bspName;
				throw errorMsg;
			}

			bspData = bspSect->asBinary();
			if ( !bspData.exists() )
			{
				BW::string errorMsg = "Could not load BSP data from file: ";
				errorMsg += bspName;
				throw errorMsg;
			}

			// Load speedtree from file
			speedTreeLocal = acquireSpeedTree( speedTree, filename );

			if ( speedTreeLocal == NULL )
			{
				BW::string errorMsg = "Could not load tree definition file: ";
				errorMsg += filename;
				throw errorMsg;
			}

			// Compare MD5 sums and load BSP tree from file
			bspTree = loadBSPIfValid( bspData, speedTreeLocal, filename, seed );

			if ( bspTree.get() == NULL )
			{
				BW::string errorMsg = "loadBSP2File: Could not load ";
				errorMsg += filename;
				errorMsg += ". File data is invalid.\n";
				ERROR_MSG( "%s", bspName.c_str() );
				throw errorMsg;
			}
		}

		// Load failed
		catch ( const BW::string & e )
		{
			ERROR_MSG( "%s\n", e.c_str() );
		}
	}

	// Not loaded?
	if ( speedTree == 0 )
	{
		bw_safe_delete(speedTreeLocal);
	}
	else
	{
		MF_ASSERT( speedTreeLocal == NULL || speedTree == speedTreeLocal );
	}

	return bspTree;
}
#else
BSPTreePtr loadBSP2File(
	CSpeedTreeRT* speedTree,
	const char* filename,
	const BW::string& bspName,
	int seed )
{
	BW_GUARD;

	MF_ASSERT( filename != NULL );

	BSPTreePtr bspTree;
	BinaryPtr bspData;

	bool fileExists;

	// Check file exists
	fileExists = BWResource::fileExists( bspName );

	// Attepmt to load file
	if ( fileExists )
	{
		//DEBUG_MSG( "Attempting to load %s\n", bspName.c_str() );

		// Open file
		DataSectionPtr bspSect = BWResource::openSection( bspName );
		MF_ASSERT( bspSect.exists() != 0 );

		bspData = bspSect->asBinary();
		MF_ASSERT( bspData.exists() != 0 );

		// Load file
		bspTree.reset( BSPTreeTool::loadBSP( bspData ) );
		if ( !bspTree.get() )
		{
			bspData = NULL;
			ERROR_MSG( "loadBSP2File: Load failed %s\n",
					bspName.c_str() );
		}
	}

	return bspTree;
}
#endif

/**
 *	Creates a BSP-tree, compiled from the speedtree.
 *	This function will save the corresponding ".bsp2" file.
 *	No memory caching if performed.
 *
 *	@param	speedTree	the tree object.
 *	@param	filename	the name of the spt file used to load the tree.
 *	@param	bspName		the name of the .bsp2 file.
 *	@param	seed		the seed to be used when computing the tree.
 *	@param	o_bbox		will hold the tree's bounding box.
 *
 *	@note				will throw std::runtime_error if tree can't be computed.
 */
#if SPEEDTREE_SUPPORT
BSPTreePtr generateBSPFromFile(
	CSpeedTreeRT * speedTree, 
	const char * filename,
	const BW::string& bspName,
	int seed,
	BoundingBox & o_bbox )
{
	BW_GUARD;

	BSPTreePtr bspTree;

	//DEBUG_MSG( "generateBSPFromFile: "
	//			"attempting to generate BSP from .spt %s\n", filename );

	CSpeedTreeRT* speedTreeLocal;

	// Load speedtree from file
	speedTreeLocal = acquireSpeedTree( speedTree, filename );

	if ( speedTreeLocal == NULL )
	{
		BW::string errorMsg = "Could not load tree definition file: ";
		errorMsg += filename;
		throw std::runtime_error( errorMsg.c_str() );
	}

	// If, at this point, we still don't have the bspTree,
	// rebuild it from the collision geometry in the spt file.
	bool emptyBSP = true;
	RealWTriangleSet triangles;
	extractCollisionVolumes(
		*speedTreeLocal, triangles, 
		emptyBSP);
	extractBranchesMesh(
		*speedTreeLocal, seed, 
		triangles, emptyBSP );

	
	bspTree.reset( BSPTreeTool::buildBSP( triangles ) );


	MD5 sptMD5;
	//since this file has been just read so it is still in the DataSectionCensus (in memory)
	BinaryPtr bin =BWResource::instance().rootSection()->readBinary( filename );
	if ( !!bin )
	{
		// Use the file data as MD5 input
		sptMD5.append( (unsigned char *)bin->cdata(), bin->len() );
	}

	MD5::Digest md5Dig;
	sptMD5.getDigest( md5Dig );

	saveBSPWithMD5( bspTree.get(), bspName, md5Dig );

	// Don't mind deleting the tree if
	// there is no support for speedtrees
	if ( speedTree == 0 )
	{
		bw_safe_delete(speedTreeLocal);
	}
	else
	{
		MF_ASSERT( ( speedTreeLocal == NULL ) ||
			( speedTree == speedTreeLocal ) );
	}

	return bspTree;
}
#endif // SPEEDTREE_SUPPORT

#if ( defined EDITOR_ENABLED ) && ( SPEEDTREE_SUPPORT )
BSPTreePtr generateBSPFromBB(
	CSpeedTreeRT* speedTree, 
	const char* filename, 
	const BW::string& bspName,
	int seed,
	BoundingBox& o_bbox )
{
	// No collision geometry exists, but we still
	// want to be able to select the tree in the editor.
	//DEBUG_MSG( "generateBSPFromFile: "
	//		"no collision geometry in %s. "
	//		"Generating from bounding box.\n", filename );

	BSPTreePtr bspTree;

	// Use the tree's bounding box as it's BSP tree.
	float bounds[6];
	speedTree->GetBoundingBox(bounds);
	const float width        = (bounds[3]-bounds[0]);
	const float height       = bounds[4] - bounds[1];
	const float depth       = (bounds[5]-bounds[2]);
	const float dimensions[] = { width, depth, height };
	const float position[]   = { ( bounds[0] + bounds[3] ) * 0.5f, bounds[1], ( bounds[2] + bounds[5] ) * 0.5f };
	Matrix rotation = Matrix::identity;

	RealWTriangleSet triangles;
	createBoxTriagles( position, rotation, dimensions, triangles );

	// make that BSP box not render in editor
	for (size_t i = 0; i < triangles.size(); i++)
		triangles[i].flags(TRIANGLE_TRANSPARENT | (0xff << TRIANGLE_MATERIALKIND_SHIFT));

	bspTree.reset( BSPTreeTool::buildBSP( triangles ) );

	return bspTree;
}
#endif // EDITOR_ENABLED

/**
 *	Adjust the bounding box to the size of the BSP tree.
 *	@param bspTree the bsp tree to shape the box to.
 *	@param o_bbox the box to shape.
 */
void adjustBoundingBox(
	const BSPTreePtr& bspTree,
	BoundingBox& o_bbox )
{

	if ( bspTree->numTriangles() != 0)
	{
		WorldTriangle* pTriangles = bspTree->triangles();
		for (uint i = 0; i < bspTree->numTriangles(); i++)
		{
			const WorldTriangle& tri = pTriangles[i];
			o_bbox.addBounds( tri.v0() );
			o_bbox.addBounds( tri.v1() );
			o_bbox.addBounds( tri.v2() );
		}
	}
	else
	{
		o_bbox.addBounds( Vector3( 0, 0, 0 ) );
	}

	return;
}

namespace { // anonymous
#if SPEEDTREE_SUPPORT

/**
 *	Loads the provided bspTree is it's MD5 hash matches the
 *	one in the given speedtree.
 *
 *	@param	bspData		bsp-tree data loaded from file.
 *	@param	speedTree	the tree object.
 *	@param	filename	the name of the spt file used to load the tree.
 *	@param	seed		the seed to be used when computing the tree.
 *
 *	@return NULL BSPTreePtr on failure.
 */
BSPTreePtr loadBSPIfValid(
	BinaryPtr      bspData,
	CSpeedTreeRT * speedTree,
	const char   * filename, 
	int            seed)
{
	BW_GUARD;
	MF_ASSERT(bspData.exists());

	BSPTreePtr bsp(BSPTreeTool::loadBSP( bspData ));
	bool result = true;

	#if SPEEDTREE_SUPPORT
		// use md5 check only if
		// speedtree is enabled

		MD5::Digest bspDig;
		BinaryPtr bspDigData = bsp->getUserData(BSPTree::MD5_DIGEST);
		if (bspDigData.exists())
		{
			BW::string bspQuoted  = bspDigData->cdata();
			bspDig.unquote(bspQuoted);
		}
		else
		{
			bspDig.unquote("12345678901234567890123456789012");
		}

		if (speedTree != NULL)
		{
			MD5 sptMD5;
			//since this file has been just read so it is still in the DataSectionCensus (in memory)
			BinaryPtr bin =BWResource::instance().rootSection()->readBinary( filename );
			if ( !!bin )
			{
				// Use the file data as MD5 input
				sptMD5.append( (unsigned char *)bin->cdata(), bin->len() );
			}

			MD5::Digest sptDig;
			sptMD5.getDigest(sptDig);

			result = bspDig == sptDig;
		}
		else
		{
			// is speedtree is NULL, don't
			result = false;
		}
	#endif // !SPEEDTREE_SUPPORT

	BSPTreePtr bspTree;
	if (result)
	{
		bspTree = bsp;
	}

	return bspTree;
}

/**
 *	Saves BSPTree to file, setting the provided
 *	MD5 digest as a user data entry into it.
 *
 *	@param	bspTree		the bsp-tree to be saved.
 *	@param	bspName		name of bsp-tree file to save.
 *	@param	md5Dig		md5Dig to save together with bsp-tree data.
 *
 *	@return				true on success, false on error.
 */
bool saveBSPWithMD5(
	BSPTree*          pBspTree,
	const BW::string & bspName,
	const MD5::Digest & md5Dig)
{
	BW_GUARD;
	BW::string quote = md5Dig.quote();
	pBspTree->setUserData(
		BSPTree::MD5_DIGEST,
		new BinaryBlock(quote.c_str(), quote.length()+1, "BinaryBlock/SpeedTreeCollision") );

	return BSPTreeTool::saveBSPInFile( pBspTree, bspName.c_str() );
}

/**
 *	If the provided speedTree is not NULL, returns it. Otherwise, tries to
 *	loaded the one specified by filename. Returns NULL if loading fails.
 *
 *	@param	speedTree	the tree object.
 *	@param	filename	the name of the spt file used to load the tree.
 *	
 *	@return				the acquired speedtree object.
 */
CSpeedTreeRT * acquireSpeedTree(CSpeedTreeRT * speedTree, const char * filename)
{
	BW_GUARD;
	if (speedTree != NULL)
	{
		return speedTree;
	}

	CSpeedTreeRT * tempSpeedTree;
	tempSpeedTree = new CSpeedTreeRT;
	const unsigned char* binData = 0;
	int binLen = 0;
	BinaryPtr bin = BWResource::instance().fileSystem()->readFile( filename );
	if ( !!bin )
	{
		binData = (unsigned char *)bin->cdata();
		binLen = bin->len();
	}
	if ( binData && binLen && !tempSpeedTree->LoadTree( binData, binLen ) )
	{
		speedTree = tempSpeedTree;
	}

	return speedTree;
}

/**
 *	Adds a triangle into the provided triangle set.
 */
inline void addTriangle(
    const Matrix     & xform,
    const Vector3    & p0,
    const Vector3    & p1,
    const Vector3    & p2,
    const Vector3    & p3,
    RealWTriangleSet & triangles,
    bool               solid)
{
    BW_GUARD;
	// Transform the points:
    Vector3 w0 = p0 + xform.applyPoint(p1 - p0);
    Vector3 w1 = p0 + xform.applyPoint(p2 - p0);
    Vector3 w2 = p0 + xform.applyPoint(p3 - p0);

    // test to see if the points are distinct enough to form 
    // a plane, and if they are add them to the list of triangles
    PlaneEq planeEq(w0, w1, w2, PlaneEq::SHOULD_NORMALISE);
    float len = planeEq.normal().length();
    if ((1.0f - c_PlaneEpsilon < len) && (len < 1.0f + c_PlaneEpsilon))
    {
    	// TODO: fix collision flags
		// set TRIANGLE_PROJECTILENOCOLLIDE on solid speedtree triangles
		WorldTriangle::Flags flags = WorldTriangle::packFlags(
			TRIANGLE_DOUBLESIDED | (solid ? TRIANGLE_PROJECTILENOCOLLIDE : TRIANGLE_NOCOLLIDE | TRIANGLE_PROJECTILENOCOLLIDE),
			(solid ? SPT_COLLMAT_SOLID : SPT_COLLMAT_LEAVES ));
        triangles.push_back(WorldTriangle(w0, w1, w2, flags));
	}
}

/**
 *	Creates a box.
 */
void createBoxTriagles(
	const float      * position,
	const Matrix     & rotation,
	const float      * dimensions,
	RealWTriangleSet & o_triangles)
{
	BW_GUARD;
	const float & px = position[0];
	const float & py = position[1];
	const float & pz = -position[2];
	const float w2 = dimensions[0]/2.0f;
	const float l2 = dimensions[1]/2.0f;
	const float & h = dimensions[2];
	const Vector3 p0(px, py, pz);
	const Vector3 b1(px-w2, py+0, pz-l2);
	const Vector3 b2(px-w2, py+0, pz+l2);
	const Vector3 b3(px+w2, py+0, pz+l2);
	const Vector3 b4(px+w2, py+0, pz-l2);
	const Vector3 t1(px-w2, py+h, pz-l2);
	const Vector3 t2(px-w2, py+h, pz+l2);
	const Vector3 t3(px+w2, py+h, pz+l2);
	const Vector3 t4(px+w2, py+h, pz-l2);

    addTriangle(rotation, p0, b3, b2, b1, o_triangles, SOLID_LEAVES);
	addTriangle(rotation, p0, b1, b4, b3, o_triangles, SOLID_LEAVES);
	addTriangle(rotation, p0, t1, t2, t3, o_triangles, SOLID_LEAVES);
	addTriangle(rotation, p0, t3, t4, t1, o_triangles, SOLID_LEAVES);
	addTriangle(rotation, p0, b1, b2, t2, o_triangles, SOLID_LEAVES);
	addTriangle(rotation, p0, t2, t1, b1, o_triangles, SOLID_LEAVES);
	addTriangle(rotation, p0, b2, b3, t3, o_triangles, SOLID_LEAVES);
	addTriangle(rotation, p0, t3, t2, b2, o_triangles, SOLID_LEAVES);
	addTriangle(rotation, p0, b3, b4, t4, o_triangles, SOLID_LEAVES);
	addTriangle(rotation, p0, t4, t3, b3, o_triangles, SOLID_LEAVES);
	addTriangle(rotation, p0, b4, b1, t1, o_triangles, SOLID_LEAVES);
	addTriangle(rotation, p0, t1, t4, b4, o_triangles, SOLID_LEAVES);
}

/**
 *	Creates the caps (poles) of a sphere.
 */
void createSphereCapTriagles(
	bool               upper,
	const float      * position,
	const Matrix     & rotation,
	const float      * dimensions,
	RealWTriangleSet & o_triangles,
	float              height)
{
	BW_GUARD;
	const int & res  = c_CollisionObjRes;

	// pivot point
	const Vector3 p0(position[0], position[1], -position[2]);

	const float & px = p0.x;
	const float & py = p0.y + height;
	const float & pz = p0.z;
	const float & r = dimensions[0];

	for (int j=0; j<res; ++j)
	{
		const float angle1 = ((0.5f*MATH_PI/res) * j) - 0.5f*MATH_PI;
		const float angle2 = ((0.5f*MATH_PI/res) * (j+1)) - 0.5f*MATH_PI;
		const float r1 = cosf(angle1) * r;
		const float h1 = sinf(angle1) * r;
		const float r2 = cosf(angle2) * r;
		const float h2 = sinf(angle2) * r;
		for (int k=0; k<res; ++k)
		{
			const float angle1 = (2.0f*MATH_PI/res) * k;
			const float angle2 = (2.0f*MATH_PI/res) * ((k+1)%res);
			const float x1 = cosf(angle1);
			const float z1 = sinf(angle1);
			const float x2 = cosf(angle2);
			const float z2 = sinf(angle2);

			Vector3 b1, b2;
			if (upper)
			{
				b1 = Vector3(px+x1*r1, py-h1, pz+z1*r1);
				b2 = Vector3(px+x2*r1, py-h1, pz+z2*r1);
				const Vector3 t1(px+x1*r2, py-h2, pz+z1*r2);
				const Vector3 t2(px+x2*r2, py-h2, pz+z2*r2);
				addTriangle(rotation, p0, b2, t1, b1, o_triangles, SOLID_LEAVES);
				addTriangle(rotation, p0, b2, t2, t1, o_triangles, SOLID_LEAVES);
			}
			else
			{
				b1 = Vector3(px+x1*r1, py+h1, pz+z1*r1);
				b2 = Vector3(px+x2*r1, py+h1, pz+z2*r1);
				const Vector3 t1(px+x1*r2, py+h2, pz+z1*r2);
				const Vector3 t2(px+x2*r2, py+h2, pz+z2*r2);
				addTriangle(rotation, p0, b1, t1, b2, o_triangles, SOLID_LEAVES);
				addTriangle(rotation, p0, t1, t2, b2, o_triangles, SOLID_LEAVES);
			}
		}
	}
}

/**
 *	Extracts the collision volumes from the provided speedtree.
 */
void extractCollisionVolumes(
	CSpeedTreeRT     & speedTree,
	RealWTriangleSet & o_triangles,
	bool             & o_emptyBSP )
{
	BW_GUARD;
	int collisionCount = speedTree.GetNumCollisionObjects();
	if (collisionCount > 0)
	{
		for (int i=0; i<collisionCount; ++i)
		{
			float position[3];
			float dimensions[3];
			float eulerAngles[3];
			CSpeedTreeRT::ECollisionObjectType type;
			speedTree.GetCollisionObject(i, type, position, dimensions, eulerAngles);

			// eulerAngles[2] is yaw, [0] is pitch, [1] is roll
			// angles are applied in roll/pitch/yaw order, yaw is reversed
			Matrix rotation;
			rotation.setRotateY(DEG_TO_RAD(-eulerAngles[2])); // yaw
			rotation.preRotateX(DEG_TO_RAD(eulerAngles[0])); // pitch
			rotation.preRotateZ(DEG_TO_RAD(eulerAngles[1])); // roll

			switch (type)
			{
			case CSpeedTreeRT::CO_BOX:
			{
				createBoxTriagles(position, rotation, dimensions, o_triangles);
				break;
			}
			case CSpeedTreeRT::CO_CAPSULE:
			{
				const int & res  = c_CollisionObjRes;
				const float & px = position[0];
				const float & py = position[1];
				const float & pz = -position[2];
				const float & r = dimensions[0];
				const float & h = dimensions[1];
				const Vector3 p0(px, py, pz);
				const Vector3 bc(px, py+0, pz);
				const Vector3 tc(px, py+h, pz);
				for (int j=0; j<res; ++j)
				{
					const float angle1 = (2.0f*MATH_PI/res) * j;
					const float angle2 = (2.0f*MATH_PI/res) * ((j+1)%res);
					const float x1 = cosf(angle1) * r;
					const float z1 = sinf(angle1) * r;
					const float x2 = cosf(angle2) * r;
					const float z2 = sinf(angle2) * r;
					const Vector3 b1(px+x1, py+0, pz+z1);
					const Vector3 b2(px+x2, py+0, pz+z2);
					const Vector3 t1(px+x1, py+h, pz+z1);
					const Vector3 t2(px+x2, py+h, pz+z2);
					addTriangle(rotation, p0, b1, t1, b2, o_triangles, SOLID_LEAVES);
					addTriangle(rotation, p0, t1, t2, b2, o_triangles, SOLID_LEAVES);
				}
				createSphereCapTriagles(true, position, rotation, dimensions, o_triangles, h);
				createSphereCapTriagles(false, position, rotation, dimensions, o_triangles);
				break;
			}
			case CSpeedTreeRT::CO_SPHERE:
			{
				createSphereCapTriagles(true, position, rotation, dimensions, o_triangles);
				createSphereCapTriagles(false, position, rotation, dimensions, o_triangles);
				break;
			}
			}
		}
		o_emptyBSP = false;
	}
}

/**
 *	Extracts the collision mesh from the provided speedtree.
 */
void extractBranchesMesh(
	CSpeedTreeRT     & speedTree,
	int                seed,
	RealWTriangleSet & o_triangles,
	bool             & o_emptyBSP )
{
	BW_GUARD;
	MD5 sptMD5;	

	static CSpeedTreeRT::SGeometry geometry;
	speedTree.GetGeometry(geometry, SpeedTree_BranchGeometry); 

	// if tree has branches
	if (geometry.m_sBranches.m_nNumLods > 0 &&
		geometry.m_sBranches.m_pNumStrips != NULL)
	{
		// get LOD 0 
		int iCount = -1;
		int stripsCount = geometry.m_sBranches.m_pNumStrips[0];
		for (int strip=0; strip<stripsCount; ++strip)
		{

			int lastIndex      = -1;
			int skipNextVertex = false;
			iCount = geometry.m_sBranches.m_pStripLengths[0][strip];

			// but only if has at 
			// least one triangle
			if (iCount < 3)
			{
				break;
			}			

			int idx1 = geometry.m_sBranches.m_pStrips[0][strip][0];
			int idx2 = geometry.m_sBranches.m_pStrips[0][strip][1];
			for( int i = 2; i < iCount; i++ )
			{
				int nIndex = geometry.m_sBranches.m_pStrips[0][strip][i];
				int idx3 = geometry.m_sBranches.m_pStrips[0][strip][i];

				if ( idx3 != idx2 && idx3 != idx1 && idx2 != idx1 )
				{
					const float *pf = &geometry.m_sBranches.m_pCoords[ idx1 * 3 ];
					Vector3 v1( pf[0], pf[1], -pf[2] );
					pf = &geometry.m_sBranches.m_pCoords[ idx2 * 3 ];
					Vector3 v2( pf[0], pf[1], -pf[2] );
					pf = &geometry.m_sBranches.m_pCoords[ idx3 * 3 ];
					Vector3 v3( pf[0], pf[1], -pf[2] );

					if (i % 2 == 1)
					{				
						addTriangle(
							Matrix::identity, 
							Vector3::zero(), v1, v2, v3,
							o_triangles, true);
					}
					else
					{				
						addTriangle(
							Matrix::identity, 
							Vector3::zero(), v1, v3, v2,
							o_triangles, true);
					}
				}

				idx1 = idx2;
				idx2 = idx3;
			}

/*
			// this is a triangle strip
			// so fetch the first two vertices
			Vector3 p0(0, 0, 0);	
			Vector3 p[3];			
			p[0] = Vector3(
				geometry.m_sBranches.m_pCoords[0*3+0],
				geometry.m_sBranches.m_pCoords[0*3+1],
				-geometry.m_sBranches.m_pCoords[0*3+2]);
			p[1] = Vector3(
				geometry.m_sBranches.m_pCoords[1*3+0],
				geometry.m_sBranches.m_pCoords[1*3+1],
				-geometry.m_sBranches.m_pCoords[1*3+2]);

			// now fetch the rest			
			for (int i=2; i<iCount; ++i)
			{
				int index = geometry.m_sBranches.m_pStrips[0][strip][i];
				p[2] = Vector3(
					geometry.m_sBranches.m_pCoords[index*3+0],
					geometry.m_sBranches.m_pCoords[index*3+1],
					-geometry.m_sBranches.m_pCoords[index*3+2]);

				// skip degenerated triangles
				if (index != lastIndex)
				{
					if (!skipNextVertex)
					{
						// reverse winding order
						// for every other triangle
						if (i % 2 == 1)
						{				
							addTriangle(
								Matrix::identity, 
								p0, p[0], p[1], p[2], 
								o_triangles, true);
						}
						else
						{				
							addTriangle(
								Matrix::identity, 
								p0, p[0], p[2], p[1],
								o_triangles, true);
						}

					}
					else
					{
						skipNextVertex = false;
					}
				}
				else
				{
					skipNextVertex = true;
				}

				p[0] = p[1];
				p[1] = p[2];
				lastIndex = index;
			}*/
		}

		if (iCount > 2)
		{	
			o_emptyBSP = false;
		}
	}
}

#endif // SPEEDTREE_SUPPORT
} // namespace anonymous
} // namespace speedtree

BW_END_NAMESPACE

// speedtree_collision.cpp

