#pragma once

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/singleton.hpp"
#include "moo/forward_declarations.hpp"
#include "moo/complex_effect_material.hpp"
#include "moo/graphics_settings.hpp"
#include "moo/device_callback.hpp"
#include "moo/vertex_buffer.hpp"
#include "moo/vertex_declaration.hpp"

BW_BEGIN_NAMESPACE

namespace speedtree
{

	//-- Enumerates all existing tree part.
	//------------------------------------------------------------------------------------------
	enum ETreePart
	{
		TREE_PART_BRANCH = 0,
		TREE_PART_FROND,
		TREE_PART_LEAF,
		TREE_PART_BILLBOARD,
		TREE_PART_COUNT
	};

	//-- Enumerates all existing special drawing state.
	//------------------------------------------------------------------------------------------
	enum ESpecialDrawingState
	{
		SPECIAL_DRAWING_STATE_DEPTH = 0,
		SPECIAL_DRAWING_STATE_ALPHA,
		SPECIAL_DRAWING_STATE_COUNT
	};

	//-- Represents available speedtree's graphics settings options.
	//----------------------------------------------------------------------------------------------
	class SpeedTreeSettings
	{
	public:
		struct Option
		{
			Option(bool highQuality, float lodBias, float zoomBias)
				: m_highQuality(highQuality), m_lodBias(lodBias), m_zoomBias(zoomBias) { }

			bool  m_highQuality;
			float m_lodBias;
			float m_zoomBias;
		};

	public:
		SpeedTreeSettings(const DataSectionPtr& cfg);
		~SpeedTreeSettings();

		const Option&	activeOption() const;
		void			setQualityOption(int optionIndex);

	private:
		BW::vector<Option>						 m_options;
		Moo::GraphicsSetting::GraphicsSettingPtr m_qualitySettings;
	};

	//-- The main purpose of this class is to eliminate overhead of setting shader constants by name.
	//-- It makes this by extracting hash handle for each shader constant and then use these hash
	//-- values as a handle name when setting shader constants. That helps reduce searching complexity
	//-- to just "accessing array element by desired index". It should be pretty fast.
	//-- Note: This map contains only common (system) properties available for every speedtree's
	//--	   renderer style. But if your render wants to have some unusual properties you can set
	//--	   them on the device as previous by the name or you can add them here.
	//----------------------------------------------------------------------------------------------
	class ConstantsMap
	{
	public:

		//-- solid set of shader's constants used by speedtree's renderer.
		//-- Warning: names should be in sync with g_constNames enumeration.
		enum EConstant
		{
			CONST_INSTANCE = 0,
			CONST_ALPHA_TEST_ENABLED,
			CONST_TEXTURED_TREES,
			CONST_DIFFUSE_MAP,
			CONST_NORMAL_MAP,
			CONST_USE_NORMAL_MAP,
			CONST_MATERIAL,
			CONST_CULL_ENABLED,
			CONST_LEAF_LIGHT_ADJUST,
			CONST_LEAF_ANGLE_SCALARS,
			CONST_LEAF_ROCK_FAR,
			CONST_WIND_MATRICES,
			CONST_LEAF_ANGLES,
			CONST_USE_HIGH_QUALITY,
			CONST_USE_Z_PRE_PASS,

			CONST_COUNT
		};

		ConstantsMap();
		~ConstantsMap();

		bool		getHandles(const Moo::EffectMaterialPtr& effect);
		D3DXHANDLE	handle(EConstant name) const { return m_handles[name]; }

	private:
		D3DXHANDLE m_handles[CONST_COUNT];
	};

	//-- Represents current state of rendering process for each unique tree part.
	//----------------------------------------------------------------------------------------------
	struct DrawingState
	{
		DrawingState()
			: m_instanced(false), m_material(NULL), m_constantsMap(NULL),
			  m_vertexFormat(NULL), m_instBuffer(NULL), m_offsetInBytes(0)	{ }

