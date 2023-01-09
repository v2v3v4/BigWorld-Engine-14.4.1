CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( appmgr )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( ALL_SRCS
	app.cpp
	app.hpp
	app.ipp
	appmgr_lib.hpp
	application_input.cpp
	application_input.hpp
	application_input.ipp
	closed_captions.cpp
	closed_captions.hpp
	closed_captions.ipp
	commentary.cpp
	commentary.hpp
	commentary.ipp
	dev_menu.cpp
	dev_menu.hpp
	dev_menu.ipp
	factory.hpp
	factory.ipp
	input_manager.cpp
	input_manager.hpp
	input_manager.ipp
	module.cpp
	module.hpp
	module.ipp
	module_manager.cpp
	module_manager.hpp
	module_manager.ipp
	options.cpp
	options.hpp
	options.ipp
	pch.cpp
	pch.hpp
	script_instance.hpp
	script_instance.ipp
	scripted_module.cpp
	scripted_module.hpp
	scripted_module.ipp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


BW_BLOB_SOURCES( BLOB_SRCS ${ALL_SRCS} )
BW_ADD_LIBRARY( appmgr
	${BLOB_SRCS}
)

BW_TARGET_LINK_LIBRARIES( appmgr INTERFACE
	ashes
	cstdmf
	input
	moo
	resmgr
	romp
	pyscript
	)

BW_PRECOMPILED_HEADER( appmgr pch.hpp )
BW_PROJECT_CATEGORY( appmgr "BigWorld Libs" )
