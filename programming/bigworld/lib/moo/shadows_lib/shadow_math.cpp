#include "pch.hpp"

#include "shadow_math.hpp"
#include "bounding.h"

using namespace math_helpers;

//------------------------------------------------------------------------------
// Interface
//------------------------------------------------------------------------------

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
							  float lightDist,
							  float shadowMapSize)
{
	D3DXVECTOR3 corners[8];
	calcFrustrumCornersWorldSpace(
		corners, view, 
		nearPlane, farPlane, 
		fov, aspectRatio);

	D3DXVECTOR3 centerWorld;
	calcCornersCenter(centerWorld, corners);

	D3DXVECTOR3 eye = centerWorld - lightDist * lightDir;
	D3DXVECTOR3 at = centerWorld;
	D3DXVECTOR3 up = SHADOW_MATH_UP_VECTOR;
	if((lightDir.x * lightDir.x + lightDir.z * lightDir.z) < 0.001) {
		up = SHADOW_MATH_HORIZON_VECTOR;
	}
	D3DXMatrixLookAtLH(&outLightView, &eye, &at, &up);

	translateCornersFromWorldSpaceToLightViewSpace(corners, corners, outLightView);

	D3DXVECTOR3 aabbMin, aabbMax;
	calcCornersAABB(aabbMin, aabbMax, corners);

	outMaxZ = aabbMax.z;
	outTexelSizeWorld.x = (aabbMax.x - aabbMin.x) / shadowMapSize;
	outTexelSizeWorld.y = (aabbMax.y - aabbMin.y) / shadowMapSize;

	D3DXMatrixOrthoOffCenterLH(&outLightProj, 
		aabbMin.x, aabbMax.x, 
		aabbMin.y, aabbMax.y,
		0.0f, aabbMax.z);

	D3DXMatrixMultiply(&outLightViewProj, &outLightView, &outLightProj);
}

void 
calcLightViewProjectionMatrixPoints(D3DXMATRIX& outLightView,
									D3DXMATRIX& outLightProj,
									D3DXMATRIX& outLightViewProj,
									D3DXVECTOR2& outTexelSizeWorld,
									float& maxZ,									
									D3DXVECTOR3 casterPoints[], 
									size_t casterPointsCount,
									const D3DXVECTOR3& lightDir,
									float lightDist,
									float shadowMapSize)
{
	D3DXVECTOR3 center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	for(size_t i = 0; i < casterPointsCount; ++i) {
		center += casterPoints[i];
	}
	center /= (float) casterPointsCount;

	D3DXVECTOR3 eye = center - lightDir * lightDist;
	D3DXVECTOR3 at = center;
	D3DXVECTOR3 up = SHADOW_MATH_UP_VECTOR;
	if((lightDir.x * lightDir.x + lightDir.z * lightDir.z) < 0.001) {
		up = SHADOW_MATH_HORIZON_VECTOR;
	}
	D3DXMatrixLookAtLH(&outLightView, &eye, &at, &up);

	D3DXVECTOR3 aabbMin;
	D3DXVECTOR3 aabbMax;
	calcPointsAABB(aabbMin, aabbMax, outLightView, casterPoints, casterPointsCount);

	maxZ = aabbMax.z;
	outTexelSizeWorld.x = (aabbMax.x - aabbMin.x) / shadowMapSize;
	outTexelSizeWorld.y	= (aabbMax.y - aabbMin.y) / shadowMapSize;

	D3DXMatrixOrthoOffCenterLH(&outLightProj,
		aabbMin.x, aabbMax.x, 
		aabbMin.y, aabbMax.y, 
		0.f, maxZ);
	D3DXMatrixMultiply(&outLightViewProj, &outLightView, &outLightProj);
}


