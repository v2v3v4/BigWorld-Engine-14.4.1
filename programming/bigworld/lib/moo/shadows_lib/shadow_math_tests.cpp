#include "shadow_math.hpp"

//------------------------------------------------------------------------------
// Config
//------------------------------------------------------------------------------

#define EPSILON 0.01

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

float
calcMatrixDistance(const D3DXMATRIX& m1, const D3DXMATRIX& m2)
{
	D3DXMATRIX diff = m2 - m1;
	float dist = 0.0f;
	for(int i = 0; i < 4; ++i) {
		for(int j = 0; j < 4; ++j) {
			dist += abs(diff.m[i][j]);
		}
	}
	return dist;
}

//------------------------------------------------------------------------------
// calcLightViewProjectionMatrix
//------------------------------------------------------------------------------

bool
test_calcLightViewProjectionMatrix()
{
	D3DXMATRIX outLightView;
	D3DXMATRIX outLightProj;
	D3DXMATRIX outLightViewProj;

	D3DXVECTOR3 eye;
	D3DXVECTOR3 at;
	D3DXVECTOR3 up;

	D3DXMATRIX view;
	eye = D3DXVECTOR3 (0.5f, 0.5f, -0.5f);
	at = D3DXVECTOR3 (0.5f, 0.5f, 0.5f);
	up = SHADOW_MATH_UP_VECTOR;
	D3DXMatrixLookAtLH(&view, &eye, &at, &up);

	float nearPlane = 0.5f;
	float farPlane = 1.5f;
	float fov = (float) D3DX_PI / 2.0f;
	float aspectRatio = 1.0f;
	D3DXVECTOR3 lightDir(1.0f, 0.0f, 0.0f);
	float lightDist = 10.0f;

	calcLightViewProjectionMatrix(
		outLightView, outLightProj, outLightViewProj,
		view, nearPlane, farPlane,
		fov, aspectRatio,
		lightDir, lightDist);

	D3DXMATRIX properLightView;
	D3DXMATRIX properLightProj;
	eye = D3DXVECTOR3 (-9.5f, 0.5f, 0.5f);
	at = D3DXVECTOR3 (0.5f, 0.5f, 0.5f);
	up = SHADOW_MATH_UP_VECTOR;
	D3DXMatrixLookAtLH(&properLightView, &eye, &at, &up);
	D3DXMatrixOrthoLH(&properLightProj, 1.0f, 3.0f, 0.0f, 11.0f);

	bool v = calcMatrixDistance(properLightView, outLightView) < EPSILON;
	bool p = calcMatrixDistance(properLightProj, outLightProj) < EPSILON;

	return (v && p);
}

//------------------------------------------------------------------------------
// calcLightViewProjectionMatrixJitteringSuppressed
//------------------------------------------------------------------------------

bool
test_calcLightViewProjectionMatrixJitteringSuppressed()
{
	D3DXMATRIX outLightView;
	D3DXMATRIX outLightProj;
	D3DXMATRIX outLightViewProj;

	D3DXVECTOR3 eye;
	D3DXVECTOR3 at;
	D3DXVECTOR3 up;

	D3DXMATRIX view;
	eye = D3DXVECTOR3 (0.5f, 0.5f, -0.5f);
	at = D3DXVECTOR3 (0.5f, 0.5f, 0.5f);
	up = SHADOW_MATH_UP_VECTOR;
	D3DXMatrixLookAtLH(&view, &eye, &at, &up);

	float nearPlane = 0.5f;
	float farPlane = 1.5f;
	float fov = (float) D3DX_PI / 2.0f;
	float aspectRatio = 1.0f;
	D3DXVECTOR3 lightDir(1.0f, 0.0f, 0.0f);
	float lightDist = 10.0f;

	calcLightViewProjectionMatrixJitteringSuppressed(
		outLightView, outLightProj, outLightViewProj,
		view, nearPlane, farPlane,
		fov, aspectRatio,
		lightDir, lightDist, 1024);

	D3DXMATRIX properLightView;
	D3DXMATRIX properLightProj;
	eye = D3DXVECTOR3 (-9.5f, 0.5f, 0.5f);
	at = D3DXVECTOR3 (0.5f, 0.5f, 0.5f);
	up = SHADOW_MATH_UP_VECTOR;
	D3DXMatrixLookAtLH(&properLightView, &eye, &at, &up);
	float d = sqrt(19.0f / 4.0f);
	D3DXMatrixOrthoLH(&properLightProj, 2 * d, 2 * d, 0.0f, lightDist + d);

	bool v = calcMatrixDistance(properLightView, outLightView) < EPSILON;
	bool p = calcMatrixDistance(properLightProj, outLightProj) < EPSILON;

	return (v && p);
}

//------------------------------------------------------------------------------
// calcLightViewProjectionMatrixPSM
//------------------------------------------------------------------------------

bool
test_calcLightViewProjectionMatrixPSM()
{
	D3DXMATRIX outLightView;
	D3DXMATRIX outLightProj;
	D3DXMATRIX outLightViewProj;

	D3DXMATRIX view;
	D3DXMatrixIdentity(&view);

	D3DXVECTOR3 lightDir;
	
	lightDir = D3DXVECTOR3(1.0f, 0.0f, 1.0f);
	calcLightViewProjectionMatrixPSM(
		outLightView, outLightProj, outLightViewProj,
		view, 1.0f, 4.0f, D3DXToRadian(90.0f), 1.0f, lightDir, 0.0f, 2048);

	lightDir = D3DXVECTOR3(1.0f, 0.0f, -1.0f);
	calcLightViewProjectionMatrixPSM(
		outLightView, outLightProj, outLightViewProj,
		view, 1.0f, 4.0f, D3DXToRadian(90.0f), 1.0f, lightDir, 0.0f, 2048);

	//bool v = calcMatrixDistance(properLightView, outLightView) < EPSILON;
	//bool p = calcMatrixDistance(properLightProj, outLightProj) < EPSILON;
	//return (v && p);

	return false;
}


//------------------------------------------------------------------------------
// Helper macro
//------------------------------------------------------------------------------

#define RUN_TEST(test)				\
	printf("run %s:\t", #test);		\
	if(test()) {					\
		printf("... ok\n");			\
	} else {						\
		printf("... bad\n");		\
	}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int
main()
{
	RUN_TEST(test_calcLightViewProjectionMatrix)
	RUN_TEST(test_calcLightViewProjectionMatrixJitteringSuppressed)
	RUN_TEST(test_calcLightViewProjectionMatrixPSM)
}

//------------------------------------------------------------------------------
// End
//------------------------------------------------------------------------------