#include "pch.hpp"

#pragma warning (disable:4786)

//~ #include "mfxexp.hpp"
#include "visual_envelope.hpp"
#include "expsets.hpp"
#include "utility.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/quick_file_writer.hpp"
#include "resmgr/xml_section.hpp"
#include "skin_splitter.hpp"

#ifndef CODE_INLINE
#include "visual_envelope.ipp"
#endif

BW_BEGIN_NAMESPACE

static inline uint32 packNormal( const Point3&n )
{
	if (ExportSettings::instance().useLegacyOrientation())
	{
		// This legacy code came from the 1.7.2 release
		return	( ( ( (uint32)(n.z * 511.0f) )  & 0x3ff ) << 22L ) |
				( ( ( (uint32)(n.y * 1023.0f) ) & 0x7ff ) << 11L ) |
				( ( ( (uint32)(-n.x * 1023.0f) ) & 0x7ff ) <<  0L );
	}
	else
	{
		return	( ( ( (uint32)(-n.z * 511.0f) )  & 0x3ff ) << 22L ) |
				( ( ( (uint32)(n.y * 1023.0f) ) & 0x7ff ) << 11L ) |
				( ( ( (uint32)(n.x * 1023.0f) ) & 0x7ff ) <<  0L );
	}
}


// -----------------------------------------------------------------------------
// Section: VisualEnvelope
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
VisualEnvelope::VisualEnvelope()
{
}


/**
 *	Destructor.
 */
VisualEnvelope::~VisualEnvelope()
{
}

bool VisualEnvelope::collectInitialTransforms( Skin& skin )
{
	bool ret = true;

	for( uint32 i = 0; i < skin.numberOfBones(); ++i )
	{
		initialTransforms_.push_back( skin.transform( i, false ) );
	}
	return ret;
}

void VisualEnvelope::normaliseInitialTransforms()
{
	MatrixVector::iterator it = initialTransforms_.begin();
	MatrixVector::iterator end = initialTransforms_.end();

	while (it != end)
	{
		matrix4<float> m = *it++;
		m = normaliseMatrix( m );
	}
}

void VisualEnvelope::initialPoseVertices()
{
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();

	while (it!=end)
	{
		BloatVertex& bloatV = *it++;
		BoneVertex& boneV = boneVertices_[ bloatV.vertexIndex ];
		bloatV.pos = /*initialTransforms_[ boneV.index1 ] */ boneV.position;
	}
}

void VisualEnvelope::relaxedPoseVertices()
{
	MatrixVector invert;

	for (uint i = 0; i < initialTransforms_.size(); i++)
	{
		matrix4<float> m = initialTransforms_[i].inverse();
		invert.push_back( m );
	}

	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();

	while (it!=end)
	{
		BloatVertex& bloatV = *it++;
		BoneVertex& boneV = boneVertices_[ bloatV.vertexIndex ];
		bloatV.pos = invert[ boneV.index1 ] * boneV.position * boneV.weight1 +
					 invert[ boneV.index2 ] * boneV.position * boneV.weight2 +
					 invert[ boneV.index3 ] * boneV.position * boneV.weight3;
	}
}


void VisualEnvelope::createVertexList( BlendedVertexVector& vertices )
{
	VertexXYZNUVIIIWW v;
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();
	int vNum = 0;
	while (it!=end)
	{
		BloatVertex& bv = *it++;
		bv.pos *= ExportSettings::instance().unitScale();

		if (ExportSettings::instance().useLegacyOrientation())
		{
			// This legacy code came from the 1.7.2 release
			v.pos[0] = -bv.pos.x;
			v.pos[1] = bv.pos.y;
			v.pos[2] = bv.pos.z;
		}
		else
		{
			v.pos[0] = bv.pos.x;
			v.pos[1] = bv.pos.y;
			v.pos[2] = -bv.pos.z;
		}


		v.normal = packNormal( bv.normal );
		v.uv[0] = bv.uv.u;
		v.uv[1] = bv.uv.v;

		normaliseBoneWeights<VertexXYZNUVIIIWW>(bv.vertexIndex, v);

		vertices.push_back( v );
	}
}


void VisualEnvelope::createVertexList( TBBlendedVertexVector& vertices )
{
	VertexXYZNUVIIIWWTB v;
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();
	int vNum = 0;
	while (it!=end)
	{
		BloatVertex& bv = *it++;
		bv.pos *= ExportSettings::instance().unitScale();
		if (ExportSettings::instance().useLegacyOrientation())
		{
			// This legacy code came from the 1.7.2 release
			v.pos[0] = -bv.pos.x;
			v.pos[1] = bv.pos.y;
			v.pos[2] = bv.pos.z;
		}
		else
		{
			v.pos[0] = bv.pos.x;
			v.pos[1] = bv.pos.y;
			v.pos[2] = -bv.pos.z;
		}

		v.normal = packNormal( bv.normal );
		v.uv[0] = bv.uv.u;
		v.uv[1] = bv.uv.v;

		normaliseBoneWeights<VertexXYZNUVIIIWWTB>(bv.vertexIndex, v);

		v.tangent = packNormal( bv.tangent );
		v.binormal = packNormal( bv.binormal );
		vertices.push_back( v );
	}
}