void 
calcLightViewProjectionMatrixSimple(D3DXMATRIX& outLightView,
									D3DXMATRIX& outLightProj,
									D3DXMATRIX& outLightViewProj,
									const D3DXVECTOR3& lightPos,
									const D3DXVECTOR3& lightDir,
									float lightDist, float w, float h)
{
	D3DXVECTOR3 eye = lightPos;
	D3DXVECTOR3 at = lightPos + lightDist * lightDir;
	D3DXVECTOR3 up = SHADOW_MATH_UP_VECTOR;
	if((lightDir.x * lightDir.x + lightDir.z * lightDir.z) < 0.001) {
		up = SHADOW_MATH_HORIZON_VECTOR;
	}
	D3DXMatrixLookAtLH(&outLightView, &eye, &at, &up);
	D3DXMatrixOrthoLH(&outLightProj, w, h, 0.0f, lightDist);
	D3DXMatrixMultiply(&outLightViewProj, &outLightView, &outLightProj);
}

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
												 )
{
	D3DXVECTOR3 cornersWorld[8];
	D3DXVECTOR3 centerWorld;
	D3DXVECTOR3 cornersLightView[8];
	D3DXVECTOR3 centerLightView;
	float radius;

	// 0) calcs in world space

	calcFrustrumCornersWorldSpace(
		cornersWorld, view, 
		nearPlane, farPlane, 
		fov, aspectRatio);

	calcCornersCenter(centerWorld, cornersWorld);
	radius = calcCornersRadius(cornersWorld, centerWorld);

	// 1) find fixed center in light view space

	calcLightViewMatrix(outView, lightDir);
	translateCornersFromWorldSpaceToLightViewSpace(cornersLightView, cornersWorld, outView);
	calcCornersCenter(centerLightView, cornersLightView);

	float texelSize = (2.0f * radius) / (float) shadowMapSize;

	centerLightView.x /= texelSize; 
	centerLightView.x = floor(centerLightView.x);
	centerLightView.x *= texelSize;

	centerLightView.y /= texelSize; 
	centerLightView.y = floor(centerLightView.y);
	centerLightView.y *= texelSize;

	// 2) translate fixed center to world space

	D3DXMATRIX invView;
	D3DXMatrixInverse(&invView, NULL, &outView);
	D3DXVec3TransformCoord(&centerWorld, &centerLightView, &invView);


	D3DXVECTOR3 eye = centerWorld - lightDist * lightDir;
	D3DXVECTOR3 at  = centerWorld;
	D3DXVECTOR3 up  = SHADOW_MATH_UP_VECTOR;
	
	if((lightDir.x * lightDir.x + lightDir.z * lightDir.z) < 0.001) 
	{
		up = SHADOW_MATH_HORIZON_VECTOR;
	}
	
	D3DXMatrixLookAtLH(&outView, &eye, &at, &up);

	float zn = 0.01f;
	float zf = lightDist + radius;// + lightDist2;
	float w  = 2.0f * radius;
	float h  = w;

	D3DXMatrixOrthoLH(&outProj, w, h, zn, zf);
	D3DXMatrixMultiply(&outViewProj, &outView, &outProj);

	// Calculation of matrix cull
	{
		//calcFrustrumCornersWorldSpace(
		//	corners, view, 
		//	nearPlane, farPlane, 
		//	fov, aspectRatio);

		D3DXVECTOR3 corners[8];
		D3DXVECTOR3 centerWorld;
		
		calcCornersCenter( centerWorld, cornersWorld );

		D3DXVECTOR3 eye = centerWorld - lightDist * lightDir;
		D3DXVECTOR3 at  = centerWorld;
		D3DXVECTOR3 up  = SHADOW_MATH_UP_VECTOR;

		if((lightDir.x * lightDir.x + lightDir.z * lightDir.z) < 0.001) 
		{
			up = SHADOW_MATH_HORIZON_VECTOR;
		}

		D3DXMATRIX outLightView;
		D3DXMatrixLookAtLH( &outLightView, &eye, &at, &up );

		translateCornersFromWorldSpaceToLightViewSpace( corners, cornersWorld, outLightView );

		D3DXVECTOR3 aabbMin, aabbMax;
		calcCornersAABB(aabbMin, aabbMax, corners);

		D3DXMATRIX outLightProj;
		D3DXMatrixOrthoOffCenterLH( &outLightProj, aabbMin.x, aabbMax.x, aabbMin.y, aabbMax.y, 0.0f, aabbMax.z );
		D3DXMatrixMultiply( &outCullViewProj, &outLightView, &outLightProj );
	}
}


