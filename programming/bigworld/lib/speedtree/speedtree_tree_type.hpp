#ifndef SPEEDTREE_TREE_TYPE_HPP
#define SPEEDTREE_TREE_TYPE_HPP

#include "cstdmf/dogwatch.hpp"
#include "moo/effect_material.hpp"
#include "moo/complex_effect_material.hpp"
#include "math/boundbox.hpp"
#include "speedtree_vertex_types.hpp"
#include <set>

#if SPEEDTREE_SUPPORT
//-- forward declaration.
class EnviroMinder;
class CSpeedTreeRT;
class CSpeedWind;
class BSPTree;

BW_BEGIN_NAMESPACE

namespace speedtree
{
	//-- forward declaration.
	struct DrawingState;
	class  ConstantsMap;

	typedef TreePartRenderData< BranchVertex >    BranchRenderData;
	typedef TreePartRenderData< LeafVertex >      LeafRenderData;
	typedef TreePartRenderData< BillboardVertex > BillboardRenderData;

	//-- Holds the actual data needed to render a specific type of tree. 
	//-- Although each instance of a speedtree in a scene will be stored  in-memory as an instance
	//-- of the SpeedTreeRenderer class, all copies of a unique speedtree (loaded from a unique
	//-- ".spt" file or generated form a unique seed number) will be represented by a single
	//-- TSpeedTreeType object.
	//----------------------------------------------------------------------------------------------
	class TSpeedTreeType
	{
	public:
		typedef SmartPointer< TSpeedTreeType > TreeTypePtr;
		typedef std::auto_ptr< CSpeedTreeRT > CSpeedTreeRTPtr;

		typedef BW::vector< TSpeedTreeType* > RendererVector;
		typedef BW::vector< RendererVector > RendererGroupVector;


		//-- Holds all data that can be cached to disk to save time when generating the geometry of a
		//-- tree (vertex and index buffers plus bounding-boxses and leaf animation coefficients. Also
		//-- implements the load() and save() functions.
		//------------------------------------------------------------------------------------------
		class TreeData
		{
		public:
			TreeData();
	
			bool save( const BW::string & filename );
			bool load( const BW::string & filename );
			void unload( );
	
			BoundingBox         boundingBox_;

			BranchRenderData    branches_;
			BranchRenderData    fronds_;
			LeafRenderData      leaves_;
			BillboardRenderData billboards_;

			float               leafRockScalar_;
			float               leafRustleScalar_;

			bool				isBush;
		};

		//-- Holds LODing values and informations for all render parts of a tree.
		//------------------------------------------------------------------------------------------
		struct LodData
		{
			bool    model3dDraw_;

			int     branchLod_;
			float   branchAlpha_;
			bool    branchDraw_;

			int     frondLod_;
			float   frondAlpha_;
			bool    frondDraw_;

			int     leafLods_[2];
			float   leafAlphaValues_[2];
			int     leafLodCount_;
			bool    leafDraw_;

			float   billboardFadeValue_;
			bool    billboardDraw_;
		};

		//--  Stores all the information required to draw a unique instance of a tree.
		//------------------------------------------------------------------------------------------
		class DrawData
		{
		public:

			static int s_count_;