		bool					m_instanced;
		Moo::EffectMaterial*	m_material;
		const ConstantsMap*		m_constantsMap;
		Moo::VertexDeclaration*	m_vertexFormat;
		Moo::VertexBuffer*		m_instBuffer;
		uint32					m_offsetInBytes;
	};

	//-- ToDo: move billboard optimizer initialization here. An ideal place for allocation
	//--	   graphics resources is in DeviceCallback methods.


	//-- Singleton class for managing some common rendering data like shaders, vertex buffer for
	//-- instancing and so on. This class allow us to be sure that any graphical resources will be
	//-- recreated if needed and destroyed in time thank to DeviceCallback base class.
	//-- It also responsible for correct changing graphics settings for SpeedTree's renderer.
	//----------------------------------------------------------------------------------------------
	class SpeedTreeRendererCommon : public Moo::DeviceCallback, public Singleton<SpeedTreeRendererCommon>
	{
	private:
		//-- make non-copyable.
		SpeedTreeRendererCommon(const SpeedTreeRendererCommon&);
		SpeedTreeRendererCommon& operator = (const SpeedTreeRendererCommon&);

	public:
		SpeedTreeRendererCommon(const DataSectionPtr& cfg);
		virtual ~SpeedTreeRendererCommon();

		//-- check status of speedtree shaders.
		bool			readyToUse() const;
	
		//-- prepare draw state of each individual tree part for rendering.
		bool			pass(Moo::ERenderingPassType type, bool instanced);

		//-- apply lod bias for trees based on the current graphics settings.
		void			applyLodBias(float& lodNear, float& lodFar);
		float			applyZoomFactor(float distance);

		DrawingState&	drawingState(ETreePart part) { return  m_drawingStates[part]; }
		DrawingState&	specialDrawingState(ESpecialDrawingState state, ETreePart part) { return  m_specialDrawingStates[state][part]; }

		uint32			maxInstBufferSizeInBytes() const;

		const Moo::VertexFormat& vertexFormat( ETreePart part, bool instancing );

		//-- interface DeviceCallback.
		virtual void	createUnmanagedObjects();
		virtual void	deleteUnmanagedObjects();
		virtual void	createManagedObjects();
		virtual void	deleteManagedObjects();

	private:

		void ensureEffectsRecompiled();

		typedef std::pair<Moo::EffectMaterialPtr, ConstantsMap>			Material;
		typedef std::pair<Moo::ComplexEffectMaterialPtr, ConstantsMap>	ComplexMaterial;

		ComplexMaterial				m_materials[TREE_PART_COUNT];
		Material					m_specialMaterials[SPECIAL_DRAWING_STATE_COUNT][TREE_PART_COUNT];
		Moo::VertexDeclaration*		m_vertDclrs[TREE_PART_COUNT][2];
		Moo::VertexBuffer			m_instVBs[TREE_PART_COUNT];
		DrawingState				m_drawingStates[TREE_PART_COUNT];
		DrawingState				m_specialDrawingStates[SPECIAL_DRAWING_STATE_COUNT][TREE_PART_COUNT];

		mutable SpeedTreeSettings	m_settings;
	};

	//----------------------------------------------------------------------------------------------
	inline void SpeedTreeRendererCommon::applyLodBias(float& lodNear, float& lodFar)
	{
		const SpeedTreeSettings::Option& o = m_settings.activeOption();

		lodNear *= o.m_lodBias;
		lodFar  *= o.m_lodBias;
	}

	//----------------------------------------------------------------------------------------------
	inline float SpeedTreeRendererCommon::applyZoomFactor(float distance)
	{
		const SpeedTreeSettings::Option& o = m_settings.activeOption();

		return distance * min(Moo::rc().lodZoomFactor() * o.m_zoomBias, 1.0f);
	}

} //-- speedtree

BW_END_NAMESPACE
