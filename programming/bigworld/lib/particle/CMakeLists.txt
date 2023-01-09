CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( particle )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

INCLUDE_DIRECTORIES( . )

SET( ALL_SRCS
	chunk_particles.cpp
	chunk_particles.hpp
	meta_particle_system.cpp
	meta_particle_system.hpp
	particle.cpp
	particle.hpp
	particle.ipp
	particle_lib.hpp
	particle_serialisation.hpp
	particle_system.cpp
	particle_system.hpp
	particle_system.ipp
	particle_system_manager.cpp
	particle_system_manager.hpp
	particles.cpp
	particles.hpp
	particles.ipp
	pch.cpp
	pch.hpp
	py_meta_particle_system.cpp
	py_meta_particle_system.hpp
	py_particle_system.cpp
	py_particle_system.hpp
	py_particle_system.ipp
)
SOURCE_GROUP( "" FILES ${ALL_SRCS} )


SET( RENDERERS_SRCS
	renderers/amp_particle_renderer.cpp
	renderers/amp_particle_renderer.hpp
	renderers/base_mesh_particle_renderer.cpp
	renderers/base_mesh_particle_renderer.hpp
	renderers/blur_particle_renderer.cpp
	renderers/blur_particle_renderer.hpp
	renderers/mesh_particle_renderer.cpp
	renderers/mesh_particle_renderer.hpp
	renderers/mesh_particle_sorter.cpp
	renderers/mesh_particle_sorter.hpp
	renderers/particle_system_draw_item.cpp
	renderers/particle_system_draw_item.hpp
	renderers/particle_system_renderer.cpp
	renderers/particle_system_renderer.hpp
	renderers/particle_system_renderer.ipp
	renderers/point_sprite_particle_renderer.cpp
	renderers/point_sprite_particle_renderer.hpp
	renderers/sprite_particle_renderer.cpp
	renderers/sprite_particle_renderer.hpp
	renderers/sprite_particle_renderer.ipp
	renderers/trail_particle_renderer.cpp
	renderers/trail_particle_renderer.hpp
	renderers/visual_particle_renderer.cpp
	renderers/visual_particle_renderer.hpp
)
SOURCE_GROUP( "renderers" FILES ${RENDERERS_SRCS} )


SET( ACTIONS_SRCS
	actions/barrier_psa.cpp
	actions/barrier_psa.hpp
	actions/barrier_psa.ipp
	actions/collide_psa.cpp
	actions/collide_psa.hpp
	actions/collide_psa.ipp
	actions/flare_psa.cpp
	actions/flare_psa.hpp
	actions/flare_psa.ipp
	actions/force_psa.cpp
	actions/force_psa.hpp
	actions/force_psa.ipp
	actions/jitter_psa.cpp
	actions/jitter_psa.hpp
	actions/jitter_psa.ipp
	actions/magnet_psa.cpp
	actions/magnet_psa.hpp
	actions/magnet_psa.ipp
	actions/matrix_swarm_psa.cpp
	actions/matrix_swarm_psa.hpp
	actions/matrix_swarm_psa.ipp
	actions/node_clamp_psa.cpp
	actions/node_clamp_psa.hpp
	actions/node_clamp_psa.ipp
	actions/orbitor_psa.cpp
	actions/orbitor_psa.hpp
	actions/orbitor_psa.ipp
	actions/particle_system_action.cpp
	actions/particle_system_action.hpp
	actions/particle_system_action.ipp
	actions/scaler_psa.cpp
	actions/scaler_psa.hpp
	actions/scaler_psa.ipp
	actions/sink_psa.cpp
	actions/sink_psa.hpp
	actions/sink_psa.ipp
	actions/source_psa.cpp
	actions/source_psa.hpp
	actions/source_psa.ipp
	actions/splat_psa.cpp
	actions/splat_psa.hpp
	actions/splat_psa.ipp
	actions/stream_psa.cpp
	actions/stream_psa.hpp
	actions/stream_psa.ipp
	actions/tint_shader_psa.cpp
	actions/tint_shader_psa.hpp
	actions/tint_shader_psa.ipp
	actions/vector_generator.cpp
	actions/vector_generator.hpp
	actions/vector_generator.ipp
)
SOURCE_GROUP( "actions" FILES ${ACTIONS_SRCS} )

BW_BLOB_SOURCES( BLOB_SRCS
	${ALL_SRCS}
	${RENDERERS_SRCS}
	${ACTIONS_SRCS}
)
BW_ADD_LIBRARY( particle ${BLOB_SRCS} )

BW_TARGET_LINK_LIBRARIES( particle INTERFACE
	chunk
	cstdmf
	duplo
	math
	model
	moo
	pyscript
	resmgr
	romp
	space
	)

IF( BW_IS_TOOLS )
	BW_TARGET_LINK_LIBRARIES( particle INTERFACE gizmo )
ENDIF()

BW_PRECOMPILED_HEADER( particle pch.hpp )
BW_PROJECT_CATEGORY( particle "BigWorld Libs" )
