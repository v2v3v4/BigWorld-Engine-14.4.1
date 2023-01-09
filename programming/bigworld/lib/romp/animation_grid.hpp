#ifndef ANIMATIONGRID_HPP
#define ANIMATIONGRID_HPP

#include "math/perlin_noise.hpp"
#include "cstdmf/stack_tracker.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class exposes a 64x4 matrix to the effect file engine, which
 *	provides perlin noise animation vectors
 */
class AnimationGridSetter : public Moo::EffectConstantValue
{
public:
	AnimationGridSetter( Vector4* grid, int size ):
		grid_( grid ),
		size_( size )
	{
	}

	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{		
		BW_GUARD;
		pEffect->SetVectorArray(constantHandle, grid_, size_);
		return true;
	}

private:
	Vector4* grid_;
	uint32 size_;
};

typedef SmartPointer<AnimationGridSetter> AnimationGridSetterPtr;


/**
 *	This class creates a grid of animation values
 *	for the flora vertex shader.
 *
 *	It blends perlin noise in with a wind amount,
 *	that is settable using the watchers ( or python
 *	watchers interface ).
 *
 *	The watchers controlling wind are in
 *	"Client Settings/Flora/Wind X"
 *	"Client Settings/Flora/Wind Z"
 */
class FloraAnimationGrid
{
public:
	FloraAnimationGrid( int width, int height );
	~FloraAnimationGrid();

	virtual void set( int constantBase );
	virtual void update( float dTime, class EnviroMinder& enviro );
private:
	Vector4*	constants_;
	float*		noise_;
	Vector4*	direction_;
	int			nConstants_;
	int			width_;
	int			height_;
	double		tTime_;
	PerlinNoise perlin_;
	AnimationGridSetterPtr	pAnimGridSetter_;
};

class FloraAnimation
{
public:
	FloraAnimation( int width );
	virtual ~FloraAnimation();

	virtual void update( float dTime, const class EnviroMinder &enviro ) = 0;

	const Vector2* getGrid() const { return m_pGrid; }

	MyPerlin&	getNoise() { return m_noise; }

	void		scale( float f ) { m_fScale = f; }
protected:
	Vector2*	m_pGrid;
	int			m_iWidth;
	MyPerlin	m_noise;
	float		m_fTotalTime;
	float		m_fScale;
};

class FloraAnimationLocal : public FloraAnimation
{
public:
	FloraAnimationLocal( int width ) : FloraAnimation( width ) {}

	virtual void update( float dTime, const class EnviroMinder &enviro );
};

class FloraAnimationGlobal : public FloraAnimation
{
public:
	FloraAnimationGlobal( int width ) : FloraAnimation( width ) {}

	const Vector2& direction() const { return m_vDirection; }

	virtual void update( float dTime, const class EnviroMinder &enviro );
protected:
	Vector2		m_vDirection;
};

BW_END_NAMESPACE

#endif // ANIMATIONGRID_HPP
