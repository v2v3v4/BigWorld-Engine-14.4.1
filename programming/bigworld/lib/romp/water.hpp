#ifndef _WATER_HPP
#define _WATER_HPP

#include <iostream>
#include "cstdmf/bw_vector.hpp"

//#define DEBUG_WATER

#include "cstdmf/smartpointer.hpp"
#include "moo/device_callback.hpp"
#include "moo/moo_math.hpp"
#include "moo/moo_dx.hpp"
#include "moo/com_object_wrap.hpp"
#include "moo/effect_material.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/render_target.hpp"
#include "moo/index_buffer.hpp"
#include "chunk/chunk_item.hpp"

#include "water_scene_renderer.hpp"

#include "math/boundbox.hpp"
#include "pyscript/script_math.hpp"
#include "resmgr/datasection.hpp"
#include "romp_collider.hpp"
#include "water_simulation.hpp"

#include "moo/vertex_buffer_wrapper.hpp"


BW_BEGIN_NAMESPACE

using Moo::VertexBufferWrapper;

#define VERTEX_TYPE Moo::VertexXYZNDUV

//NOTE: if this value changes, it should also be changed
//		in the shaders (file:water.fx)
#define WATER_REFLECTION_FADE_ENABLED 1

//NOTE: if this value changes, it should also be changed
// in the shaders (file:water_common.fxh)
#define SIM_BORDER_SIZE 3


namespace Moo
{
	class Material;
	class BaseTexture;
	class GraphicsSetting;

	typedef SmartPointer< BaseTexture > BaseTexturePtr;
	class DrawContext;
}

typedef SmartPointer<Moo::GraphicsSetting> GraphicsSettingPtr;

class Water;
typedef SmartPointer< Water > WaterPtr;
class Waters;
class BackgroundTask;
class EffectParameterCache;

class WaterEdgeWaves;

/**
 *	This class is represents a division of a water surface
 */
class WaterCell : public SimulationCell
{
public:
	typedef BW::list< SimulationCell* > SimulationCellPtrList;
	typedef BW::list< WaterCell* > WaterCellPtrList;
	typedef BW::vector< WaterCell > WaterCellVector;

	// Construction
	WaterCell();

	// Initialisation / deletion
	bool init( Water* water, Vector2 start, Vector2 size );
	void initEdge( int index, WaterCell* cell );
	void createManagedIndices();
	void deleteManaged();

	// Drawing
	bool bind();
	void draw( int nVerts );

	// Simulation activity
	int indexCount() const;
	virtual void deactivate();
	bool simulationActive() const;
	void initSimulation( int size, float cellSize );
	virtual void bindNeighbour( Moo::EffectMaterialPtr effect, int edge );
	void checkEdgeActivity( SimulationCellPtrList& activeList );
	Moo::RenderTargetPtr simTexture();// Retrieve the simulation texture
	void checkActivity( SimulationCellPtrList& activeList, WaterCellPtrList& edgeList );
	uint32 bufferIndex() const { return vBufferIndex_; }

	WaterCell* edgeCell( uint i ) const { MF_ASSERT_DEBUG(i<4); return edgeCells_[i]; }

	// General
	const Vector2& min();
	const Vector2& max();
	const Vector2& size();

	float cellDistance() const { return cellDistance_; }
	void updateDistance(const Vector3& camPos);

private:
	template< class T >
	void buildIndices( );

	void activateEdgeCell( uint index );

	Vector2								min_, max_, size_;//TODO: replace with bounding box?
	WaterCell*							edgeCells_[4];
	Moo::IndexBuffer					indexBuffer_;
	uint32								nIndices_;
	uint32								vBufferIndex_;
	Water*								water_;
	int									xMax_, zMax_;
	int									xMin_, zMin_;

	float								cellDistance_; // distance to eye
};

/**
 *	This class is a body of water which moves and displays naturally.
 */
class Water : public ReferenceCount, public Moo::DeviceCallback
{
public:
	enum
	{
		ALWAYS_VISIBLE,
		INSIDE_ONLY,
		OUTSIDE_ONLY
	};

	typedef SmartPointer< Moo::VertexBufferWrapper< VERTEX_TYPE > > VertexBufferPtr;	

