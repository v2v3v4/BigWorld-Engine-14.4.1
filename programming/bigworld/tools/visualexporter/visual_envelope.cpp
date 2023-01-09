#pragma warning (disable:4786)

#include "mfxexp.hpp"
#include "visual_envelope.hpp"
#include "mfxexp.hpp"
#include "utility.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/quick_file_writer.hpp"
#include "resmgr/xml_section.hpp"
#include "iparamm2.h"
#include "iskin.h"
#include "cstdmf/debug.hpp"
#include "skin_splitter.hpp"

DECLARE_DEBUG_COMPONENT2("Exporter",0);

BW_BEGIN_NAMESPACE

extern TimeValue timeNow();

static inline uint32 packNormal( const Point3&nn )
{
	Point3 n = nn;
	n.Normalize();

	n.x = Math::clamp(-1.f, n.x, 1.f);
	n.y = Math::clamp(-1.f, n.y, 1.f);
	n.z = Math::clamp(-1.f, n.z, 1.f);

	if (ExportSettings::instance().useLegacyOrientation())
	{
		return	( ( ( (uint32)(-n.y * 511.0f) )  & 0x3ff ) << 22L ) |
				( ( ( (uint32)(n.z * 1023.0f) ) & 0x7ff ) << 11L ) |
				( ( ( (uint32)(-n.x * 1023.0f) ) & 0x7ff ) <<  0L );
	}
	else
	{
		return	( ( ( (uint32)(n.y * 511.0f) )  & 0x3ff ) << 22L ) |
				( ( ( (uint32)(n.z * 1023.0f) ) & 0x7ff ) << 11L ) |
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

void VisualEnvelope::getScaleValues( BW::vector< Point3 > & scales )
{
	MatrixVector::iterator it = initialTransforms_.begin();
	MatrixVector::iterator end = initialTransforms_.end();

	while (it != end)
	{
		Matrix3& m = *it++;
		scales.push_back( Point3( 1.f / Length( m.GetRow(0) ),
								  1.f / Length( m.GetRow(1) ),
								  1.f / Length( m.GetRow(2) ) ) );
	}
}

bool VisualEnvelope::collectInitialTransforms(IPhysiqueExport* phyExport)
{
	bool ret = true;

	NodeVector::iterator it = boneNodes_.begin();
	NodeVector::iterator end = boneNodes_.end();

	while (it != end && ret)
	{
		INode * node = *it++;
		Matrix3 itm;
		ret = (MATRIX_RETURNED == phyExport->GetInitNodeTM( node, itm ) );
		if (ret)
		{
			initialTransforms_.push_back(itm);
		}
	}
	return ret;
}

bool VisualEnvelope::collectInitialTransforms(ISkin* pSkin)
{
	bool ret = true;

	NodeVector::iterator it = boneNodes_.begin();
	NodeVector::iterator end = boneNodes_.end();

	while (it != end && ret)
	{
		INode * node = *it++;
		Matrix3 itm;
		if (SKIN_INVALID_NODE_PTR == pSkin->GetBoneInitTM( node, itm ))
		{
			if (SKIN_INVALID_NODE_PTR == pSkin->GetSkinInitTM( node, itm ))
			{
				itm.IdentityMatrix();
			}
		}
		initialTransforms_.push_back(itm);
	}
	return ret;
}

void VisualEnvelope::normaliseInitialTransforms()
{
	MatrixVector::iterator it = initialTransforms_.begin();
	MatrixVector::iterator end = initialTransforms_.end();

	while (it != end)
	{
		Matrix3& m = *it++;
		m = normaliseMatrix( m );
	}
}

void VisualEnvelope::scaleBoneVertices( BW::vector< Point3 >& scales )
{
	BoneVVector::iterator it = boneVertices_.begin();
	BoneVVector::iterator end = boneVertices_.end();
	
	while (it != end)
	{
		BoneVertex& bv = *it++;
		assert( bv.boneIndex_ < scales.size() );
		bv.position_ = bv.position_ * scales[ bv.boneIndex_ ];
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
		bloatV.pos = /*initialTransforms_[ boneV.boneIndex_ ] **/ boneV.position_;
	}
}

void VisualEnvelope::relaxedPoseVertices()
{

	MatrixVector normalInvert;
	MatrixVector invert;
		
	for (int i = 0; i < initialTransforms_.size(); i++)
	{
		Matrix3 m = Inverse( initialTransforms_[i] );
		invert.push_back( m );
		m.SetRow(3, Point3::Origin);
		normalInvert.push_back( m );
	}

	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();

	while (it!=end)
	{
		BloatVertex& bloatV = *it++;
		BoneVertex& boneV = boneVertices_[ bloatV.vertexIndex ];
		bloatV.pos = invert[ boneV.boneIndex_ ] * boneV.position_;
		bloatV.normal = normalInvert[ boneV.boneIndex_ ] * bloatV.normal;
		bloatV.normal = Normalize( bloatV.normal );
		bloatV.tangent = normalInvert[ boneV.boneIndex_ ] * bloatV.tangent;
		bloatV.tangent = Normalize( bloatV.tangent );
		bloatV.binormal = normalInvert[ boneV.boneIndex_ ] * bloatV.binormal;
		bloatV.binormal = Normalize( bloatV.binormal );
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
			v.pos[0] = -bv.pos.x;
			v.pos[1] = bv.pos.z;
			v.pos[2] = -bv.pos.y;
		}
		else
		{
			v.pos[0] = bv.pos.x;
			v.pos[1] = bv.pos.z;
			v.pos[2] = bv.pos.y;
		}

		v.normal = packNormal( bv.normal );
		v.uv[0] = bv.uv.x;
		v.uv[1] = bv.uv.y;
		v.index = boneVertices_[ bv.vertexIndex ].boneIndex_ * 3;
		v.index2 = boneVertices_[ bv.vertexIndex ].boneIndex2_ * 3;
		v.index3 = boneVertices_[ bv.vertexIndex ].boneIndex3_ * 3;
		v.weight = (uint8) (255.f * boneVertices_[ bv.vertexIndex ].weight_);
		v.weight2 = (uint8) (255.f * boneVertices_[ bv.vertexIndex ].weight2_);
		assert( (v.index / 3) < boneNodes_.size() );
		assert( (v.index2 / 3) < boneNodes_.size() );
		assert( (v.index3 / 3) < boneNodes_.size() );
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
			v.pos[0] = -bv.pos.x;
			v.pos[1] = bv.pos.z;
			v.pos[2] = -bv.pos.y;
		}
		else
		{
			v.pos[0] = bv.pos.x;
			v.pos[1] = bv.pos.z;
			v.pos[2] = bv.pos.y;
		}

		v.normal = packNormal( bv.normal );
		v.uv[0] = bv.uv.x;
		v.uv[1] = bv.uv.y;
		v.index = boneVertices_[ bv.vertexIndex ].boneIndex_ * 3;
		v.index2 = boneVertices_[ bv.vertexIndex ].boneIndex2_ * 3;
		v.index3 = boneVertices_[ bv.vertexIndex ].boneIndex3_ * 3;
		v.weight = (uint8) (255.f * boneVertices_[ bv.vertexIndex ].weight_);
		v.weight2 = (uint8) (255.f * boneVertices_[ bv.vertexIndex ].weight2_);
		v.tangent = packNormal( bv.tangent );
		v.binormal = packNormal( bv.binormal );
		assert( (v.index / 3) < boneNodes_.size() );
		assert( (v.index2 / 3) < boneNodes_.size() );
		assert( (v.index3 / 3) < boneNodes_.size() );
		vertices.push_back( v );
	}
}

