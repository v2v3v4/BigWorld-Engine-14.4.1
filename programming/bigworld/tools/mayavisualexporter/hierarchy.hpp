#ifndef __hierarchy_hpp__
#define __hierarchy_hpp__

#include "matrix.hpp"
#include "mesh.hpp"

BW_BEGIN_NAMESPACE

class Hierarchy
{
public:
	Hierarchy( Hierarchy* parent );
	~Hierarchy();

	void clear();

	void getSkeleton( Skin& skin );
	void getMeshes( Mesh& mesh );

	void removeNode( const BW::string& name );
	BW::string removeNode( Hierarchy* node );
	void addNode( const BW::string& name, Hierarchy& h );
	Hierarchy* recursiveFind( const BW::string& name );
	Hierarchy* getParent() const { return parent_; }


	void addNode( const BW::string& path, MDagPath dag, BW::string accumulatedPath = "" );

	// this is for adding a child only 1 level below (for fixup nodes)
	void addNode( const BW::string& path, matrix4<float> worldTransform );

	void dump( const BW::string& parent = "" );

	void customName(BW::string customName);
	void name(BW::string name);
	BW::string name();
	BW::string path();
	BW::string dag();
	BW::vector<BW::string> children( bool orderChildren = false );
	Hierarchy& child( const BW::string& name );

	Hierarchy& find( const BW::string& path );

	uint32 numberOfFrames();

	// total number of objects in hierarchy, exluding the root node
	uint32 count();

	matrix4<float> transform( double frame, bool relative = false );

protected:
	BW::string _name;

	BW::string _customPath; // for manually added nodes, eg fixup nodes
	matrix4<float> _relativeTransform; // for manually added nodes
	matrix4<float> _worldTransform; // for manually added nodes

	MDagPath _path;

	BW::map<BW::string, Hierarchy*> _children;
	Hierarchy* parent_;
};

bool numChildDescending(
	std::pair< BW::string, Hierarchy* > lhs,
	std::pair< BW::string, Hierarchy* > rhs );

BW_END_NAMESPACE

#endif // __hierarchy_hpp__