			DrawData()
			{
				s_count_+= sizeof(this);

				lodLevel_ = -1.f;
				initialised_ = false;

				RESOURCE_COUNTER_ADD( ResourceCounters::DescriptionPool("Speedtree/DrawData", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
					sizeof(*this), 0 )
			}

			~DrawData()
			{
				s_count_-=sizeof(this);

				RESOURCE_COUNTER_SUB( ResourceCounters::DescriptionPool("Speedtree/DrawData", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
					sizeof(*this), 0 )
			}

			bool	initialised_;
			LodData lod_;
			float   lodLevel_;
			float   leavesAlpha_;
			float   branchAlpha_;

#pragma pack(push, 1)
			//-- Tightly packed data for instancing. Total memory consumption is 40 bytes.
			struct PackedGPU
			{
				Vector4 m_rotation;
				Vector3 m_position;
				int16	m_scale[3];
				int16	m_materialID;
				uint8	m_windOffset;
				uint8	m_blendFactor;
				uint8	m_alphaRef;
				uint8	m_padding;
			};
#pragma pack(pop)

#pragma pack(push, 4)
			//-- data of each individual instance. Total memory consumption is 64 bytes.
			struct Instance
			{
				Vector4 m_rotation;
				Vector3 m_position;
				float	m_uniformScale;
				Vector3 m_scale;
				float	m_materialID;
				float	m_windOffset;
				float	m_blendFactor;
				float	m_alphaRef;				
				float	m_padding;

				//-- Used fixed point arithmetic. Layout [(1-fracBits) bits - (fracBits) bits]
				template<typename T>
				inline T pack(float data, uint8 fracBits) const
				{
					return static_cast<T>(data * ((1 << fracBits) - 1) + 0.5f);
				}

				//--
				inline PackedGPU packForInstancing() const
				{
					PackedGPU packed;
					packed.m_rotation	  = m_rotation;
					packed.m_position	  = m_position;
					packed.m_scale[0]	  = pack<int16>(m_scale[0], 9);
					packed.m_scale[1]	  = pack<int16>(m_scale[1], 9);
					packed.m_scale[2]	  = pack<int16>(m_scale[2], 9);
					packed.m_materialID	  = max<int16>(static_cast<int16>(m_materialID), 0);
					packed.m_alphaRef	  = pack<uint8>(m_alphaRef, 8);
					packed.m_windOffset	  = min<uint8>(static_cast<uint8>(m_windOffset), 255);
					packed.m_blendFactor  = pack<uint8>(m_blendFactor, 8);
					packed.m_padding	  = 0;
					return packed;
				}
			};
#pragma pack(pop)

			Instance m_instData;
		};

		//-- Notice that here we use aligned versions of BW::vector container so our data has to be
		//-- 4 byte aligned.
		typedef BW::vector<DrawData*> DrawDataVector;
		typedef AVectorNoDestructor<DrawData::Instance> Instances;

		//-- ToDo: reconsider this assumption.
		static const uint G_MAX_LOD_COUNT = 6;

		//-- Used to store already sorted tree parts.
		//-- Note: each tree part sorted by LOD values.
		//------------------------------------------------------------------------------------------
		struct DrawDataCollector
		{
			Instances		m_branches  [G_MAX_LOD_COUNT];
			Instances		m_fronds    [G_MAX_LOD_COUNT];
			Instances		m_leaves    [G_MAX_LOD_COUNT];
			Instances		m_billboards;
			DrawDataVector	m_semitransparentTrees;

			void sort();

			//-- Note: billboards may be drawn with billboard_optimizer.*pp stuff.
		};

		//-- draw data collectors.
		DrawDataCollector	drawDataCollector_;

		TSpeedTreeType();
		~TSpeedTreeType();

		static void			fini();

		// tick
		static void			tick(float dTime);
		static void			update();

	private:
		static bool			recycleTreeTypeObjects();
		static bool			doSyncInit();
		static void			clearDXResources( bool forceDelete );

	public:

		//-- animation
		void				updateWind();

		//-- collect trees draw data.
		void				collectTreeDrawData(DrawData& drawData, bool collectBillboards, bool semiTransparent);

		void				computeDrawData(const Matrix& world, float windOffset, DrawData& oDrawData);
		void				uploadInstanceConstants(Moo::EffectMaterial& effect, const ConstantsMap& constsMap, const DrawData::Instance& inst);
		uint				uploadInstancingVB(Moo::VertexBuffer& vb, uint32 numInst, const DrawData::Instance* instances, uint& offsetInBytes);
		void				computeLodData(LodData& oLod);
		static bool			isTreeSemiTransparent(DrawData& data) { return (data.leavesAlpha_ != 1.0f) || (data.branchAlpha_ != 1.0f); }

		//-- actual drawing.
		static void			drawTrees(Moo::ERenderingPassType pass);
		static void			drawOptBillboards(Moo::ERenderingPassType pass);
		static void			drawSemitransparentTrees(bool skipDrawing = false);

		//-- drawing method for desired custom material. For example for semi-transparent trees.
		//-- Note: These method don't take advantage of some internal optimizations like instancing,
		//--	   material sorting, render state changes reduction mechanism cos' often these objects
		//--	   are really very few. So this mechanism of speedtree customization rendering works well
		//--	   only with a small amount of trees. If you need custom rendering of relatively big amount
		//--	   of trees take a look at ComplexEffectMaterial and reconsider possibility to add a new
		//--	   stage to the pipeline among the other like RENDERING_PASS_REFLECTION and so on.
		void				customDrawBranch(DrawingState& ds, DrawData& drawData);
		void				customDrawFrond(DrawingState& ds, DrawData& drawData);
		void				customDrawLeaf(DrawingState& ds, DrawData& drawData);
		void				customDrawBillBoard(DrawingState& ds, DrawData& drawData);

		//-- setup common shaders constant for each tree part.
		static void			prepareRenderContext();
		void				setBranchRenderStates(Moo::EffectMaterial& effect, const ConstantsMap& constsMap) const;
		void				setFrondRenderStates(Moo::EffectMaterial& effect, const ConstantsMap& constsMap) const;
		void				setLeafRenderStates(Moo::EffectMaterial& effect, const ConstantsMap& constsMap) const;
		void				setBillboardRenderStates(Moo::EffectMaterial& effect, const ConstantsMap& constsMap) const;

		// -- texture streaming data collection
		static float calculateUVSpaceDensityLeaves( TSpeedTreeType& rt, const Moo::VertexFormat& format );
		static float calculateUVSpaceDensityFronds( TSpeedTreeType& rt, const Moo::VertexFormat& format );
		static float calculateUVSpaceDensityBranches( TSpeedTreeType& rt, const Moo::VertexFormat& format );
		static float calculateUVSpaceDensityBillboards( TSpeedTreeType& rt, const Moo::VertexFormat& format );
		void generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup );

		//--
		static void			drawBranches(DrawingState& ds, bool clear = true);
		static void			drawFronds(DrawingState& ds, bool clear = true);
		static void			drawLeaves(DrawingState& ds, bool clear = true);
		static void			drawBillboards(DrawingState& ds, bool clear = true);

		//-- saving some global info about the environment.
		static void 		saveWindInformation( EnviroMinder * envMinder );
		static void 		saveLightInformation( EnviroMinder * envMinder );

		//-- setup
		static TreeTypePtr getTreeTypeObject( const char * filename, uint seed );
		static TreeData		generateTreeData(
			CSpeedTreeRT& speedTree, CSpeedWind& speedWind, const BW::string& filename,
			int seed, int windAnglesCount
			);

		void				asyncInit();
		void				syncInit();
		void				releaseResources();

		typedef BW::vector<TSpeedTreeType *> InitVector;
		static InitVector  s_syncInitList_;
		static SimpleMutex s_syncInitLock_;
		static SimpleMutex s_vertexListLock_;

		// Tree definition data
		typedef std::auto_ptr< BSPTree > BSPTreePtr;
		CSpeedTreeRTPtr speedTree_;

		//-- Represents unique wind parameters for each unique type of tree. This class also helps
		//-- to set these parameters on rendering device.
		//-- If a particular speedtree type doesn't want to have its own wind parameters it 
		//-- automatically uses default wind parameters.
		//-------------------------------------------------------------------------------------------
		class WindAnimation
		{
		public:
			WindAnimation();
			~WindAnimation();

			bool			init( const BW::string & iniFile );
			void			update();
			
			bool			hasLeaves() const		{ return m_hasLeaves; }
			void			hasLeaves(bool value)	{ m_hasLeaves = value; }

			//-- wind matrices.
			uint			matrixCount() const		{ return uint(m_windMatrices.size()); }	
			const Matrix*	matrices() const		{ return &m_windMatrices[0]; }

			//-- wind leaf angles.
			uint			leafAnglesCount() const	{ return m_leafAnglesCount; }
			const float*	leafAnglesTable() const { return &m_anglesTable[0]; }

			CSpeedWind*		speedWind()				{ return m_speedWind.get(); }

		private:
			bool					  m_hasLeaves;
			float					  m_lastTickTime;
			uint					  m_leafAnglesCount;
			BW::vector<float>		  m_anglesTable;
			BW::vector<Matrix>		  m_windMatrices;
			std::auto_ptr<CSpeedWind> m_speedWind;
		};

