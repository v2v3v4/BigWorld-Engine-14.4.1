#include "pch.hpp"

#include "mesh_piece.hpp"
#include "node.hpp"
#include "moo/primitive_file_structs.hpp"
#include "resmgr/quick_file_writer.hpp"
#include "physics2/bsp.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"
#include "portal.hpp"
#include "skin_splitter.hpp"
#include "nvmeshmender.h"


BW_BEGIN_NAMESPACE

using namespace VisualManipulator;

MeshPiece::MeshPiece() :
	dualUV_(false),
	vertexColours_(false),
	tangents_(false)
{
}

uint32 MeshPiece::addVertex( const BloatVertex& bv )
{
	VertexVector::iterator it = std::find( vertices_.begin(), vertices_.end(), bv );
	if (it != vertices_.end())
	{
		return uint32(it - vertices_.begin());
	}

	bounds_.addBounds( bv.pos );

	vertices_.push_back( bv );
	return uint32(vertices_.size() - 1);
}


void MeshPiece::addTriangle( const BloatVertex& bv1, const BloatVertex& bv2, const BloatVertex& bv3, uint32 materialIndex )
{
	Triangle t;
	t.a = addVertex( bv1 );
	t.b = addVertex( bv2 );
	t.c = addVertex( bv3 );
	t.materialIndex = materialIndex;
	triangles_.push_back( t );
}

void MeshPiece::save( DataSectionPtr pVisualSection, DataSectionPtr pPrimSection, bool skinned,
					 const MaterialMap& materials)
{
	BW::string vertexSectionName = name_.size() ? name_  + ".vertices" : "vertices";
	BW::string indexSectionName = name_.size() ? name_  + ".indices" : "indices";

	writeIndices( pPrimSection, indexSectionName );
	writeVertices( pPrimSection, vertexSectionName, skinned );

	BW::vector<BW::string> streams;

	if (dualUV_)
	{
		BW::string uvSectionName = name_.size() ? name_ + ".uv2" : "uv2";
		writeUVStream( pPrimSection, uvSectionName );
		streams.push_back( uvSectionName );
	}

	if (vertexColours_)
	{
		BW::string vcSectionName = name_.size() ? name_ + ".colour" : "colour";
		writeColourStream( pPrimSection, vcSectionName );
		streams.push_back( vcSectionName );
	}

	DataSectionPtr pRenderSet = pVisualSection->newSection( "renderSet" );
	if (pRenderSet)
	{
		pRenderSet->writeBool( "treatAsWorldSpaceObject", skinned );
		pRenderSet->writeStrings( "node", joints_ );
		DataSectionPtr pGeometry = pRenderSet->newSection( "geometry" );
		if (pGeometry)
		{
			pGeometry->writeString( "vertices", vertexSectionName );

			pGeometry->writeStrings( "stream", streams );
			pGeometry->writeString( "primitive", indexSectionName );

			MaterialVector::iterator matIt = materials_.begin();

			uint32 pgIndex = 0;
			while (matIt != materials_.end())
			{
				DataSectionPtr pPG = pGeometry->newSection( "primitiveGroup" );
				pPG->setInt( pgIndex++ );

				MaterialMap::const_iterator existingMaterial = materials.find( *matIt );
				if (existingMaterial != materials.end())
				{
					DataSectionPtr pMaterialSection = pPG->newSection( "material" );
					pMaterialSection->copySections( existingMaterial->second );
				}
				else
				{
					pPG->writeString( "material/identifier", matIt->c_str() );
					pPG->writeString( "material/fx", skinned ? 
						"shaders/std_effects/lightonly_skinned.fx" : 
						"shaders/std_effects/lightonly.fx" );
				}
				++matIt;
			}
		}
	}
}

