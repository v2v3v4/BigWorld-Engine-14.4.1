CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( ual )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardMFCProject )
INCLUDE( BWStandardLibrary )

INCLUDE_DIRECTORIES( ${BW_SOURCE_DIR}/tools )

SET( WINDOWS_SRCS
	pch.cpp
	pch.hpp
	ual_lib.hpp
	ual_resource.h
)
SOURCE_GROUP( "Windows" FILES ${WINDOWS_SRCS} )


SET( ALL_SRCS
	filter_holder.cpp
	filter_holder.hpp
	filter_spec.cpp
	filter_spec.hpp
	filters_ctrl.cpp
	filters_ctrl.hpp
	folder_tree.cpp
	folder_tree.hpp
	list_cache.cpp
	list_cache.hpp
	list_file_provider.cpp
	list_file_provider.hpp
	list_multi_provider.cpp
	list_multi_provider.hpp
	list_xml_provider.cpp
	list_xml_provider.hpp
	smart_list_ctrl.cpp
	smart_list_ctrl.hpp
	ual_callback.hpp
	ual_dialog.cpp
	ual_dialog.hpp
	ual_drop_manager.cpp
	ual_drop_manager.hpp
	ual_favourites.cpp
	ual_favourites.hpp
	ual_history.cpp
	ual_history.hpp
	ual_manager.cpp
	ual_manager.hpp
	ual_name_dlg.cpp
	ual_name_dlg.hpp
	ual_vfolder_loader.cpp
	ual_vfolder_loader.hpp
	vfolder_file_provider.cpp
	vfolder_file_provider.hpp
	vfolder_multi_provider.cpp
	vfolder_multi_provider.hpp
	vfolder_xml_provider.cpp
	vfolder_xml_provider.hpp
	xml_item_list.cpp
	xml_item_list.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( THUMBNAILS_SRCS
	icon_thumb_prov.cpp
	icon_thumb_prov.hpp
	image_thumb_prov.cpp
	image_thumb_prov.hpp
	model_thumb_prov.cpp
	model_thumb_prov.hpp
	thumbnail_manager.cpp
	thumbnail_manager.hpp
	xml_thumb_prov.cpp
	xml_thumb_prov.hpp
)
SOURCE_GROUP( "Thumbnails" FILES ${THUMBNAILS_SRCS} )


BW_BLOB_SOURCES( BLOB_SRCS
	${WINDOWS_SRCS}
	${ALL_SRCS}
	${THUMBNAILS_SRCS}
)
BW_ADD_LIBRARY( ual ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( ual INTERFACE
	controls
	guimanager
	guitabs
	post_processing
	tools_common
	)

BW_PRECOMPILED_HEADER( ual pch.hpp )
BW_PROJECT_CATEGORY( ual "BigWorld Libs" )
