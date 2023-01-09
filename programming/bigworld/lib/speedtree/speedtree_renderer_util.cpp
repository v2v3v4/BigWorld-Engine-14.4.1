/***
 *

This file contains the implementations of various speedtree renderer utilities.

 *
 ***/
#include "pch.hpp"

#include "speedtree_config.hpp"

// BW Tech Headers
#include "cstdmf/debug.hpp"

#if SPEEDTREE_SUPPORT // -------------------------------------------------------

#include "resmgr/bwresource.hpp"

// SpeedTree API
#include <SpeedTreeRT.h>
#include <SpeedTreeAllocator.h>

#include "speedtree_renderer_util.hpp"


BW_BEGIN_NAMESPACE


namespace speedtree
{	

//TODO: ensure all of these are managed pool....

// The common vertex/index buffer storage:
// Fronds/Branches:
CommonTraits::VertexBufferPtr		CommonTraits::s_vertexBuffer_;
STVector< BranchVertex >			CommonTraits::s_vertexList_;

CommonTraits::IndexBufferPtr		CommonTraits::s_indexBuffer_;
STVector< uint32 >					CommonTraits::s_indexList_;

// Leaves:
LeafTraits::VertexBufferPtr			LeafTraits::s_vertexBuffer_;
STVector< LeafVertex >				LeafTraits::s_vertexList_;

LeafTraits::IndexBufferPtr			LeafTraits::s_indexBuffer_;
STVector< uint32 >					LeafTraits::s_indexList_;

// Billboards:
BillboardTraits::VertexBufferPtr	BillboardTraits::s_vertexBuffer_;
STVector< BillboardVertex >			BillboardTraits::s_vertexList_;

BillboardTraits::IndexBufferPtr		BillboardTraits::s_indexBuffer_;
STVector< uint32 >					BillboardTraits::s_indexList_;

}