bool VisualEnvelope::init( Skin& skin, Mesh& mesh )
{
	bool ret = false;

	if( skin.count() < 1 || mesh.positions().size() != skin.numberOfVertices( mesh.fullName() ) )
		return ret;

	VisualMesh::init( mesh );

	BW::vector<BoneVertex>& vertices = skin.vertices( mesh.fullName() );

	for( BW::vector<BoneVertex>::iterator it = vertices.begin(); it != vertices.end(); ++it )
		boneVertices_.push_back( *it );

	collectInitialTransforms( skin );
	bb_.setBounds(	reinterpret_cast<const Vector3&>( vertices_[0].pos ),
					reinterpret_cast<const Vector3&>( vertices_[0].pos ) );
	for (int i = 1; i < (int)vertices_.size(); i++)
	{
		bb_.addBounds( reinterpret_cast<const Vector3&>( vertices_[i].pos ) );
	}

	flipTriangleWindingOrder();
	removeDuplicateVertices();
	sortTriangles();

	BoneSet& bones = skin.boneSet( mesh.fullName() );

	for( uint32 i = 0; i < bones.bones.size(); ++i )
		boneNodes_.push_back( bones.bones[i] );

	if ( ExportSettings::instance().bumpMapped() )
	{
		makeBumped();
	}

	removeDuplicateVertices();
	sortTriangles();

	ret = true;
	return ret;
}


namespace
{

// Helper functions for the skin splitter to access bone weights and indices

void getWeights( const BoneVertex& bv, float* weights )
{
	weights[0] = bv.weight1;
	weights[1] = bv.weight2;
	weights[2] = bv.weight3;
}


void getIndices( const BoneVertex& bv, int* indices )
{
	indices[0] = bv.index1;
	indices[1] = bv.index2;
	indices[2] = bv.index3;
}

}


bool VisualEnvelope::split( size_t boneLimit, BW::vector<VisualEnvelopePtr>& splitEnvelopes )
{
	bool ret = true;

	if (this->boneNodesCount() > boneLimit)
	{
		TriangleVector::iterator triIt = triangles_.begin();
		uint32 splitIndex = 0;
		do 
		{
			int materialIndex = triIt->materialIndex;

			// Extract triangles that share a common material, this is the
			// first split point
			TriangleVector materialTriangles;
			materialTriangles.reserve( triangles_.size() );
			while (triIt != triangles_.end() && triIt->materialIndex == materialIndex)
			{
				materialTriangles.push_back( *triIt );
				materialTriangles.back().materialIndex = 0;
				++triIt;
			}

			// Create the bone relationship list
			SkinSplitter splitter( materialTriangles, vertices_, boneVertices_ );

			BW::vector<uint32> nodeList;
			TriangleVector triangles;
			triangles.reserve( materialTriangles.size() );

			bool success = true;
			// Extract a list of bones that contain an isolated set of triangles that
			// can be split out
			while ( splitter.size() != 0 && 
				(ret = splitter.createList( static_cast<uint32>(boneLimit), nodeList )) )
			{
				triangles.clear();

				VisualEnvelopePtr pEnvelope = new VisualEnvelope;

				// Split out the triangles that are covered by the split nodes
				splitter.splitTriangles( materialTriangles, triangles, nodeList, vertices_, boneVertices_, boneNodes_.size() );

				BW::vector<int> nodeRemap( boneNodes_.size(), int(boneNodes_.size()) );

				// Create a remap list for the isolated nodes
				for (uint32 iNode = 0; iNode < nodeList.size(); ++iNode)
				{
					uint32 remapIndex = nodeList[iNode];
					pEnvelope->boneNodes_.push_back( boneNodes_[remapIndex] );
					nodeRemap[remapIndex] = iNode;
				}

				pEnvelope->boneVertices_ = this->boneVertices_;
				pEnvelope->vertices_ = this->vertices_;
				pEnvelope->triangles_ = triangles;
				pEnvelope->materials_.push_back( materials_[materialIndex] );

				for (BoneVVector::iterator it = pEnvelope->boneVertices_.begin();
					it != pEnvelope->boneVertices_.end();
					++it)
				{
					it->index1 = nodeRemap[it->index1];
					if (it->weight2 > 0.f)
					{
						it->index2 = nodeRemap[it->index2];
					}
					else
					{
						it->index2 = it->index1;
					}

					
					if (it->weight3 > 0.f)
					{
						it->index3 = nodeRemap[it->index3];
					}
					else
					{
						it->index3 = it->index1;
					}
				}

				pEnvelope->sortTriangles();

				pEnvelope->vertexColours_ = vertexColours_;
				pEnvelope->dualUV_ = dualUV_;

				// Create a name for the new mesh piece
				char newName[4096];
				bw_snprintf( newName, 1024, "%s_split_%d", identifier_.c_str(), splitIndex );
				pEnvelope->identifier_ = newName;

				pEnvelope->bb_ = bb_;

				pEnvelope->object = object;

				splitEnvelopes.push_back( pEnvelope );

				++splitIndex;
			}
		} while (triIt != triangles_.end() && ret);
	}
	else
	{
		splitEnvelopes.push_back( this );
	}

	return ret;
}