void MeshPiece::saveBSP( DataSectionPtr pPrimSection, const BW::string& bspResName ) const
{
	RealWTriangleSet tris;
	tris.reserve( triangles_.size() );

	TriangleVector::const_iterator triIt = triangles_.begin();

	while (triIt != triangles_.end())
	{
		const Triangle& tri = *triIt;
		WorldTriangle wt( vertices_[tri.a].pos, vertices_[tri.b].pos, vertices_[tri.c].pos, tri.materialIndex );
		if (wt.normal().length() >= 0.0001f)
		{
			tris.push_back( wt );
		}
		++triIt;
	}
	
	BSPTree* pTree = BSPTreeTool::buildBSP( tris );
	BinaryPtr pBSPBinary = BSPTreeTool::saveBSPInMemory( pTree );
	delete pTree;
	
	if (pBSPBinary)
	{
		pPrimSection->writeBinary( "bsp2", pBSPBinary );
		DataSectionPtr materialIDsSection = 
			BWResource::openSection( "temp_bsp_materials.xml", true, XMLSection::creator() );
		materialIDsSection->writeStrings( "id", const_cast<BW::vector<BW::string>&>(materials_) );
		pPrimSection->writeBinary( "bsp2_materials", materialIDsSection->asBinary() );
	}

	::DeleteFile( bw_acptow( BWResolver::resolveFilename(bspResName) ).c_str() );
}


template <typename IndexType>
void writeIndexStream( QuickFileWriter& f, const TriangleVector& triangles )
{
	BW::vector<IndexType> indices;
	indices.reserve( triangles.size() * 3);
	TriangleVector::const_iterator it = triangles.begin();

	while (it != triangles.end())
	{
		indices.push_back( IndexType(it->a) );
		indices.push_back( IndexType(it->b) );
		indices.push_back( IndexType(it->c) );
		++it;
	}

	f << indices;
}


void MeshPiece::writeIndices( DataSectionPtr pPrimSection, const BW::string& sectionName )
{
	uint32 maxIndex = 0;
	
	BW::vector<Moo::PrimitiveGroup> primGroups;

	Moo::PrimitiveGroup pg;
	pg.nPrimitives_ = 0;
	pg.nVertices_ = 0;
	pg.startIndex_ = 0;
	pg.startVertex_ = 0;
	uint32 currentMaterialIndex = 0;

	TriangleVector::iterator triIt = triangles_.begin();
	for (uint32 triIndex = 0; triIndex < triangles_.size(); triIndex++)
	{
		const Triangle& tri = triangles_[triIndex];
		if (tri.materialIndex != currentMaterialIndex)
		{
			primGroups.push_back( pg );
			pg.startIndex_ = triIndex * 3;
			pg.startVertex_ = tri.a;
			pg.nVertices_ = 0;
			pg.nPrimitives_ = 0;
			currentMaterialIndex = tri.materialIndex;
		}

		++pg.nPrimitives_;

		uint32 triMaxIndex = std::max( tri.a, std::max( tri.b , tri.c ) );
		uint32 pgMaxIndex = std::max<uint32>( triMaxIndex + 1, pg.startVertex_ + pg.nVertices_ );
		pg.startVertex_ = std::min( std::min( tri.a, tri.b), std::min<uint32>( tri.c, pg.startVertex_ ) );
		pg.nVertices_ = pgMaxIndex - pg.startVertex_;

		maxIndex = std::max( triMaxIndex, maxIndex);
	}

	primGroups.push_back( pg );


	Moo::IndexHeader indexHeader;
	memset( &indexHeader, 0, sizeof(indexHeader) );

	bool use32BitIndices = maxIndex > 65535;

	bw_snprintf( indexHeader.indexFormat_, sizeof(indexHeader.indexFormat_), use32BitIndices ? "list32" : "list" );
	indexHeader.nIndices_ = static_cast<int>(triangles_.size() * 3);
	indexHeader.nTriangleGroups_ = static_cast<int>(primGroups.size());

	QuickFileWriter f;

	f << indexHeader;

	if (use32BitIndices)
	{
		writeIndexStream<uint32>(f, triangles_);
	}
	else
	{
		writeIndexStream<uint16>(f, triangles_);
	}
	
	f << primGroups;

	pPrimSection->writeBinary( sectionName, f.output() );
}