speedtree::LeafRenderData speedtree::createLeafRenderData(
	CSpeedTreeRT      & speedTree,
	CSpeedWind	      & speedWind,
	const BW::string & filename,
	const BW::string & datapath,
	int                 windAnglesCount,
	float             & leafRockScalar,
	float             & leafRustleScalar )
{
	BW_GUARD;
	LeafRenderData result =	getCommonRenderData< LeafTraits >( speedTree, datapath );

	// extract geometry for all lod levels
	static CSpeedTreeRT::SGeometry geometry;
	speedTree.GetGeometry( geometry, SpeedTree_LeafGeometry );

	typedef CSpeedTreeRT::SGeometry::SLeaf SLeaf;
	typedef SLeaf::SCard SCard;
	typedef SLeaf::SMesh SMesh;

	if (geometry.m_nNumLeafLods > 0)
	{
		const SLeaf & testData = geometry.m_pLeaves[0];
		const SCard & testCard = testData.m_pCards[0];
		if (testCard.m_pTexCoords[0] - testCard.m_pTexCoords[2] == 1.0 &&
			testCard.m_pTexCoords[5] - testCard.m_pTexCoords[3] == 1.0)
		{
			speedtreeError(filename.c_str(), 
				"Tree looks like it's not using composite maps.");
		}
	}

	static const int c_vertPerCard = 4;
	static const int c_indPerCard  = 6;
	int leafVerticesCount = 0;
	for (int lod=0; lod<geometry.m_nNumLeafLods; ++lod)
	{
		// we need to go through all leaves data
		// to find out how many vertices we need
		// to reserve for leaf indices and vertices.

		const SLeaf & data = geometry.m_pLeaves[lod];

		int leafCount = data.m_nNumLeaves;
		for (int leaf=0; leaf<leafCount; ++leaf)
		{
			int cardIndex = data.m_pLeafCardIndices[leaf];
			const SMesh * leafMesh = data.m_pCards[cardIndex].m_pMesh;
			if (leafMesh != NULL)
			{
				leafVerticesCount += leafMesh->m_nNumVertices;
			}
			else
			{				
				leafVerticesCount += c_vertPerCard;
			}
		}
	}

	result.verts_ = LeafTraits::s_vertexList_.newSlot( leafVerticesCount );
	for (int lod=0; lod<geometry.m_nNumLeafLods; ++lod)
	{
		typedef LeafTraits::RenderDataType::LodData LodData;
		result.lod_.push_back( LodData() );
		LodData & lodData = result.lod_.back();
		const SLeaf & data = geometry.m_pLeaves[lod];

        if (lod == 0)
        {
            leafRockScalar   = data.m_fLeafRockScalar;
            leafRustleScalar = data.m_fLeafRustleScalar;
        }

		int leafCount = data.m_nNumLeaves;
		if (leafCount > 0)
		{
			lodData.index_ = LeafTraits::s_indexList_.newSlot( );
			// build the actual leaf mesh data
			{
				BW::vector<const SMesh *> meshes;
				for (int leaf=0; leaf<leafCount; ++leaf)
				{
					// retrieve leaf card										
					int cardIndex = data.m_pLeafCardIndices[leaf];
					const SCard & leafCard = data.m_pCards[cardIndex];
					const SMesh * leafMesh = data.m_pCards[cardIndex].m_pMesh;

					// (in speedtree, z is up)
					Vector3 cardCenter(
						+data.m_pCenterCoords[leaf*3 + 0],
						+data.m_pCenterCoords[leaf*3 + 1],
						-data.m_pCenterCoords[leaf*3 + 2]);

					Vector3 cardNormal = Vector3(
						-data.m_pBinormals[leaf*12 + 0],
						-data.m_pBinormals[leaf*12 + 1],
						+data.m_pBinormals[leaf*12 + 2]);

					Vector3 cardTangent = Vector3(
						-data.m_pTangents[leaf*12 + 0],
						-data.m_pTangents[leaf*12 + 1],
						+data.m_pTangents[leaf*12 + 2]);

					Vector3 cardBinormal = Vector3(
						+data.m_pNormals[leaf*12 + 0],
						+data.m_pNormals[leaf*12 + 1],
						-data.m_pNormals[leaf*12 + 2]);

					Matrix cardTransform;
					cardTransform.row(0, Vector4(cardTangent, 0)  );
					cardTransform.row(1, Vector4(cardBinormal, 0) );
					cardTransform.row(2, Vector4(cardNormal, 0)   );
					cardTransform.row(3, Vector4(cardCenter, 1)   );

					// no, use leaf card
					//const int baseIndex = std::distance(vbBegin, vbIt);
					const size_t baseIndex = LeafTraits::s_vertexList_.size() - result.verts_->start();
					// does this card have a 
					// mesh or a billboarded card?
					if (leafMesh != NULL)
					{
						// yes, add mesh vertices
						for (int v=0; v<leafMesh->m_nNumVertices; ++v)
						{
							LeafVertex vert;

							// position (in speedtree, z is up) 
							Vector3 position(
								+leafMesh->m_pCoords[v*3 + 0],
								+leafMesh->m_pCoords[v*3 + 1],
								-leafMesh->m_pCoords[v*3 + 2]);
							vert.pos_ = cardTransform.applyPoint( position );

							// normals							
							Vector3 normal(
								+leafMesh->m_pNormals[v*3 + 0],
								+leafMesh->m_pNormals[v*3 + 1],
								-leafMesh->m_pNormals[v*3 + 2]);
							vert.normal_ = cardTransform.applyVector(normal);

							#if SPT_ENABLE_NORMAL_MAPS
								Vector3 tangent(
									+leafMesh->m_pTangents[v*3 + 0],
									+leafMesh->m_pTangents[v*3 + 1],
									-leafMesh->m_pTangents[v*3 + 2]);
								vert.tangent_ = cardTransform.applyVector(tangent);
							#endif // SPT_ENABLE_NORMAL_MAPS

							// wind info
							float fWindMatrixIndex1 = 
								float( int( data.m_pWindMatrixIndices[0][leaf] *
											10.0f / speedWind.GetNumWindMatrices() ) );

							float fWindMatrixWeight1 = c_fWindWeightCompressionRange * 
														data.m_pWindWeights[0][leaf];
							float fWindMatrixIndex2 = 
								float( int( data.m_pWindMatrixIndices[1][leaf] *
											10.0f / speedWind.GetNumWindMatrices() ) );

							float fWindMatrixWeight2 = c_fWindWeightCompressionRange *
														data.m_pWindWeights[1][leaf];

							vert.tcWindInfo_.z = fWindMatrixIndex1 + fWindMatrixWeight1;
							vert.tcWindInfo_.w = fWindMatrixIndex2 + fWindMatrixWeight2;

							// texture coords
							vert.tcWindInfo_.x = leafMesh->m_pTexCoords[v*2 + 0];
							vert.tcWindInfo_.y = leafMesh->m_pTexCoords[v*2 + 1];

							// rock and rustle info
							vert.rotGeomInfo_.x = DEG_TO_RAD(leafCard.m_afAngleOffsets[0]);
							vert.rotGeomInfo_.y = DEG_TO_RAD(leafCard.m_afAngleOffsets[1]);

							// geometry info
							vert.rotGeomInfo_.z = 1.0f;
							vert.rotGeomInfo_.w = 1.0f;

							// extra info
							vert.extraInfo_.x = float(leaf % windAnglesCount);
							vert.extraInfo_.y = c_vertPerCard;
							vert.extraInfo_.z = data.m_pDimming[leaf];
							//TODO: pack the vertex data better.

							// pivot into
							vert.pivotInfo_.x = 0.0f;
							vert.pivotInfo_.y = 0.0f;

							LeafTraits::s_vertexList_.push_back( vert );
						}
						lodData.index_->count( lodData.index_->count() + leafMesh->m_nNumIndices );

						// add mesh indices
						for (int v=0; v<leafMesh->m_nNumIndices; ++v)
						{
							size_t index = baseIndex + leafMesh->m_pIndices[v] /*+ result.verts_->start()*/;
							MF_ASSERT( index < UINT_MAX );
							LeafTraits::s_indexList_.push_back( ( uint32 )index );
						}						
					}
					else
					{
						// no, add card vertices
						for (int v=0; v<c_vertPerCard; ++v)
						{
							LeafVertex vert;

							vert.pos_ = cardCenter;

							vert.normal_ = Vector3(
									+data.m_pNormals[leaf*12 + v*3 + 0],
									+data.m_pNormals[leaf*12 + v*3 + 1],
									-data.m_pNormals[leaf*12 + v*3 + 2]);

							#if SPT_ENABLE_NORMAL_MAPS
								vert.tangent_ = Vector3(
									+data.m_pTangents[leaf*12 + v*3 + 0],
									+data.m_pTangents[leaf*12 + v*3 + 1],
									-data.m_pTangents[leaf*12 + v*3 + 2]);
							#endif // SPT_ENABLE_NORMAL_MAPS

							// wind info
							//TODO: put this in a function
							float fWindMatrixIndex1 = 
								float( int( data.m_pWindMatrixIndices[0][leaf] *
										10.0f / speedWind.GetNumWindMatrices() ) );

							float fWindMatrixWeight1 = c_fWindWeightCompressionRange *
														data.m_pWindWeights[0][leaf];
							float fWindMatrixIndex2 = 
								float( int( data.m_pWindMatrixIndices[1][leaf] *
										10.0f / speedWind.GetNumWindMatrices() ) );

							float fWindMatrixWeight2 = c_fWindWeightCompressionRange *
														data.m_pWindWeights[1][leaf];

							vert.tcWindInfo_.z = fWindMatrixIndex1 + fWindMatrixWeight1;
							vert.tcWindInfo_.w = fWindMatrixIndex2 + fWindMatrixWeight2;

							// texture coords
							vert.tcWindInfo_.x = leafCard.m_pTexCoords[v*2 + 0];
							vert.tcWindInfo_.y = leafCard.m_pTexCoords[v*2 + 1];

							// rock and rustle info
							vert.rotGeomInfo_.x = DEG_TO_RAD(leafCard.m_afAngleOffsets[0]);
							vert.rotGeomInfo_.y = DEG_TO_RAD(leafCard.m_afAngleOffsets[1]);

							// geometry info
							vert.rotGeomInfo_.z = leafCard.m_fWidth;
							vert.rotGeomInfo_.w = leafCard.m_fHeight;

							// extra info
							vert.extraInfo_.x = float(leaf % windAnglesCount);
							vert.extraInfo_.y = float(v);
							vert.extraInfo_.z = data.m_pDimming[leaf];

							// pivot into
							vert.pivotInfo_.x = leafCard.m_afPivotPoint[0] - 0.5f;
							vert.pivotInfo_.y = -(leafCard.m_afPivotPoint[1] - 0.5f);

							//++vbIt;
							LeafTraits::s_vertexList_.push_back( vert );
						}
						lodData.index_->count( lodData.index_->count() 
												+ c_indPerCard );

						// add card indices
						static const int indexOffset[] = {0, 1, 2, 0, 2, 3};
						for (int i=0; i<c_indPerCard; ++i)
						{
							size_t index = baseIndex + indexOffset[i];
							MF_ASSERT( index <= UINT_MAX );
							LeafTraits::s_indexList_.push_back( ( uint32 )index );

						}
					}
				}
			}
		}
	}

	return result;
}