	/**
	 * TODO: to be documented.
	 */
	typedef struct _WaterState
	{
		bool load( DataSectionPtr pSection, 
				  const Vector3 & defaultPosition, 
				  const Matrix & mappingTransfom );

		Vector3		position_;
		Vector2		size_;
		float		orientation_;		
		float		tessellation_;
		float		textureTessellation_; // wave frequency???
		float		consistency_;
		float		fresnelConstant_;
		float		fresnelExponent_;
		float		reflectionScale_; //combine into one waveAmplitude?
		float		refractionScale_;
		float		refractionPower_; // power of water contrast
		float		animationSpeed_; // speed of water normals animation
		Vector2		scrollSpeed1_;
		Vector2		scrollSpeed2_;
		Vector2		waveScale_;
		float		sunPower_;
		float		sunScale_;
		float		windVelocity_;
		float		depth_;
		Vector4		reflectionTint_;
		Vector4		refractionTint_;
		float		simCellSize_;
		float		smoothness_;
		bool		useEdgeAlpha_;
		bool		useSimulation_;
		uint32		visibility_;  // used by the editor to create references
		bool		reflectBottom_;

		Vector4		deepColour_;// = float3(0,0.21,0.35);
		float		fadeDepth_;
		float		foamIntersection_;
		float		foamMultiplier_;
		float		foamTiling_;
		float		foamFrequency_; // frequency of foam sliding
		float		foamAmplitude_; // amplitude of foam sliding
		float		foamWidth_;		// width of foam line
		bool		bypassDepth_;

		float		freqX; // frequency of waves. Works only for highest quality
		float		freqZ;
		float		waveHeight; //height of waves. Works only for highest quality
		float		causticsPower; //regulates the briteness of caustics. Standard value is 8.0f

		uint32		waveTextureNumber_;

		bool		useShadows_;
	} WaterState;

	typedef struct _WaterResources
	{
		bool load( DataSectionPtr pSection, 
				  const BW::string & transparenctTable );

		BW::string	foamTexture_;
		BW::string	waveTexture_;
		BW::string	reflectionTexture_;
		BW::string transparencyTable_;
	} WaterResources;
	

	// Construction / Destruction
	Water( const WaterState& state, const WaterResources& resources, RompColliderPtr pCollider = NULL );
	//~Water();

	// Initialisation / deletion
	void rebuild( const WaterState& state, const WaterResources& resources );
	void deleteManagedObjects( );
	void createManagedObjects( );
	void deleteUnmanagedObjects( );
	void createUnmanagedObjects( );	
	bool recreateForD3DExDevice() const { return true; }
	uint32	 createIndices( );
	int	 createWaveIndices( );	
	void createCells( );
	void tick();

	static void deleteWater(Water* water);
	static bool stillValid(Water* water);

	uint32 remapVertex( uint32 index );
	template <class T>
	uint32 remap(BW::vector<T>& dstIndices,
					const BW::vector<uint32>& srcIndices);
	// a list of vertex buffers with their related mappings.
	BW::vector< BW::map<uint32, uint32> > remappedVerts_;


#ifdef EDITOR_ENABLED
	void deleteData( );
	void saveTransparencyTable( );

	void drawRed(bool value) { drawRed_ = value; }
	bool drawRed() const { return drawRed_; }

	void highlight(bool value) { highlight_ = value; }
	bool highlight() const { return highlight_; }

	int numCells() const { return (int)cells_.size(); }
#endif //EDITOR_ENABLED

	bool addMovement( const Vector3& from, const Vector3& to, const float diameter );

	// Drawing
	uint32 drawMark() const		{ return drawMark_; }
	void drawMark( uint32 m )	{ drawMark_ = m; }

	Vector4 getFogColour() const { return state_.refractionTint_; }
	float getDepth() const { return state_.depth_; }

	void clearRT();
	void drawMask();
	void updateSimulation( float dTime );
	void draw( Waters & group, float dTime );

	bool shouldDraw() const;
	bool needSceneUpdate() const;
	void needSceneUpdate( bool value );
	bool canReflect(float* retDist = 0) const;

	void updateVisibility();
	bool visible() const { return visible_; }	

	// General
	const Vector3&	position() const;
	const Vector2&	size() const;
	float orientation() const;

	const Vector3& cameraPos() const { return camPos_; }

	const BoundingBox& bb() const { return  bbActual_; }

	bool checkVolume();
	bool isInside( MatrixProviderPtr pMat );
	bool isInsideBoundary( Matrix m );
	PyObjectPtr pPyVolume()	{ return pPyVolume_; }

	void addVisibleChunk( Chunk* pChunk ) { visibleChunks_.push_back( pChunk ); }
	void clearVisibleChunks() { visibleChunks_.clear(); outsideVisible_ = false; }
	const BW::vector<Chunk*>& visibleChunks() const { return visibleChunks_; }
	bool outsideVisible() const { return outsideVisible_; }
	void outsideVisible( bool state ) { outsideVisible_ = state; }

