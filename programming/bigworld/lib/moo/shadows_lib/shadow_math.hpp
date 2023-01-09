#ifndef SHADOW_MATH_H
#define SHADOW_MATH_H

#include "cstdmf/bw_vector.hpp"

// D3DX10 and D3DX9 math look the same. You can include either one into your project.

#include <d3dx9math.h>
//#include <D3DX10math.h>

#ifndef SHADOW_MATH_UP_VECTOR
#define SHADOW_MATH_UP_VECTOR D3DXVECTOR3(0.0f, 1.0f, 0.0f)
#endif

#ifndef SHADOW_MATH_HORIZON_VECTOR
#define SHADOW_MATH_HORIZON_VECTOR D3DXVECTOR3(0.0f, 0.0f, 1.0f)
#endif

struct ShadowMathCamera {
	float	nearPlane;
	float	farPlane;
	float	aspectRatio;
	float	viewHeight;
};

//------------------------------------------------------------------------------
// Interface
//------------------------------------------------------------------------------

//void 
//calcLightViewProjectionMatrixOffCenter(D3DXMATRIX& outLightView,
//									   D3DXMATRIX& outLightProj,
//									   D3DXMATRIX& outLightViewProj,
//									   const D3DXMATRIX& view,
//									   float nearPlane, float farPlane,
//									   float fov, float aspectRatio,
//									   const D3DXVECTOR3& lightDir,
//									   float lightDist);

void 
calcLightViewProjectionMatrix(D3DXMATRIX& outLightView,
							  D3DXMATRIX& outLightProj,
							  D3DXMATRIX& outLightViewProj,
							  D3DXVECTOR2& outTexelSizeWorld,
							  float& outMaxZ,
							  const D3DXMATRIX& view,
							  float nearPlane, float farPlane,
							  float fov, float aspectRatio,
							  const D3DXVECTOR3& lightDir,
							  float lightDist, float shadowMapSize);

void 
calcLightViewProjectionMatrixPoints(D3DXMATRIX& outLightView,
									D3DXMATRIX& outLightProj,
									D3DXMATRIX& outLightViewProj,
									D3DXVECTOR2& texelWorldSize,
									float&		maxZ,
									D3DXVECTOR3 casterPoints[], 
									size_t casterPointsCount,
									const D3DXVECTOR3& lightDir,
									float lightDist,
									float mapSize
									);

void 
calcLightViewProjectionMatrixSimple(D3DXMATRIX& outLightView,
									D3DXMATRIX& outLightProj,
									D3DXMATRIX& outLightViewProj,
									const D3DXVECTOR3& lightPos,
									const D3DXVECTOR3& lightDir,
									float lightDist, float w, float h);

void 
calcLightViewProjectionMatrixJitteringSuppressed(D3DXMATRIX& outView,
												 D3DXMATRIX& outProj,
												 D3DXMATRIX& outViewProj,
												 D3DXMATRIX& outCullViewProj,
												 const D3DXMATRIX& view,
												 float nearPlane, float farPlane,
												 float fov, float aspectRatio,
												 const D3DXVECTOR3& lightDir,
												 float lightDist, int shadowMapSize
												 );

void 
calcLightViewProjectionMatrixPSM(D3DXMATRIX& outView,
								 D3DXMATRIX& outProj,
								 D3DXMATRIX& outViewProj,
								 const D3DXMATRIX& cameraView,
								 float nearPlane, float farPlane,
								 float fov, float aspectRatio,
								 const D3DXVECTOR3& lightDir,
								 bool isSlideBack, float& slideBack, float minInfinityZ,
								 float lightDist, int shadowMapSize);

//void calcLightViewProjectionMatrixSpot(BW::vector<D3DXMATRIX> out, );

//------------------------------------------------------------------------------
// Common
//------------------------------------------------------------------------------

void 
calcLightViewMatrix(D3DXMATRIX& out, const D3DXVECTOR3& lightDir);

void 
calcLightViewMatrixCentered(D3DXMATRIX& out, 
							const D3DXVECTOR3& lightDir,
							const D3DXVECTOR3& lightLockAt,
							float lightDist);

void 
calcCornersAABB(D3DXVECTOR3& outMin, D3DXVECTOR3& outMax, const D3DXVECTOR3 corners[8]);

void 
calcPointsAABB(D3DXVECTOR3& outMin, D3DXVECTOR3& outMax, const D3DXVECTOR3 points[], size_t pointsCount);

void 
calcPointsAABB(D3DXVECTOR3& outMin, D3DXVECTOR3& outMax, 
			   const D3DXMATRIX& basis, 
			   const D3DXVECTOR3 points[], size_t pointsCount);

void 
calcCropMatrixForCorners(D3DXMATRIX& out, const D3DXVECTOR4& lightDir);

void 
calcFrustrumCornersWorldSpace(D3DXVECTOR3 out[8], 
							  const D3DXMATRIX& view, 
							  float nearPlane, float farPlane, 
							  float fov, float aspectRatio);

void 
translateCornersFromWorldSpaceToLightViewSpace(D3DXVECTOR3 out[8], 
											   D3DXVECTOR3 in[8], 
											   const D3DXMATRIX& lightView);

void 
calcCornersCenter(D3DXVECTOR3& outCenter, 
				  D3DXVECTOR3 corners[8]);

float
calcCornersRadius(D3DXVECTOR3 corners[8],
				  const D3DXVECTOR3& center);

void 
scaleCorners(D3DXVECTOR3 outCorners[8],
			 D3DXVECTOR3 inCorners[8],
			 const D3DXVECTOR3& center,
			 float scale);

void 
applyTextureScaleBiasMat(D3DXMATRIX& out, const D3DXMATRIX& in,
						 const D3DXVECTOR2& textureRectUL,  // upper-left 
						 const D3DXVECTOR2& textureRectBR); // button-right);

void 
applyTextureScaleBiasMat(D3DXMATRIX& out, const D3DXMATRIX& in);

//------------------------------------------------------------------------------
// Parallel Split Shadow Maps
//------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
// out vector looks like:
//
// out.size() = numSplits + 1;
//
// out[0]			- nearPlane
// out[1]			- border between Split1 and Split2
// ...
// out[numSplits]	- farPlane
//
void 
calcSplitDistances(float out[], int numSplits, 
				   float nearPlane, float farPlane, 
				   float lambda = 0.5f);

//------------------------------------------------------------------------------
// Spot
//------------------------------------------------------------------------------
// TODO

//------------------------------------------------------------------------------
// Perspective Shadow Maps
//------------------------------------------------------------------------------
// TODO

//------------------------------------------------------------------------------
// End
//------------------------------------------------------------------------------

#endif // SHADOW_MATH_H