template<typename VertexType>
void writeVertexStream( QuickFileWriter& f, const VertexVector& vertices, const BW::string& format )
{
	Moo::VertexHeader vertexHeader;
	memset( &vertexHeader, 0, sizeof(vertexHeader) );

	vertexHeader.nVertices_ = static_cast<int>(vertices.size());
	bw_snprintf( vertexHeader.vertexFormat_, sizeof(vertexHeader.vertexFormat_), format.c_str() );

	f << vertexHeader;
	
	BW::vector< VertexType > outVerts;
	outVerts.reserve( vertices.size() );

	VertexVector::const_iterator vertIt = vertices.begin();

	VertexType v;

	while (vertIt != vertices.end())
	{
		vertIt->output( v );
		outVerts.push_back( v );
		++vertIt;
	}

	f << outVerts;
}

void MeshPiece::writeVertices( DataSectionPtr pPrimSection, const BW::string& sectionName, bool skinned )
{
	QuickFileWriter f;

	if (skinned)
	{
		if (tangents_)
		{
			writeVertexStream<Moo::VertexXYZNUVIIIWWTB>( f, vertices_, "xyznuviiiwwtb" );
		}
		else
		{
			writeVertexStream<Moo::VertexXYZNUVIIIWW>( f, vertices_, "xyznuviiiww" );
		}
	}
	else
	{
		if (tangents_)
		{
			writeVertexStream<Moo::VertexXYZNUVTB>( f, vertices_, "xyznuvtb" );
		}
		else
		{
			writeVertexStream<Moo::VertexXYZNUV>( f, vertices_, "xyznuv" );
		}
	}

	pPrimSection->writeBinary( sectionName, f.output() );
}


void MeshPiece::writeUVStream( DataSectionPtr pPrimSection, const BW::string& sectionName ) const
{
	BW::vector<Vector2> uvs;
	VertexVector::const_iterator vertIt = vertices_.begin();
	while (vertIt != vertices_.end())
	{
		uvs.push_back( vertIt->uv2 );
		++vertIt;
	}

	QuickFileWriter qf;
	qf << uvs;
	pPrimSection->writeBinary( sectionName, qf.output() );
}


void MeshPiece::writeColourStream( DataSectionPtr pPrimSection, const BW::string& sectionName ) const
{
	BW::vector<DWORD> colours;
	VertexVector::const_iterator vertIt = vertices_.begin();
	while (vertIt != vertices_.end())
	{
		colours.push_back( D3DCOLOR_COLORVALUE(vertIt->colour.x, vertIt->colour.y, vertIt->colour.z, vertIt->colour.w ) );
		++vertIt;
	}

	QuickFileWriter qf;
	qf << colours;
	pPrimSection->writeBinary( sectionName, qf.output() );
}


BoundingBox  MeshPiece::skinBounds( const BW::vector<NodePtr>& nodes )
{
	BW::vector<Matrix> worldTransforms;
	for (size_t i = 0; i < nodes.size(); i++)
	{
		worldTransforms.push_back( nodes[i]->worldTransform() );
	}

	BoundingBox bb;

	VertexVector::const_iterator vertIt = vertices_.begin();
	while (vertIt != vertices_.end())
	{
		BloatVertex v = *vertIt;
		Vector3 skinnedPos(0,0,0);
		for (size_t i = 0; i < 4; i++)
		{
			skinnedPos += worldTransforms[v.indices[0]].applyPoint( v.pos ) * v.weights[i];
		}
		bb.addBounds( skinnedPos );
		++vertIt;
	}
	return bb;
}