		//-- default wind animation as a fall-back and instance of wind animation as an per-type wind
		//-- parameters.
		WindAnimation*			pWind_;
		static WindAnimation	s_defaultWind_;

		static WindAnimation* setupSpeedWind(
			CSpeedTreeRT& speedTree, const BW::string& datapath, const BW::string & defaultWindIni
			);

		//-- Controls set of unique identifiers. 
		//------------------------------------------------------------------------------------------
		class Unique
		{
		public:
			typedef uint Handle;

		public:
			Unique() : m_count(0) { }

			Handle getNew()
			{
				uint newID = m_count++;

				if (!m_freeIndices.empty())
				{
					newID = m_freeIndices.back();
					m_freeIndices.pop_back();
				}

				return newID;
			}

			void getBack(Handle uid)
			{
				MF_ASSERT(m_count > 0);

				--m_count;
				m_freeIndices.push_back(uid);
			}

		private:
			uint				m_count;
			BW::vector<Handle>	m_freeIndices;
		};
		
		//-- index into materials set.
		Unique::Handle	  materialHandle_;
		static Unique	  s_materialHandles_;

		// Tree data
		uint              seed_;
		BW::string	      filename_;
		TreeData          treeData_;
				
		// bsp
		BSPTreePtr        bspTree_;

		int               bbTreeType_;

		//-- reference counting
		void incRef() const;
		void decRef() const;
		int  refCount() const;

		mutable int refCounter_;

		// Static tree definitions map
		typedef BW::StringRefMap< TSpeedTreeType * > TreeTypesMap;
		static TreeTypesMap		s_typesMap_;

		static bool				s_frameStarted_;
		static float			s_time_;

		// Auxiliary static data
		static bool			s_useMapWind;
		static float		s_windVelX_;
		static float		s_windVelZ_;

		static void loadBushNames();
		static BW::vector< BW::string > vBushNames_;
		static bool isBushName( const BW::string &strName );

		static Matrix		s_invView_;
		static Matrix		s_view_;
		static Matrix		s_lodCamera_; //-- this is invView matrix for the camera at wich lod calculates.
		static Matrix		s_projection_;
		static Matrix		s_viewProj_;
		static Moo::Colour	s_sunDiffuse_;
		static Moo::Colour	s_sunAmbient_;
		static Vector3		s_sunDirection_;

		//-- matrices for semitransparent rendering.
		static Matrix		s_alphaPassViewMat;
		static Matrix		s_alphaPassProjMat;

		// Watched parameters
		static bool  s_enviroMinderLight_;
		static bool  s_drawBoundingBox_;
		static bool	 s_enableCulling_;
		static bool	 s_drawNormals_;
		static bool  s_enableInstaning_;
		static bool  s_enableZPrePass_;
		static bool  s_enableZDistSorting_;
		static bool  s_drawTrees_;
		static bool  s_drawLeaves_;
		static bool  s_drawBranches_;
		static bool  s_drawFronds_;
		static bool  s_drawBillboards_;
		static bool  s_texturedTrees_;
		static bool  s_playAnimation_;
		static float s_leafRockFar_;
		static float s_maxLod_;
		static float s_lodMode_;
		static float s_lodNear_;
		static float s_lodFar_;
		static float s_lod0Yardstick_;
		static float s_lod0Variance_;

#if ENABLE_BB_OPTIMISER
		static bool  s_optimiseBillboards_;
#endif

		static int s_speciesCount_;
		static int s_uniqueCount_;
		static int s_visibleCount_;
		static int s_deferredCount_;
		static int s_lastPassCount_;
		static int s_passNumCount_;
		static int s_totalCount_;

		static BW::string s_speedWindFile_;

		static DogWatch s_globalWatch_;
		static DogWatch s_prepareWatch_;
		static DogWatch s_drawWatch_;
		static DogWatch s_primitivesWatch_;
	};

} //-- namespace speedtree

BW_END_NAMESPACE

#endif SPEEDTREE_SUPPORT
#endif //SPEEDTREE_TREE_TYPE_HPP
