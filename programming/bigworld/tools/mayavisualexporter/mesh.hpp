#ifndef __mesh_hpp__
#define __mesh_hpp__

#include "vertex.hpp"
#include "face.hpp"
#include "material.hpp"

BW_BEGIN_NAMESPACE

class Mesh
{
public:
	Mesh();
	~Mesh();

	// returns the number of meshes
	uint32 count();

	// initialises Mesh for mesh "index" - access functions now use this
	// mesh. this must be called before accessing vertex data
	bool initialise( uint32 index, bool objectSpace = true );

	// frees up memory used by the mesh
	void finalise();

	MDagPathArray& meshes();

	BW::string name();
	BW::string nodeName();
	BW::string fullName();

	BW::vector<Point3>& positions();
	BW::vector<Point3>& normals();
	BW::vector<Point3>& binormals();
	BW::vector<Point3>& tangents();
	BW::vector<Point4>& colours();
	BW::vector<Point2>& uvs( uint32 set );
	uint32 uvSetCount() const;
	BW::vector<Face>& faces();

	BW::vector<material>& materials();

	void trim( BW::vector<MObject>& skins );

protected:
	// dag paths to meshes
	MDagPathArray _meshes;

	// geometry data
	BW::string _name;
	BW::string _nodeName;
	BW::string _fullName;
	BW::vector<Point3> _positions;
	BW::vector<Point3> _normals;
	BW::vector<Point3> _binormals;
	BW::vector<Point3> _tangents;
	BW::vector<Point4> _colours;
	BW::vector<BW::vector<Point2>> _uvs;
	BW::vector<Face> _faces;
	BW::vector<material> _materials;
};

BW_END_NAMESPACE

#endif // __mesh_hpp__
