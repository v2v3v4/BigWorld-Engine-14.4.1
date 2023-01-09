LIB = detour

SRCS =													\
	Source/DetourAlloc									\
	Source/DetourCommon									\
	Source/DetourNavMesh								\
	Source/DetourNavMeshBuilder							\
	Source/DetourNavMeshQuery							\
	Source/DetourNode									\
	../DebugUtils/Source/DebugDraw						\
	../DebugUtils/Source/DetourDebugDraw				\
	../DetourTileCache/Source/DetourTileCacheBuilder	\


CPPFLAGS +=							\
	-IInclude						\
	-I../DebugUtils/Include			\
	-I../DetourTileCache/Include	\


ifndef MF_ROOT
export MF_ROOT := $(subst /bigworld/src/lib/third_party/recastnavigation/Detour,,$(CURDIR))
endif

include $(MF_ROOT)/bigworld/src/build/common.mak
