CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( duplo )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

SET( OTHER_ATTACHMENTS_SRCS
	foot_print_renderer.cpp
	foot_print_renderer.hpp
	py_loft.cpp
	py_loft.hpp
	py_splodge.cpp
	py_splodge.hpp
)
IF( BW_IS_CLIENT )
	SET( OTHER_ATTACHMENTS_SRCS ${OTHER_ATTACHMENTS_SRCS}
		foot_trigger.cpp
		foot_trigger.hpp )
ENDIF()
SOURCE_GROUP( "Other_Attachments" FILES ${OTHER_ATTACHMENTS_SRCS} )


SET( FASHION_MODEL_SRCS
	ik_constraint_system.cpp
	ik_constraint_system.hpp
	pydye.cpp
	pydye.hpp
	pyfashion.cpp
	pyfashion.hpp
	py_material_fashion.cpp
	py_material_fashion.hpp
	py_transform_fashion.cpp
	py_transform_fashion.hpp
	tracker.cpp
	tracker.hpp
	tracker.ipp
)
SOURCE_GROUP( "Fashion" FILES ${FASHION_MODEL_SRCS} )

SET( ALL_SRCS
	box_attachment.cpp
	box_attachment.hpp
	decal.cpp
	decal.hpp
	duplo_lib.hpp
	pch.cpp
	pch.hpp
	py_attachment.cpp
	py_attachment.hpp
	py_model_renderer.cpp
	py_model_renderer.hpp
	skeleton_collider.cpp
	skeleton_collider.hpp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( MOTOR_MODEL_SRCS
	bouncer.cpp
	bouncer.hpp
	homer.cpp
	homer.hpp
	linear_homer.cpp
	linear_homer.hpp
	motor.cpp
	motor.hpp
	orbitor.cpp
	orbitor.hpp
	oscillator.cpp
	oscillator.hpp
	propellor.cpp
	propellor.hpp
	servo.cpp
	servo.hpp
)
SOURCE_GROUP( "Motor\\Model" FILES ${MOTOR_MODEL_SRCS} )


SET( CHUNKEMBODIMENTS_SRCS
	chunk_attachment.cpp
	chunk_attachment.hpp
	chunk_dynamic_obstacle.cpp
	chunk_dynamic_obstacle.hpp
	chunk_embodiment.cpp
	chunk_embodiment.hpp
)
SOURCE_GROUP( "ChunkEmbodiments" FILES ${CHUNKEMBODIMENTS_SRCS} )


SET( MODEL_SRCS
	action_queue.cpp
	action_queue.hpp
	pymodel.cpp
	pymodel.hpp
	pymodel.ipp
	pymodelnode.cpp
	pymodelnode.hpp
	py_model_obstacle.cpp
	py_model_obstacle.hpp
)
SOURCE_GROUP( "Model" FILES ${MODEL_SRCS} )


BW_BLOB_SOURCES( BLOB_SRCS
	${OTHER_ATTACHMENTS_SRCS}
	${FASHION_MODEL_SRCS}
	${DRAW_OVERRIDES_SRCS}
	${ALL_SRCS}
	${MOTOR_MODEL_SRCS}
	${CHUNKEMBODIMENTS_SRCS}
	${MODEL_SRCS}
)
BW_ADD_LIBRARY( duplo ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( duplo INTERFACE
	chunk
	cstdmf
	math
	model
	moo
	pyscript
	resmgr
	romp
	)

IF( BW_FMOD_SUPPORT )
	BW_TARGET_LINK_LIBRARIES( duplo INTERFACE fmodsound )
	TARGET_INCLUDE_DIRECTORIES( duplo PRIVATE ${FMOD_INCLUDE_DIRS} )
ENDIF()

IF( BW_UMBRA_SUPPORT )
	TARGET_INCLUDE_DIRECTORIES( duplo PRIVATE
		"${BW_SOURCE_DIR}/third_party/umbra/interface" )
ENDIF()

BW_PRECOMPILED_HEADER( duplo pch.hpp )
BW_PROJECT_CATEGORY( duplo "BigWorld Libs" )
