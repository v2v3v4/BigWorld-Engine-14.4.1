#include "pch.hpp"
#include "animation_grid.hpp"
#include "math/perlin_noise.hpp"
#include "flora_vertex_type.hpp"
#include "cstdmf/debug.hpp"
#include "romp/enviro_minder.hpp"
#include "romp/weather.hpp"
#include "moo/effect_visual_context.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

//Multplies the wind by the animation noise grid to get the final
//wind animation value.
static float s_noiseFactor = 0.3f;


FloraAnimationGrid::FloraAnimationGrid( int width, int height ):
	width_( width ),
	height_( height ),
	tTime_( 0.0 )
{
	BW_GUARD;
	nConstants_ = width_*height_;
	constants_ = new Vector4[nConstants_];
	noise_ = new float[nConstants_];
	direction_ = new Vector4[nConstants_];
	pAnimGridSetter_ = new AnimationGridSetter(constants_,nConstants_);

	//Create the watchers once only
	static bool s_createdWatchers = false;
	if (!s_createdWatchers)
	{
		MF_WATCH( "Client Settings/Flora/Noise Factor",
			s_noiseFactor,
			Watcher::WT_READ_WRITE,
			"Multiplier for flora noise animation grid.  Affects overall flora movement." );		
		s_createdWatchers = true;
	}
}


FloraAnimationGrid::~FloraAnimationGrid()
{
	BW_GUARD;
	if( *Moo::rc().effectVisualContext().getMapping( "FloraAnimationGrid" ) == pAnimGridSetter_.get() )
	{
		*Moo::rc().effectVisualContext().getMapping( "FloraAnimationGrid" ) = NULL;
	}

	pAnimGridSetter_ = NULL;
	bw_safe_delete_array(constants_);
	bw_safe_delete_array(noise_);
	bw_safe_delete_array(direction_);
}


void FloraAnimationGrid::set( int constantBase )
{
	BW_GUARD;
	Moo::rc().device()->SetVertexShaderConstantF( constantBase, Vector4( 0.f, 0.f, 255.f, 0.f ), 1 );
	Moo::rc().device()->SetVertexShaderConstantF( constantBase+1, (float*)constants_, nConstants_ );
}


void FloraAnimationGrid::update( float dTime, EnviroMinder& enviro )
{
	BW_GUARD;
	*Moo::rc().effectVisualContext().getMapping( "FloraAnimationGrid" ) = pAnimGridSetter_;

	float windX = enviro.weather()->wind().x;
	float windZ = enviro.weather()->wind().y;

#ifdef EDITOR_ENABLED
	//Checking for 0.f time elapsed from previous frame. (means game is paused)
	//To generate the same animation consistently for automated testing screenshots.
	if (dTime == 0.f)
	{
		srand( 0 );
	}
#endif

	tTime_ += (double)dTime;  
	float animationTime = (float)( fmod( tTime_, 1000.0 ) );

	//TODO : this function just animates the x components for now.
	for ( int z=0; z<height_; z++ )
	{
		//remember - 4 values per x value ( a constant is a vector4 )
		for ( int x=0; x<width_; x++ )
		{
			int idx = x+(z*width_);

			Vector4* pVec = &constants_[x+(z*width_)];
			noise_[idx] = perlin_.noise3( Vector3((float)x,(float)z,animationTime) );
			noise_[idx] *= (-1.f * s_noiseFactor);
			pVec->x = noise_[idx] * windX;
			pVec->y = 0.f;
			pVec->z = noise_[idx] * windZ;
			pVec->w = 0.f;
		}
	}
}


FloraAnimation::FloraAnimation( int width )
{
	m_iWidth = width;

	m_pGrid = new Vector2[width];

	m_fTotalTime = 0;
}

FloraAnimation::~FloraAnimation()
{
	delete[] m_pGrid;
}

void FloraAnimationLocal::update( float dTime, const EnviroMinder &enviro )
{
	m_fTotalTime += dTime;

	Vector2 vWind = enviro.weather()->wind() * enviro.weather()->floraWindFactor();
	float fWind = logf( vWind.length() + 1.0f ) * m_fScale;

	for( int i = 0; i < m_iWidth; i++ )
	{
		Vector2 &vNoise = m_pGrid[i];
		vNoise.x = ( m_noise.noise1( m_fTotalTime + i + 1024 ) - 0.5f ) * 2.0f * fWind;
		vNoise.y = ( m_noise.noise1( m_fTotalTime + i ) - 0.5f ) * 2.0f * fWind;
	}
}

void FloraAnimationGlobal::update( float dTime, const class EnviroMinder &enviro )
{
	m_fTotalTime += dTime;

	Vector2 vWind = enviro.weather()->wind() * enviro.weather()->floraWindFactor() * m_fScale;
	m_vDirection = vWind;
	m_vDirection.normalise();

	for( int i = 0; i < m_iWidth; i++ )
	{
		Vector2 &vNoise = m_pGrid[i];
		float fNoise = m_noise.noise1( i + m_fTotalTime );
		vNoise = vWind * fNoise;
	}
}

BW_END_NAMESPACE

// animation_grid.cpp