void MeshPiece::merge( MeshPiecePtr pOther, const BW::vector<NodePtr>& nodes )
{
	assert(nodes.size() > 0);

	BW::vector<Matrix> worldTransforms;
	BW::vector<Matrix> vectorTransforms;

	for (size_t i = 0; i < nodes.size(); i++)
	{
		worldTransforms.push_back( nodes[i]->worldTransform() );
		Matrix vectorTrans;
		vectorTrans.invert( worldTransforms.back() );
		vectorTrans.transpose();
		vectorTransforms.push_back( vectorTrans );
	}

	BW::vector<uint32> indexRemap;

	VertexVector::iterator vertIt = pOther->vertices().begin();

	while (vertIt != pOther->vertices().end())
	{
		BloatVertex v = *vertIt;

		if (nodes.size() > 1)
		{
			Vector3 vertexPos(0,0,0);
			Vector3 vertexNormal(0,0,0);
			for (size_t i = 0; i < 4; i++)
			{
				vertexPos += worldTransforms[v.indices[0]].applyPoint( v.pos ) * v.weights[i];
				vertexNormal += vectorTransforms[v.indices[0]].applyVector( v.normal ) * v.weights[i];
			}
			v.pos = vertexPos;
			v.normal = vertexNormal;
		}
		else
		{
			v.pos = worldTransforms.front().applyPoint( v.pos );
			v.normal = vectorTransforms.front().applyVector( v.normal );
		}

		v.normal.normalise();

		indexRemap.push_back( this->addVertex( v ) );
		++vertIt;
	}

	uint32 materialBase = static_cast<uint32>(materials_.size());

	materials_.insert(materials_.end(), pOther->materials().begin(), pOther->materials().end() );

	TriangleVector::iterator triangleIt = pOther->triangles().begin();
	while (triangleIt != pOther->triangles().end())
	{
		Triangle tri;
		tri.a = indexRemap[triangleIt->a];
		tri.b = indexRemap[triangleIt->b];
		tri.c = indexRemap[triangleIt->c];
		tri.materialIndex = triangleIt->materialIndex + materialBase;
		triangles_.push_back( tri );

		++triangleIt;
	}

	dualUV_ = dualUV_ || pOther->dualUV_;
	vertexColours_ = vertexColours_ || pOther->vertexColours_;
}


void MeshPiece::split( BW::vector< MeshPiecePtr >& meshPieces, uint32 boneLimit )
{
	// Only split if we are affected by more joints than the bone limit
	if (joints_.size() > boneLimit)
	{
		TriangleVector::iterator triIt = triangles_.begin();
		uint32 splitIndex = 0;
		do 
		{
			uint32 materialIndex = triIt->materialIndex;

			// Extract triangles that share a common material, this is the
			// first split point
			TriangleVector materialTriangles;
			materialTriangles.reserve( triangles_.size() );
			while (triIt != triangles_.end() && triIt->materialIndex == materialIndex)
			{
				materialTriangles.push_back( *triIt );
				++triIt;
			}

			// Create the bone relationship list
			SkinSplitter splitter( materialTriangles, vertices_ );

			BW::vector<uint32> nodeList;
			TriangleVector triangles;
			triangles.reserve( materialTriangles.size() );

			bool success = true;
			// Extract a list of bones that contain an isolated set of triangles that
			// can be split out
			while ( splitter.size() != 0 && (success = splitter.createList( boneLimit, nodeList )) )
			{
				triangles.clear();

				// Split out the triangles that are covered by the split nodes
				splitter.splitTriangles( materialTriangles, triangles, nodeList, vertices_, joints_.size() );

				// Create a new mesh from the split triangles
				MeshPiecePtr pMesh = new MeshPiece;
				pMesh->dualUV_ = dualUV_;
				pMesh->vertexColours_ = vertexColours_;
				pMesh->materials_.push_back( materials_[triangles.front().materialIndex] );
				BW::vector<uint32> nodeRemap( joints_.size(), uint32(-1) );

				// Create a remap list for the isolated nodes
				for (uint32 iNode = 0; iNode < nodeList.size(); ++iNode)
				{
					uint32 remapIndex = nodeList[iNode];
					pMesh->joints_.push_back( joints_[remapIndex] );
					nodeRemap[remapIndex] = iNode;
				}

				// Add the triangles for the new mesh
				TriangleVector::iterator remapTri = triangles.begin();
				while (remapTri != triangles.end())
				{
					const Triangle& tri = *remapTri;

					pMesh->addTriangle( vertices_[tri.a], vertices_[tri.b], vertices_[tri.c], 0 );

					++remapTri;
				}

				// Remap the bone indices in the new mesh
				VertexVector::iterator vertIt = pMesh->vertices_.begin();
				while(vertIt != pMesh->vertices_.end())
				{
					for (uint32 i = 0; i < 4; ++i)
					{
						vertIt->indices[i] = nodeRemap[vertIt->indices[i]];
					}
					++vertIt;
				}

				// Create a name for the new mesh piece
				char newName[1024];
				bw_snprintf( newName, 1024, "%s_split_%d", name_.c_str(), splitIndex );
				pMesh->name_ = newName;
				meshPieces.push_back( pMesh );

				++splitIndex;
			}
		} while (triIt != triangles_.end());
	}
	else
	{
		meshPieces.push_back( this );
	}
}


