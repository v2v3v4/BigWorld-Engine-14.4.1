#include "pch.hpp"

#include "speedtree_collision.hpp"
#include "billboard_optimiser.hpp"
#include "speedtree_util.hpp"
#include "speedtree_tree_type.hpp"
#include "speedtree_renderer_common.hpp"
#include "moo/vertex_streams.hpp"
#include "moo/primitive_helper.hpp"

// SpeedTree API
#include <SpeedTreeRT.h>
#include "speedtree_renderer_util.hpp"

BW_BEGIN_NAMESPACE

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
	using namespace speedtree;

	//-- templated version of bind model method.
	//-- Note: To minimize redundant stage changes and with knowledge that unique tree's types count
	//--	   is around 8-12 we can combine all geometry data including the lod's data for each
	//--	   individual tree part in one big buffer. But the bigworld's engine is dynamic by the
	//--	   nature i.e. it actively uses data streaming so we need to care about dynamic inserting
	//--	   and removing desired tree types from these index and vertex buffers. To make it works
	//--	   each tree type has information about its location in these big buffers. This information
	//--	   is stored in the BufferSlot class.
	//--	   Because we have dynamic streaming in a background thread we have to use some sync
	//--	   primitives to prevent conditional racing.
	//----------------------------------------------------------------------------------------------
	template <class BASE>
	void bindModelData()
	{
		//-- Check for existence.
		if (BASE::s_indexBuffer_ == NULL)
		{
			BASE::s_indexBuffer_ = new IndexBufferWrapper;
		}

		//-- copy the data.
		{
			SimpleMutexHolder mutexHolder(TSpeedTreeType::s_vertexListLock_);

			if (!BASE::s_indexList_.empty() && BASE::s_indexList_.dirty())
			{
				if (BASE::s_indexBuffer_->reset(int(BASE::s_indexList_.size())))
				{
					//--TODO: 32-bit indices....
					BASE::s_indexBuffer_->copy( (int*)&BASE::s_indexList_[0] );
					BASE::s_indexList_.dirty( false );
				}
			}
		}

		//-- Check for existence.
		if (BASE::s_vertexBuffer_ == NULL)
		{
			BASE::s_vertexBuffer_ = new VertexBufferWrapper<typename BASE::VertexType>;
		}

		//-- copy the data.
		{
			SimpleMutexHolder mutexHolder(TSpeedTreeType::s_vertexListLock_);

			if (!BASE::s_vertexList_.empty() && BASE::s_vertexList_.dirty())
			{
				if (BASE::s_vertexBuffer_->reset(int(BASE::s_vertexList_.size())))
				{
					BASE::s_vertexBuffer_->copy(&BASE::s_vertexList_[0]);
					BASE::s_vertexList_.dirty(false);
				}
			}
		}

		//-- now bind the buffers.
		BASE::s_vertexBuffer_->activate();
		BASE::s_indexBuffer_->activate();
	}
	
	//-- Note: Here we are using templated version of speedtree drawing. The main method called
	//--	   drawTreePart is a template and uses *DrawingTraits structures as a template argument.
	//--	   This approach widely known as a "template strategy pattern". There are a lot of examples
	//--	   of using this pattern, one of them is Traits structure used in the std::string class to
	//--	   describe some special properties of char type (char_traits<char>).

	//-- make some short cuts to improve readability.
	typedef TSpeedTreeType				TreeType;
	typedef Moo::EffectMaterial			Effect;
	typedef TSpeedTreeType::Instances	Instances;
	typedef TSpeedTreeType::LodData		LodData;

	//----------------------------------------------------------------------------------------------
	struct BranchDrawingCommonTraits
	{
		typedef BranchRenderData RenderData;
		typedef CommonTraits	 TreePartTraits;

		static void				setRenderStates	 (TreeType& rt, Effect& eff, const ConstantsMap& cm)	{ rt.setBranchRenderStates(eff, cm); }
		static RenderData&		getRenderData	 (TreeType& rt)											{ return rt.treeData_.branches_; }
		static D3DPRIMITIVETYPE	getPrimitiveType ()														{ return D3DPT_TRIANGLESTRIP; }	
		static uint				getPrimitiveCount(const RenderData& rd, uint lod)						{ return rd.lod_[lod].index_->count() - 2; }
	};

	//----------------------------------------------------------------------------------------------
	struct FrondDrawingCommonTraits
	{
		typedef BranchRenderData RenderData;
		typedef CommonTraits	 TreePartTraits;

		static void				setRenderStates	 (TreeType& rt, Effect& eff, const ConstantsMap& cm)	{ rt.setFrondRenderStates(eff, cm); }
		static RenderData&		getRenderData	 (TreeType& rt)											{ return rt.treeData_.fronds_; }
		static D3DPRIMITIVETYPE	getPrimitiveType ()														{ return D3DPT_TRIANGLESTRIP; }
		static uint				getPrimitiveCount(const RenderData& rd, uint lod)						{ return rd.lod_[lod].index_->count() - 2; }
	};

	//----------------------------------------------------------------------------------------------
	struct LeafDrawingCommonTraits
	{
		typedef LeafRenderData	RenderData;
		typedef LeafTraits		TreePartTraits;

		static void				setRenderStates	 (TreeType& rt, Effect& eff, const ConstantsMap& cm)	{ rt.setLeafRenderStates(eff, cm); }
		static RenderData&		getRenderData	 (TreeType& rt)											{ return rt.treeData_.leaves_; }
		static D3DPRIMITIVETYPE	getPrimitiveType ()														{ return D3DPT_TRIANGLELIST; }
		static uint				getPrimitiveCount(const RenderData& rd, uint lod)						{ return rd.lod_[lod].index_->count() / 3; }
	};

	//----------------------------------------------------------------------------------------------
	struct BillboardDrawingCommonTraits
	{
		typedef BillboardRenderData RenderData;
		typedef BillboardTraits		TreePartTraits;

		static void				setRenderStates	 (TreeType& rt, Effect& eff, const ConstantsMap& cm)	{ rt.setBillboardRenderStates(eff, cm); }
		static RenderData&		getRenderData	 (TreeType& rt)											{ return rt.treeData_.billboards_; }
		static D3DPRIMITIVETYPE	getPrimitiveType ()														{ return D3DPT_TRIANGLELIST; }
		static uint				getPrimitiveCount(const RenderData& rd, uint lod)						{ return rd.lod_[lod].index_->count() / 3; }
	};

	//----------------------------------------------------------------------------------------------
	struct BranchDrawingTraits : public BranchDrawingCommonTraits
	{
		static uint				getLodCount		 ()									{ return TSpeedTreeType::G_MAX_LOD_COUNT; }		
		static Instances&		getInstances	 (TreeType& rt, uint lod)			{ return rt.drawDataCollector_.m_branches[lod]; }
	};

	//----------------------------------------------------------------------------------------------
	struct FrondDrawingTraits : public FrondDrawingCommonTraits
	{
		static uint				getLodCount		 ()									{ return TSpeedTreeType::G_MAX_LOD_COUNT; }	
		static Instances&		getInstances	 (TreeType& rt, uint lod)			{ return rt.drawDataCollector_.m_fronds[lod]; }
	};

	//----------------------------------------------------------------------------------------------
	struct LeafDrawingTraits : public LeafDrawingCommonTraits
	{
		static uint				getLodCount		 ()									{ return TSpeedTreeType::G_MAX_LOD_COUNT; }
		static Instances&		getInstances	 (TreeType& rt, uint lod)			{ return rt.drawDataCollector_.m_leaves[lod]; }
	};

	//----------------------------------------------------------------------------------------------
	struct BillboardDrawingTraits : public BillboardDrawingCommonTraits
	{
		static uint				getLodCount		 ()									{ return 1; }
		static Instances&		getInstances	 (TreeType& rt, uint lod)			{ return rt.drawDataCollector_.m_billboards; }
	};

	//-- Note: This method assumes that most expensive driver operations are changing shader, vertex
	//--	   declaration and shader's constants. This assumption based on the MSDN's paper 
	//--	   "Accurately Profiling Direct3D API Calls (Direct3D9)".
	//----------------------------------------------------------------------------------------------
	template<typename DrawingTraits, bool instanced>
	void drawTreesPart(DrawingState& ds, bool clear)
	{
		BW_GUARD;

		//-- 1. set desired shader only once for all trees.
		if (ds.m_material->begin())
		{
			//-- Note: assume that we always have one pass in each technique. That is true for almost
			//--	   all shaders in the engine and is definitely true for SpeedTree's shaders.
			if (ds.m_material->beginPass(0))
			{
				//-- set properties actual for current tree part. I.e. vertex declaration, instancing
				//-- vertex buffer and so on.
				Moo::rc().setVertexDeclaration(ds.m_vertexFormat->declaration());
				bindModelData<typename DrawingTraits::TreePartTraits>();

				if (instanced)
				{
					Moo::rc().device()->SetStreamSourceFreq(
						Moo::InstancingStream::STREAM_NUMBER, (D3DSTREAMSOURCE_INSTANCEDATA | 1)
						);
				}

				//-- 2. then iterate over the whole set of currently active tree types.
				TSpeedTreeType::TreeTypesMap::const_iterator it = TSpeedTreeType::s_typesMap_.begin();
				for (; it != TSpeedTreeType::s_typesMap_.end(); ++it)
				{
					TSpeedTreeType&					 rt = *it->second;
					const typename DrawingTraits::RenderData& rd = DrawingTraits::getRenderData(rt);

					//-- set properties actual for current tree type.
					DrawingTraits::setRenderStates(rt, *ds.m_material, *ds.m_constantsMap);

					if (instanced)
					{
						ds.m_material->commitChanges();
					}

					//-- 3. then iterate over the whole set of LOD levels for each individual tree type.
					for (uint lod = 0; lod < DrawingTraits::getLodCount(); ++lod)
					{
						//-- skip lod if it's empty.
						TSpeedTreeType::Instances& instances = DrawingTraits::getInstances(rt, lod);
						if (instances.empty())
							continue;

						//-- prepare common constants.
						uint instCount = uint(instances.size());
						uint vertCount = rd.verts_->count();
						int  vertBase  = rd.verts_->start();
						int  primCount = DrawingTraits::getPrimitiveCount(rd, lod);
						int  primStart = rd.lod_[lod].index_->start();

						if (instanced)
						{
							//-- 4. fill instancing buffer with new values use DRAW_NO_OVERWRITE flag for that.
							uint prevOffset = rt.uploadInstancingVB(
								*ds.m_instBuffer, instCount, &instances[0], ds.m_offsetInBytes
								);

							//-- set up source stream frequency and install instancing buffer with
							//-- desired offset.
							Moo::rc().device()->SetStreamSourceFreq(
								0, (D3DSTREAMSOURCE_INDEXEDDATA | instCount)
								);
							ds.m_instBuffer->set(
								Moo::InstancingStream::STREAM_NUMBER, prevOffset, sizeof(TSpeedTreeType::DrawData::PackedGPU)
								);

							//-- 5. draw instances with one draw call.
							Moo::rc().drawIndexedInstancedPrimitive(
								DrawingTraits::getPrimitiveType(), vertBase, 0, vertCount, primStart, primCount, instCount
								);
						}
						else
						{
							//-- iterate over the whole set of instances and draw them independently.
							for (uint j = 0; j < instCount; ++j)
							{
								rt.uploadInstanceConstants(*ds.m_material, *ds.m_constantsMap, instances[j]);

								ds.m_material->commitChanges();

								Moo::rc().drawIndexedPrimitive(
									DrawingTraits::getPrimitiveType(), vertBase, 0, vertCount, primStart, primCount
									);
							}
						}

						//-- prepare instances container for the next frame.
						if (clear)
						{
							instances.clear();
						}
					}
				}

				if (instanced)
				{
					Moo::rc().device()->SetStreamSourceFreq(0, 1);
					Moo::rc().device()->SetStreamSourceFreq(Moo::InstancingStream::STREAM_NUMBER, 1);
					Moo::VertexBuffer::reset(Moo::InstancingStream::STREAM_NUMBER);
				}

				ds.m_material->endPass();
			}
			ds.m_material->end();
		}
	}

	//----------------------------------------------------------------------------------------------
	struct BranchCustomDrawingTraits : public BranchDrawingCommonTraits
	{
		static bool		needToDraw	(const LodData& lodData)			{ return TSpeedTreeType::s_drawBranches_ && lodData.branchDraw_; }		
		static uint		getLodCount	(const LodData& lodData)			{ return 1; }
		static uint		getLod		(const LodData& lodData, uint idx)	{ return lodData.branchLod_; }
		static float	getAlphaRef	(const LodData& lodData, uint idx)	{ return lodData.branchAlpha_ / 255.0f; }
	};

	//----------------------------------------------------------------------------------------------
	struct FrondCustomDrawingTraits : public FrondDrawingCommonTraits
	{
		static bool		needToDraw	(const LodData& lodData)			{ return TSpeedTreeType::s_drawFronds_ && lodData.frondDraw_; }		
		static uint		getLodCount	(const LodData& lodData)			{ return 1; }
		static uint		getLod		(const LodData& lodData, uint idx)	{ return lodData.frondLod_; }
		static float	getAlphaRef	(const LodData& lodData, uint idx)	{ return lodData.frondAlpha_ / 255.0f; }
	};

	//----------------------------------------------------------------------------------------------
	struct LeafCustomDrawingTraits : public LeafDrawingCommonTraits
	{
		static bool		needToDraw	(const LodData& lodData)			{ return TSpeedTreeType::s_drawLeaves_ && lodData.leafDraw_ && lodData.leafLodCount_ > 0; }
		static uint		getLodCount	(const LodData& lodData)			{ return lodData.leafLodCount_; }
		static uint		getLod		(const LodData& lodData, uint idx)	{ return lodData.leafLods_[idx]; }
		static float	getAlphaRef	(const LodData& lodData, uint idx)	{ return lodData.leafAlphaValues_[idx] / 255.0f; }
	};

	//----------------------------------------------------------------------------------------------
	struct BillboardCustomDrawingTraits : public BillboardDrawingCommonTraits
	{
		static bool		needToDraw	(const LodData& lodData)			{ return TSpeedTreeType::s_drawBillboards_ && lodData.billboardDraw_; }		
		static uint		getLodCount	(const LodData& lodData)			{ return 1; }
		static uint		getLod		(const LodData& lodData, uint idx)	{ return 0; }
		static float	getAlphaRef	(const LodData& lodData, uint idx)
		{
			const float& minAlpha  = BillboardVertex::s_minAlpha_;
			const float& maxAlpha  = BillboardVertex::s_maxAlpha_;
			const float& fadeValue = lodData.billboardFadeValue_;

			return 1.0f + fadeValue * (minAlpha / maxAlpha - 1.0f);
		}
	};

	//----------------------------------------------------------------------------------------------
	template<typename DrawingTraits>
	void drawTreePart(
		TSpeedTreeType& rt, Moo::EffectMaterial& effect, const ConstantsMap& constsMap,
		Moo::VertexDeclaration* vertexFormat, TSpeedTreeType::DrawData& drawData)
	{
		const TSpeedTreeType::LodData&		lod  = drawData.lod_;
		TSpeedTreeType::DrawData::Instance&	inst = drawData.m_instData;
		typename DrawingTraits::RenderData&	rd   = DrawingTraits::getRenderData(rt);

		if (DrawingTraits::needToDraw(lod) && !rd.lod_.empty())
		{
			//-- set properties actual for current tree type.
			Moo::rc().setVertexDeclaration(vertexFormat->declaration());
			bindModelData<typename DrawingTraits::TreePartTraits>();
			DrawingTraits::setRenderStates(rt, effect, constsMap);

			if (effect.begin())
			{
				for (uint i = 0; i < DrawingTraits::getLodCount(lod); ++i)
				{
					uint32 lodIdx = DrawingTraits::getLod(lod, i);
					//-- check valid status of desired lod.
					if (!rd.lod_[lodIdx].index_)
						continue;

					//-- prepare common constants.
					uint vertCount = rd.verts_->count();
					int  vertBase  = rd.verts_->start();
					int  primCount = DrawingTraits::getPrimitiveCount(rd, lodIdx);
					int  primStart = rd.lod_[lodIdx].index_->start();

					//-- prepare per-instance data.
					inst.m_alphaRef = DrawingTraits::getAlphaRef(lod, i);

					for (uint pass = 0; pass < effect.numPasses(); ++pass)
					{
						if (effect.beginPass(pass))
						{
							rt.uploadInstanceConstants(effect, constsMap, inst);

							effect.commitChanges();

							Moo::rc().drawIndexedPrimitive(
								DrawingTraits::getPrimitiveType(), vertBase, 0, vertCount, primStart, primCount
								);		

							effect.endPass();
						}
					}
				}
				effect.end();
			}
		}
	}

	//----------------------------------------------------------------------------------------------
	template<typename DrawingTraits>
	float calculateUVSpaceDensity( TSpeedTreeType& rt, 
		const Moo::VertexFormat& vertFormat )
	{
		const typename DrawingTraits::RenderData& rd = DrawingTraits::getRenderData(rt);

		// Check if the vert data is loaded
		if (!rd.verts_)
		{
			// Treat the usage of this object as inconsequential
			return FLT_MAX;
		}

		const typename DrawingTraits::TreePartTraits::VertexType * baseVertData =
			&DrawingTraits::TreePartTraits::s_vertexList_[0];
		const uint32 * baseIndexData = 
			&DrawingTraits::TreePartTraits::s_indexList_[0];

		float minUvDensity = FLT_MAX;
		for (uint lod = 0; lod < rd.lod_.size(); ++lod)
		{
			if (!rd.lod_[lod].index_)
			{
				continue;
			}

			//-- prepare common constants.
			uint vertCount = rd.verts_->count();
			int  vertBase  = rd.verts_->start();
			uint vertDataSize = vertCount * 
				sizeof(typename DrawingTraits::TreePartTraits::VertexType);
			int  primCount = DrawingTraits::getPrimitiveCount(rd, lod);
			int  primStart = rd.lod_[lod].index_->start();
			Moo::PrimitiveHelper::PrimitiveType primType = 
				Moo::PrimitiveHelper::primitiveTypeFromDeviceEnum( 
				DrawingTraits::getPrimitiveType() );

			// Work out the LOD buffers
			const typename DrawingTraits::TreePartTraits::VertexType * vertData =
				&baseVertData[vertBase];
			const uint32 * indexData = 
				&baseIndexData[primStart];

			float geoMinUVDensity =
				calculateGeometryUVSpaceDensity<DrawingTraits>(
					vertFormat,
					vertData,
					vertDataSize,
					indexData,
					primCount,
					primType);

			minUvDensity = min(minUvDensity, geoMinUVDensity);
		}

		if (minUvDensity == FLT_MAX)
		{
			minUvDensity = 0.0f;
		}

		return minUvDensity;
	}

	float calculateStandardGeometryUVSpaceDensity(
		const Moo::VertexFormat& vertFormat, 
		const void * vertData, 
		uint vertDataSize, 
		const uint32 * indexData, 
		int primCount, 
		Moo::PrimitiveHelper::PrimitiveType primType )
	{
		Moo::UvDensityTriGenFunctor densityFunct(
			vertFormat, vertData, vertDataSize, 0, 0 );
		if (!densityFunct.isValid())
		{
			WARNING_MSG("TSpeedTreeType::calculateUVSpaceDensity<DrawingTraits>:"
				" Invalid texel density functor\n");
			return FLT_MAX;
		}

		// Iterate over triangles, calculating texel density, updating minimum
		Moo::PrimitiveHelper::generateTrianglesFromIndices( indexData, 0, 
			primCount, primType, densityFunct );

		return densityFunct.getMinUvDensity();
	}

	template<typename DrawingTraits>
	float calculateGeometryUVSpaceDensity( 
		const Moo::VertexFormat& vertFormat, 
		const typename DrawingTraits::TreePartTraits::VertexType * vertData, 
		uint vertDataSize, 
		const uint32 * indexData, 
		int primCount, 
		Moo::PrimitiveHelper::PrimitiveType primType ) 
	{
		return calculateStandardGeometryUVSpaceDensity( vertFormat,
			vertData,
			vertDataSize,
			indexData,
			primCount,
			primType);
	}

	// Leaf specific UV space density calculation. Because
	// leaves are generated in the shader according to screen space.
	// Need to calculate the positions of all the vert data appropriately
	template<>
	float calculateGeometryUVSpaceDensity<LeafDrawingTraits>( 
		const Moo::VertexFormat& vertFormat, 
		const typename LeafDrawingTraits::TreePartTraits::VertexType * vertData, 
		uint vertDataSize, 
		const uint32 * indexData, 
		int primCount, 
		Moo::PrimitiveHelper::PrimitiveType primType ) 
	{
		// Generate proper positions for all the verts
		uint32 vertStride = vertFormat.streamStride( 0 );
		uint32 numVerts = vertDataSize / vertStride;

		LeafDrawingTraits::TreePartTraits::VertexType* pModifiedVerts =
			(LeafDrawingTraits::TreePartTraits::VertexType*)bw_malloc( vertDataSize );
		memcpy( pModifiedVerts, vertData, vertDataSize );

		const Vector3 leafUnitSquare[5] =
		{
			Vector3(+0.5f, +0.5f, 0.0f), 
			Vector3(-0.5f, +0.5f, 0.0f), 
			Vector3(-0.5f, -0.5f, 0.0f), 
			Vector3(+0.5f, -0.5f, 0.0f),
			Vector3( 0.0f,  0.0f, 0.0f)
		};

		// Iterate over all verts and modify the position as the shader does
		for (uint32 index = 0; index < numVerts; ++index)
		{
			// Base it entirely on "corner" because that is what gives
			// the triangle its size. The texture streaming does not
			// need anything else but world size and UV space size to calc.

			/*
			float3 corner    = g_leafUnitSquare[i.extraInfo.y].xyz;
			corner.xy	    += i.pivotInfo.xy;
			corner.xyz	    *= inst.m_uniformScale * saturate(g_cameraPos.w * 7.0f);
			//-- adjust by pivot point so rotation occurs around the correct point
			corner.xyz	    *= i.rotGeomInfo.zwz;
			*/

			uint32 offsetIndex = uint32(vertData[index].extraInfo_.y);
			if (offsetIndex >= 4)
			{
				// Invalid offset, OR these triangles aren't point verts
				// No modification necessary
				continue;
			}

			Vector3 cornerPos = leafUnitSquare[offsetIndex];
			cornerPos += Vector3(vertData[index].pivotInfo_.x, vertData[index].pivotInfo_.y, 0.0f);
			
			pModifiedVerts[index].pos_ = cornerPos;
		}

		float result = calculateStandardGeometryUVSpaceDensity( vertFormat,
			pModifiedVerts,
			vertDataSize,
			indexData,
			primCount,
			primType);

		bw_free( pModifiedVerts );

		return result;
	}

	// Leaf specific UV space density calculation. Because
	// leaves are generated in the shader according to screen space.
	// Need to calculate the positions of all the vert data appropriately
	template<>
	float calculateGeometryUVSpaceDensity<BillboardDrawingTraits>( 
		const Moo::VertexFormat& vertFormat, 
		const typename BillboardDrawingTraits::TreePartTraits::VertexType * vertData, 
		uint vertDataSize, 
		const uint32 * indexData, 
		int primCount, 
		Moo::PrimitiveHelper::PrimitiveType primType ) 
	{
		// Copy the UV's from TexCoord1 into TexCoord0
		uint32 vertStride = vertFormat.streamStride( 0 );
		uint32 numVerts = vertDataSize / vertStride;

		BillboardDrawingTraits::TreePartTraits::VertexType* pModifiedVerts =
			(BillboardDrawingTraits::TreePartTraits::VertexType*)bw_malloc( vertDataSize );
		memcpy( pModifiedVerts, vertData, vertDataSize );

		// Iterate over all verts and modify the position as the shader does
		for (uint32 index = 0; index < numVerts; ++index)
		{
			// The UVDensity calculation is currently looking at TexCoord0
			// for UV's, so copy them into that position for now
			pModifiedVerts[index].alphaNormal_ = Vector3( 
				pModifiedVerts[index].tc_.x,
				pModifiedVerts[index].tc_.y, 
				0.0f);
		}

		float result = calculateStandardGeometryUVSpaceDensity( vertFormat,
			pModifiedVerts,
			vertDataSize,
			indexData,
			primCount,
			primType);

		bw_free( pModifiedVerts );

		return result;
	}
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.


