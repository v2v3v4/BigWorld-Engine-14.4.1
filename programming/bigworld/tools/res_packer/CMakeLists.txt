CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( res_packer )

INCLUDE( BWStandardProject )

SET( ALL_SRCS
	base_packer.hpp
	config.hpp
	main.cpp
	msg_handler.cpp
	msg_handler.hpp
	packer_helper.cpp
	packer_helper.hpp
	packers.cpp
	packers.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( PACKERS_SRCS
	cdata_packer.cpp
	cdata_packer.hpp
	chunk_packer.cpp
	chunk_packer.hpp
	fx_packer.cpp
	fx_packer.hpp
	image_packer.cpp
	image_packer.hpp
	model_anim_packer.cpp
	model_anim_packer.hpp
	xml_packer.cpp
	xml_packer.hpp
)
SOURCE_GROUP( "Packers" FILES ${PACKERS_SRCS} )

BW_ADD_EXECUTABLE( res_packer
	${ALL_SRCS}
	${PACKERS_SRCS}
)

BW_TARGET_LINK_LIBRARIES( res_packer
	memhook
	cstdmf
	math
	model
	moo
	physics2
	resmgr
	romp
	scene
	space
	chunk
	terrain
	duplo
	particle
	pyscript
	script
	
	# Third party
	libpng
	nedalloc
	zip
	libpython
	re2
	)

IF( BW_FMOD_SUPPORT )
	BW_TARGET_LINK_LIBRARIES( res_packer
		fmodsound
	)
ENDIF()

IF( BW_SPEEDTREE_SUPPORT )
	BW_TARGET_LINK_LIBRARIES( res_packer
		speedtree
	)
ENDIF()

SET_TARGET_PROPERTIES( res_packer PROPERTIES
		DEBUG_OUTPUT_NAME
		"res_packer_d" )

SET_TARGET_PROPERTIES( res_packer PROPERTIES
		HYBRID_OUTPUT_NAME
		"res_packer" )

BW_SET_BINARY_DIR( res_packer "${BW_GAME_DIR}/tools/res_packer/${BW_PLATFORM}" )

BW_PROJECT_CATEGORY( res_packer "Executables" )