void MeshPiece::mergeMaterials()
{
	BW::vector<uint32> materialRemaps;
	BW::vector<uint32> duplicates;
	materialRemaps.reserve( materials_.size() );

	uint32 nRemoved = 0;

	for (uint32 mIndex = 0; mIndex < materials_.size(); ++mIndex)
	{
		uint32 dupeIndex = 0;
		while (dupeIndex < mIndex && materials_[dupeIndex] != materials_[mIndex])
		{
			++dupeIndex;
		}
		
		if (dupeIndex != mIndex)
		{
			materialRemaps.push_back( materialRemaps[dupeIndex] );
			duplicates.push_back( mIndex );
			++nRemoved;
		}
		else
		{
			materialRemaps.push_back( mIndex - nRemoved );
		}
	}

	if (duplicates.size())
	{
		TriangleVector::iterator triIt = triangles_.begin();
		while (triIt != triangles_.end())
		{
			triIt->materialIndex = materialRemaps[triIt->materialIndex];
			++triIt;
		}

		while (duplicates.size())
		{
			materials_.erase( materials_.begin() + duplicates.back() );
			duplicates.pop_back();
		}
	}
}


void MeshPiece::sortTrianglesByMaterial()
{
	VertexVector newVerts;
	BW::vector<uint32> vertexRemap;
	const uint32 NO_VERTEX = uint32(-1);

	newVerts.reserve( vertices_.size() );
	
	TriangleVector::iterator targetIt = triangles_.begin();
	for (uint32 materialIndex = 0; materialIndex < materials_.size(); ++materialIndex)
	{
		vertexRemap.clear();
		vertexRemap.resize( vertices_.size(), NO_VERTEX );
		TriangleVector::iterator srcIt = targetIt;
		while (srcIt != triangles_.end())
		{
			if (srcIt->materialIndex == materialIndex)
			{
				if (srcIt != targetIt)
				{
					std::swap( *srcIt, *targetIt );
				}
				if (vertexRemap[targetIt->a] == NO_VERTEX)
				{
					vertexRemap[targetIt->a] = (uint)newVerts.size();
					newVerts.push_back( vertices_[targetIt->a] );
				}
				if (vertexRemap[targetIt->b] == NO_VERTEX)
				{
					vertexRemap[targetIt->b] = (uint)newVerts.size();
					newVerts.push_back( vertices_[targetIt->b] );
				}
				if (vertexRemap[targetIt->c] == NO_VERTEX)
				{
					vertexRemap[targetIt->c] = (uint)newVerts.size();
					newVerts.push_back( vertices_[targetIt->c] );
				}
				
				targetIt->a = vertexRemap[targetIt->a];
				targetIt->b = vertexRemap[targetIt->b];
				targetIt->c = vertexRemap[targetIt->c];

				++targetIt;
			}
			++srcIt;
		}
	}
	vertices_ = newVerts;
}

