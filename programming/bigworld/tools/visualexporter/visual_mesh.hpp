#ifndef VISUAL_MESH_HPP
#define VISUAL_MESH_HPP


#include "cstdmf/bw_vector.hpp"
#include "max.h"
#include "stdmat.h"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"
#include "math/boundbox.hpp"
#include "moo/index_buffer.hpp"
#include "moo/primitive_file_structs.hpp"
#include "moo/vertex_formats.hpp"

BW_BEGIN_NAMESPACE

inline bool operator < ( Point2 p1, Point2 p2 )
{
	if( p1.x < p2.x )	return true;
	if( p1.x > p2.x )	return false;
	if( p1.y < p2.y )	return true;
	if( p1.y > p2.y )	return false;
	return false;
}

inline bool operator > ( Point2 p1, Point2 p2 )
{
	if( p1.x > p2.x )	return true;
	if( p1.x < p2.x )	return false;
	if( p1.y > p2.y )	return true;
	if( p1.y < p2.y )	return false;
	return false;
}

inline bool operator < ( Point3 p1, Point3 p2 )
{
	if( p1.x < p2.x )	return true;
	if( p1.x > p2.x )	return false;
	if( p1.y < p2.y )	return true;
	if( p1.y > p2.y )	return false;
	if( p1.z < p2.z )	return true;
	if( p1.z > p2.z )	return false;
	return false;
}

inline bool operator > ( Point3 p1, Point3 p2 )
{
	if( p1.x > p2.x )	return true;
	if( p1.x < p2.x )	return false;
	if( p1.y > p2.y )	return true;
	if( p1.y < p2.y )	return false;
	if( p1.z > p2.z )	return true;
	if( p1.z < p2.z )	return false;
	return false;
}

inline bool operator < ( Point4 p1, Point4 p2 )
{
	if( p1.x < p2.x )	return true;
	if( p1.x > p2.x )	return false;
	if( p1.y < p2.y )	return true;
	if( p1.y > p2.y )	return false;
	if( p1.z < p2.z )	return true;
	if( p1.z > p2.z )	return false;
	if( p1.w < p2.w )	return true;
	if( p1.w > p2.w )	return false;
	return false;
}

inline bool operator > ( Point4 p1, Point4 p2 )
{
	if( p1.x > p2.x )	return true;
	if( p1.x < p2.x )	return false;
	if( p1.y > p2.y )	return true;
	if( p1.y < p2.y )	return false;
	if( p1.z > p2.z )	return true;
	if( p1.z < p2.z )	return false;
	if( p1.w > p2.w )	return true;
	if( p1.w < p2.w )	return false;
	return false;
}

class QuickFileWriter;
typedef SmartPointer<class VisualMesh> VisualMeshPtr;