	const Matrix& transform() const { return transform_; }

	static VertexBufferPtr	s_quadVertexBuffer_;
	static float			s_sceneCullDistance_;	
	static float			s_sceneCullFadeDistance_;

	static float			s_fTexScale;
	static float			s_fFreqX;
	static float			s_fFreqZ;
	static float			s_fBankLineInterpolationFactor;

    static bool backgroundLoad();
    static void backgroundLoad(bool background);

	static Moo::RenderTargetPtr	s_rt;

private:
	typedef BW::vector< uint32 > WaterAlphas;
	typedef BW::vector< std::pair<Vector2, float> > BankDist; // distance from vertex to the nearest bank
	typedef BW::vector< bool > WaterRigid;
	typedef BW::vector< int32 > WaterIndices;
	typedef BW::map<float, WaterScene*> WaterRenderTargetMap;

	~Water();

	void visible(bool val) { visible_=val; }

	WaterState							state_;
	WaterResources						resources_;

	Matrix								transform_;
	Matrix								invTransform_;

	Vector3								camPos_;

	uint32								gridSizeX_;
	uint32								gridSizeZ_;

	float								time_;
	float								lastDTime_;

	WaterRigid							wRigid_;
	WaterAlphas							wAlpha_;
	BankDist							bankDist; // distance from vertex to the nearest bank
	WaterIndices						wIndex_;

	BW::vector<VertexBufferPtr>		vertexBuffers_;
	uint32								nVertices_;

	Moo::EffectMaterialPtr				simulationEffect_;

	static WaterRenderTargetMap			s_renderTargetMap_;

	WaterCell::WaterCellVector			cells_;
	WaterCell::SimulationCellPtrList	activeSimulations_;
	WaterCell::WaterCellPtrList			edgeList_;

	WaterScene*							waterScene_;

#ifdef EDITOR_ENABLED
	bool								drawRed_;
	bool								highlight_;
#endif

	bool								visible_;
	bool								needSceneUpdate_;
	bool								raining_;
	bool								drawSelf_;
	bool								initialised_;
	bool								enableSim_;
	bool								reflectionCleared_;
	bool								createdCells_;
	float								simpleReflection_; // 0: normal, 1: simple, lerpable

	EffectParameterCache				*paramCache_;

#ifdef DEBUG_WATER
	static int							s_debugCell_;
	static int							s_debugCell2_;
	static bool							s_debugSim_;
#endif //DEBUG_WATER
	static bool                         s_backgroundLoad_;
	static bool                         s_cullCells_;
	static float						s_cullDistance_;
	static float						s_maxSimDistance_;

	BoundingBox							bb_;
	BoundingBox							bbDeep_;
	BoundingBox							bbActual_;

	uint32								drawMark_;

	Moo::BaseTexturePtr*				normalTextures_;
	Moo::BaseTexturePtr					screenFadeTexture_;
	Moo::BaseTexturePtr					foamTexture_;
	Moo::BaseTexturePtr					reflectionCubeMap_;
	RompColliderPtr						pCollider_;	
	PyObjectPtr							pPyVolume_;

	float								animTime_;
	BW::vector<Chunk*>					visibleChunks_;
	bool								outsideVisible_;

	Water(const Water&);
	Water& operator=(const Water&);

	friend class Waters;
	friend class WaterCell;

	void buildBankDistTable();
	void buildTransparencyTable( );
	bool loadTransparencyTable( );
	void setup2ndPhase();
	void startResLoader();
	void renderSimulation(float dTime);
	void resetSimulation();
	void releaseWaterScene();

	void setupRenderState( float dTime );
	//void resetRenderState(  );
	bool init();

	static void doCreateTables( void * self );
	static void onCreateTablesComplete( void * self );
	static void doLoadResources( void * self );
	static void onLoadResourcesComplete( void * self );	
};


/**
 *	This class helpfully collects a number of bodies of water that
 *	share common settings and methods.
 */
class Waters : public BW::vector< Water* >
{
	typedef Waters This;
public:
	~Waters();
	static Waters& instance();

	void init( );
	void fini( );

	void drawDrawList( float dTime );
	void checkVolumes();

	void updateSimulations( float dTime );

	void tick( float dTime );

	void drawMasks();

	uint drawCount() const;

	void selectTechnique();
	
	void playerPos( Vector3 pos );

	float rainAmount() const;
	void rainAmount( float rainAmount );

