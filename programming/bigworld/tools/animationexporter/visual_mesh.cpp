#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4530 )

#include <strstream>
#include <algorithm>
#include "mfxexp.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/quick_file_writer.hpp"
#include "utility.hpp"
#include "expsets.hpp"
#include "cstdmf/binaryfile.hpp"
#ifdef MAX_RELEASE_R14
#include "MorpherApi.h"
#else
#include "wm3.h"
#endif

#include "visual_mesh.hpp"

#ifndef CODE_INLINE
#include "visual_mesh.ipp"
#endif

BW_BEGIN_NAMESPACE

/**
 *	Helper method that returns the current time.
 *	@return The current time.
 */
TimeValue timeNow()
{
	return ExportSettings::instance().staticFrame() * GetTicksPerFrame();
}

// -----------------------------------------------------------------------------
// Section: VisualMesh
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
VisualMesh::VisualMesh()
{
}


/**
 *	Destructor.
 */
VisualMesh::~VisualMesh()
{
}


/**
 *	This method finds the angle between two edges in a triangle.
 *
 *	@param tri	The triangle.
 *	@param index The index of the vertex whose angle we want.
 *	@return The angle (in radians) between the two edges.
 */
float VisualMesh::normalAngle( const Triangle& tri, uint32 index )
{
	Point3 v1 = vertices_[tri.index[(index + 1 ) % 3]].pos - vertices_[tri.index[index]].pos;
	Point3 v2 = vertices_[tri.index[(index + 2 ) % 3]].pos - vertices_[tri.index[index]].pos;


	float len = Length( v1 );
	if (len == 0)
		return 0;
	v1 /= len;

	len = Length( v2 );
	if (len == 0)
		return 0;
	v2 /= len;

	float normalAngle = DotProd( v1, v2 );

	normalAngle = min( 1.f, max( normalAngle, -1.f ) );

	return acosf( normalAngle );	
}


/**
 *	This method adds a normal to all vertices that should be influenced by it.
 *
 *	@param normal			The normal to add.
 *	@param realIndex			The index of the vertex in the max mesh.
 *	@param smoothingGroup	The smoothinggroup of this normal.
 *	@param index				The index of the vertex this normal has to influence.
 */
void VisualMesh::addNormal( const Point3& normal, int realIndex, int smoothingGroup, int index )
{
	for (uint i = 0; i < vertices_.size(); i++)
	{
		BloatVertex& bv = vertices_[i];
		if ((bv.vertexIndex == realIndex && 
			(bv.smoothingGroup & smoothingGroup)) || 
			index == i )
		{
			bv.normal += normal;
		}
	}
}	


/**
 *	This method creates vertex normals for all vertices in the vertex list.
 */
void VisualMesh::createNormals( )
{
	TriangleVector::iterator it = triangles_.begin();
	TriangleVector::iterator end = triangles_.end();

	while (it!=end)
	{
		Triangle& tri = *it++;
		Point3 v1 = vertices_[tri.index[1]].pos - vertices_[tri.index[0]].pos;
		Point3 v2 = vertices_[tri.index[2]].pos - vertices_[tri.index[0]].pos;
		Point3 normal = v1^v2;
		normal = Normalize( normal );

		addNormal( normal * normalAngle( tri, 0), tri.realIndex[0], tri.smoothingGroup, tri.index[0] );
		addNormal( normal * normalAngle( tri, 1), tri.realIndex[1], tri.smoothingGroup, tri.index[1] );
		addNormal( normal * normalAngle( tri, 2), tri.realIndex[2], tri.smoothingGroup, tri.index[2] );
	}

	BVVector::iterator vit = vertices_.begin();
	BVVector::iterator vend = vertices_.end();
	while (vit != vend)
	{
		BloatVertex& bv = *vit++;
		bv.normal = Normalize( bv.normal );
	}
}


/**
 *	This method takes a max material and stores the material data we want to export.
 *
 *	@param mtl	The max material to export.
 */