/**
 *	This class is used to convert 3dsMax objects into BigWorld visual
 *	mesh objects.
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
		Point4  colour;
		Point2	uv;
		Point2	uv2;
		int		vertexIndex;
		int		smoothingGroup;
		Point3	binormal;
		Point3	tangent;
		int		meshIndex;			// for creating MeshParticles
		bool operator == ( const BloatVertex& bv ) const 
		{ 
			return  this->pos == bv.pos &&
			this->normal == bv.normal &&
			this->colour == bv.colour &&
			this->uv == bv.uv &&
			this->uv2 == bv.uv2 &&
			this->vertexIndex == bv.vertexIndex &&
			(this->smoothingGroup & bv.smoothingGroup) &&
			this->binormal == bv.binormal &&
			this->tangent == bv.tangent &&
			this->meshIndex == bv.meshIndex;
		};

		bool operator < ( const BloatVertex& bv ) const 
		{
			if( pos < bv.pos )	return true;
			if( pos > bv.pos )	return false;

			if( normal < bv.normal )	return true;
			if( normal > bv.normal )	return false;

			if( colour < bv.colour )	return true;
			if( colour > bv.colour )	return false;

			if( uv < bv.uv )	return true;
			if( uv > bv.uv )	return false;

			if( uv2 < bv.uv2 )	return true;
			if( uv2 > bv.uv2 )	return false;

			if( vertexIndex < bv.vertexIndex )	return true;
			if( vertexIndex > bv.vertexIndex )	return false;

			if( binormal < bv.binormal )	return true;
			if( binormal > bv.binormal )	return false;

			if( tangent < bv.tangent )	return true;
			if( tangent > bv.tangent )	return false;

			if( meshIndex < bv.meshIndex )	return true;
			if( meshIndex > bv.meshIndex )	return false;

			return false; // enough
		};

		Vector3 v3Pos() const	{ return Vector3(pos.x,pos.y,pos.z); }
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
		  mapIdMeaning( 0 ),
		  selfIllum( 0 ),
		  inUse( false )
		{
		}

		//return all resources used
		void resources( BW::vector<BW::string>& retResources );

		BW::string identifier;
		BW::string mapIdentifier;
		int mapIdMeaning;	// 0 = none, 1 = bitmap, 2 = mfm
		float selfIllum;
		Color ambient;
		Color diffuse;
		Color specular;
		bool inUse;

		typedef std::pair< BW::string, BW::string > TextureOverride;
		typedef std::pair< BW::string, Point4 > VectorOverride;
		typedef std::pair< BW::string, BOOL > BoolOverride;
		typedef std::pair< BW::string, float > FloatOverride;
		typedef std::pair< BW::string, int > IntOverride;

		typedef BW::vector< TextureOverride > TextureOverrides;
		typedef BW::vector< VectorOverride > VectorOverrides;
		typedef BW::vector< BoolOverride > BoolOverrides;
		typedef BW::vector< FloatOverride > FloatOverrides;
		typedef BW::vector< IntOverride > IntOverrides;

		TextureOverrides textureOverrides_;
		VectorOverrides vectorOverrides_;
		BoolOverrides boolOverrides_;
		FloatOverrides floatOverrides_;
		IntOverrides intOverrides_;

		BW::string fxFile_;

		bool operator==( const Material& m ) const
		{
			return identifier == m.identifier &&
				mapIdMeaning == m.mapIdMeaning &&
				mapIdentifier == m.mapIdentifier &&
				selfIllum == m.selfIllum &&
				fxFile_ == m.fxFile_ &&
				textureOverrides_ == m.textureOverrides_ &&
				vectorOverrides_ == m.vectorOverrides_ &&
				boolOverrides_ == m.boolOverrides_ &&
				floatOverrides_ == m.floatOverrides_ &&
				intOverrides_ == m.intOverrides_;

		}

		enum SkinType
		{
			NO_SKIN,
			SOFT_SKIN
		};

		void save( DataSectionPtr pGeometry,
					DataSectionPtr spExistingVisual,
					uint32 pgid,
					SkinType skin = NO_SKIN,
					bool skinnedShader = false );

		bool findAndCopyExistingMaterial( const BW::string& identifier,
				DataSectionPtr pPrimGroup,
				DataSectionPtr spExistingVisual ) const;

		DataSectionPtr findExistingMaterial( const BW::string& identifier,
				DataSectionPtr spExistingVisual ) const;

		bool checkFilename( BW::string& fxName, const BW::string& fxPostfix );
	};

	// Type definitions
	typedef BW::vector< BloatVertex >	BVVector;
	typedef BW::vector< Triangle >		TriangleVector;
	typedef BW::vector< Material >		MaterialVector;

	
	// Constructor / destructor
	VisualMesh();
	virtual ~VisualMesh();

	// Public interface
	bool				init( INode* node, bool checkMats = true );
	void				resources( BW::vector<BW::string>& retResources );
	void				snapVertices( bool snapVertices ) { snapVertices_ = snapVertices; }
	bool				checkDXMaterials( const BW::string& fxPostfix = "" );
	bool				largeIndices() const { return largeIndices_; }
	bool				dualUV() const { return dualUV_; }
	void				dualUV( bool value ){ dualUV_ = value; }
	bool				vertexColours() const { return vertexColours_; }
	void				vertexColours( bool value ){ vertexColours_ = value; }
	const BoundingBox&	bb() const { return bb_; };
	virtual void		add(	VisualMesh & destMesh, int forceMeshIndex = -1,
								bool worldSpace = true );
	virtual bool		save(	DataSectionPtr pVisualSection, DataSectionPtr spExistingVisual,
								const BW::string& primitiveFile, bool useIdentifier );
	virtual bool		savePrimXml( const BW::string& xmlFile );
	virtual bool		isVisualEnvelope() { return false; }
	BW::string			getIdentifier() { return this->identifier_; }

protected:
	typedef BW::vector<Moo::PrimitiveGroup> PGVector;
	typedef BW::vector<Moo::VertexXYZNUV> VertexVector;
	typedef BW::vector<Moo::VertexXYZNUV2> UV2VertexVector;
	typedef BW::vector<Moo::VertexXYZNDUV> DiffuseVertexVector;
	typedef BW::vector<Moo::VertexXYZNUVI> IndexedVertexVector;
	typedef BW::vector<Moo::VertexXYZNUVITB> IndexedTBVertexVector;
	typedef BW::vector<Moo::VertexXYZNUVTB> TBVertexVector;

	int		addVertex( const BloatVertex& bv );
	void	createNormals( );
	void	getNormalsFromModifier( INode* pNode );
	float	normalAngle( const Triangle& tri, uint32 index );
	void	addNormal( const Point3& normal, int realIndex, int smoothingGroup, int index );
	void	addMaterial( StdMat* mtl );
	void	addDXMaterial( Mtl* mtl );
	void	gatherMaterials( INode* node );
	int		findOrAddMaterial( const Material & mat );
	void	flipTriangleWindingOrder();
	void	sortTriangles();
	void	removeDuplicateVertices();
	bool	createPrimitiveGroups( PGVector& primGroups, Moo::IndicesHolder& indices );
	bool	createMeshParticlePrimGroups( PGVector& primGroups );
	void	createVertexList( VertexVector& vertices );
	void	createVertexList( UV2VertexVector& vertices );
	void	createVertexList( DiffuseVertexVector& vertices );
	void	createVertexList( IndexedVertexVector& vertices );
	void	createVertexList( TBVertexVector& vertices );
	void	createVertexList( IndexedTBVertexVector& vertices );
	void	makeBumped();


	VisualMesh( const VisualMesh& );
	VisualMesh& operator=( const VisualMesh& );

	typedef BW::vector<int>	RemapVector;

	RemapVector					materialRemap_;
	BW::vector< Point3 >		vertexPositions_;

	bool			vertexColours_;
	bool			dualUV_;
	BVVector		vertices_;
	TriangleVector	triangles_;
	MaterialVector	materials_;

	BW::string identifier_;
	BoundingBox bb_;

	Matrix3		world_;
	bool		snapVertices_;
	bool		largeIndices_;
};

BW_END_NAMESPACE

#endif // VISUAL_MESH_HPP