void MeshPiece::calculateTangentsAndBinormals()
{
	TriangleVector newTriangles;
	VertexVector newVertices;
	newTriangles.reserve( triangles_.size() );

	TriangleVector::iterator triIt = triangles_.begin();
	while (triIt != triangles_.end())
	{
		uint32 materialIndex = triIt->materialIndex;
		
		TriangleVector::iterator triEndIt = triIt;

		uint32 maxIndex = std::max( triEndIt->a, std::max( triEndIt->b, triEndIt->c ) );
		uint32 minIndex = std::min( triEndIt->a, std::min( triEndIt->b, triEndIt->c ) );

		do
		{
			maxIndex = std::max( maxIndex, std::max( triEndIt->a, std::max( triEndIt->b, triEndIt->c ) ) );
			minIndex = std::min( minIndex, std::min( triEndIt->a, std::min( triEndIt->b, triEndIt->c ) ) );
			++triEndIt;
		} while (triEndIt != triangles_.end() && triEndIt->materialIndex == materialIndex);

		BW::vector<MeshMender::Vertex> menderVertices;
		BW::vector<unsigned int> menderIndices;
		BW::vector<unsigned int> vertexMappings;

		menderVertices.reserve( maxIndex - minIndex + 1 );
		
		for (uint32 iVert = minIndex; iVert <= maxIndex; ++iVert)
		{
			MeshMender::Vertex v;
			const BloatVertex& bv = vertices_[iVert];
			v.pos = bv.pos;
			v.s = bv.uv.x;
			v.t = 1.f - bv.uv.y;
			v.normal = bv.normal;
			menderVertices.push_back( v );
		}

		while (triIt != triEndIt)
		{
			menderIndices.push_back( triIt->a - minIndex );
			menderIndices.push_back( triIt->b - minIndex );
			menderIndices.push_back( triIt->c - minIndex );
			++triIt;
		}
		
		MeshMender mender;
		mender.Mend( menderVertices, menderIndices, vertexMappings,
			0.5f, 0.5f, 0.5f, 
			0,
			MeshMender::DONT_CALCULATE_NORMALS,
			MeshMender::DONT_RESPECT_SPLITS,
			MeshMender::DONT_FIX_CYLINDRICAL );

		newVertices.reserve( newVertices.size() + menderVertices.size() );
		uint32 newIndexBase = static_cast<uint32>(newVertices.size());

		for (uint32 iVert = 0; iVert < menderVertices.size(); ++iVert)
		{
			BloatVertex bv = vertices_[vertexMappings[iVert] + minIndex];
			bv.pos = (Vector3&)menderVertices[iVert].pos;
			bv.normal = (Vector3&)menderVertices[iVert].normal;
			bv.binormal = (Vector3&)menderVertices[iVert].binormal;
			bv.tangent = (Vector3&)menderVertices[iVert].tangent;
			bv.uv.x = menderVertices[iVert].s;
			bv.uv.y = 1.f - menderVertices[iVert].t;
			newVertices.push_back( bv );
		}
	
		for (uint32 iTris = 0; iTris < menderIndices.size(); iTris += 3)
		{
			Triangle tri;
			tri.a = menderIndices[iTris] + newIndexBase;
			tri.b = menderIndices[iTris + 1] + newIndexBase;
			tri.c = menderIndices[iTris + 2] + newIndexBase;
			tri.materialIndex = materialIndex;
			newTriangles.push_back( tri );
		}
	}
	vertices_ = newVertices;
	triangles_ = newTriangles;
	tangents_ = true;
}


namespace
{
	bool endsWith( const BW::string& s, const BW::string& token )
	{
		BW::string::size_type tlen = token.length();
		BW::string::size_type slen = s.length();

		return slen >= tlen && 0 == s.compare( slen - tlen, tlen, token );
	}
}


bool MeshPiece::isBSP() const
{
	return endsWith( name_, "_bsp" );
}


bool MeshPiece::isHull() const
{
	return endsWith( name_, "_hull" );
}


bool MeshPiece::isPortal() const
{
	return Portal::isPortal( name_ );
}


MaterialMap MeshPiece::gatherMaterials( DataSectionPtr pVisualSection )
{
	MaterialMap existingMaterials;
	BW::vector<DataSectionPtr> rendersets;
	pVisualSection->openSections( "renderSet", rendersets );
	for (size_t rsi = 0; rsi < rendersets.size(); rsi++)
	{
		BW::vector<DataSectionPtr> geometries;
		rendersets[rsi]->openSections( "geometry", geometries );
		for (size_t gmi = 0; gmi < geometries.size(); gmi++)
		{
			BW::vector<DataSectionPtr> primitiveGroups;
			geometries[gmi]->openSections( "primitiveGroup", primitiveGroups );
			for (size_t pgi = 0; pgi < primitiveGroups.size(); pgi++)
			{
				DataSectionPtr pMaterial = primitiveGroups[pgi]->openSection( "material" );
				if (pMaterial)
				{
					existingMaterials.insert( std::make_pair( pMaterial->readString( "identifier" ), pMaterial ) );
				}
			}
		}
	}
	return existingMaterials;
}

BW_END_NAMESPACE