bool VisualEnvelope::init( INode* node, MFXNode* root )
{
	bool ret = false;

	Modifier* pPhyMod = MFXExport::findPhysiqueModifier( node );
	Modifier* pSkinMod = MFXExport::findSkinMod( node );
	if (pPhyMod)
	{
		IPhysiqueExport* phyExport = (IPhysiqueExport*)pPhyMod->GetInterface( I_PHYINTERFACE );
		if (phyExport)
		{
			if( MATRIX_RETURNED != phyExport->GetInitNodeTM( node, initialObjectTransform_ ) )
			{
				initialObjectTransform_ = node->GetNodeTM( timeNow() );
			}

			// Disable the physique modifier, and grab the original vertex positions so
			// that we only need to grab the weights for each vertex, should also work
			// better with edit mesh modifier etc.
			pPhyMod->DisableMod();
			if (!VisualMesh::init( node, false))
				return false;				
			pPhyMod->EnableMod();

			IPhyContextExport* contextExport = (IPhyContextExport*)phyExport->GetContextInterface( node );
			if( contextExport )
			{
				// Set the blending precision to be rigid.
				contextExport->AllowBlending( TRUE );
				contextExport->ConvertToRigid( TRUE );

				// iterate over all the vertices in this skin
				int nVertices = contextExport->GetNumberVertices();
				for( int i = 0; i < nVertices; i++ )
				{
					Point3 p(initialObjectTransform_ * vertexPositions_[i]);

					// Get the vertex interface for the vertex
					IPhyVertexExport *vex = contextExport->GetVertexInterface( i );
					if (vex->GetVertexType() == RIGID_NON_BLENDED_TYPE)
					{
						IPhyRigidVertex *rv = (IPhyRigidVertex*) vex;

						INode *n = rv->GetNode();

						assert( n != NULL );

						boneVertices_.push_back( BoneVertex( p, boneIndex( n ) ) );
					}
					else if (vex->GetVertexType() == RIGID_BLENDED_TYPE)
					{
						IPhyBlendedRigidVertex* rbv = (IPhyBlendedRigidVertex*) vex;
						int nNodes = rbv->GetNumberNodes();

						typedef BW::multimap< float, int > IndexWeights;
						IndexWeights indexWeights;

						// Iterate over nodes
						for (int i = 0; i < nNodes; i++)
						{
							INode* n = rbv->GetNode(i);

							if( n == NULL )
								continue;

							float weight = rbv->GetWeight(i);

							int bi = boneIndex(n);
							
							// see if this bone index is already being used
							// by this vertex, if it is, add the two bone 
							// weights together. This shouldn't happen, but
							// it has been seen.

							IndexWeights::iterator it = indexWeights.begin();
							while (it != indexWeights.end())
							{
								if (it->second == bi)
								{
									weight += it->first;
									indexWeights.erase( it );
									break;
								}
								it++;
							}
							indexWeights.insert( std::make_pair( weight, bi ));
						}

						// grab the three greatest weights and bone indices 
						// for this vertex

						float weight = 0.f;
						float weight2 = 0.f;
						float weight3 = 0.f;
						int index = 0;

						IndexWeights::iterator it = indexWeights.end();
						it--;
						weight = it->first;
						index = it->second;
						
						int index2 = index;
						int index3 = index;
						
						if (it != indexWeights.begin())
						{
							it--;
							weight2 = it->first;
							index2 = it->second;
							if (it != indexWeights.begin())
							{
								it--;
								weight3 = it->first;
								index3 = it->second;
							}
						}
						float sumWeights = weight + weight2 + weight3;

						boneVertices_.push_back( BoneVertex( p, index, index2, index3,
							weight / sumWeights, weight2 / sumWeights, weight3 / sumWeights ) );
					}
					else
					{
#ifdef _UNICODE
						MessageBox( GetForegroundWindow(),
							L"VisualEnvelope::init - unknown vertex type in physiqued object\n",
							L"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );
#else
						MessageBox( GetForegroundWindow(),
							"VisualEnvelope::init - unknown vertex type in physiqued object\n",
							"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );
#endif
						return false;
					}
					contextExport->ReleaseVertexInterface( vex );
				}
				if (!collectInitialTransforms(phyExport))
				{
#ifdef _UNICODE
					MessageBox( GetForegroundWindow(),
						L"Unable to collect initial transform in physiqued object\n"
						L"The visual may not appear as it should inside the BigWorld engine.",
						L"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );
#else
					MessageBox( GetForegroundWindow(),
						"Unable to collect initial transform in physiqued object\n"
						"The visual may not appear as it should inside the BigWorld engine.",
						"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );
#endif
					return false;
				}

				if (!ExportSettings::instance().allowScale())
				{
					BW::vector< Point3 > scales;
					getScaleValues( scales );
					normaliseInitialTransforms();
				}
				phyExport->ReleaseContextInterface( contextExport );
			}
		}
		pPhyMod->ReleaseInterface(I_PHYINTERFACE, phyExport);

		for (int i = 0; i < boneNodes_.size(); i++)
		{
			MFXNode* x = root->find( boneNodes_[ i ] );

			BW::string id = trimWhitespaces(x->getIdentifier()) + BW::string( "BlendBone" ) + identifier_;
			MFXNode* xx = x->find( id );
			if (!xx)
			{
				xx = new MFXNode;
				xx->setIdentifier( id );

				xx->setTransform( Inverse( initialTransforms_[boneIndex(x->getMaxNode())]) );
				x->addChild( xx );
			}
			x = xx;

			x->include( true );
			x->includeAncestors();
		}

		ret = checkDXMaterials("_skinned");
	}
	else if (pSkinMod)
	{
		ISkin* pSkin = (ISkin*)pSkinMod->GetInterface( I_SKIN );
		if (pSkin)
		{
			ISkinContextData* pContext = pSkin->GetContextInterface(node);
			if (pContext)
			{
				pSkin->GetSkinInitTM( node, initialObjectTransform_, true );
				
				pSkinMod->DisableMod();
				if (!VisualMesh::init( node, false ))
					return false;
				pSkinMod->EnableMod();

				int nVertices = pContext->GetNumPoints();
				for (int i = 0; i < nVertices; i++)
				{
					int nNodes = pContext->GetNumAssignedBones(i);

					typedef BW::multimap< float, int > IndexWeights;
					IndexWeights indexWeights;

					assert( i < vertexPositions_.size() );
					Point3 p(initialObjectTransform_ * vertexPositions_[i]);

					for (int j = 0; j < nNodes; j++)
					{
						Matrix3 transform;
						INode* n = pSkin->GetBone(pContext->GetAssignedBone(i, j));

						assert( n != NULL );

						float weight = pContext->GetBoneWeight(i, j);
						
						indexWeights.insert( std::make_pair( weight, boneIndex(n) ));
					}

					if (nNodes == 0)
					{
						indexWeights.insert( std::make_pair( 1.f, boneIndex( node ) ) );
					}

					float weight = 0.f;
					float weight2 = 0.f;
					float weight3 = 0.f;
					int index = 0;

					IndexWeights::iterator it = indexWeights.end();
					it--;
					weight = it->first;
					index = it->second;

					int index2 = index;
					int index3 = index;
					
					if (it != indexWeights.begin())
					{
						it--;
						weight2 = it->first;
						index2 = it->second;
						if (it != indexWeights.begin())
						{
							it--;
							weight3 = it->first;
							index3 = it->second;
						}
					}
					float sumWeights = weight + weight2 + weight3;

					boneVertices_.push_back( BoneVertex( p, index, index2, index3,
						weight / sumWeights, weight2 / sumWeights, weight3 / sumWeights ) );
				}

				// Some times there are 0 vertices in the skin modifier, 
				// if this is the case, we skin all vertices to the parent node
				if (nVertices == 0)
				{
					int index = boneIndex( node );
					for (size_t i = 0; i < vertexPositions_.size(); i++)
					{
						Point3 p(initialObjectTransform_ * vertexPositions_[i]);
						boneVertices_.push_back( BoneVertex( p, index ) );
					}
				}

				collectInitialTransforms( pSkin );

				if (!ExportSettings::instance().allowScale())
				{
					BW::vector< Point3 > scales;
					getScaleValues( scales );
					normaliseInitialTransforms();
				}
			}
		}

		for (int i = 0; i < boneNodes_.size(); i++)
		{
			MFXNode* x = root->find( boneNodes_[ i ] );

			BW::string id = trimWhitespaces(x->getIdentifier()) + BW::string( "BlendBone" ) + identifier_;
			MFXNode* xx = x->find( id );
			if (!xx)
			{
				xx = new MFXNode;
				xx->setIdentifier( id );

				xx->setTransform( Inverse( initialTransforms_[boneIndex(x->getMaxNode())]) );
				x->addChild( xx );
			}
			x = xx;

			x->include( true );
			x->includeAncestors();
		}
		ret = checkDXMaterials("_skinned");
	}

	if (ret)
	{
		initialPoseVertices();
		
		//clear the normals currently in there

		BVVector::iterator vit = vertices_.begin();
		BVVector::iterator vend = vertices_.end();
		while (vit != vend)
		{
			BloatVertex& bv = *vit++;
			bv.normal = Point3::Origin;
		}
		
		createNormals();
		getNormalsFromModifier(node);

		bb_.setBounds(	reinterpret_cast<const Vector3&>( vertices_[0].pos ),
						reinterpret_cast<const Vector3&>( vertices_[0].pos ) );
		for (int i = 1; i < vertices_.size(); i++)
		{
			bb_.addBounds( reinterpret_cast<const Vector3&>( vertices_[i].pos ) );
		}

		flipTriangleWindingOrder();
		sortTriangles();

		if (ExportSettings::instance().bumpMapped())
		{
			makeBumped();
		}

		originalIdentifier_ = identifier_;
	}

	return ret;
}

bool VisualEnvelope::save( DataSectionPtr pVisualSection, 
						   DataSectionPtr spExistingVisual, 
						   const BW::string& primitiveFile,
						   bool useIdentifier )
{
	DataSectionPtr pFile = BWResource::openSection( primitiveFile );
	if (!pFile)
		pFile = new BinSection( primitiveFile, new BinaryBlock( 0, 0, "BinaryBlock/VisualEnvelope" ) );

	if (pFile)
	{
		PGVector primGroups;
		Moo::IndicesHolder indices;

		if (!createPrimitiveGroups( primGroups, indices ))
			return false;

		if (indices.entrySize() != 2)
		{
#ifdef _UNICODE
			::MessageBox(
				0,
				L"Skinned models cannot have more than 65535 vertices!",
				L"Vertex count error!",
				MB_ICONERROR );
#else
			::MessageBox(
				0,
				"Skinned models cannot have more than 65535 vertices!",
				"Vertex count error!",
				MB_ICONERROR );
#endif
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

		pFile->writeBinary( identifier_ + ".indices", f.output() );

		f = QuickFileWriter();

		BW::vector<Vector2> uv2;
		BW::vector<DWORD> colours;

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

		BW::string vertName;
		vertName = identifier_ + ".vertices";
		pFile->writeBinary( vertName, f.output() );

		// Write extra vertex data streams
		BW::vector<BW::string> streams;
		if (dualUV_)
		{
			// Retreive the second UV channel.

			BVVector::iterator it = vertices_.begin();
			BVVector::iterator end = vertices_.end();
			while (it!=end)
			{
				BloatVertex& bv = *it++;
				Vector2 uv;
				uv.x = bv.uv2.x;
				uv.y = bv.uv2.y;				
				uv2.push_back( uv );
			}

			f = QuickFileWriter();
			f << uv2;

			streams.push_back( identifier_ + ".uv2" );

			pFile->writeBinary( streams.back(), f.output() );
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

			streams.push_back( identifier_ + ".colour" );

			pFile->writeBinary( streams.back(), f.output() );
		}

		pFile->save( primitiveFile );

		DataSectionPtr pRenderSet = pVisualSection->newSection( "renderSet" );
		if (pRenderSet)
		{
			pRenderSet->writeBool( "treatAsWorldSpaceObject", true );
			for (int i = 0; i < boneNodes_.size(); i++)
			{
				DataSectionPtr pNode = pRenderSet->newSection( "node" );
				if (pNode)
				{
#ifdef _UNICODE
					pNode->setString( trimWhitespaces( bw_wtoutf8( boneNodes_[i]->GetName() ) ) + BW::string("BlendBone") + originalIdentifier_ );
#else
					pNode->setString( trimWhitespaces( boneNodes_[i]->GetName() ) + BW::string("BlendBone") + originalIdentifier_ );
#endif
				}
			}
			

			DataSectionPtr pGeometry = pRenderSet->newSection( "geometry" );
			if (pGeometry)
			{
				pGeometry->writeString( "vertices", vertName );
				pGeometry->writeStrings( "stream", streams );
				pGeometry->writeString( "primitive", identifier_ + ".indices" );

				MaterialVector::iterator it = materials_.begin();
				MaterialVector::iterator end = materials_.end();

				uint32 primGroup = 0;
				for (uint32 i = 0; i < materials_.size(); i++)
				{
					Material& mat = materials_[i];
					if (mat.inUse) mat.save( pGeometry,
							spExistingVisual,
							primGroup++,
							Material::SOFT_SKIN, 
							/*boneNodes_.size() <= 17*/ true  );
				}
			}
		}
	}
	return true;
}


namespace
{

// Helper functions for the skin splitter to access bone weights and indices

void getWeights( const VisualEnvelope::BoneVertex& v, float* weights )
{
	weights[0] = v.weight_;
	weights[1] = v.weight2_;
	weights[2] = v.weight3_;
}


void getIndices( const VisualEnvelope::BoneVertex& v, int* indices )
{
	indices[0] = v.boneIndex_;
	indices[1] = v.boneIndex2_;
	indices[2] = v.boneIndex3_;
}

}


/**
 *
 */
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
					it->boneIndex_ = nodeRemap[it->boneIndex_];
					it->boneIndex2_ = nodeRemap[it->boneIndex2_];
					it->boneIndex3_ = nodeRemap[it->boneIndex3_];
				}

				pEnvelope->sortTriangles();

				pEnvelope->vertexColours_ = vertexColours_;
				pEnvelope->dualUV_ = dualUV_;

				// Create a name for the new mesh piece
				char newName[1024];
				bw_snprintf( newName, 1024, "%s_split_%d", identifier_.c_str(), splitIndex );
				pEnvelope->identifier_ = newName;

				pEnvelope->bb_ = bb_;

				pEnvelope->originalIdentifier_ = originalIdentifier_;

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
	for (uint32 i = 0; i < indices.size(); i++)
	{
		DataSectionPtr pIndex = pIndices->newSection( "Index" );
		pIndex->setInt( (uint32)indices[i] );
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
			pVertex->writeLong( "Tangent", tbbvvIt->tangent );
			pVertex->writeLong( "Binormal", tbbvvIt->binormal );
		}
	}

	spFileRoot->save( xmlFile );

	return true;
}


int VisualEnvelope::boneIndex( INode* node )
{
	NodeVector::iterator it = std::find( boneNodes_.begin(), boneNodes_.end(), node );
	
	if (it != boneNodes_.end())
	{
		int ret = 0;
		while (it != boneNodes_.begin())
		{
			++ret;
			--it;
		}
		return ret;
	}
	
	boneNodes_.push_back( node );
	return static_cast<int>(boneNodes_.size() - 1);
}

BW_END_NAMESPACE

