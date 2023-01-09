#include "pch.hpp"
// Raymond : Merge TC's optimisation {
#define TC_OPTIMISATION
#include "cstdmf/bw_unordered_map.hpp"
// }

#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4530 )

#include <algorithm>
//~ #include "mfxexp.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/quick_file_writer.hpp"
#include "resmgr/xml_section.hpp"
#include "utility.hpp"
#include "expsets.hpp"
//~ //#include "wm3.h"

#include "visual_mesh.hpp"

#include "NVMeshMender.h"

#ifndef CODE_INLINE
#include "visual_mesh.ipp"
#endif

#include "resmgr/auto_config.hpp"

#include "cstdmf/log_msg.hpp"

BW_BEGIN_NAMESPACE

static AutoConfigString s_defaultMaterial( "exporter/defaultMaterial" );
static AutoConfigString s_defaultSkinnedMaterial( "exporter/defaultSkinnedMaterial" );

static inline uint32 packNormal( const vector3<float>&nn )
{
	vector3<float> n = nn;
	n.normalise();

	n.x = Math::clamp(-1.f, n.x, 1.f);
	n.y = Math::clamp(-1.f, n.y, 1.f);
	n.z = Math::clamp(-1.f, n.z, 1.f);
	
	// Flipped by the Mesh::initialise method
return	( ( ( (uint32)(n.z * 511.0f) )  & 0x3ff ) << 22L ) |
		( ( ( (uint32)(n.y * 1023.0f) ) & 0x7ff ) << 11L ) |
		( ( ( (uint32)(n.x * 1023.0f) ) & 0x7ff ) <<  0L );
}

//~ TimeValue timeNow()
//~ {
	//~ // FIX
	//~ return 0;//~ ExportSettings::instance().staticFrame() * GetTicksPerFrame();
//~ }

// -----------------------------------------------------------------------------
// Section: VisualMesh
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
VisualMesh::VisualMesh() :
	identifier_( "" ),
	nodeIdentifier_( "Scene Root" ),
	snapVertices_( false ),
	largeIndices_( false ),
	dualUV_( false ),
	vertexColours_( false )
{
}


/**
 *	Destructor.
 */
VisualMesh::~VisualMesh()
{
}

//return all resources used
void VisualMesh::resources( BW::vector<BW::string>& retResources )
{
	MaterialVector::iterator it = materials_.begin();
	MaterialVector::iterator end = materials_.end();
	while (it != end)
	{
		Material& m = *it++;
		m.resources( retResources );
	}
}

/**
 * This method finds the angle between two edges in a triangle
 * @param tri the triangle
 * @param index the vertex reference index to find the angle from
 * @return the angle (in radians) between the two edges.
 */
float VisualMesh::normalAngle( const Triangle& tri, uint32 index )
{
	vector3<float> v1 = vertices_[tri.index[(index + 1 ) % 3]].pos - vertices_[tri.index[index]].pos;
	vector3<float> v2 = vertices_[tri.index[(index + 2 ) % 3]].pos - vertices_[tri.index[index]].pos;

	// FIX - check
	//~ float len = Length( v1 );
	float len = v1.magnitude();
	
	if (len == 0)
		return 0;
	v1 /= len;

	// FIX - check
	//~ len = Length( v2 );
	len = v2.magnitude();
	if (len == 0)
		return 0;
	v2 /= len;

	float normalAngle = v1.dot( v2 );

	normalAngle = min( 1.f, max( normalAngle, -1.f ) );

	return acosf( normalAngle );
}

/**
 * This method adds a normal to all vertices that should be influenced by it.
 * @param normal the normal to add
 * @param realIndex the index of the vertex in the max mesh.
 * @param smoothingGroup the smoothinggroup of this normal.
 * @param index the index of a vertex this normal has to influence.
 */
