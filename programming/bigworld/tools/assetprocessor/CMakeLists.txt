CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( assetprocessor )

INCLUDE( BWStandardProject )


ADD_DEFINITIONS( -DASSETPROCESSOR_EXPORTS )

BW_SET_OPTIONAL_FILES( SCRIPT_FILES_SRCS
	${BW_GAME_DIR}/tools/misc/asset_processor/AssetDatabase.py
	${BW_GAME_DIR}/tools/misc/asset_processor/AssetProcessor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/BSP2VersionUpdate.py
	${BW_GAME_DIR}/tools/misc/asset_processor/BWAutoImport.py
	${BW_GAME_DIR}/tools/misc/asset_processor/Base64ToUTF8.py
	${BW_GAME_DIR}/tools/misc/asset_processor/CDataConverter.py
	${BW_GAME_DIR}/tools/misc/asset_processor/ChunkFileProcessor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/ChunkFileValidator.py
	${BW_GAME_DIR}/tools/misc/asset_processor/DeprecatedGuiSizeModeFix.py
	${BW_GAME_DIR}/tools/misc/asset_processor/DeprecatedMaterialProperty.py
	${BW_GAME_DIR}/tools/misc/asset_processor/EffectNameChanger.py
	${BW_GAME_DIR}/tools/misc/asset_processor/EffectPropertyFix.py
	${BW_GAME_DIR}/tools/misc/asset_processor/FXFileProcessor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/FileProvider.py
	${BW_GAME_DIR}/tools/misc/asset_processor/FileUtils.py
	${BW_GAME_DIR}/tools/misc/asset_processor/GraphFileProcessor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/GuiFileProcessor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/HardskinnedVertexFormatChange.py
	${BW_GAME_DIR}/tools/misc/asset_processor/ImageFileProcessor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/InvalidXMLTagsProcessor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/LogFile.py
	${BW_GAME_DIR}/tools/misc/asset_processor/MFMFileProcessor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/MaterialFileCaseChecker.py
	${BW_GAME_DIR}/tools/misc/asset_processor/MaterialSectionProcessorBase.py
	${BW_GAME_DIR}/tools/misc/asset_processor/ModelFileProcessor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/OverlaidFXTextureStageFix.py
	${BW_GAME_DIR}/tools/misc/asset_processor/ParticleFileProcessor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/PatrolPathConvertor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/Queue.py
	${BW_GAME_DIR}/tools/misc/asset_processor/RedundantEffectStatesFix.py
	${BW_GAME_DIR}/tools/misc/asset_processor/ScriptFileProcessor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/SpeedtreeNameFix.py
	${BW_GAME_DIR}/tools/misc/asset_processor/UnusedAssetFinder.py
	${BW_GAME_DIR}/tools/misc/asset_processor/VisualFileProcessor.py
	${BW_GAME_DIR}/tools/misc/asset_processor/XMLFileProcessor.py
)
SOURCE_GROUP( "Script_Files" FILES ${SCRIPT_FILES_SRCS} )

SET( SOURCE_FILES_SRCS
	AssetProcessor.cpp
	asset_processor_script.cpp
	asset_processor_script.hpp
	assetprocessor.hpp
	pch.cpp
	pch.hpp
)
SOURCE_GROUP( "Source_Files" FILES ${SOURCE_FILES_SRCS} )

SET( CMAKE_SHARED_LIBRARY_SUFFIX ".pyd" )

BW_ADD_LIBRARY( assetprocessor SHARED 
	${SCRIPT_FILES_SRCS}
	${SOURCE_FILES_SRCS}
)

BW_TARGET_LINK_LIBRARIES( assetprocessor
	cstdmf
	moo
	pyscript
	resmgr
	)

SET_TARGET_PROPERTIES( assetprocessor PROPERTIES
	DEBUG_OUTPUT_NAME
	"_AssetProcessor_d" )
SET_TARGET_PROPERTIES( assetprocessor PROPERTIES
	HYBRID_OUTPUT_NAME
	"_AssetProcessor" )

BW_SET_BINARY_DIR( assetprocessor "${BW_GAME_DIR}/tools/assetprocessor/${BW_PLATFORM}" )

BW_PRECOMPILED_HEADER( assetprocessor pch.hpp )
BW_PROJECT_CATEGORY( assetprocessor "Executables" )