void VisualMesh::addMaterial( StdMat* mtl )
{
	Material m;
	m.ambient = mtl->GetAmbient( timeNow() );
	m.diffuse = mtl->GetDiffuse( timeNow() );
	m.specular = mtl->GetSpecular( timeNow() );
	m.selfIllum = mtl->GetSelfIllum( timeNow() );
#ifdef _UNICODE
	bw_wtoutf8( mtl->GetName(), m.identifier );
#else
	m.identifier = mtl->GetName();
#endif
	
	if( mtl->MapEnabled( ID_DI ) )
	{
		Texmap *map = mtl->GetSubTexmap( ID_DI );
		if( map )
		{
			if( map->ClassID() == Class_ID( BMTEX_CLASS_ID, 0x00 ) )
			{
#ifdef _UNICODE
				BW::string mapName = "";
				bw_wtoutf8( ((BitmapTex *)map)->GetMapName(), mapName );
				mapName = toLower( mapName );
#else
				BW::string mapName = toLower(((BitmapTex *)map)->GetMapName());
#endif
				BW::string textureName = unifySlashes( mapName );
				textureName = BWResolver::dissolveFilename( textureName );
				if (textureName.length())
				{
					m.hasMap = true;
					m.mapIdentifier = textureName;
				}
			}
		}
	}

	uint i;
	for (i = 0; i < materials_.size(); i++)
	{
		if (materials_[i] == m)
			break;
	}
	materialRemap_.push_back( i );
	if (i == materials_.size())		
		materials_.push_back( m );
}


/**
 *	This method gathers all the materials contained in a specific node.
 *
 *	@param node	The node to gather materials from.
 */
void VisualMesh::gatherMaterials( INode* node )
{
	Mtl* mtl = node->GetMtl();
	if (mtl)
	{
		if (mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0))
		{
			addMaterial( (StdMat*)mtl );
		}
		else if (mtl->NumSubMtls())
		{
			for (int i = 0; i < mtl->NumSubMtls(); i++)
			{
				Mtl* submat = mtl->GetSubMtl( i );
				if (submat && submat->ClassID() == Class_ID(DMTL_CLASS_ID, 0))
				{
					addMaterial( (StdMat*)submat );
				}
				else
				{
					Material m;
#ifdef _UNICODE
					if ( submat )
					{
						BW::string submatName = "";
						bw_wtoutf8( submat->GetName(), submatName );
						m.identifier = submatName;
					}
					else
					{
						m.identifier = "Default";
					}
#else
					m.identifier = submat ? submat->GetName() : "Default";
#endif
					materialRemap_.push_back( static_cast<int>(materials_.size()) );
					materials_.push_back( m );
				}
			}
		}
		else
		{
			Material m;
#ifdef _UNICODE
			bw_wtoutf8( mtl->GetName(), m.identifier );
#else
			m.identifier = mtl->GetName();
#endif	
			materialRemap_.push_back( static_cast<int>(materials_.size()) );
			materials_.push_back( m );
		}
	}
	else
	{
		Material m;
		m.identifier = "Default";
		materialRemap_.push_back( static_cast<int>(materials_.size()) );
		materials_.push_back( m );
	}

	if (!materialRemap_.size())
	{
		materialRemap_.push_back( 0 );
	}
}


/**
 *	This method swaps the triangle winding order for all triangles.
 */
void VisualMesh::flipTriangleWindingOrder()
{
	TriangleVector::iterator it = triangles_.begin();
	TriangleVector::iterator end = triangles_.end();

	while (it!=end)
	{
		Triangle& tri = *it++;
		std::swap( tri.index[1], tri.index[2] );
		std::swap( tri.realIndex[1], tri.realIndex[2] );
	}

}


/**
 *	A helper compare function that orders two triangles based on their material index.
 *
 *	@param	t1	The first triangle.
 *	@param	t2	The second triangle.
 *	@return	Does t1 preceed t2 based on their material index.
 */
bool triangleSorter( const VisualMesh::Triangle& t1, const VisualMesh::Triangle& t2 )
{
	return t1.materialIndex < t2.materialIndex;
}


/**
 *	This method sorts the triangles.
 */
void VisualMesh::sortTriangles()
{
	std::sort( triangles_.begin(), triangles_.end(), triangleSorter );
}


/**
 *	This method creates our primitive groups.
 *
 *	@param primGroups the output primitivegroups
 *	@param indices the outpur indices
 */
