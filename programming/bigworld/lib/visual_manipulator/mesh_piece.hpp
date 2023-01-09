#ifndef VISUAL_MANIPULATOR_MESH_PIECE_HPP
#define VISUAL_MANIPULATOR_MESH_PIECE_HPP

#include "bloat_vertex.hpp"
#include "triangle.hpp"
#include "resmgr/datasection.hpp"
#include "math/boundbox.hpp"
#include "cstdmf/smartpointer.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

namespace VisualManipulator
{

typedef BW::vector< BloatVertex > VertexVector;
typedef BW::vector< Triangle > TriangleVector;
typedef BW::vector< BW::string > JointVector;
typedef BW::vector< BW::string > MaterialVector;
typedef BW::map<BW::string, DataSectionPtr> MaterialMap;

class Node;
typedef SmartPointer<Node> NodePtr;
class MeshPiece;
typedef SmartPointer<MeshPiece> MeshPiecePtr;

/*
 *	This class implements a piece of a mesh that can be saved to a visual file
 */
class MeshPiece : public ReferenceCount
{
public:
	MeshPiece();

	uint32 addVertex( const BloatVertex& bv );
	void addTriangle( const BloatVertex& bv1, const BloatVertex& bv2, 
		const BloatVertex& bv3, uint32 materialIndex );
	
	VertexVector& vertices() { return vertices_; }
	TriangleVector&  triangles() { return triangles_; }
	JointVector&  joints() { return joints_; }
	MaterialVector& materials() { return materials_; }

	BoundingBox& bounds() { return bounds_; }
	BoundingBox  skinBounds( const BW::vector<NodePtr>& nodes );

	const BW::string& name() { return name_; }
	void name( const BW::string& name ) { name_ = name; }

	void save( DataSectionPtr pVisualSection, DataSectionPtr pPrimSection, bool skinned, const MaterialMap& materials );
	void saveBSP( DataSectionPtr pPrimSection, const BW::string& modelFilename ) const;

	void merge( MeshPiecePtr pOther, const BW::vector<NodePtr>& nodes );
	void split( BW::vector< MeshPiecePtr >& meshPieces, uint32 boneLimit );
	
	void mergeMaterials();
	void sortTrianglesByMaterial();
	void calculateTangentsAndBinormals();

	bool isBSP() const;
	bool isHull() const;
	bool isPortal() const;

	bool dualUV() const { return dualUV_; }
	void dualUV( bool state ) { dualUV_ = state; }
	bool vertexColours() const { return vertexColours_; }
	void vertexColours( bool state ) { vertexColours_ = state; }

	static MaterialMap gatherMaterials( DataSectionPtr pVisualSection );
private:

	void writeIndices( DataSectionPtr pPrimSection, const BW::string& sectionName );
	void writeVertices( DataSectionPtr pPrimSection, const BW::string& sectionName, bool skinned );

	void writeUVStream( DataSectionPtr pPrimSection, const BW::string& sectionName ) const;
	void writeColourStream( DataSectionPtr pPrimSection, const BW::string& sectionName ) const;

	VertexVector vertices_;
	TriangleVector triangles_;
	JointVector joints_;
	MaterialVector materials_;
	BW::string name_;

	BoundingBox bounds_;

	bool dualUV_;
	bool vertexColours_;
	bool tangents_;
};

}

BW_END_NAMESPACE

#endif // VISUAL_MANIPULATOR_MESH_PIECE_HPP