void 
calcLightViewProjectionMatrixPSM(D3DXMATRIX& outView,
								 D3DXMATRIX& outProj,
								 D3DXMATRIX& outViewProj,
								 const D3DXMATRIX& inCameraView,
								 float nearPlane, float farPlane,
								 float fov, float aspectRatio,
								 const D3DXVECTOR3& lightDir,
								 bool isSlideBack, float& slideBack, float minInfinityZ,
								 float lightDist, int shadowMapSize)
{	
	D3DXMATRIX cameraView = inCameraView;
	D3DXMATRIX cameraProj;
	D3DXMATRIX cameraViewProj;

	D3DXVECTOR4 ppLightPosW;

	// 0) Build SlideBack

	slideBack = 0.0f;

	if(isSlideBack) {
		float infZ = farPlane / (farPlane - nearPlane);
		if(infZ < minInfinityZ) {
			D3DXMATRIX virtualCameraView; // offset from the camera observer
			D3DXMATRIX virtualCameraProj; // replaces observer

			float additionalSlide = minInfinityZ * (farPlane - nearPlane) - farPlane + 0.0001f;
			slideBack = additionalSlide;
			float newNearPlane = nearPlane + additionalSlide;
			float newFarPlane = farPlane + additionalSlide;

			D3DXMatrixTranslation(&virtualCameraView, 0.f, 0.f, slideBack);

			float hf = farPlane * tanf(fov / 2.0f);
			float wf = aspectRatio * hf;

			float hn = newNearPlane * (hf / newFarPlane);
			float wn = newNearPlane * (wf / newFarPlane);

			D3DXMatrixPerspectiveLH(&virtualCameraProj, wn, hn, newNearPlane, newFarPlane);

			// applying a slide-back to input data
			D3DXMatrixMultiply(&cameraView, &cameraView, &virtualCameraView);
			cameraProj = virtualCameraProj;
		} else {
			// TODO:
			// 1) Building the necessary matrix of the input data.
			D3DXMatrixPerspectiveFovLH(&cameraProj, fov, aspectRatio, nearPlane, farPlane);		
		}
	} else {
		// 1) Building the necessary matrix of the input data.
		D3DXMatrixPerspectiveFovLH(&cameraProj, fov, aspectRatio, nearPlane, farPlane);		
	}
	D3DXMatrixMultiply(&cameraViewProj, &cameraView, &cameraProj);

	// 2) LightDir translate into post-perspective space.

	D3DXVECTOR3 cameraLightDir;
	D3DXVec3TransformNormal(&cameraLightDir, &lightDir, &cameraView);
	D3DXVec3Normalize(&cameraLightDir, &cameraLightDir);

	ppLightPosW = D3DXVECTOR4(cameraLightDir.x, cameraLightDir.y, cameraLightDir.z, 0.0f);
	D3DXVec4Transform(&ppLightPosW, &ppLightPosW, &cameraProj);

	// ???
	bool invertedLight = ppLightPosW.w > 0.0f;

	//if ( (fabsf(ppLight.w) <= 0.001) )

	D3DXVECTOR3 ppLightPos;
	ppLightPos.x = ppLightPosW.x / ppLightPosW.w;
	ppLightPos.y = ppLightPosW.y / ppLightPosW.w;
	ppLightPos.z = ppLightPosW.z / ppLightPosW.w;

	const float ppCubeRadius = 1.5f;
	const D3DXVECTOR3 ppCubeCenter(0.f, 0.f, 0.5f);
	const D3DXVECTOR3 yAxis(0.0f, 1.0f, 0.0f);

	D3DXMATRIX ppLightView;
	D3DXMATRIX ppLightProj;

	//D3DXVECTOR3 eye(0.f, 3.f, 0.5f);
	//D3DXVECTOR3 at(0.f, 0.f, 0.5f);
	//D3DXVECTOR3 up(0.f, 0.f, 1.0f);
	//D3DXMatrixLookAtLH(&ppLightView, &eye, &at, &up);
	//D3DXMatrixOrthoLH(&ppLightProj, 2.f, 1.f, 0.5f, 3.0f);

	if(!invertedLight) {
		D3DXVECTOR3 lookAt = ppCubeCenter - ppLightPos;

		float distance = D3DXVec3Length(&lookAt);
		lookAt = lookAt / distance;

		// this code is super-cheese.  treats the unit-box as a sphere
		// lots of problems, looks like hell, and requires that MinInfinityZ >= 2
		D3DXMatrixLookAtLH(&ppLightView, &ppLightPos, &ppCubeCenter, &yAxis);

		float fFovy = 2.f*atanf(ppCubeRadius/distance);
		float fAspect = 1.f;
		float fNear = max(0.001f, distance - ppCubeRadius);
		float fFar = distance + ppCubeRadius;
		D3DXMatrixPerspectiveFovLH(&ppLightProj, fFovy, fAspect, fNear, fFar);	

	} else {
		BoundingCone viewCone;

		//  project the entire unit cube into the shadow map  
		BW::vector<BoundingBox> justOneBox;
		BoundingBox unitCube;
		unitCube.minPt = D3DXVECTOR3(-1.f, -1.f, 0.f);
		unitCube.maxPt = D3DXVECTOR3( 1.f, 1.f, 1.f );
		justOneBox.push_back(unitCube);
		D3DXMATRIX tmpIdentity;
		D3DXMatrixIdentity(&tmpIdentity);
		viewCone = BoundingCone(&justOneBox, &tmpIdentity, &ppLightPos);

		ppLightView = viewCone.m_LookAt;

		// construct the inverse projection matrix -- clamp the fNear value for sanity (clamping at too low
		// a value causes significant underflow in a 24-bit depth buffer)
		// the multiplication is necessary since I'm not checking shadow casters
		viewCone.fNear = max(0.001f, viewCone.fNear*0.3f);
		float ppNear = -viewCone.fNear;
		float ppFar  = viewCone.fNear;
		D3DXMatrixPerspectiveLH( 
			&ppLightProj,
			2.f*tanf(viewCone.fovx)*ppNear, 
			2.f*tanf(viewCone.fovy)*ppNear, 
			ppNear, ppFar);
	}
	
	// prepare
	D3DXMATRIX ppLightViewProj;
	D3DXMatrixMultiply(&ppLightViewProj, &ppLightView, &ppLightProj);

	// forming outView and outProj
	outView = cameraView;
	D3DXMatrixMultiply(&outProj, &cameraProj, &ppLightViewProj);

	// finale
	D3DXMatrixMultiply(&outViewProj, &outView, &outProj);
}

