#ifndef VISUAL_MESH_HPP
#define VISUAL_MESH_HPP


#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_set.hpp"
#include "max.h"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"
#include "math/boundbox.hpp"

BW_BEGIN_NAMESPACE

class QuickFileWriter;
typedef SmartPointer<class VisualMesh> VisualMeshPtr;

/**
 *	This class is used to convert 3dsMax objects into BigWorld visual
 *	mesh object.
 */
class VisualMesh : public ReferenceCount
{
public:
	/**
	 *	The vertex format captured from 3dsMax.
	 */
	struct BloatVertex
	{
		Point3	pos;
		Point3	normal;
		Point2	uv;
		int		vertexIndex;
		int		smoothingGroup;
		bool operator == ( const BloatVertex& bv ) const 
		{ 
			return  this->pos == bv.pos &&
			this->normal == bv.normal &&
			this->uv == bv.uv &&
			this->vertexIndex == bv.vertexIndex &&
			(this->smoothingGroup & bv.smoothingGroup);
		};
	};

	/**
	 *	The triangle format captured from 3dsMax.
	 */
	struct Triangle
	{
		int		index[3];
		int		realIndex[3];
		int		smoothingGroup;
		int		materialIndex;
	};

	/**
	 *	The material format captured from 3dsMax.
	 */
	struct Material
	{
		Material()
		: identifier( "empty" ),
		  hasMap( false ),
		  selfIllum( 0 ),
		  inUse( false )
		{
		}
		BW::string identifier;
		bool hasMap;
		BW::string mapIdentifier;
		float selfIllum;
		Color ambient;
		Color diffuse;
		Color specular;
		bool inUse;
		bool operator== (const Material& m) const
		{
			return identifier == m.identifier &&
				hasMap == m.hasMap &&
				mapIdentifier == m.mapIdentifier &&
				selfIllum == m.selfIllum;

		}
	};

	// Type definitions
	typedef BW::vector< BloatVertex >  BVVector;
	typedef BW::vector< Triangle > TriangleVector;
	typedef BW::vector< Material > MaterialVector;

	// Constructor / destructor
	VisualMesh();
	virtual ~VisualMesh();

	// Public interface
	bool				init(INode* node);
	virtual	void		save( DataSectionPtr spVisualSection, const BW::string& primitiveFile );
	const BoundingBox&	bb() const { return bb_; };

protected:
	struct VertexXYZNUV
	{
		float pos[3];
		float normal[3];
		float uv[2];
	};

	struct VertexHeader
	{
		char	vertexFormat[ 64 ];
		int		nVertices;
	};

	struct IndexHeader
	{
		char	indexFormat[ 64 ];
		int		nIndices;
		int		nTriangleGroups;
	};

	struct PrimitiveGroup
	{
		int startIndex;
		int nPrimitives;
		int startVertex;
		int nVertices;
	};

	typedef BW::vector<uint16> IndexVector;
	typedef BW::vector<PrimitiveGroup> PGVector;
	typedef BW::vector<VertexXYZNUV> VertexVector;

	uint16	addVertex( const BloatVertex& bv );
	void	createNormals( );
	float	normalAngle( const Triangle& tri, uint32 index );
	void	addNormal( const Point3& normal, int realIndex, int smoothingGroup, int index );
	void	addMaterial( StdMat* mtl );
	void	gatherMaterials( INode* node );
	void	flipTriangleWindingOrder();
	void	sortTriangles();
	void	createPrimitiveGroups( PGVector& primGroups, IndexVector& indices );
	void	createVertexList( VertexVector& vertices );

	VisualMesh( const VisualMesh& );
	VisualMesh& operator=( const VisualMesh& );

	// Member variables
	typedef BW::vector<int> RemapVector;
	RemapVector		materialRemap_;

	BW::string		identifier_;
	BoundingBox		bb_;
	BVVector		vertices_;
	TriangleVector	triangles_;
	MaterialVector	materials_;
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "visual_mesh.ipp"
#endif

#endif // VISUAL_MESH_HPP
