#ifndef __skin_hpp__
#define __skin_hpp__

#include "bonevertex.hpp"
#include "matrix4.hpp"

BW_BEGIN_NAMESPACE

class Skin
{
public:
	Skin();
	~Skin();

	// returns the number of skins
	uint32 count();

	// initialises Skin for skin "index" - access functions now use this
	// skin cluster. this must be called before accessing the vertex weights
	bool initialise( uint32 index );

	// frees up memory used by the initialised skin
	void finalise();

	// returns the name of the currently initialised skin cluster
	BW::string name();

	// returns all found skins
	BW::vector<MObject>& skins();

	// returns dag paths of influenced meshes
	MDagPathArray& meshes();

	// access functions
	uint32 numberOfBones();
	
	// returns number of vertices for geometry object path (0 if not influenced by skin)
	uint32 numberOfVertices( const BW::string& path );

	// returns number of vertex data for geometry object path (empty BW::vector if not influenced by skin)
	BW::vector<BoneVertex>& vertices( const BW::string& path );
	
	// returns a BW::vector of object paths influenced by the skin
	BW::vector<BW::string> paths();

	// returns the bone names (full dag name or partial dag name)
	BW::string boneNameFull( uint32 bone );
	BW::string boneNamePartial( uint32 bone );
	
	// returns the MDagPath for bone
	MDagPath boneDAG( uint32 bone );
	
	// returns the BoneSet for bone
	BoneSet& boneSet( BW::string mesh );
	
	// returns the initial transform for bone index bone
	matrix4<float> transform( uint32 bone, bool relative = false );
	
	// returns the initial transform for the bone boneName (where boneName is the full bone name)
	matrix4<float> transform( const BW::string& boneName, bool relative = false );

	// this method removes skin bits that do not influence any of the meshes in the mDagPathArray
	void trim( MDagPathArray& meshes );
	
protected:
	// name of currently initialised skin cluster
	BW::string _name;

	// BW::vector of skin cluster MObjects in the maya scene
	BW::vector<MObject> _skins;

	// maps joint paths to bone indexes, refers to *current* initialised skin only
	BW::map<BW::string, uint32> _boneIndexes;
	BW::map<uint32, MDagPath> _bonePaths;

	// dag paths to influenced meshes
	MDagPathArray _meshes;

	// bone sets for each mesh - same index should be used as the _meshes array
	BW::vector<BoneSet> _boneSets;

	// number of bones influencing current skin
	uint32 _bones;

	// blended vertex data for each influenced geometry object
	BW::map< BW::string, BW::vector<BoneVertex> > _vertices;

	// initial bone transforms, to get the path of the object at an index, use the
	// same index into _bonePaths
	BW::vector< matrix4<float> > _transforms;

	// in case we want relative transforms
	BW::vector< matrix4<float> > _relativeTransforms;
	
	// empty bone vertex BW::vector
	static BW::vector<BoneVertex> _empty;
};

BW_END_NAMESPACE

#endif // __skin_hpp__