speedtree::TreePartRenderData< speedtree::BillboardVertex > speedtree::createBillboardRenderData(
	CSpeedTreeRT      & speedTree,
	const BW::string & datapath )
{
	BW_GUARD;
	TreePartRenderData< BillboardVertex > result = getCommonRenderData< BillboardTraits >( speedTree, datapath );

	// generate billboard geometry
	speedTree.SetLodLevel(0);
	static CSpeedTreeRT::SGeometry geometry;
	speedTree.GetGeometry( geometry, SpeedTree_BillboardGeometry );

	typedef CSpeedTreeRT::SGeometry::S360Billboard S360Billboard;
	S360Billboard & data360 = geometry.m_s360Billboard;
	if (data360.m_nNumImages < 0)
	{
		// If m_s360Billboard.m_nNumImages is < 0,
		// UpdateBillboardGeometry crashes, so return, no billboards here.
		ERROR_MSG( "createBillboardRenderData: tree '%s' does not have billboards. "
			"Please ensure it is exported for realtime.\n", datapath.c_str() );
		return result;
	}

	speedTree.UpdateBillboardGeometry( geometry );

	// horizontal billboard
	typedef CSpeedTreeRT::SGeometry::SHorzBillboard SHorzBillboard;
	SHorzBillboard & dataHor = geometry.m_sHorzBillboard;
	bool drawHorizontalBB = dataHor.m_pCoords != NULL;

	// 360 degrees billboards
	const int bbCount = data360.m_nNumImages + (drawHorizontalBB ? 1 : 0);

	if (data360.m_pCoords == NULL || data360.m_nNumImages <= 0)
	{
		INFO_MSG( "createBillboardRenderData: tree '%s' does not have billboards, "
			"it will not be rendered if far away.\n", datapath.c_str() );
		return result;
	}

	typedef BillboardTraits::RenderDataType::LodData LodData;
	result.lod_.push_back( LodData() );
	LodData & lodData = result.lod_.back();

	static const int vertexCount = 6;
	{
		//-- now create optimized data for instancing.
		result.verts_  = BillboardTraits::s_vertexList_.newSlot(bbCount * vertexCount);
		lodData.index_ = BillboardTraits::s_indexList_.newSlot(bbCount * vertexCount);

		for (int32 idx = 0; idx < bbCount * vertexCount; ++idx)
		{
			BillboardTraits::s_indexList_.push_back(idx);
		}

		// fill vertex buffer	
		Vector3 center(0, 0.5f * data360.m_fHeight, 0);
		for (int bb = 0; bb < bbCount; ++bb)
		{
			// matching the billboard rotation with 
			// the tree's took a bit of experimentation. 
			// If, in future versions of speedtree, it 
			// stops matching, this is the place to tweak.
			Matrix bbSpace(Matrix::identity);
			bbSpace.setRotateY( -bb*(2*MATH_PI/data360.m_nNumImages) - (MATH_PI/2) );

			static const short vIdx[vertexCount] = { 0, 1, 2, 0, 2, 3 };

			if ( bb<(bbCount - (drawHorizontalBB ? 1 : 0)) )
			{
				static const float xSignal[4] = { +1.0f, -1.0f, -1.0f, +1.0f };
				static const float ySignal[4] = { +1.0f, +1.0f, -0.0f, -0.0f };

				for (int v = 0; v < vertexCount; ++v)
				{
					speedtree::BillboardVertex vertex;

					Vector3 tangVector = bbSpace.applyToUnitAxisVector(0);
					vertex.pos_.x = 0.5f * tangVector.x * xSignal[vIdx[v]] * data360.m_fWidth;
					vertex.pos_.y = ySignal[vIdx[v]] * data360.m_fHeight;
					vertex.pos_.z = 0.5f * tangVector.z * xSignal[vIdx[v]] * data360.m_fWidth;

					vertex.lightNormal_  = vertex.pos_ - center;
					vertex.lightNormal_.normalise();

					vertex.alphaNormal_.x = -tangVector.z;
					vertex.alphaNormal_.y = +tangVector.y;
					vertex.alphaNormal_.z = +tangVector.x;

					#if SPT_ENABLE_NORMAL_MAPS
						vertex.tangent_  = Vector3(0.0f, 1.0f, 0.0f);
						vertex.binormal_ = tangVector;
					#endif // SPT_ENABLE_NORMAL_MAPS

					vertex.tc_.x = data360.m_pTexCoordTable[(bb)*8 + vIdx[v]*2 + 0];
					vertex.tc_.y = data360.m_pTexCoordTable[(bb)*8 + vIdx[v]*2 + 1];

					BillboardTraits::s_vertexList_.push_back(vertex);
				}
			}
			else
			{
				for (int v = 0; v < vertexCount; ++v)
				{
					speedtree::BillboardVertex vertex;

					vertex.pos_.x = +dataHor.m_pCoords[vIdx[v]*3 + 0];
					vertex.pos_.y = +dataHor.m_pCoords[vIdx[v]*3 + 1];
					vertex.pos_.z = +dataHor.m_pCoords[vIdx[v]*3 + 2];

					vertex.lightNormal_  = Vector3(0, 1, 0);
					vertex.alphaNormal_  = Vector3(0, 1, 0);

					#if SPT_ENABLE_NORMAL_MAPS
						vertex.tangent_  = Vector3(0.0f, 0.0f, 1.0f);
						vertex.binormal_ = Vector3(1.0f, 0.0f, 0.0f);
					#endif // SPT_ENABLE_NORMAL_MAPS

					vertex.tc_.x = dataHor.m_afTexCoords[vIdx[v]*2 + 0];
					vertex.tc_.y = dataHor.m_afTexCoords[vIdx[v]*2 + 1];

					BillboardTraits::s_vertexList_.push_back(vertex);
				}
			}	
		}
	}

	return result;
}

BW_END_NAMESPACE

#endif // SPEEDTREE_SUPPORT