void VisualMesh::addNormal( const vector3<float>& normal, int realIndex, int smoothingGroup, int index )
{
	for (int i = 0; i < (int)vertices_.size(); i++)
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

//~ /**
 //~ * This method creates vertex normals for all vertices in the vertex list.
 //~ */
//~ void VisualMesh::createNormals( )
//~ {
	//~ TriangleVector::iterator it = triangles_.begin();
	//~ TriangleVector::iterator end = triangles_.end();

	//~ while (it!=end)
	//~ {
		//~ Triangle& tri = *it++;
		//~ vector3<float> v1 = vertices_[tri.index[1]].pos - vertices_[tri.index[0]].pos;
		//~ vector3<float> v2 = vertices_[tri.index[2]].pos - vertices_[tri.index[0]].pos;
		//~ // FIX
		//~ vector3<float> normal = v1.cross( v2 );//~v1^v2;
		//~ // FIX
		//~ normal.normalise();//~ = Normalize( normal );

		//~ addNormal( normal * normalAngle( tri, 0), tri.realIndex[0], tri.smoothingGroup, tri.index[0] );
		//~ addNormal( normal * normalAngle( tri, 1), tri.realIndex[1], tri.smoothingGroup, tri.index[1] );
		//~ addNormal( normal * normalAngle( tri, 2), tri.realIndex[2], tri.smoothingGroup, tri.index[2] );
	//~ }

	//~ BVVector::iterator vit = vertices_.begin();
	//~ BVVector::iterator vend = vertices_.end();
	//~ while (vit != vend)
	//~ {
		//~ BloatVertex& bv = *vit++;
		//~ // FIX
		//~ bv.normal.normalise(); //~ = Normalize( bv.normal );
	//~ }
//~ }

/**
 * This method gets a max material and stores the material data we want to export.
 */
void VisualMesh::addMaterial( material& mtl )
{
	Material m;
	// FIX - removed colours for now
	//~ m.a
	//~ m.diffuse = mtl->GetDiffuse( timeNow() );
	//~ m.specular = mtl->GetSpecular( timeNow() );
	//~ m.selfIllum = mtl->GetSelfIllum( timeNow() );
	m.identifier = mtl.name;//~ mtl->GetName();

	// material related names in max...
	//MtlBase->GetName()
	//MtlBase->GetFullName()
	//BitmapTex->GetMapName()
	//MtlBase->GetSubTexmapSlotName(int i)
	//MtlBase->GetSubTexmapTVName(int i)
	//Mtl->GetSubMtlSlotName(int i)
	//Mtl->GetSubMtlTVName(int i)
	//StdMat2->GetMapName(int i)

	// FIX
	//~ if( mtl->MapEnabled( ID_DI ) )
	//~ {
		//~ Texmap *map = mtl->GetSubTexmap( ID_DI );
		//~ if (map && map->ClassID() == Class_ID( BMTEX_CLASS_ID, 0x00 ))
		//~ {
			//~ BW::string submapName =
				//~ unifySlashes( toLower( BW::string( map->GetName() ) ) );
			//~ BW::string textureName =
					//~ unifySlashes( toLower( ((BitmapTex *)map)->GetMapName() ) );

			//~ if (submapName.length() > 4 &&
				//~ submapName.substr( submapName.length()-4 ) == ".mfm")
			//~ {
				//~ m.mapIdentifier = submapName;
				//~ m.mapIdMeaning = 2;
			//~ }
			//~ else
			//~ {
				//~ textureName = BWResolver::dissolveFilename( textureName );
				//~ if (textureName.length())
				//~ {
					//~ m.mapIdentifier = textureName;
					//~ m.mapIdMeaning = 1;
				//~ }
			//~ }
		//~ }
	//~ }

	// Replace environment macros in the material map file, place in texture name ('%MACRONAME%' -> '<whatever is in environment>')
	BW::string textureName;
	bool success = replaceEnvironmentMacros( mtl.mapFile, &textureName );
	if(!success)
	{	
		BW::string errMsg(
			"Material \"" + mtl.name + "\" has an invalid texture name:\n\n\"" + mtl.mapFile + "\"\n\n" +
			"Can't resolve this name, have you configured all the environment variables in your maya.env?\n" +
			"Texture may not be visible in game.\n");
		if ( ! LogMsg::automatedTest() )
		{
			::MessageBoxA(	0,
					errMsg.c_str(),
					"Visual Exporter Warning!",
					MB_ICONWARNING );
		}
		addError( errMsg.c_str() );
	}
	
	// Dissolve filename from an absolute to a relative path, if appropriate. Also changes all '\' to '/'.
	BW::string identifier = BWResolver::dissolveFilename( textureName );

	if (ExportSettings::instance().copyExternalTextures()&&
		BWUtil::isAbsolutePath(identifier))
	{
		if (ExportSettings::instance().copyTexturesTo().empty())
		{
			identifier = "";
		}
		else
		{
			StringRef textureFileName = BWResource::getFilename( textureName );
			BW::string newTextureName = 
				ExportSettings::instance().copyTexturesTo() + "/" + textureFileName;
			if (BWResource::instance().fileSystem()->copyFileOrDirectory(
				textureName, newTextureName ))
			{
				identifier = BWResolver::dissolveFilename( newTextureName );
			}
			else
			{
				BW::string errMsg(
					"Could not copy texture " + textureName + " to " +
					newTextureName + "\n");
				addError( errMsg.c_str() );
			}
		}
	}

	if (textureName.length())
	{
		m.mapIdentifier = identifier;
		m.mapIdMeaning = mtl.mapIdMeaning;
	}

	materialRemap_.push_back( this->findOrAddMaterial( m ) );
}

/**
 * This method gathers all the materials contained in a specific node.
 */
void VisualMesh::gatherMaterials( Mesh& mesh )
{
	for( uint32 i = 0; i < mesh.materials().size(); ++i )
		addMaterial( mesh.materials()[i] );
	//~ Mtl* mtl = node->GetMtl();
	//~ if (mtl)
	//~ {
		//~ if (mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0))
		//~ {
			//~ addMaterial( (StdMat*)mtl );
		//~ }
		//~ else if (mtl->NumSubMtls())
		//~ {
			//~ for (int i = 0; i < mtl->NumSubMtls(); i++)
			//~ {
				//~ Mtl* submat = mtl->GetSubMtl( i );
				//~ if (submat == NULL)
				//~ {
					//~ Material m;
					//~ materials_.push_back( m );
				//~ }
				//~ if (submat->ClassID() == Class_ID(DMTL_CLASS_ID, 0))
				//~ {
					//~ addMaterial( (StdMat*)submat );
				//~ }
				//~ else
				//~ {
					//~ Material m;
					//~ m.identifier = submat->GetName();
					//~ materials_.push_back( m );
				//~ }
			//~ }
		//~ }
		//~ else
		//~ {
			//~ Material m;
			//~ m.identifier = mtl->GetName();
			//~ materials_.push_back( m );
		//~ }
	//~ }
	//~ else
	//~ {
		//~ Material m;
		//~ materials_.push_back( m );
	//~ }

	if (!materialRemap_.size())
	{
		materialRemap_.push_back( 0 );
	}
}


/**
 *	This method finds or adds the given visual material to our list
 */
int VisualMesh::findOrAddMaterial( const Material & mat )
{
	uint32 i;
	for (i = 0; i < materials_.size(); i++)
	{
		if (materials_[i] == mat) return i;
	}

	materials_.push_back( mat );
	return i;
}


/**
 * This method swaps the triangle winding order for all triangles.
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



VisualMesh::BVVector* g_verts = 0;

bool triangleSorter( const VisualMesh::Triangle& t1, const VisualMesh::Triangle& t2 )
{
	//return t1.materialIndex < t2.materialIndex;

	if (t1.materialIndex == t2.materialIndex)
		return (*g_verts)[t1.index[0]].meshIndex < (*g_verts)[t2.index[0]].meshIndex;
	else
		return t1.materialIndex < t2.materialIndex;
}

/**
 * This method sorts the triangles.
 */
void VisualMesh::sortTriangles()
{
	g_verts = &vertices_;
	std::sort( triangles_.begin(), triangles_.end(), triangleSorter );

	if (triangles_.size())
	{
		BW::vector<int> vertexIndices(vertices_.size(), -1);
		BVVector vs;

		int mIndex = triangles_.front().materialIndex;
		for (int i = 0; i <= (int)triangles_.size(); i++)
		{
			if (i == triangles_.size() || triangles_[i].materialIndex != mIndex)
			{
				if( i != triangles_.size() )
					mIndex = triangles_[i].materialIndex;
				vertexIndices.resize( 0 );
				vertexIndices.resize( vertices_.size(), -1 );
			}
			if (i!=triangles_.size())
			{
				Triangle& tri = triangles_[i];
				for (int j = 0; j < 3; j++)
				{
					int ind = tri.index[j];
					if (vertexIndices[ind] == -1)
					{
						vertexIndices[ind] = static_cast<int>(vs.size());
						vs.push_back( vertices_[ind] );
					}
					tri.index[j] = vertexIndices[ind];
				}
			}
		}
		vertices_ = vs;
	}
}

/**
 * This method creates our primitive groups.
 * @param primGroups the output primitivegroups
 * @param indices the output indices
 */
bool VisualMesh::createPrimitiveGroups( PGVector& primGroups, Moo::IndicesHolder& indices )
{
	// If there are no materials return
	if (materials_.size() == 0)
	{
		BW::string errMsg(
			identifier_ + " does not have any materials.\n" +
			"This object will not be exported.  Adding a material will fix this problem.");
		if ( ! LogMsg::automatedTest() )
		{
			::MessageBox(	0,
						bw_utf8tow( errMsg ).c_str(),
						L"Material error!",
						MB_ICONERROR );
		}
		addError( errMsg.c_str() );
		return false;
	}

	if (triangles_.size())
	{
		int mIndex = triangles_.front().materialIndex;

		int firstVertex = triangles_.front().index[0];
		int lastVertex = triangles_.front().index[0];
		int firstTriangle = 0;

		int offset = 0;
		indices.setSize( static_cast<uint32>(triangles_.size() * 3),
			Moo::IndicesReference::bestFormat( static_cast<uint32>(vertices_.size()) ) );

		for (int i = 0; i <= (int)triangles_.size(); i++)
		{
			Triangle& tri = i == triangles_.size() ? triangles_.back() : triangles_[i];
			if (i == triangles_.size() || tri.materialIndex != mIndex)
			{
				Moo::PrimitiveGroup pg;
				pg.startVertex_ = firstVertex;
				pg.nVertices_ = lastVertex - firstVertex + 1;
				pg.startIndex_ = firstTriangle * 3;
				pg.nPrimitives_ = i - firstTriangle;
				primGroups.push_back( pg );	
				materials_[ mIndex ].inUse = true;
				if( i != triangles_.size() )
				{
					firstVertex = tri.index[0];
					lastVertex = tri.index[0];
					mIndex = tri.materialIndex;
				}
				firstTriangle = i;
			}
			if (i!=triangles_.size())
			{
				indices.set( offset, tri.index[0] );
				++offset;
				indices.set( offset, tri.index[1] );
				++offset;
				indices.set( offset, tri.index[2] );
				++offset;
				firstVertex = min( firstVertex, min( tri.index[0], min( tri.index[1], tri.index[2] ) ) );
				lastVertex  = max( lastVertex,  max( tri.index[0], max( tri.index[1], tri.index[2] ) ) );
			}
		}

		//Mesh particles add an extra 15 primitive groups, defined by the index
		//of the vertices.
		if (ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES)
		{
			return this->createMeshParticlePrimGroups(primGroups);		
		}
	}
	return true;
}


/**
 *	This method adds 15 primitive groups, based on meshIndex.
 *	This allows the mesh particles to be drawn all together (prim group 0)
 *	or one-by-one (prim group 1 - 15 incl.), which is used when doing
 *	slower, correct sorting of alpha-blended mesh particles.
 */
bool VisualMesh::createMeshParticlePrimGroups( PGVector& primGroups )
{
	//Mesh particles should have one primitive group, representing the
	//entire mesh.
	//MF_ASSERT( primGroups.size() == 1 )
	if (primGroups.size() != 1)
	{
		if ( ! LogMsg::automatedTest() )
		{
			MessageBox( GetForegroundWindow(),
				L"VisualMesh::createMeshParticlePrimGroups - Mesh particles must use only 1 material/mesh.\n",
				L"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );
		}
		addError( "VisualMesh::createMeshParticlePrimGroups - "\
					"Mesh particles must use only 1 material/mesh." );
		return false;
	}

	if (triangles_.size())
	{
		Triangle& tri = triangles_[0];
		
		int firstVertex = tri.index[0];
		int lastVertex = firstVertex;
		int mIndex = vertices_[firstVertex].meshIndex;
		int firstTriangle = 0;

		for (uint32 i = 0; i <= triangles_.size(); i++)
		{
			Triangle& tri = i == triangles_.size() ? triangles_.back() : triangles_[i];
			if (i == triangles_.size() || vertices_[tri.index[0]].meshIndex != mIndex)
			{
				Moo::PrimitiveGroup pg;
				pg.startVertex_ = firstVertex;
				pg.nVertices_ = lastVertex - firstVertex + 1;
				pg.startIndex_ = firstTriangle * 3;
				pg.nPrimitives_ = i - firstTriangle;
				primGroups.push_back( pg );

				firstVertex = lastVertex = tri.index[0];
				mIndex = vertices_[firstVertex].meshIndex;
				firstTriangle = i;
			}

			firstVertex = min( firstVertex, min( tri.index[0], min( tri.index[1], tri.index[2] ) ) );
			lastVertex  = max( lastVertex,  max( tri.index[0], max( tri.index[1], tri.index[2] ) ) );
		}
	}

	size_t nPrimGroups = primGroups.size();

	//Mesh particles should now always have 16 primitive groups.
	//Made this an assert because the MeshParticleRenderer assumes
	//there are 16 primitivegroups.
	MF_ASSERT( primGroups.size() == 16 )

	return true;
}


/**
 * This method creates our output vertex list.
 * @vertices the output vertex list.
 */
void VisualMesh::createVertexList( VertexVector& vertices )
{
	Moo::VertexXYZNUV v;
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();
	while (it!=end)
	{
		BloatVertex& bv = *it++;
		bv.pos *= ExportSettings::instance().unitScale();

		// Flipped by the Mesh::initialise method
		v.pos_[0] = bv.pos.x;
		v.pos_[1] = bv.pos.y;
		v.pos_[2] = bv.pos.z;
		v.normal_[0] = bv.normal.x;
		v.normal_[1] = bv.normal.y;
		v.normal_[2] = bv.normal.z;
		v.uv_[0] = bv.uv.u;
		v.uv_[1] = bv.uv.v;
		vertices.push_back( v );
	}
}


/**
 * This method creates our output vertex list.
 * @vertices the output vertex list.
 */
void VisualMesh::createVertexList( UV2VertexVector& vertices )
{
	Moo::VertexXYZNUV2 v;
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();
	while (it!=end)
	{
		BloatVertex& bv = *it++;
		bv.pos *= ExportSettings::instance().unitScale();

		// Flipped by the Mesh::initialise method
		v.pos_[0] = bv.pos.x;
		v.pos_[1] = bv.pos.y;
		v.pos_[2] = bv.pos.z;
		v.normal_[0] = bv.normal.x;
		v.normal_[1] = bv.normal.y;
		v.normal_[2] = bv.normal.z;
		v.uv_[0] = bv.uv.u;
		v.uv_[1] = bv.uv.v;
		v.uv2_[0] = bv.uv2.u;
		v.uv2_[1] = bv.uv2.v;

		vertices.push_back( v );
	}
}


/**
 * This method creates our output vertex list.
 * @vertices the output vertex list.
 */
void VisualMesh::createVertexList( DiffuseVertexVector& vertices )
{
	Moo::VertexXYZNDUV v;
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();
	while (it!=end)
	{
		BloatVertex& bv = *it++;
		bv.pos *= ExportSettings::instance().unitScale();

		// Flipped by the Mesh::initialise method
		v.pos_[0] = bv.pos.x;
		v.pos_[1] = bv.pos.y;
		v.pos_[2] = bv.pos.z;
		v.normal_[0] = bv.normal.x;
		v.normal_[1] = bv.normal.y;
		v.normal_[2] = bv.normal.z;
		v.uv_[0] = bv.uv.u;
		v.uv_[1] = bv.uv.v;

		v.colour_ = D3DCOLOR_COLORVALUE(bv.colour.x,bv.colour.y,bv.colour.z,bv.colour.w);

		vertices.push_back( v );
	}
}


void VisualMesh::createVertexList( IndexedVertexVector& vertices )
{
	Moo::VertexXYZNUVI v;

	BVVector::iterator lastit = vertices_.begin();
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();

	while (it!=end)
	{
		MF_ASSERT( lastit->meshIndex <= it->meshIndex );
		lastit = it;

		BloatVertex& bv = *it++;

		bv.pos *= ExportSettings::instance().unitScale();

		// Flipped by the Mesh::initialise method
		v.pos_[0] = bv.pos.x;
		v.pos_[1] = bv.pos.y;
		v.pos_[2] = bv.pos.z;
		v.normal_[0] = bv.normal.x;
		v.normal_[1] = bv.normal.y;
		v.normal_[2] = bv.normal.z;
		v.uv_[0] = bv.uv.u;
		v.uv_[1] = bv.uv.v;
		v.index_ = (float) bv.meshIndex * 3;

		vertices.push_back( v );
	}
}

void VisualMesh::createVertexList( TBVertexVector& vertices )
{
	Moo::VertexXYZNUVTB v;
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();
	while (it!=end)
	{
		BloatVertex& bv = *it++;
		bv.pos *= ExportSettings::instance().unitScale();

		// Flipped by the Mesh::initialise method
		v.pos_[0] = bv.pos.x;
		v.pos_[1] = bv.pos.y;
		v.pos_[2] = bv.pos.z;
		v.normal_ = packNormal( bv.normal );
		v.uv_[0] = bv.uv.u;
		v.uv_[1] = bv.uv.v;
		v.tangent_ = packNormal( bv.tangent );
		v.binormal_ = packNormal( bv.binormal );
		vertices.push_back( v );
	}
}

struct PG
{
	VisualMesh::BVVector verts;
	VisualMesh::TriangleVector tris;
};

void VisualMesh::makeBumped()
{
	if (triangles_.size())
	{
		// separate the primitive groups.
		BW::vector< PG > pgs;

		int mIndex = triangles_.front().materialIndex;

		int firstVertex = triangles_.front().index[0];
		int lastVertex = triangles_.front().index[0];
		int firstTriangle = 0;

		for (int i = 0; i <= (int)triangles_.size(); i++)
		{
			Triangle& tri = (i == triangles_.size()) ? triangles_.back() : triangles_[i];
			if (i == triangles_.size() || tri.materialIndex != mIndex)
			{
				PG pg;
				pg.verts.assign( vertices_.begin() + firstVertex, vertices_.begin() + lastVertex + 1 );
				for (int j = firstTriangle; j < i; j++)
				{
					Triangle t = triangles_[ j ];
					t.index[0] -= firstVertex;
					t.index[1] -= firstVertex;
					t.index[2] -= firstVertex;
					pg.tris.push_back( t );
				}

				if (i != triangles_.size())
				{
					firstVertex = tri.index[0];
					lastVertex = tri.index[0];
					firstTriangle = i;
					mIndex = tri.materialIndex;
				}

				pgs.push_back( pg );
			}
			if (i!=triangles_.size())
			{
				firstVertex = min( firstVertex, min( tri.index[0], min( tri.index[1], tri.index[2] ) ) );
				lastVertex  = max( lastVertex,  max( tri.index[0], max( tri.index[1], tri.index[2] ) ) );
			}
		}

		// bump map the primitivegroups individually
		for (int i = 0; i < (int)pgs.size(); i++)
		{
			PG& p = pgs[i];
			TriangleVector& tris = p.tris;
			BVVector& verts = p.verts;

			BW::vector< MeshMender::Vertex > theVerts;
			BW::vector< unsigned int > theIndices;
			BW::vector< unsigned int > mappingNewToOld;

			//fill up the vectors with your mesh's data
			for( int j = 0; j < (int)verts.size(); ++j )
			{
				MeshMender::Vertex v;
				v.pos.x = verts[j].pos.x;
				v.pos.y = verts[j].pos.y;
				v.pos.z = verts[j].pos.z;
				v.normal.x = verts[j].normal.x;
				v.normal.y = verts[j].normal.y;
				v.normal.z = verts[j].normal.z;
				v.binormal.x = verts[j].binormal.x;
				v.binormal.y = verts[j].binormal.y;
				v.binormal.z = verts[j].binormal.z;
				v.tangent.x = verts[j].tangent.x;
				v.tangent.y = verts[j].tangent.y;
				v.tangent.z = verts[j].tangent.z;
				v.s = verts[j].uv.u;
				v.t = 1.f - verts[j].uv.v;
				//meshmender will computer normals, tangents, and binormals, no need to fill those in.
				//however, if you do not have meshmender compute the normals, you _must_ pass in valid
				//normals to meshmender
				theVerts.push_back(v);
			}

			for (int j = 0; j < (int)tris.size(); j++)
			{
				theIndices.push_back( tris[j].index[0] );
				theIndices.push_back( tris[j].index[1] );
				theIndices.push_back( tris[j].index[2] );
			}

			MeshMender mender;
			//pass it in to the mender to do it's stuff
			mender.Mend( theVerts,  theIndices, mappingNewToOld,
						0.5f,
						0.5f,
						0.5f,
						0,
						MeshMender::DONT_CALCULATE_NORMALS_OR_BINORMALS,
						MeshMender::DONT_RESPECT_SPLITS,
						ExportSettings::instance().fixCylindrical() ?
							MeshMender::FIX_CYLINDRICAL :
							MeshMender::DONT_FIX_CYLINDRICAL );

			BVVector newVerts; // new vector as we still need to copy the extra data from verts
			for( int j = 0; j < (int)theVerts.size(); ++j )
			{
				BloatVertex bv;
				bv.pos.x = theVerts[j].pos.x;
				bv.pos.y = theVerts[j].pos.y;
				bv.pos.z = theVerts[j].pos.z;
				bv.normal.x = theVerts[j].normal.x;
				bv.normal.y = theVerts[j].normal.y;
				bv.normal.z = theVerts[j].normal.z;
				bv.binormal.x = theVerts[j].binormal.x;
				bv.binormal.y = theVerts[j].binormal.y;
				bv.binormal.z = theVerts[j].binormal.z;
				bv.tangent.x = theVerts[j].tangent.x;
				bv.tangent.y = theVerts[j].tangent.y;
				bv.tangent.z = theVerts[j].tangent.z;
				bv.uv.u = theVerts[j].s;
				bv.uv.v = 1.f - theVerts[j].t;
				
				bv.uv2 = verts[ mappingNewToOld[j] ].uv2;

				bv.colour = verts[ mappingNewToOld[j] ].colour;

				bv.smoothingGroup = verts[ mappingNewToOld[j] ].smoothingGroup;
				bv.vertexIndex = verts[ mappingNewToOld[j] ].vertexIndex;
				newVerts.push_back( bv );
			}

			verts.clear();

			for( int j = 0; j < (int)newVerts.size(); ++j )
				verts.push_back( newVerts[j] );

			Triangle tri = tris.front();
			tris.clear();
			for( int j = 0; j < (int)theIndices.size(); j += 3 )
			{
				tri.index[0] = theIndices[j];
				tri.index[1] = theIndices[j + 1];
				tri.index[2] = theIndices[j + 2];
				tris.push_back( tri );
			}
		}

		// stitch the primitivegroups back together
		vertices_.clear();
		triangles_.clear();

		for (int i = 0; i < (int)pgs.size(); i++)
		{
			PG& p = pgs[i];
			TriangleVector& tris = p.tris;
			BVVector& verts = p.verts;
			int firstVert = static_cast<int>(vertices_.size());

			vertices_.insert( vertices_.end(), verts.begin(), verts.end() );
			for (int j = 0; j < (int)tris.size(); j++)
			{
				Triangle tri = tris[j];
				tri.index[0] += firstVert;
				tri.index[1] += firstVert;
				tri.index[2] += firstVert;
				triangles_.push_back( tri );
			}
		}
	}
}

static bool closeEnough( float a, float b )
{
	return fabs( a - b ) < 0.0001f;
}

static bool pointCloseEnough( Vector4 a, Vector4 b )
{
	return closeEnough( a.x, b.x )
		&& closeEnough( a.y, b.y )
		&& closeEnough( a.z, b.z )
		&& closeEnough( a.w, b.w );
}

static bool pointCloseEnough( vector3<float> a, vector3<float> b )
{
	return closeEnough( a.x, b.x )
		&& closeEnough( a.y, b.y )
		&& closeEnough( a.z, b.z );
}

static bool pointCloseEnough( Point2 a, Point2 b )
{
	return closeEnough( a.u, b.u )
		&& closeEnough( a.v, b.v );
}

static bool directionCloseEnough( vector3<float> a, vector3<float> b )
{
	// 0.02 == 2 * pi / 360 (roughly);
	return (a.dot( b ) > cos(0.02)) || (a == b);
}

static bool closeEnough( const VisualMesh::BloatVertex& a, const VisualMesh::BloatVertex& b )
{
	return  pointCloseEnough( a.pos, b.pos ) &&
		directionCloseEnough( a.normal, b.normal ) &&
		pointCloseEnough( a.uv, b.uv ) &&
		pointCloseEnough( a.uv2, b.uv2 ) &&
		pointCloseEnough( a.colour, b.colour ) &&
//		a.smoothingGroup == b.smoothingGroup &&
		directionCloseEnough( a.binormal, b.binormal ) &&
		directionCloseEnough( a.tangent, b.tangent ) &&
		a.meshIndex == b.meshIndex;
}

// Raymond : Merge TC's optimisation {
#ifdef TC_OPTIMISATION
inline uint64 hashKey( const vector3<float>& v )
{
	uint64 x = uint64(v.x);
	uint64 y = uint64(v.y);
	uint64 z = uint64(v.z);
	x <<= 20;
	y <<= 10;
	return (x + y + z);
}
#endif//TC_OPTIMISATION
// }

void VisualMesh::removeDuplicateVertices()
{
// Raymond : Merge TC's optimisation {
#ifdef TC_OPTIMISATION
	int j1 = 0;
// In VS 2002, hash_multimap is inside namespace std
// In VS 2003, hash_multimap is inside namespace stdext
// I cannot find an elegant way for that, so MACRO is the rescue
#if	_MSC_VER < 1300
	#error this file is intended to be compiled with VC 2002 or above
#endif//_MSC_VER < 1300

	BW::unordered_multimap <uint64, int> vertexMap;
	BW::unordered_multimap <uint64, int> :: const_iterator lowerBound, upperBound, iter;
	BW::unordered_map <int, int> vertexRemap;
	typedef std::pair <uint64, int> Int64_Pair;
	typedef std::pair <int, int> Int_Pair;
	for (uint j1 = 0; j1 < vertices_.size(); j1++)
	{
		BloatVertex& bv1 = vertices_[j1];
		uint64 key = hashKey( bv1.pos );
		int found = -1;
		lowerBound = vertexMap.lower_bound( key );
		upperBound = vertexMap.upper_bound( key );
		for (iter = lowerBound; iter != upperBound; iter++)
		{
			int hashedIndex = iter->second;
			BloatVertex& bv2 = vertices_[ hashedIndex ];
			if (closeEnough( bv1, bv2 ))
			{
				found = hashedIndex;
				break;
			}
		}
		if (found == -1)
		{
			vertexMap.insert( Int64_Pair( key, j1 ) );
			vertexRemap.insert( Int_Pair( j1, j1 ) );
		}
		else
			vertexRemap.insert( Int_Pair( j1, found ) );
	}
#endif//TC_OPTIMISATION
// }

	int nDupes = 0;
	// iterate through all triangles
	for (int i = 0; i < (int)triangles_.size(); ++i)
	{
		Triangle& tri = triangles_[i];
// Raymond : Merge TC's optimisation {
#ifdef TC_OPTIMISATION
		tri.index[0] = vertexRemap[tri.index[0]];
		tri.index[1] = vertexRemap[tri.index[1]];
		tri.index[2] = vertexRemap[tri.index[2]];
#endif//TC_OPTIMISATION
#ifndef TC_OPTIMISATION
// }


		// for each vertex
		for (int j = 0; j < 3; ++j)
		{
			// find if there is an earlier vertex that matches
			for (int k = 0; k < tri.index[j]; ++k)
			{
				// make the triangle point at that one instead
				if (closeEnough( vertices_[tri.index[j]], vertices_[k] ))
					tri.index[j] = k;
			}
		}
// Raymond : Merge TC's optimisation {
#endif//TC_OPTIMISATION
// }
	}

	// (sortTriangles will removed the unreferenced vertices)
}

/**
 * This method saves the visual mesh.
 * @param spVisualSection DataSection of the .visual file.
 * @param spExistingVisual DataSection of the existing .visual file.  May be 0
 * @param primitiveFile the filename of the primitive record.
 */
bool VisualMesh::save( DataSectionPtr spVisualSection,
	DataSectionPtr spExistingVisual,
	const BW::string& primitiveFile,
	bool useIdentifier )
{
	BW::string idPrefix;

	if (useIdentifier)
		idPrefix = identifier_ + ".";
	
	DataSectionPtr spFile = BWResource::openSection( primitiveFile );
	if (!spFile)
		spFile = new BinSection( primitiveFile, new BinaryBlock( 0, 0, "BinaryBlock/VisualMesh" ) );

	if (spFile)
	{
		flipTriangleWindingOrder();
		removeDuplicateVertices();
		sortTriangles();

		if (ExportSettings::instance().bumpMapped() &&
			!(ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES))
		{
			makeBumped();
		}

		removeDuplicateVertices();
		sortTriangles();

		PGVector primGroups;
		Moo::IndicesHolder indices;

		if (!createPrimitiveGroups( primGroups, indices ))
			return false;

		Moo::IndexHeader ih;
		largeIndices_ = indices.entrySize() != 2;
		bw_snprintf( ih.indexFormat_, sizeof(ih.indexFormat_),
			largeIndices_ ? "list32" : "list" );
		ih.nIndices_ = indices.size();
		ih.nTriangleGroups_ = static_cast<int>(primGroups.size());

		QuickFileWriter f;
		f << ih;
		f.write( indices.indices(), indices.size() * indices.entrySize() );
		f << primGroups;

		spFile->writeBinary( idPrefix + "indices", f.output() );
		
		f = QuickFileWriter();

		BW::vector<Vector2> uv2;
		BW::vector<DWORD> colours;
		
		if ( ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES )
		{
			IndexedVertexVector vertices;
			createVertexList( vertices );
			Moo::VertexHeader vh;
			vh.nVertices_ = static_cast<int>(vertices.size());
			bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuvi" );
			f << vh;
			f << vertices;
	
		}
		else if ( ExportSettings::instance().bumpMapped() )
		{
			TBVertexVector vertices;
			createVertexList( vertices );
			Moo::VertexHeader vh;
			vh.nVertices_ = static_cast<int>(vertices.size());
			bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuvtb" );
			f << vh;
			f << vertices;
		}
		else
		{
			VertexVector vertices;
			createVertexList( vertices );
			Moo::VertexHeader vh;
			vh.nVertices_ = static_cast<int>(vertices.size());
			bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuv" );
			f << vh;
			f << vertices;
		}

		spFile->writeBinary( idPrefix + "vertices", f.output() );

		// Write the extra vertex data streams.
		BW::vector<BW::string> streams;
		if (dualUV_)
		{
			BVVector::iterator it = vertices_.begin();
			BVVector::iterator end = vertices_.end();
			while (it!=end)
			{
				BloatVertex& bv = *it++;
				Vector2 uv;
				uv.x = bv.uv2.u;
				uv.y = bv.uv2.v;				
				uv2.push_back( uv );
			}
		
			f = QuickFileWriter();
			f << uv2;			
			
			streams.push_back( idPrefix + "uv2" );

			spFile->writeBinary( streams.back(), f.output() );
		}

		if (vertexColours_)
		{
			BVVector::iterator it = vertices_.begin();
			BVVector::iterator end = vertices_.end();
			while (it!=end)
			{
				BloatVertex& bv = *it++;
				DWORD colour = D3DCOLOR_COLORVALUE(bv.colour.x,bv.colour.y,bv.colour.z,bv.colour.w);
				colours.push_back( colour );
			}
		
			f = QuickFileWriter();
			f << colours;
			
			streams.push_back( idPrefix + "colour" );

			spFile->writeBinary( streams.back(), f.output() );
		}
	
		spFile->save( primitiveFile );

		DataSectionPtr spRenderSet = spVisualSection->newSection( "renderSet" );
		if (spRenderSet)
		{
			spRenderSet->writeBool( "treatAsWorldSpaceObject", false );
			spRenderSet->writeString( "node", nodeIdentifier_ );
			
			DataSectionPtr spGeometry = spRenderSet->newSection( "geometry" );
	
			if (spGeometry)
			{
				spGeometry->writeString( "vertices", idPrefix + "vertices" );

				spGeometry->writeStrings( "stream", streams );
				spGeometry->writeString( "primitive", idPrefix + "indices" );

				MaterialVector::iterator it = materials_.begin();
				MaterialVector::iterator end = materials_.end();
	
				uint32 primGroup = 0;
	
				for (uint32 i = 0; i < materials_.size(); i++)
				{
					Material& mat = materials_[i];;
					if (mat.inUse) mat.save( spGeometry, spExistingVisual, primGroup++ );
				}
			}
		}
	}
	return true;
}

/**
 * This method saves the visual mesh in XML format.  Note that this
 * method is intended for debugging purposes only.
 *
 * @param	xmlFile	The filename of the XML file.
 */
bool VisualMesh::savePrimXml(	const BW::string& xmlFile )
{
	DataSectionPtr spFileRoot = BWResource::openSection( xmlFile );
	if (!spFileRoot)
		spFileRoot = new XMLSection( "VisualMeshPrimXml" );

	DataSectionPtr spFile = spFileRoot->newSection( identifier_ );

	flipTriangleWindingOrder();
	removeDuplicateVertices();
	sortTriangles();

	if (ExportSettings::instance().bumpMapped() && 
		!(ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES))
	{
		makeBumped();
	}

	removeDuplicateVertices();
	sortTriangles();

	PGVector primGroups;
	Moo::IndicesHolder indices;

	if (!createPrimitiveGroups( primGroups, indices ))
		return false;

	Moo::IndexHeader ih;
	largeIndices_ = indices.entrySize() != 2;
	bw_snprintf( ih.indexFormat_, sizeof(ih.indexFormat_),
		largeIndices_ ? "list32" : "list" );
	ih.nIndices_ = indices.size();
	ih.nTriangleGroups_ = static_cast<int>(primGroups.size());

	// Print Index Header contents here
	DataSectionPtr pIH = spFile->newSection( "IndexHeader" );
	pIH->writeString( "IndexFormat", ih.indexFormat_ );
	pIH->writeInt( "NumIndices", ih.nIndices_ );
	pIH->writeInt( "NumPrimGroups", ih.nTriangleGroups_ );

	// Write out the indices
	DataSectionPtr pIndices = spFile->newSection( "Indices" );
	for (uint32 i = 0; i < indices.size(); ++i)
	{
		DataSectionPtr pIndex = pIndices->newSection( "Index" );
		pIndex->setInt( (uint32)indices[ i ] );
	}

	DataSectionPtr pPrimGroups = spFile->newSection( "PrimGroups" );
	PGVector::iterator pgvIt;
	for (pgvIt = primGroups.begin(); pgvIt != primGroups.end(); ++pgvIt)
	{
		DataSectionPtr pPrimGroup = pPrimGroups->newSection( "PrimGroup" );
		pPrimGroup->writeInt( "NumPrimGroups", pgvIt->nPrimitives_ );
		pPrimGroup->writeInt( "NumVertices", pgvIt->nVertices_ );
		pPrimGroup->writeInt( "StartIndex", pgvIt->startIndex_ );
		pPrimGroup->writeInt( "StartVertex", pgvIt->startVertex_ );
	}

	if ( ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES )
	{
		IndexedVertexVector vertices;
		createVertexList( vertices );
		Moo::VertexHeader vh;
		vh.nVertices_ = static_cast<int>(vertices.size());
		bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuvi" );

		// Print Vertex Header contents here
		DataSectionPtr pVH = spFile->newSection( "VertexHeader" );
		pVH->writeString( "VertexFormat", vh.vertexFormat_ );
		pVH->writeInt( "NumVertices", vh.nVertices_ );

		DataSectionPtr pVertices = spFile->newSection( "Vertices" );
		IndexedVertexVector::iterator ivvIt;
		for (ivvIt = vertices.begin(); ivvIt != vertices.end(); ++ivvIt)
		{
			DataSectionPtr pVertex = pVertices->newSection( "Vertex" );
			pVertex->writeVector3( "Pos", Vector3(ivvIt->pos_[0], ivvIt->pos_[1], ivvIt->pos_[2]) );
			pVertex->writeVector3( "Normal", Vector3(ivvIt->normal_[0], ivvIt->normal_[1], ivvIt->normal_[2]) );
			pVertex->writeVector2( "UV", Vector2(ivvIt->uv_[0], ivvIt->uv_[1]) );
			pVertex->writeFloat( "Index", ivvIt->index_ );
		}
	}
	else if ( ExportSettings::instance().bumpMapped() )
	{
		TBVertexVector vertices;
		createVertexList( vertices );
		Moo::VertexHeader vh;
		vh.nVertices_ = static_cast<int>(vertices.size());
		bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuvtb" );

		// Print Vertex Header contents here
		DataSectionPtr pVH = spFile->newSection( "VertexHeader" );
		pVH->writeString( "VertexFormat", vh.vertexFormat_ );
		pVH->writeInt( "NumVertices", vh.nVertices_ );

		DataSectionPtr pVertices = spFile->newSection( "Vertices" );
		TBVertexVector::iterator tbvvIt;
		for (tbvvIt = vertices.begin(); tbvvIt != vertices.end(); ++tbvvIt)
		{
			DataSectionPtr pVertex = pVertices->newSection( "Vertex" );
			pVertex->writeVector3( "Pos", Vector3(tbvvIt->pos_[0], tbvvIt->pos_[1], tbvvIt->pos_[2]) );
			pVertex->writeLong( "Normal", tbvvIt->normal_ );
			pVertex->writeVector2( "UV", Vector2(tbvvIt->uv_[0], tbvvIt->uv_[1]) );
			pVertex->writeInt( "Tangent", tbvvIt->tangent_ );
			pVertex->writeInt( "Binormal", tbvvIt->binormal_ );
		}
	}
	else
	{
		VertexVector vertices;
		createVertexList( vertices );
		Moo::VertexHeader vh;
		vh.nVertices_ = static_cast<int>(vertices.size());
		bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuv" );

		// Print Vertex Header contents here
		DataSectionPtr pVH = spFile->newSection( "VertexHeader" );
		pVH->writeString( "VertexFormat", vh.vertexFormat_ );
		pVH->writeInt( "NumVertices", vh.nVertices_ );

		DataSectionPtr pVertices = spFile->newSection( "Vertices" );
		VertexVector::iterator vvIt;
		for (vvIt = vertices.begin(); vvIt != vertices.end(); ++vvIt)
		{
			DataSectionPtr pVertex = pVertices->newSection( "Vertex" );
			pVertex->writeVector3( "Pos", Vector3(vvIt->pos_[0], vvIt->pos_[1], vvIt->pos_[2]) );
			pVertex->writeVector3( "Normal", Vector3(vvIt->normal_[0], vvIt->normal_[1], vvIt->normal_[2]) );
			pVertex->writeVector2( "UV", Vector2(vvIt->uv_[0], vvIt->uv_[1]) );
		}
	}

	spFileRoot->save( xmlFile );

	return true;
}

//return all resources used by the material.
void VisualMesh::Material::resources( BW::vector<BW::string>& retResources )
{
	/*
	//no fx export on maya
	if (this->fxFile_.length())
	{
		retResources.push_back( unifySlashes( toLower( fxFile_ ) ) );
		for (uint32 i = 0; i < textureOverrides_.size(); i++)
		{
			BW::string textureName = unifySlashes( toLower( textureOverrides_[i].second ) );
			retResources.push_back( textureName );
		}
	}
	else*/
	{
		if (mapIdentifier.length() > 4 && mapIdMeaning != 2)
		{
			BW::string mfmID = mapIdentifier.substr( 0, mapIdentifier.length() - 4 ) + ".mfm";
			DataSectionPtr mfmsect = BWResource::openSection( mfmID );
			if (mfmsect)
			{
				if ((mfmsect->openSection("mfm") ||
					mfmsect->openSection("fx") ||
					mfmsect->openSection("property")) &&
					!(mfmsect->openSection("Src_Blend_type") ||
					mfmsect->openSection("Dest_Blend_type") ||
					mfmsect->openSection("Self_Illumination") ||
					mfmsect->openSection("Alpha_blended")))
				{
					mapIdentifier = mfmID;
					mapIdMeaning = 2;
				}
			}
		}

		if (mapIdMeaning == 1)
		{
			if (mapIdentifier.length())
			{
				retResources.push_back( mapIdentifier );
			}
		}
		else if (mapIdMeaning == 2)
		{
			if (mapIdentifier.length())
			{
				retResources.push_back( mapIdentifier );
			}
		}
		else// mapIdMeaning == 0
		{
			// push back both default materials, as we don't know which one we are using yet
			if( !s_defaultMaterial.value().empty() )
			{
				retResources.push_back( s_defaultMaterial.value() );
			}
			if( !s_defaultSkinnedMaterial.value().empty() )
			{
				retResources.push_back( s_defaultSkinnedMaterial.value() );
			}
		}
	}
}

/**
 *	Save this material description into the given data section.
 *	Tries to copy materials from the existing visual data section, if it
 *	exists, and has materials with the same IDs.
 */
//void VisualMesh::Material::save( DataSectionPtr spGeometry, uint32 pgid, bool skinned, int nodeCount ) const
void VisualMesh::Material::save( DataSectionPtr spGeometry,
								DataSectionPtr spExistingVisual,
								uint32 pgid,
								bool skinned,
								int nodeCount,
								VisualMesh::Material::SkinType skin,
								bool skinnedShader ) const
{
	DataSectionPtr pPG = spGeometry->newSection( "primitiveGroup" );
	if (pPG)
	{
		pPG->setInt( pgid );

		BW::string idPostfix = skin == SOFT_SKIN ? "_skinned" : "";

		// copy existing bitmap/fx combination
		if ( !this->findAndCopyExistingMaterial( identifier + idPostfix, pPG, spExistingVisual ) )
		{
			DataSectionPtr pMaterial = pPG->newSection( "material" );
			pMaterial->writeString( "identifier", identifier + idPostfix );

			// write bitmap/fx combination
			if ( ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES )
			{
				pMaterial->writeString( "fx", "shaders/std_effects/mesh_particle.fx" );
				if ( mapIdentifier.length() )
				{
					DataSectionPtr pProp = pMaterial->newSection( "property" );
					pProp->setString( "diffuseMap" );
					pProp->writeString( "Texture", mapIdentifier );
				}
			}
			else if ( mapIdMeaning == 1 )
			{
				// select fx file based on type of mesh and texture
	//			if ( skinned == false )
				if (skin == NO_SKIN)
				{
					BW::string fx = "shaders/std_effects/lightonly";
					if (mapIdentifier.length() > 8 &&
						mapIdentifier.substr( mapIdentifier.length() - 8, 4 ) == "_afx")
					{
						fx += "_add";
					}

					fx += ".fx";
					pMaterial->writeString( "fx", fx );
				}
				else
				{
					// use hardware skinning for objects with 17 bones or less
					if (skinnedShader)
						pMaterial->writeString( "fx", "shaders/std_effects/lightonly_skinned.fx" );
					else
						pMaterial->writeString( "fx", "shaders/std_effects/lightonly.fx" );
				}
				
				// add texture
				if ( mapIdentifier.length() )
				{
					DataSectionPtr pProp = pMaterial->newSection( "property" );
					pProp->setString( "diffuseMap" );
					pProp->writeString( "Texture", mapIdentifier );
				}
			}
			// else use mfm settings
			else if ( mapIdMeaning == 2 )
			{
				pMaterial->writeString( "mfm", mapIdentifier );
			}
			else
			{
				if (skin == NO_SKIN)
				{
					if( !s_defaultMaterial.value().empty() )
					{
						pMaterial->writeString( "mfm", s_defaultMaterial.value() );
					}
				}
				else
				{
					if( !s_defaultSkinnedMaterial.value().empty() )
					{
						pMaterial->writeString( "mfm", s_defaultSkinnedMaterial.value() );
					}
				}
			}
		}
/**/
// BW 1.5 Dodgy maya exporter code
/*		if (selfIllum != 0.f)
			pPG->writeFloat( "material/selfIllumination", selfIllum );

		switch (mapIdMeaning)
		{
		case 1:
			pPG->writeString( "material/textureormfm", mapIdentifier );
			break;
		case 2:
			pPG->writeString( "material/mfm", mapIdentifier );
			break;
		}
		pPG->writeString( "material/identifier", identifier );
/**/
	}
}


/**
 *	Look for the material named identifier in the existing visual file.  If found,
 *	then copy the material into the pPrimGroup section and return True.  Otherwise
 *	do nothing to pMaterial and return False.
 *	We use the pPrimGroup section to copy into, because the copy process includes
 *	copying the "material" node.
 */
bool VisualMesh::Material::findAndCopyExistingMaterial(
	const BW::string& identifier,
	DataSectionPtr pPrimGroup,
	DataSectionPtr spExistingVisual ) const
{
	if (!spExistingVisual)
		return false;

	DataSectionPtr pExistingMaterial = this->findExistingMaterial( identifier, spExistingVisual );
	if (pExistingMaterial)
	{
		pPrimGroup->copy( pExistingMaterial, false );
		return true;
	}
	else
	{
		return false;
	}
}


/**
 *	Look for material named 'identifier' in the given data section, and
 *	return it if it exists.
 */
DataSectionPtr VisualMesh::Material::findExistingMaterial( const BW::string& identifier,
														  DataSectionPtr spExistingVisual ) const
{
	if (!spExistingVisual)
		return NULL;

	BW::vector<DataSectionPtr> renderSets;
	spExistingVisual->openSections( "renderSet", renderSets );
	for (uint32 r=0; r<renderSets.size(); r++)
	{
		BW::vector<DataSectionPtr> geometries;
		renderSets[r]->openSections( "geometry", geometries );
		for (uint32 g=0; g<geometries.size(); g++)
		{
			BW::vector<DataSectionPtr> primGroups;
			geometries[g]->openSections( "primitiveGroup", primGroups );
			for (uint32 p=0; p<primGroups.size(); p++)
			{
				DataSectionPtr pMaterial = primGroups[p]->openSection( "material" );
				if (pMaterial->readString( "identifier" ) == identifier)
					return primGroups[p];
			}
		}
	}

	return NULL;
}


/**
 *	This method adds the visual mesh to another.
 */
void VisualMesh::add( VisualMesh & destMesh, int forceMeshIndex, bool worldSpace )
{
	uint32 i;

	// find or add all our materials to the dest mesh
	RemapVector		reremap;
	for (i = 0; i < materials_.size(); i++)
	{
		reremap.push_back( destMesh.findOrAddMaterial( materials_[i] ) );
	}

	if (reremap.empty())
	{
		reremap.push_back( 0 );
	}

	// append our transformed vertices to its vertices
	uint32 startVertex = static_cast<uint32>(destMesh.vertices_.size());
	for (i = 0; i < vertices_.size(); i++)
	{
		BloatVertex bv = vertices_[i];
		// FIX - ### may need to transpose world_ first
		//~ bv.pos = world_.PointTransform( bv.pos );
		if( worldSpace )
			bv.pos = world_ * bv.pos;
		// FIX
		//~ bv.normal = Normalize( world_.VectorTransform( bv.normal ) );
		// FIX - ### may need to transpose world_ first
		//~ bv.normal = world_.VectorTransform( bv.normal );
		bv.normal = world_ * bv.normal;
		bv.normal.normalise();
		if (forceMeshIndex != -1)
			bv.meshIndex = forceMeshIndex;
		destMesh.vertices_.push_back( bv );
	}

	// append our remapped and shuffled triangles to its triangles
	for (i = 0; i < triangles_.size(); i++)
	{
		Triangle tr = triangles_[i];
		tr.index[0] += startVertex;
		tr.index[1] += startVertex;
		tr.index[2] += startVertex;
		tr.materialIndex = reremap[ tr.materialIndex ];
		destMesh.triangles_.push_back( tr );
	}
}



/**
 * This method adds a vertex to our vertex list.
 * @param bv vertex to add to the vertexlist.
 * @return index of the vertex
 */
uint32 VisualMesh::addVertex( const VisualMesh::BloatVertex& bv )
{
// Raymond : Merge TC's optimisation {
#ifndef TC_OPTIMISATION
// }
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
// Raymond : Merge TC's optimisation {
#endif//TC_OPTIMISATION
// }
	{
		BloatVertex sbv = bv;
		vertices_.push_back( sbv );
		return static_cast<uint32>(vertices_.size() - 1);
	}
}

/**
 * This method inits our visual mesh
 * @param mesh the mesh we want to export
 * @param node the node of the mesh we want to export.
 */
bool VisualMesh::init( Mesh& mesh )
{
	errors_.clear();
	bool ret = false;
	if( mesh.positions().size() > 0 && mesh.normals().size() > 0 
		&& mesh.binormals().size() > 0 && mesh.tangents().size() > 0 
		&& mesh.uvSetCount() > 0 &&mesh.uvs(0).size() > 0 && mesh.faces().size() > 0 )
	{
		identifier_ = trimWhitespaces( mesh.name() );
		nodeIdentifier_ = trimWhitespaces( mesh.nodeName() );
		fullName_ = mesh.fullName();

		gatherMaterials( mesh );

		int mapCount = mesh.uvSetCount();

		bool hasUVs = mesh.uvs(0).size() > 0;

		dualUV_ = hasUVs && mapCount > 1;

		vertexColours_ = mesh.colours().size() > 0;

		BloatVertex bv;
		bv.normal = vector3<float>();
		bv.uv = Point2();
		bv.uv2 = Point2();
		bv.colour = Point4();
		bv.binormal = vector3<float>();
		bv.tangent = vector3<float>();

		bool isStatic = ExportSettings::instance().exportMode() == ExportSettings::STATIC ||
			ExportSettings::instance().exportMode() == ExportSettings::STATIC_WITH_NODES;

		for (int i = 0; i < (int)mesh.faces().size(); i++)
		{
			Face f = mesh.faces()[i];

			Triangle tr;
			tr.materialIndex = materialRemap_[f.materialIndex];
			tr.smoothingGroup = 0;
			for (int j = 0; j < 3; j++)
			{
				bv.pos = mesh.positions()[ f.positionIndex[j] ];
				
				// FIX - added normals from MAYA (doesn't use createNormals below)
				bv.normal = mesh.normals()[ f.normalIndex[j] ];
				bv.normal.normalise();

				bv.binormal = mesh.binormals()[ f.tangentIndex[j] ];
				bv.binormal.normalise();

				bv.tangent = mesh.tangents()[ f.tangentIndex[j] ];
				bv.tangent.normalise();
				
				bv.vertexIndex = f.positionIndex[j];
				bv.smoothingGroup = 0;
				if (hasUVs)
				{
					//~ bv.uv = reinterpret_cast<Point2&>( mesh->tVerts[ mesh->tvFace[i].t[j] ] );
					bv.uv = reinterpret_cast<Point2&>( mesh.uvs(0)[ f.uvIndex[j] ] ); 
					bv.uv.v = 1 - bv.uv.v;
				}

				if (dualUV_)
				{
					bv.uv2 = reinterpret_cast<Point2&>( mesh.uvs(1)[ f.uvIndex2[j] ] ); 
					bv.uv2.v = 1 - bv.uv2.v;
				}

				if (vertexColours_)
				{
					bv.colour = mesh.colours()[ f.colourIndex[j] ];
				}

				bv.meshIndex = 0;

				if (snapVertices_)
					bv.pos = snapPoint3( bv.pos );
				tr.index[j] = addVertex( bv );
				tr.realIndex[j] = bv.vertexIndex;
			}
			triangles_.push_back( tr );
		}

		world_.identity();

		// initialise boundingbox
		if (snapVertices_)
			bb_.setBounds(	reinterpret_cast<const Vector3&>( snapPoint3( world_ * vertices_[ 0 ].pos ) ),
							reinterpret_cast<const Vector3&>( snapPoint3( world_ * vertices_[ 0 ].pos ) ) );
		else
			bb_.setBounds(	reinterpret_cast<const Vector3&>( world_ * vertices_[ 0 ].pos ),
							reinterpret_cast<const Vector3&>( world_ * vertices_[ 0 ].pos ) );
		
		for (uint32 i = 1; i < vertices_.size(); i++)
		{
			if (snapVertices_)
				bb_.addBounds( reinterpret_cast<const Vector3&>( snapPoint3( world_ * vertices_[ i ].pos ) ) );
			else
				bb_.addBounds( reinterpret_cast<const Vector3&>( world_ * vertices_[ i ].pos ) );
		}

		ret = true;
	}
	else
	{
		if( mesh.positions().size() > 0 && mesh.normals().size() > 0 && mesh.faces().size() > 0 )
		{
			char msg[1024];
			bw_snprintf( msg, sizeof(msg), "Failed to export mesh %s because there are no uvs", mesh.fullName().c_str() );
			if ( ! LogMsg::automatedTest() )
			{
				::MessageBox( GetForegroundWindow(), bw_utf8tow( msg ).c_str(),
							 L"Visual Exporter Error", MB_OK );
			}
			addError( msg );
		}
	}
	return ret;
	
	//~ return false;
}

void VisualMesh::addError( const char * format, ... )
{
	va_list argPtr;
	va_start( argPtr, format );
	char buf[4096];
	_vsnprintf( buf, sizeof(buf), format, argPtr );
	buf[sizeof( buf ) - 1] = '\0';

	errors_.push_back( BW::string( buf ) );

	va_end(argPtr);
}

/** If check() failed, get the errors that occured, one per line */
BW::string VisualMesh::errorText()
{
	BW::string s;
	BW::vector<BW::string>::iterator i = errors_.begin();
	for (; i != errors_.end(); ++i)
	{
		if (i != errors_.begin())
			s += '\n';

		s += *i;
	}
	return s;
}

/**
 * Strips the reference file name prefix from object strings.
 *
 *	@param	nodeName	The string to be stripped of its prefix.
 *	@return				The string with its prefix stripped.
 */
BW::string stripReferencePrefix( BW::string & nodeName )
{
	// Search for a colon
	BW::string result;
	BW::string::size_type endOfPrefix = nodeName.find_first_of( ":", 0 );
	if (endOfPrefix < nodeName.size())
	{
		return nodeName.substr( endOfPrefix + 1 );
	}
	else
	{
		return nodeName;
	}
}

BW_END_NAMESPACE