bool VisualEnvelope::save( DataSectionPtr spVisualSection, DataSectionPtr spExistingVisual, const BW::string& primitiveFile,
	bool useIdentifier )
{
	DataSectionPtr spFile = BWResource::openSection( primitiveFile );
	if (!spFile)
		spFile = new BinSection( primitiveFile, new BinaryBlock( 0, 0, "BinaryBlock/VisualEnvelope" ) );

	if (spFile)
	{
		PGVector primGroups;
		Moo::IndicesHolder indices;

		if (!createPrimitiveGroups( primGroups, indices ))
			return false;

		if (indices.entrySize() != 2)
		{
			::MessageBox(
				0,
				L"Skinned models cannot have more than 65535 vertices!",
				L"Vertex count error!",
				MB_ICONERROR );
			return false;
		}

		Moo::IndexHeader ih;
		bw_snprintf( ih.indexFormat_, sizeof(ih.indexFormat_), "list" );
		ih.nIndices_ = indices.size();
		ih.nTriangleGroups_ = static_cast<int>(primGroups.size());

		QuickFileWriter f;
		f << ih;
		f.write( indices.indices(), indices.size() * indices.entrySize() );
		f << primGroups;

		spFile->writeBinary( identifier_.substr( identifier_.find_last_of( "|" ) + 1 ) + ".indices", f.output() );

		f = QuickFileWriter();

		BW::vector<Vector2> uv2;
		BW::vector<DWORD> colours;
		{
			if (!ExportSettings::instance().bumpMapped())
			{
				BlendedVertexVector vertices;
				createVertexList( vertices );
				Moo::VertexHeader vh;
				vh.nVertices_ = static_cast<int>(vertices.size());
				bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuviiiww" );
				f << vh;
				f << vertices;
			}
			else
			{
				TBBlendedVertexVector vertices;
				createVertexList( vertices );
				Moo::VertexHeader vh;
				vh.nVertices_ = static_cast<int>(vertices.size());
				bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuviiiwwtb" );
				f << vh;
				f << vertices;
			}
		}

		BW::string vertName = identifier_.substr( identifier_.find_last_of( "|" ) + 1 ) + ".vertices";

		spFile->writeBinary( vertName, f.output() );

		// Write extra vertex data streams
		BW::vector<BW::string> streams;
		if (dualUV_)
		{
			// Retrieve the second UV channel.
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

			streams.push_back( identifier_.substr( identifier_.find_last_of( "|" ) + 1 ) + ".uv2" );

			spFile->writeBinary( streams.back(), f.output() );
		}

		if (vertexColours_)
		{
			// Retrieve the vertex colour info.
			
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

			streams.push_back( identifier_.substr( identifier_.find_last_of( "|" ) + 1 ) + ".colour" );

			spFile->writeBinary( streams.back(), f.output() );
		}

		spFile->save( primitiveFile );

		DataSectionPtr spRenderSet = spVisualSection->newSection( "renderSet" );
		if (spRenderSet)
		{
			spRenderSet->writeBool( "treatAsWorldSpaceObject", true );
			for( int i = 0; i < (int)boneNodes_.size(); i++ )
			{
				DataSectionPtr spNode = spRenderSet->newSection( "node" );
				if (spNode)
				{
					spNode->setString( (boneNodes_[i].substr( boneNodes_[i].find_last_of( "|" ) + 1 ) + "_BlendBone") );
				}
			}

			DataSectionPtr spGeometry = spRenderSet->newSection( "geometry" );
			if (spGeometry)
			{
				spGeometry->writeString( "vertices", vertName );

				spGeometry->writeStrings( "stream", streams );

				spGeometry->writeString( "primitive", identifier_.substr( identifier_.find_last_of( "|" ) + 1 ) + ".indices" );

				MaterialVector::iterator it = materials_.begin();
				MaterialVector::iterator end = materials_.end();

				uint32 primGroup = 0;
				for (uint32 i = 0; i < materials_.size(); i++)
				{
					Material& mat = materials_[i];
					if (mat.inUse) mat.save( spGeometry, spExistingVisual, primGroup++, true, (int)boneNodes_.size(),
						Material::SOFT_SKIN, boneNodes_.size() <= 17  );
				}
			}
		}
	}
	return true;
}