namespace speedtree
{

	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::drawBranches(DrawingState& ds, bool clear)
	{
		BW_GUARD;
		if (ds.m_instanced)	drawTreesPart<BranchDrawingTraits, true>(ds, clear);
		else				drawTreesPart<BranchDrawingTraits, false>(ds, clear);
	}

	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::drawFronds(DrawingState& ds, bool clear)
	{
		BW_GUARD;
		if (ds.m_instanced)	drawTreesPart<FrondDrawingTraits, true>(ds, clear);
		else				drawTreesPart<FrondDrawingTraits, false>(ds, clear);
	}

	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::drawLeaves(DrawingState& ds, bool clear)
	{
		BW_GUARD;
		if (ds.m_instanced)	drawTreesPart<LeafDrawingTraits, true>(ds, clear);
		else				drawTreesPart<LeafDrawingTraits, false>(ds, clear);
	}

	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::drawBillboards(DrawingState& ds, bool clear)
	{
		BW_GUARD;
		if (ds.m_instanced)	drawTreesPart<BillboardDrawingTraits, true>(ds, clear);
		else				drawTreesPart<BillboardDrawingTraits, false>(ds, clear);
	}

	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::customDrawBranch(DrawingState& ds, DrawData& drawData)
	{
		BW_GUARD;
		drawData.m_instData.m_blendFactor = drawData.branchAlpha_;
		drawData.m_instData.m_materialID = static_cast<float>(materialHandle_ * 4 + 0);
		drawTreePart<BranchCustomDrawingTraits>(*this, *ds.m_material, *ds.m_constantsMap, ds.m_vertexFormat, drawData);
	}

	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::customDrawFrond(DrawingState& ds, DrawData& drawData)
	{
		BW_GUARD;
		drawData.m_instData.m_blendFactor = drawData.branchAlpha_;
		drawData.m_instData.m_materialID = static_cast<float>(materialHandle_ * 4 + 1);
		drawTreePart<FrondCustomDrawingTraits>(*this, *ds.m_material, *ds.m_constantsMap, ds.m_vertexFormat, drawData);
	}

	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::customDrawLeaf(DrawingState& ds, DrawData& drawData)
	{
		BW_GUARD;
		drawData.m_instData.m_blendFactor = drawData.leavesAlpha_;
		drawData.m_instData.m_materialID = static_cast<float>(materialHandle_ * 4 + 2);
		drawTreePart<LeafCustomDrawingTraits>(*this, *ds.m_material, *ds.m_constantsMap, ds.m_vertexFormat, drawData);
	}

	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::customDrawBillBoard(DrawingState& ds, DrawData& drawData)
	{
		BW_GUARD;
		drawData.m_instData.m_blendFactor = 1.0f;
		drawData.m_instData.m_materialID = static_cast<float>(materialHandle_ * 4 + 3);
		drawTreePart<BillboardCustomDrawingTraits>(*this, *ds.m_material, *ds.m_constantsMap, ds.m_vertexFormat, drawData);
	}

	//----------------------------------------------------------------------------------------------
	float TSpeedTreeType::calculateUVSpaceDensityLeaves( TSpeedTreeType& rt, const Moo::VertexFormat& format )
	{
		return calculateUVSpaceDensity<LeafDrawingTraits>( rt, format );
	}

	//----------------------------------------------------------------------------------------------
	float TSpeedTreeType::calculateUVSpaceDensityFronds( TSpeedTreeType& rt, const Moo::VertexFormat& format )
	{
		return calculateUVSpaceDensity<FrondDrawingTraits>( rt, format );
	}

	//----------------------------------------------------------------------------------------------
	float TSpeedTreeType::calculateUVSpaceDensityBranches( TSpeedTreeType& rt, const Moo::VertexFormat& format )
	{
		return calculateUVSpaceDensity<BranchDrawingTraits>( rt, format );
	}

	//----------------------------------------------------------------------------------------------
	float TSpeedTreeType::calculateUVSpaceDensityBillboards( TSpeedTreeType& rt, const Moo::VertexFormat& format )
	{
		return calculateUVSpaceDensity<BillboardDrawingTraits>( rt, format );
	}

	//----------------------------------------------------------------------------------------------
} //-- speedtree

BW_END_NAMESPACE