//------------------------------------------------------------------------------
// Common
//------------------------------------------------------------------------------

void 
calcLightViewMatrix(D3DXMATRIX& out, const D3DXVECTOR3& lightDir)
{
	D3DXVECTOR3 eye(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 at = lightDir;
	D3DXVECTOR3 up = SHADOW_MATH_UP_VECTOR;
	if((lightDir.x * lightDir.x + lightDir.z * lightDir.z) < 0.001) {
		up = SHADOW_MATH_HORIZON_VECTOR;
	}
	D3DXMatrixLookAtLH(&out, &eye, &at, &up);
}

void 
calcLightCropMatrix(D3DXMATRIX& out, const D3DXVECTOR4& lightDir)
{
	
}

void 
calcCornersAABB(D3DXVECTOR3& outMin, D3DXVECTOR3& outMax, const D3DXVECTOR3 corners[8])
{
	calcPointsAABB(outMin, outMax, corners, 8);
}

void 
calcPointsAABB(D3DXVECTOR3& outMin, D3DXVECTOR3& outMax, const D3DXVECTOR3 points[], size_t pointsCount)
{
	outMin = D3DXVECTOR3( FLT_MAX, FLT_MAX, FLT_MAX);
	outMax = D3DXVECTOR3(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	for(size_t i = 0; i < pointsCount; ++i) {
		if(points[i].x > outMax.x) outMax.x = points[i].x;
		if(points[i].x < outMin.x) outMin.x = points[i].x;
		if(points[i].y > outMax.y) outMax.y = points[i].y;
		if(points[i].y < outMin.y) outMin.y = points[i].y;
		if(points[i].z > outMax.z) outMax.z = points[i].z;
		if(points[i].z < outMin.z) outMin.z = points[i].z;		
	}
}

void 
calcPointsAABB(D3DXVECTOR3& outMin, D3DXVECTOR3& outMax, 
			   const D3DXMATRIX& basis, 
			   const D3DXVECTOR3 points[], size_t pointsCount)
{
	outMin = D3DXVECTOR3( FLT_MAX, FLT_MAX, FLT_MAX);
	outMax = D3DXVECTOR3(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	for(size_t i = 0; i < pointsCount; ++i) {
		D3DXVECTOR3 tmp;
		D3DXVec3TransformCoord(&tmp, &points[i], &basis);
		if(tmp.x > outMax.x) outMax.x = tmp.x;
		if(tmp.x < outMin.x) outMin.x = tmp.x;
		if(tmp.y > outMax.y) outMax.y = tmp.y;
		if(tmp.y < outMin.y) outMin.y = tmp.y;
		if(tmp.z > outMax.z) outMax.z = tmp.z;
		if(tmp.z < outMin.z) outMin.z = tmp.z;		
	}
}


void 
calcFrustrumCornersWorldSpace(D3DXVECTOR3 out[8], 
							  const D3DXMATRIX& view, 
							  float nearPlane, float farPlane, 
							  float fov, float aspectRatio)
{
	D3DXMATRIX invView;
	D3DXMatrixInverse(&invView, NULL, &view);

	// lightView basis in world space
	D3DXVECTOR3 vX = invView.m[0]; // apply to UnitAxisVector(0)
	D3DXVECTOR3 vY = invView.m[1]; // apply to UnitAxisVector(1)
	D3DXVECTOR3 vZ = invView.m[2]; // apply to UnitAxisVector(2)
	D3DXVECTOR3 vC = invView.m[3]; // apply to Origin

	D3DXVec3Normalize(&vX, &vX);
	D3DXVec3Normalize(&vY, &vY);
	D3DXVec3Normalize(&vZ, &vZ);

	// calculate the near plane width and height.
	float nearHeight = tanf(fov * 0.5f) * nearPlane;
	float nearWidth  = nearHeight * aspectRatio;

	// calculate the far plane width and height.
	float farHeight = tanf(fov * 0.5f) * farPlane;
	float farWidth  = farHeight * aspectRatio;

	D3DXVECTOR3 nearCenter = vC + vZ * nearPlane;
	D3DXVECTOR3 farCenter  = vC + vZ * farPlane;

	// near points.
	out[0] = nearCenter - vX * nearWidth - vY * nearHeight;
	out[1] = nearCenter - vX * nearWidth + vY * nearHeight;
	out[2] = nearCenter + vX * nearWidth + vY * nearHeight;
	out[3] = nearCenter + vX * nearWidth - vY * nearHeight;

	// far points.
	out[4] = farCenter - vX * farWidth - vY * farHeight;
	out[5] = farCenter - vX * farWidth + vY * farHeight;
	out[6] = farCenter + vX * farWidth + vY * farHeight;
	out[7] = farCenter + vX * farWidth - vY * farHeight;
}

void 
translateCornersFromWorldSpaceToLightViewSpace(D3DXVECTOR3 out[8], 
											   D3DXVECTOR3 in[8], 
											   const D3DXMATRIX& lightView)
{
	for(int i = 0; i < 8; ++i) {
		D3DXVECTOR4 tmp;
		D3DXVec3Transform(&tmp, &in[i], &lightView);
		out[i] = (FLOAT*) tmp;
	}
}

void 
calcCornersCenter(D3DXVECTOR3& outCenter, 
				  D3DXVECTOR3 corners[8])
{
	outCenter = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	for(int i = 0; i < 8; ++i) {
		outCenter += corners[i];
	}
	outCenter /= 8;
}

float
calcCornersRadius(D3DXVECTOR3 corners[8],
				  const D3DXVECTOR3& center)
{
	float radius = - FLT_MAX;
	for(int i = 0; i < 8; ++i) {
		D3DXVECTOR3 tmp = corners[i] - center;
		float r = sqrt(tmp.x * tmp.x + tmp.y * tmp.y + tmp.z * tmp.z);
		if(r > radius) {
			radius = r;
		}
	}
	return radius;
}

void 
scaleCorners(D3DXVECTOR3 outCorners[8],
			 D3DXVECTOR3 inCorners[8],
			 const D3DXVECTOR3& center,
			 float scale)
{
	for(int i = 0; i < 8; ++i) {
		outCorners[i] = center + scale * (inCorners[i] - center);
	}
}

//------------------------------------------------------------------------------
// Interface
//------------------------------------------------------------------------------

void
applyTextureScaleBiasMat(D3DXMATRIX& out, const D3DXMATRIX& in)
{
	float texOffset = 0.5f; //+ (0.5f / (float)m_shadowMapSize);
	D3DXMATRIX textureScaleBiasMat = D3DXMATRIX(
		0.5f,        0.0f,    0.0f,   0.0f,
		0.0f,       -0.5f,    0.0f,   0.0f,
		0.0f,        0.0f,    1.0f,   0.0f,
		texOffset,	 texOffset,    0.0f,   1.0f
		);

	D3DXMatrixMultiply(&out, &in, &textureScaleBiasMat);
}

void 
applyTextureScaleBiasMat(D3DXMATRIX& out, const D3DXMATRIX& in,
						 const D3DXVECTOR2& textureRectUL, // upper-left
						 const D3DXVECTOR2& textureRectBR) // button-right
{
	float texOffset = 0.5f; //+ (0.5f / (float)m_shadowMapSize);
	D3DXMATRIX textureScaleBiasMat = D3DXMATRIX(
		0.5f,        0.0f,    0.0f,   0.0f,
		0.0f,       -0.5f,    0.0f,   0.0f,
		0.0f,        0.0f,    1.0f,   0.0f,
		texOffset,	 texOffset,    0.0f,   1.0f
	);

	float offsetX = textureRectUL.x;
	float offsetY = textureRectUL.y;
	float scaleX = textureRectBR.x - textureRectUL.x;
	float scaleY = textureRectBR.y - textureRectUL.y;

	D3DXMATRIX rectFitMatrix = D3DXMATRIX(
		scaleX,  0.0f,    0.0f, 0.0f,
		0.0f,    scaleY,  0.0f, 0.0f,
		0.0f,    0.0f,    1.0f, 0.0f,
		offsetX, offsetY, 0.0f, 1.0f
	);

	D3DXMATRIX res;
	D3DXMatrixMultiply(&res, &textureScaleBiasMat, &rectFitMatrix);
	D3DXMatrixMultiply(&out, &in, &res);
}

//------------------------------------------------------------------------------
// Parallel Split Shadow Maps
//------------------------------------------------------------------------------

void calcSplitDistances(float out[], int numSplits, float nearPlane, float farPlane, float lambda)
{
	// Practical split scheme:
	//
	// CLi = n * (f / n) ^ (i / numSplits)
	// CUi = n + (f - n) * (i / numSplits)
	// Ci = CLi * (lambda) + CUi * (1 - lambda)
	//
	// lambda scales between logarithmic and uniform

	//out.clear();

	out[0] = nearPlane;
	for (int i = 1; i < numSplits; ++i) 
	{
		float dimension, logScheme, uniformScheme;
		dimension	  = (float) i / (float) numSplits;
		logScheme	  = nearPlane * powf((farPlane / nearPlane), dimension);
		uniformScheme = nearPlane + (farPlane - nearPlane) * dimension;
		out[i] = logScheme * lambda + uniformScheme * (1.f - lambda);
	}
	out[numSplits] = farPlane;
}

//------------------------------------------------------------------------------
// Perspective Shadow Maps
//------------------------------------------------------------------------------

// TODO

//------------------------------------------------------------------------------
// End
//------------------------------------------------------------------------------