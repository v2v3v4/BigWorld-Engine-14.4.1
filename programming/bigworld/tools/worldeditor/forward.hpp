#ifndef WE_FORWARD_HPP
#define WE_FORWARD_HPP


#include "cstdmf/smartpointer.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

// Forward declarations for the global namespace:
// (please keep in alphabetical order)
class	App;
class	ArrayPropertiesHelper;
class	BackgroundTask;
class	BasePropertiesHelper;
class	BlockConverter;
class	BoundingBox;
class	Chunk;
class	ChunkItem;
class	ChunkItemEditor;
class	ChunkSpace;
class   ChunkStationNode;
class	ChunkWatcher;
class	ClosedCaptions;
class	DataSection;
class	DataType;
class	EditorChunkEntity;
class	EditorChunkItemLinkable;
class	EditorChunkLink;
class   EditorChunkStationNode; 
class	EditorChunkTerrain;
class	EditorChunkVLO;
class	EditorPropertyContainer;
class	EditorRenderable;
class	EntityPropertyParser;
class	EntityPropertyTypeParser;
class   EnviroMinder;
class   ImportImage;
class	LimitSlider;
class	LinkerUndoAddLinkOperation;
class	LinkerUndoChangeLinkOperation;
class	GeneralProperty;
class	Gizmo;
class	GridCoord;
class	MatrixProxy;
class   PlaneEq;
class	ProgressTask;
class	PropertiesHelper;
class	PropertyIndex;
class	SequenceDataType;
class	SheetOptions;
class	SheetTerrain;
class	SpaceHeightMap;
class	StaticLightValues;
class   StationGraph;
class	SuperModelProgressDisplay;
class	TerrainPaintBrush;
class	TexProjMatrixProxy;
class   TimeOfDay;
class	UalDialog;
class	UalItemInfo;
class	WEPythonAdapter;
class	WorldEditorCamera;
class	WorldEditorDoc;
class	WorldTriangle;
class	XConsole;


// Forward typedefs of SmartPointers in the global namespace:
// (please keep in alphabetical order) 
typedef SmartPointer<BlockConverter>			BlockConverterPtr;
typedef SmartPointer<ChunkItem>					ChunkItemPtr;
typedef SmartPointer<ChunkSpace>				ChunkSpacePtr;
typedef SmartPointer<ChunkWatcher>				ChunkWatcherPtr;
typedef SmartPointer<DataSection>				DataSectionPtr;
typedef SmartPointer<DataType>					DataTypePtr;
typedef SmartPointer<EditorChunkEntity>			EditorChunkEntityPtr;
typedef SmartPointer<EditorChunkStationNode>	EditorChunkStationNodePtr;
typedef SmartPointer<EditorChunkTerrain>		EditorChunkTerrainPtr;
typedef SmartPointer<EditorChunkLink>			EditorChunkLinkPtr;
typedef SmartPointer<EditorRenderable>			EditorRenderablePtr;
typedef SmartPointer<EntityPropertyParser>		EntityPropertyParserPtr;
typedef SmartPointer<EntityPropertyTypeParser>	EntityPropertyTypeParserPtr;
typedef SmartPointer<Gizmo>						GizmoPtr;
typedef SmartPointer<SequenceDataType>			SequenceDataTypePtr;
typedef SmartPointer<SpaceHeightMap>			SpaceHeightMapPtr;
typedef SmartPointer<TerrainPaintBrush>			TerrainPaintBrushPtr;
typedef SmartPointer<TexProjMatrixProxy>		TexProjMatrixProxyPtr;


typedef BW::vector<Chunk*>						ChunkPtrVector;
typedef BW::vector<EditorChunkTerrainPtr>		EditorChunkTerrainPtrs;


namespace Moo
{
	// Forward declarations for the Moo namespace:
	// (please keep in alphabetical order)
	class	BaseTexture;
	class 	DirectionalLight;
	class	EffectMaterial;
	class	LightContainer;
	class 	OmniLight;
	class	RenderContext;
	class 	SpotLight;
	class	VertexDeclaration;
	struct	VertexXYZL;


	// Forward typedefs of SmartPointers in the Moo namespace:
	// (please keep in alphabetical order)
	typedef SmartPointer<BaseTexture>			BaseTexturePtr;
	typedef SmartPointer<DirectionalLight>		DirectionalLightPtr;
	typedef SmartPointer<EffectMaterial>		EffectMaterialPtr;
	typedef SmartPointer<LightContainer>		LightContainerPtr;
	typedef SmartPointer<OmniLight>				OmniLightPtr;
	typedef SmartPointer<SpotLight>				SpotLightPtr;
}


namespace StaticLighting
{
	// Forward declarations for the StaticLighting namespace:
	// (please keep in alphabetical order)
	class StaticLightContainer;
}


namespace Terrain
{
	// Forward declarations for the Terrain namespace:
	// (please keep in alphabetical order)
	class	BaseTerrainBlock;
	class	EditorBaseTerrainBlock;
	class	EditorTerrainBlock2;
	class	TerrainSettings;
	class	TerrainTextureLayer;


	// Forward typedefs of SmartPointers in the Terrain namespace:
	// (please keep in alphabetical order)
	typedef SmartPointer<BaseTerrainBlock>			BaseTerrainBlockPtr;
	typedef SmartPointer<EditorBaseTerrainBlock>	EditorBaseTerrainBlockPtr;
	typedef SmartPointer<EditorTerrainBlock2>		EditorTerrainBlock2Ptr;
	typedef SmartPointer<TerrainSettings>			TerrainSettingsPtr;
}


namespace TerrainUtils
{
	// Forward declarations for the TerrainUtils namespace:
	// (please keep in alphabetical order)
	struct TerrainFormat;
}

BW_END_NAMESPACE

#endif // WE_FORWARD_HPP
