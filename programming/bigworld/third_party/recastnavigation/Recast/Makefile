LIB = recast

SRCS =										\
	../DebugUtils/Source/DebugDraw			\
	../DebugUtils/Source/RecastDebugDraw	\
	./Source/Recast							\
	./Source/RecastAlloc					\
	./Source/RecastArea						\
	./Source/RecastContour					\
	./Source/RecastFilter					\
	./Source/RecastLayers					\
	./Source/RecastMesh						\
	./Source/RecastMeshDetail				\
	./Source/RecastRasterization			\
	./Source/RecastRegion					\

CPPFLAGS +=							\
	-IInclude						\
	-I../DebugUtils/Include			\


ifndef MF_ROOT
export MF_ROOT := $(subst /bigworld/src/lib/third_party/recastnavigation/Recast,,$(CURDIR))
endif

include $(MF_ROOT)/bigworld/src/build/common.mak