void VisualMesh::createPrimitiveGroups( PGVector& primGroups, IndexVector& indices )
{
	if (triangles_.size())
	{
		int mIndex = triangles_.front().materialIndex;

		int firstVertex = triangles_.front().index[0];
		int lastVertex = triangles_.front().index[0];
		int firstTriangle = 0;

		for (uint i = 0; i <= triangles_.size(); i++)
		{
			Triangle& tri = triangles_[i];
			if (i == triangles_.size() || tri.materialIndex != mIndex)
			{
				PrimitiveGroup pg;
				pg.startVertex = firstVertex;
				pg.nVertices = lastVertex - firstVertex + 1;
				pg.startIndex = firstTriangle * 3;
				pg.nPrimitives = i - firstTriangle;
				primGroups.push_back( pg );	
				firstVertex = tri.index[0];
				lastVertex = tri.index[0];
				firstTriangle = i;
				materials_[ mIndex ].inUse = true;
				mIndex = tri.materialIndex;
			}
			if (i!=triangles_.size())
			{
				indices.push_back( tri.index[0] );
				indices.push_back( tri.index[1] );
				indices.push_back( tri.index[2] );
				firstVertex = min( firstVertex, min( tri.index[0], min( tri.index[1], tri.index[2] ) ) );
				lastVertex  = max( lastVertex,  max( tri.index[0], max( tri.index[1], tri.index[2] ) ) );
			}
		}
	}
}


/**
 *	This method creates our output vertex list.
 *
 *	@param	vertices The output vertex list.
 */
void VisualMesh::createVertexList( VertexVector& vertices )
{
	VertexXYZNUV v;
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();
	while (it!=end)
	{
		BloatVertex& bv = *it++;
		bv.pos *= ExportSettings::instance().unitScale();
		v.pos[0] = bv.pos.x;
		v.pos[1] = bv.pos.z;
		v.pos[2] = bv.pos.y;
		v.normal[0] = bv.normal.x;
		v.normal[1] = bv.normal.z;
		v.normal[2] = bv.normal.y;
		v.uv[0] = bv.uv.x;
		v.uv[1] = bv.uv.y;
		vertices.push_back( v );
	}
}


/**
 *	This method saves the visual mesh.
 *
 *	@param spVisualSection DataSection of the .visual file.
 *	@param primitiveFile The filename of the primitive record.
 */
void VisualMesh::save( DataSectionPtr spVisualSection, const BW::string& primitiveFile )
{
	DataSectionPtr spFile = BWResource::openSection( primitiveFile );
	if (!spFile)
		spFile = new BinSection( primitiveFile, new BinaryBlock( 0, 0, "BinaryBlock/VisualMesh" ) );

	if (spFile)
	{
		flipTriangleWindingOrder();
		sortTriangles();

		PGVector primGroups;
		IndexVector indices;

		createPrimitiveGroups( primGroups, indices );

		IndexHeader ih;
		bw_snprintf( ih.indexFormat, sizeof(ih.indexFormat), "list" );
		ih.nIndices = static_cast<int>(indices.size());
		ih.nTriangleGroups = static_cast<int>(primGroups.size());

		QuickFileWriter f;
		f << ih;
		f << indices;
		f << primGroups;

		spFile->writeBinary( identifier_ + ".indices", f.output() );

		f = QuickFileWriter();

		VertexVector vertices;
		createVertexList( vertices );
		VertexHeader vh;
		vh.nVertices = static_cast<int>(vertices.size());
		bw_snprintf( vh.vertexFormat, sizeof(vh.vertexFormat), "xyznuv" );
		f << vh;
		f << vertices;

		spFile->writeBinary( identifier_ + ".vertices", f.output() );

		spFile->save( primitiveFile );

		DataSectionPtr spRenderSet = spVisualSection->newSection( "renderSet" );
		if (spRenderSet)
		{
			spRenderSet->writeBool( "treatAsWorldSpaceObject", false );
			spRenderSet->writeString( "node", identifier_ );
			DataSectionPtr spGeometry = spRenderSet->newSection( "geometry" );
			if (spGeometry)
			{
				spGeometry->writeString( "vertices", identifier_ + ".vertices" );
				spGeometry->writeString( "primitive", identifier_ + ".indices" );

				MaterialVector::iterator it = materials_.begin();
				MaterialVector::iterator end = materials_.end();

				uint32 primGroup = 0;
				for (uint32 i = 0; i < materials_.size(); i++)
				{
					Material& mat = materials_[i];;
					if (mat.inUse)
					{
						DataSectionPtr spPG = spGeometry->newSection( "primitiveGroup" );
						if (spPG)
						{
							spPG->setInt( primGroup++ );
							spPG->writeString( "material/identifier", mat.identifier );
							if (mat.hasMap)
								spPG->writeString( "material/texture", mat.mapIdentifier );
						}
					}
				}
			}
		}
	}
}


