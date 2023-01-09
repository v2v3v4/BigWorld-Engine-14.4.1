#ifndef __pch_hpp__
#define __pch_hpp__

#define _BOOL

#include <iosfwd>

#include <maya/MSimple.h>
#include <maya/MTypes.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDag.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MPointArray.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItGeometry.h>
#include <maya/MFloatArray.h>
#include <maya/MAnimControl.h>
#include <maya/MTime.h>
#include <maya/MFnTransform.h>
#include <maya/MFloatMatrix.h>
#include <maya/MMatrix.h>
#include <maya/MQuaternion.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MPlug.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlugArray.h>
#include <maya/MFnPhongShader.h>
#include <maya/MFnBlinnShader.h>
#include <maya/MFnLambertShader.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MFnBlendShapeDeformer.h>

#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>

#include "cstdmf/cstdmf_lib.hpp"
#include "resmgr/resmgr_lib.hpp"

#include <string>

//------------------------------------------------------------------------------
//	Sized Integer types
//------------------------------------------------------------------------------
// signed
typedef char				int8;
typedef short				int16;
typedef __int32				int32;

// unsigned
typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned __int32	uint32;


#include "expsets.hpp"
#include "bonevertex.hpp"
#include "boneset.hpp"
#include "skin.hpp"
#include "blendshapes.hpp"

#include "matrix3.hpp"
#include "matrix4.hpp"

extern HINSTANCE hInstance;
#endif // __pch_hpp__