/**
 * This method saves the visual envelope in XML format.  Note that this
 * method is intended for debugging purposes only.
 *
 * @param	xmlFile	The filename of the XML file.
 */
bool VisualEnvelope::savePrimXml( const BW::string& xmlFile )
{
	DataSectionPtr spFileRoot = BWResource::openSection( xmlFile );
	if (!spFileRoot)
		spFileRoot = new XMLSection( "VisualEnvelopePrimXml" );

	DataSectionPtr spFile = spFileRoot->newSection( identifier_ );

	PGVector primGroups;
	Moo::IndicesHolder indices;

	if (!createPrimitiveGroups( primGroups, indices ))
		return false;

	if (indices.entrySize() != 2)
	{
		return false;
	}
	
	Moo::IndexHeader ih;
	bw_snprintf( ih.indexFormat_, sizeof(ih.indexFormat_), "list" );
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

	if (!ExportSettings::instance().bumpMapped())
	{
		BlendedVertexVector vertices;
		createVertexList( vertices );
		Moo::VertexHeader vh;
		vh.nVertices_ = static_cast<int>(vertices.size());
		bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuviiiww" );

		// Print Vertex Header contents here
		DataSectionPtr pVH = spFile->newSection( "VertexHeader" );
		pVH->writeString( "VertexFormat", vh.vertexFormat_ );
		pVH->writeInt( "NumVertices", vh.nVertices_ );

		DataSectionPtr pVertices = spFile->newSection( "Vertices" );
		BlendedVertexVector::iterator bvvIt;
		for (bvvIt = vertices.begin(); bvvIt != vertices.end(); ++bvvIt)
		{
			DataSectionPtr pVertex = pVertices->newSection( "Vertex" );
			pVertex->writeVector3( "Pos", Vector3(bvvIt->pos[0], bvvIt->pos[1], bvvIt->pos[2]) );
			pVertex->writeLong( "Normal", bvvIt->normal );
			pVertex->writeVector2( "UV", Vector2(bvvIt->uv[0], bvvIt->uv[1]) );
			pVertex->writeInt( "Index1", bvvIt->index );
			pVertex->writeInt( "Index2", bvvIt->index2 );
			pVertex->writeInt( "Index3", bvvIt->index3 );
			pVertex->writeInt( "Weight", bvvIt->weight );
			pVertex->writeInt( "Weight2", bvvIt->weight2 );
		}
	}
	else
	{
		TBBlendedVertexVector vertices;
		createVertexList( vertices );
		Moo::VertexHeader vh;
		vh.nVertices_ = static_cast<int>(vertices.size());
		bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuviiiwwtb" );

		// Print Vertex Header contents here
		DataSectionPtr pVH = spFile->newSection( "VertexHeader" );
		pVH->writeString( "VertexFormat", vh.vertexFormat_ );
		pVH->writeInt( "NumVertices", vh.nVertices_ );

		DataSectionPtr pVertices = spFile->newSection( "Vertices" );
		TBBlendedVertexVector::iterator tbbvvIt;
		for (tbbvvIt = vertices.begin(); tbbvvIt != vertices.end(); ++tbbvvIt)
		{
			DataSectionPtr pVertex = pVertices->newSection( "Vertex" );
			pVertex->writeVector3( "Pos", Vector3(tbbvvIt->pos[0], tbbvvIt->pos[1], tbbvvIt->pos[2]) );
			pVertex->writeLong( "Normal", tbbvvIt->normal );
			pVertex->writeVector2( "UV", Vector2(tbbvvIt->uv[0], tbbvvIt->uv[1]) );
			pVertex->writeInt( "Index1", tbbvvIt->index );
			pVertex->writeInt( "Index2", tbbvvIt->index2 );
			pVertex->writeInt( "Index3", tbbvvIt->index3 );
			pVertex->writeInt( "Weight", tbbvvIt->weight );
			pVertex->writeInt( "Weight2", tbbvvIt->weight2 );
			pVertex->writeInt( "Tangent", tbbvvIt->tangent );
			pVertex->writeInt( "Binormal", tbbvvIt->binormal );
		}
	}

	spFileRoot->save( xmlFile );

	return true;
}

BW_END_NAMESPACE