/**
 *	This method adds a vertex to our vertex list.
 *
 *	@param bv Vertex to add to the vertexlist.
 *	@return Index of the vertex in the vector.
 */
uint16 VisualMesh::addVertex( const VisualMesh::BloatVertex& bv )
{
	BVVector::iterator it = std::find( vertices_.begin(), vertices_.end(), bv );

	if (it != vertices_.end())
	{
		it->smoothingGroup |= bv.smoothingGroup;
		uint16 i = 0;
		while (it != vertices_.begin())
		{
			++i;
			--it;
		}
		return i;
	}
	else
	{
		vertices_.push_back( bv );
		return static_cast<uint16>(vertices_.size() - 1);
	}
}

/**
 *	This method initalises the visual mesh.
 *
 *	@param node The node of the mesh we want to export.
 *	@return	Success or failure.
 */
bool VisualMesh::init( INode* node )
{
	bool needDel = false;
	ConditionalDeleteOnDestruct<TriObject> triObject( MFXExport::getTriObject( node, timeNow(), needDel));
	if (! (&*triObject))
		return false;
	triObject.del( needDel );
	Mesh* mesh = &triObject->mesh;

	bool ret = false;
	if (mesh->getNumFaces() && mesh->getNumVerts())
	{
#ifdef _UNICODE
		identifier_ = trimWhitespaces( bw_wtoutf8( node->GetName() ) );
#else
		identifier_ = trimWhitespaces( node->GetName() );
#endif
		
		gatherMaterials( node );
		bool hasUVs = mesh->getNumTVerts() && mesh->tvFace;
		BloatVertex bv;
		bv.normal = Point3::Origin;
		bv.uv = Point2::Origin;

		Matrix3 meshMatrix = node->GetObjectTM( timeNow() );
		Matrix3 nodeMatrix = node->GetNodeTM( timeNow() );
		if( !ExportSettings::instance().allowScale() )
			nodeMatrix = normaliseMatrix( nodeMatrix );

		Matrix3 scaleMatrix = meshMatrix * Inverse( nodeMatrix );

		for (int i = 0; i < mesh->getNumFaces(); i++)
		{
			Face f = mesh->faces[ i ];

			Triangle tr;
			tr.materialIndex = materialRemap_[ f.getMatID() % materialRemap_.size() ];
			tr.smoothingGroup = f.smGroup;
			for (int j = 0; j < 3; j++)
			{
				bv.pos = scaleMatrix * mesh->verts[ f.v[j] ];
				bv.vertexIndex = f.v[j];
				bv.smoothingGroup = f.smGroup;
				if (hasUVs)
				{
					bv.uv = reinterpret_cast<Point2&>( mesh->tVerts[ mesh->tvFace[i].t[j] ] );
					bv.uv.y = 1 - bv.uv.y;
				}
				tr.index[j] = addVertex( bv );
				tr.realIndex[j] = bv.vertexIndex;
			}
			triangles_.push_back( tr );
		}

		bool nodeMirrored = isMirrored( nodeMatrix );
		bool meshMirrored = isMirrored( meshMatrix );

		if (nodeMirrored ^ meshMirrored)
			flipTriangleWindingOrder();

/*		if (isMirrored( meshMatrix ))
			flipTriangleOrder();

		if (isMirrored( nodeMatrix ))
			flipTriangleWindingOrder();*/

		createNormals( );

		// initialise boundingbox
		bb_.setBounds(
			reinterpret_cast<const Vector3&>( meshMatrix * mesh->verts[ 0 ] ),
			reinterpret_cast<const Vector3&>( meshMatrix * mesh->verts[ 0 ] ));
		for (int i = 1; i < mesh->getNumVerts(); i++)
		{
			bb_.addBounds( reinterpret_cast<const Vector3&>( meshMatrix * mesh->verts[ i ] ) );
		}

		if (nodeMirrored)
			flipTriangleWindingOrder();

		ret = true;
	}

	return ret;
}

BW_END_NAMESPACE

// visual_mesh.cpp