	static uint32 addWaterVolumeListener( MatrixProviderPtr dynamicSource,
		PyObjectPtr callback );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETDATA, addWaterVolumeListener,
		ARG( MatrixProviderPtr, ARG( PyObjectPtr, END ) ) )

	static void delWaterVolumeListener( uint32 id );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, delWaterVolumeListener,
		ARG( uint32, END ) )

	void checkAllListeners();
	bool insideVolume() const;
	void drawWireframe( bool wire );

	float movementImpact() const;
	void addMovement( const Vector3& from, const Vector3& to, const float diameter );

	float time() const { return time_; }

	static void drawWaters( bool draw );
	static void addToDrawList( Water * pWater );	

	static bool simulationEnabled();
	static void simulationEnabled( bool val);

	static void loadResources( void * self );
	static void loadResourcesComplete( void * self );

	static bool drawReflection();
	static void drawReflection( bool val );

	static uint32		nextDrawMark() { return s_nextMark_; }

	static uint32		lastDrawMark() { return s_lastDrawMark_; }

	static bool	highQualityMode() { return s_highQuality_; }

#if EDITOR_ENABLED
	static void projectView( bool val ) { s_projectView_ = val; }
	static bool projectView( ) { return s_projectView_; }
	static bool			s_projectView_;
#endif
#if ENABLE_WATCHERS
	bool isVisible;
#endif

private:
	Waters();

	void insideVolume( bool val );

#ifdef SPLASH_ENABLED
	SplashManager	splashManager_;
#endif

	bool			insideWaterVolume_;		//eye inside a water volume?
	bool			drawWireframe_;			//force all water to draw in wireframe.
	float			movementImpact_;		//simulation impact scaling	
	float			rainAmount_;			//the amount of rain affecting the water.
	float			impactTimer_;			//used to generate the stationary pulses
	float			time_;					//global time used to sync all waters
	Vector3			playerPos_;

	//-- Represents set of internal water rendering states.
	struct QualityOption
	{
		//-- set of individual features or exclusive flags for drawing water.
		enum EFlag
		{
			DRAW_TREES			=	1 << 0,
			DRAW_DYNAMICS		=	1 << 1,
			DRAW_PLAYER			=	1 << 2,
			USE_SIMPLE_SCENE	=	1 << 3,
			USE_CUBE_MAP		=	1 << 4,
			USE_SIMULATION		=	1 << 5,
			USE_HIGH_QUALITY	=	1 << 6
		};

		QualityOption(float textureSize, uint32 flagsSet)
			:	m_textureSize(textureSize), m_flagsSet(flagsSet) { }

		float  m_textureSize;
		uint32 m_flagsSet;

		bool isSet( EFlag option ) const;
	};
	typedef BW::vector<QualityOption> QualityOptions;
	QualityOptions		m_qualityOptions;
	GraphicsSettingPtr	qualitySettings_;

	void setQualityOption(int optionIndex);
	
	int				shaderCap_;

	BackgroundTask*	loadingTask1_; 

	class VolumeListener
	{
	public:
		VolumeListener( MatrixProviderPtr source, PyObjectPtr callback ):
			source_( source ),
			callback_( callback ),
			water_( NULL )			
		{
			id_ = s_id++;
		};

		uint32 id() const { return id_; }
		MatrixProviderPtr& source() { return source_; }
		PyObjectPtr callback() { return callback_; }
		bool inside() const { return (water_ != NULL); }
		void water( Water* w ) { water_ = w; }
		Water* water() const { return water_; }
	private:
		MatrixProviderPtr source_;
		PyObjectPtr callback_;
		Water* water_;
		uint32 id_;
		static uint32 s_id;		
	};
	typedef BW::vector<VolumeListener>	VolumeListeners;
	VolumeListeners	listeners_;

	static bool		s_highQuality_;
	static bool		s_forceUseCubeMap_; 	// force use the cube map on the medium and low settings quality.
	static bool		s_drawWaters_;			//global water draw flag.	
	static bool		s_simulationEnabled_;	//global sim enable flag.
	static bool		s_drawReflection_;
	static float	s_autoImpactInterval_;
	static uint32	s_nextMark_;
	static uint32	s_lastDrawMark_;

	static Moo::EffectMaterialPtr		s_effect_;
	static Moo::EffectMaterialPtr		s_simulationEffect_;

	friend class Water;
	friend class SimulationCell;
};

#ifdef CODE_INLINE
#include "water.ipp"
#endif

BW_END_NAMESPACE

#endif //_WATER_HPP
/*water.hpp*/
