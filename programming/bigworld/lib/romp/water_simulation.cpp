#include "pch.hpp"

// -----------------------------------------------------------------------------
// Section: Includes
// -----------------------------------------------------------------------------

#include "water.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/dogwatch.hpp"

#include "moo/dynamic_vertex_buffer.hpp"
#include "moo/dynamic_index_buffer.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/effect_material.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/graphics_settings.hpp"

#include "enviro_minder.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk.hpp"

#include "speedtree/speedtree_renderer.hpp"

#include "cstdmf/watcher.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "resmgr/auto_config.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "water_simulation.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: Statics
// -----------------------------------------------------------------------------
// SimulationCell statics:
float							SimulationCell::s_hitTime = -1.f;
float							SimulationCell::s_waveSpeedSquared = 150.0f;
float							SimulationCell::s_landScaleX = 50.f;
float							SimulationCell::s_landScaleY = 20.f;
float							SimulationCell::s_landScaleZ = 0.0f;

// SimulationManager statics:
SimulationManager*				SimulationManager::s_instance_=NULL;
float							SimulationManager::s_maxIdleTime_ = 20.0f;
int								SimulationManager::s_maxTextureBlocks_ = 6;

// General:
static int						s_rainDeadZone = 6;
static int						s_rtSerial = 0;

//Auto config strings
static AutoConfigString s_simCookieMap( "environment/waterSimCookieMap" );
static AutoConfigString s_nullSimMap( "environment/waterNullSimMap" );

// -----------------------------------------------------------------------------
// Section: Defines
// -----------------------------------------------------------------------------

#define MAX_SIM_TEXTURE_SIZE 256
#define MAX_SIM_MOVEMENTS 10


// -----------------------------------------------------------------------------
// Section: SimulationTextureBlock
// -----------------------------------------------------------------------------

/**
 *	SimulationTextureBlock constructor
 */
SimulationTextureBlock::SimulationTextureBlock() :
		width_(0), height_(0),
		locked_( false ),
		simulationIndex_( 0 ) 
{
	BW_GUARD;
	for (int i=0; i<3; i++)
		simTexture_[i] = NULL;
}


/**
 *	SimulationTextureBlock constructor with width/height
 */
SimulationTextureBlock::SimulationTextureBlock( int width, int height ) :
		width_( width ), height_( height ),
		locked_( false ),
		simulationIndex_( 0 )
{
	BW_GUARD;
	for (int i=0; i<3; i++)
		simTexture_[i] = NULL;
}


/**
 *	Locks a texture block to allow simulation
 */
bool SimulationTextureBlock::lock()
{
	BW_GUARD;
	if ( locked_ )
		return false;
	else
	{
		locked_=true;
		clear();
		return true;
	}
}


/**
 *	Initialise a texture block
 */
void SimulationTextureBlock::init( int width, int height )
{
	BW_GUARD;
	width_ = width;
	height_ = height;

	char rtidx[3];
	rtidx[2] = 0;

	// Create the render targets
	for (int i=0; i<3; i++)
	{
		rtidx[0] = '1'+ s_rtSerial/10;
		rtidx[1] = '1'+ s_rtSerial%10;
		s_rtSerial++;
		simTexture_[i] = new Moo::RenderTarget( BW::string("SimulationTarget") + rtidx );
	}
}

/**
 *	Clean up.
 */
void SimulationTextureBlock::fini( )
{
	BW_GUARD;
	for (int i=0; i<3; i++)
	{
		simTexture_[i] = NULL;
	}
}

/**
 *	Create the simulation textures for this block
 */
void SimulationTextureBlock::recreate()
{
	BW_GUARD;
	for (int i=0; i<3; i++)
	{
		//ATI dont support filtering on FP.. TODO: try manually filter?
		if ( Moo::rc().deviceInfo( Moo::rc().deviceIndex() ).identifier_.VendorId == 0x1002 )
			simTexture_[i]->create( width_, height_, true , D3DFMT_A16B16G16R16, NULL );
		else
			simTexture_[i]->create( width_, height_, true , D3DFMT_A16B16G16R16F, NULL ); //FP texture		
	}
	clear();
	unlock();
}


/**
 *	Clear the blocks textures
 */
void SimulationTextureBlock::clear()
{
	BW_GUARD;
	Moo::rc().pushRenderState(D3DRS_COLORWRITEENABLE);

	for (int i=0; i<3; i++)
	{
		if ( simTexture_[i]->push() )
		{
			Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
			Moo::rc().device()->Clear(0, NULL, D3DCLEAR_TARGET, 0x0000ff00, 1.f, 0 ); //clear to green
			simTexture_[i]->pop();		
		}
	}

	Moo::rc().popRenderState();
}


// -----------------------------------------------------------------------------
// Section: SimulationManager
// -----------------------------------------------------------------------------

#ifdef _DEBUG
bool SimulationManager::s_paused_ = false;
#endif

/**
 *	SimulationManager constructor
 */
SimulationManager::SimulationManager() : 
			availableBlocks_( 0 ),
			simulationSize_( MAX_SIM_TEXTURE_SIZE ),			
			cookieTexture_( NULL ),
			nullSim_( NULL )
{ 
	BW_GUARD;
	initInternal();
}

/**
 *	SimulationManager destructor
 */
SimulationManager::~SimulationManager()
{ 
	BW_GUARD;
	finiInternal();
}


/**
 *	Reset all simulation blocks
 */
void SimulationManager::resetSimulations()
{
	BW_GUARD;
	for (uint i=0; i<blocks_.size(); i++)
	{
		blocks_[i].clear();
	}	
	rainSimulation_.clear();
}


/**
 *	Tick all simulation blocks
 */
void SimulationManager::tickSimulations()
{
	BW_GUARD;
	for (uint i=0; i<blocks_.size(); i++)
	{
		blocks_[i].tickSimulation();
	}
}


/**
 *	Loads the required resources
 */
void SimulationManager::loadResources()
{
	BW_GUARD;
	cookieTexture_ = Moo::TextureManager::instance()->get(s_simCookieMap, true, true, true, "texture/water");
	nullSim_ = Moo::TextureManager::instance()->get(s_nullSimMap, true, true, true, "texture/water");
}


/**
 *	Rebuilds the texture blocks
 */
void SimulationManager::recreateBlocks()
{
	BW_GUARD;
	// Create the render textures
	if (Moo::rc().supportsTextureFormat( D3DFMT_A16B16G16R16F ))
	{
		for (uint i=0; i<blocks_.size(); i++)
		{
			blocks_[i].recreate();
		}
		rainSimulation_.recreate();
	}
}


/**
 *	Generates/ticks rain simulation texture.
 */
void SimulationManager::simulateRain( float dTime, float amount, Moo::EffectMaterialPtr effect )
{
	BW_GUARD;
	if (amount)
		rainSimulation_.resetIdleTimer();
	else
		rainSimulation_.updateTime(dTime);

	rainSimulation_.simulate(dTime,amount,effect);
	rainSimulation_.tick();
}


/**
 *	Returns an available texture block and locks it for use
 */
SimulationTextureBlock* SimulationManager::requestSimulationTextureBlock( SimulationCell * cell )
{
	BW_GUARD;
	MF_ASSERT(cell);
	if (cell->perturbed() && cell->simulationBlock() == NULL)
	{
		TextureBlocks::iterator it = blocks_.begin();
		for (;it != blocks_.end(); it++)
		{
			if ( (*it).lock() )
				return &(*it);
		}
	}
	return 0;
}


/**
 *	Releases a texture block from being used
 */
void SimulationManager::releaseSimulationTextureBlock( SimulationTextureBlock* block )
{
	BW_GUARD;
	MF_ASSERT(block);	
	block->unlock();
}

// counter of references. Is needed to release simulation resources when all water objects are being deleted
static int refCount = 0;
/**
 *	Initialises the simulation manager instance
 */
/*static*/ void SimulationManager::init()
{
	BW_GUARD;
	if (s_instance_==NULL)
	{
		s_instance_ = new SimulationManager();
	}
	refCount++;
}

/**
 *	Destroy the Sim Manager instance.
 */
/*static*/ void SimulationManager::fini()
{
	BW_GUARD;
	refCount--;
	if (refCount <= 0 && s_instance_)
	{
		delete s_instance_;
		s_instance_ = NULL;
		refCount = 0;
	}
}

/**
 *	Initialises the simulation manager
 */
void SimulationManager::initInternal()
{
	BW_GUARD;
	INFO_MSG("Initialising SimulationManager\n");
	// If there arent any blocks.... make some
	if (blocks_.size() == 0)
		blocks_.resize(s_maxTextureBlocks_);

	for (uint i=0; i<blocks_.size(); i++)
	{
		blocks_[i].init(simulationSize_, simulationSize_);
	}

	rainSimulation_.init(simulationSize_);
}


/**
 *	Clean up.
 */
void SimulationManager::finiInternal()
{
	BW_GUARD;
	INFO_MSG("Destroying SimulationManager\n");

	cookieTexture_ = NULL;
	nullSim_ = NULL;

	blocks_.clear();
	rainSimulation_.fini();
}


/**
 *	Setup the simulation to be drawn.
 */
void SimulationManager::setupSim( SimulationTextureBlock* block,
									ComObjectWrap<ID3DXEffect>& pEffect,
									float dTime )
{
	BW_GUARD;
	MF_ASSERT(block);		

	pEffect->SetTexture( "previousHeightMap", block->previous()->pTexture());
	pEffect->SetTexture( "heightMap", block->current()->pTexture());

	if ( cookieTexture() )
		pEffect->SetTexture("cookieMap", cookieTexture()->pTexture());

	pEffect->SetFloat("psDeltaTime", dTime);

	//TODO: need to unlock width & height??
	MF_ASSERT( block->width() == block->height() );
	int size = block->width(); // as per the assert above.. assuming height == width
#ifndef EDITOR_ENABLED
	static bool once = true;

	if (once)
	{
		once = false;
#endif
		float invSizeShifted = (1.0f/float(size));
		Vector4 delta_x0y1( -invSizeShifted, 0.0f, 0.0f, 0.0f );
		Vector4 delta_x2y1( invSizeShifted, 0.0f, 0.0f, 0.0f );
		Vector4 delta_x1y0(  0.0f, -invSizeShifted, 0.0f, 0.0f );
		Vector4 delta_x1y2(  0.0f, invSizeShifted, 0.0f, 0.0f );
		Vector4 gridSize(	SimulationCell::s_landScaleX / size, //TODO: move this static?
 										0.0f,		
 										SimulationCell::s_landScaleY / size,
										0.0f );

		pEffect->SetVector( "psSimulationTexCoordDelta_x0y1", &delta_x0y1);
		pEffect->SetVector( "psSimulationTexCoordDelta_x2y1", &delta_x2y1);
		pEffect->SetVector( "psSimulationTexCoordDelta_x1y0", &delta_x1y0);
		pEffect->SetVector( "psSimulationTexCoordDelta_x1y2", &delta_x1y2);
		pEffect->SetFloat( "dampening", 0.9f );
		pEffect->SetVector( "psSimulationGridSize", &gridSize);

#ifndef EDITOR_ENABLED
	}
#endif
	//TODO: cleanup the variable uploads...
	pEffect->SetFloat( "psSimulationWaveSpeedSquared",
						Math::clamp(0.f, SimulationCell::s_waveSpeedSquared, 400.f) ); // TODO: move this static?
	pEffect->SetFloat( "psSimulationOneHalfTimesDeltaTimeSquared",
						dTime * dTime );
	pEffect->SetInt( "perturb", 0 );
	pEffect->CommitChanges();

}


/**
 *	Draw the movements to the block and update the simulation. 
 * (assumes the block is the current render target)
 */
void SimulationManager::simulateBlock( Moo::EffectMaterialPtr effect, SimulationTextureBlock* block, int borderSize/*=0*/ )
{
	BW_GUARD;
	//TODO: want to unlock width/height?
	int size = block->width(); 

	DX::Viewport d3dvp;
	d3dvp.X = borderSize; d3dvp.Y = borderSize;
	d3dvp.Width = size-(2*borderSize); d3dvp.Height = size-(2*borderSize);

	d3dvp.MinZ = 0.f; d3dvp.MaxZ = 1.f;

	Moo::rc().setViewport( &d3dvp );
	Moo::rc().world( Matrix::identity );
	Moo::rc().view( Matrix::identity );
	Moo::rc().projection( Matrix::identity );
	Moo::rc().setFVF(VERTEX_TYPE::fvf());

	for( uint32 i = 0; i < min(Moo::rc().maxSimTextures(),(uint16)5); i++ )
	{
		Moo::rc().setTexture( i, NULL );
	}
	// Render the vertex buffer
	Water::s_quadVertexBuffer_->activate();	

	effect->begin();
	effect->beginPass(0);
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
								D3DCOLORWRITEENABLE_RED | 
								D3DCOLORWRITEENABLE_GREEN | 
								D3DCOLORWRITEENABLE_BLUE | 
								D3DCOLORWRITEENABLE_ALPHA );
	Moo::rc().drawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
	effect->endPass();
	effect->end();
}


/**
 *	Draw the movements update the simulation.
 */
void SimulationManager::drawMovements( Moo::EffectMaterialPtr effect, Simulation* sim )
{
	BW_GUARD;
	ComObjectWrap<ID3DXEffect> pEffect = effect->pEffect()->pEffect();

	Movements::const_iterator it = sim->movements().begin();
	bool firstMove=true;
	//int movementCount=0;
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA );

	//note: removed the max sim movement restriction....
	//for (; it != sim->movements().end() && movementCount < MAX_SIM_MOVEMENTS; it++ )
	for (; it != sim->movements().end(); it++ )
	{
		//movementCount++;
		if (firstMove)
		{
			sim->resetIdleTimer();
			pEffect->SetInt("perturb", 1);

			firstMove = false;
			effect->begin();
			effect->beginPass(0);
		}
		float x=0, y=0;
		Vector3 from = (it->first).first;
		Vector3 to = (it->first).second;
		float displacement = to.y;

		pEffect->SetFloat( "impactStrength",
			Math::clamp(0.f, Waters::instance().movementImpact()*displacement,1000.f)  );

		float diameter = it->second;
		float scale = Math::clamp(0.01f, diameter / sim->size(), 1.f);

		x =  ((to.x - 0.5f)*2.f);
		y = -((to.z - 0.5f)*2.f);
		Matrix trans;
		trans.setScale(scale, scale, scale);

		trans.postTranslateBy( Vector3(x,y,0) );
		pEffect->SetMatrix("WorldViewProj", &trans);

		pEffect->CommitChanges();
		Moo::rc().drawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);				
	}
	if (!firstMove)
	{
		effect->endPass();
		effect->end();   
	}
}


// -----------------------------------------------------------------------------
// Section: Simulation
// -----------------------------------------------------------------------------

/**
 *	Simulation constructor
 */
Simulation::Simulation() : 
	size_( 256 ),
	perturb_( true ),
	idleTime_(999),
	lastSimTime_(0),
	bActive_( false ),
	lastMovementFrame_( uint32(-1) )
{
	BW_GUARD;
}


// -----------------------------------------------------------------------------
// Section: RainSimulation
// -----------------------------------------------------------------------------

/**
 *	RainSimulation constructor
 */
RainSimulation::RainSimulation()
{
	BW_GUARD;
	perturbed(false);
	activate();
	//resetIdleTimer();
}

#define GEN_RANDOM_RAIN() float(Math::clamp(s_rainDeadZone, rand() % 100, (100-s_rainDeadZone) )) / 100.f

/**
 *	Generates/ticks rain simulation texture.
 */
void RainSimulation::simulate( float dTime, float amount, Moo::EffectMaterialPtr effect )
{
	BW_GUARD;
	ComObjectWrap<ID3DXEffect> pEffect = effect->pEffect()->pEffect();
	pEffect->SetInt( "edge", 0 );
	SimulationManager::instance().setupSim( &simulationTarget_, pEffect, dTime );

	if (idleTime() == 0.f)
	{
		static float time = 0.f;
		time += dTime;
		float maxDropRate = 1.f/110.f;
		float minDropRate = 1.f/20.f;
		float dropRate = Math::lerp(amount, minDropRate, maxDropRate);
		while (time >= dropRate)
		{
			time -= dropRate;
			float x = GEN_RANDOM_RAIN();
			float y = GEN_RANDOM_RAIN();
			addMovement(Vector3(x, 0.0f, y), Vector3(x, 0.5f, y), 0.5);
		}
	}

	if ( simulationTarget_.push() )
	{
		SimulationManager::instance().simulateBlock( effect, &simulationTarget_, Waters::highQualityMode() ? SIM_BORDER_SIZE : 0 );
		if (idleTime() == 0.f)
			SimulationManager::instance().drawMovements( effect, this );
		clearMovements();
		simulationTarget_.pop();
	}

	//needed?
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, 
								D3DCOLORWRITEENABLE_RED |
								D3DCOLORWRITEENABLE_GREEN |
								D3DCOLORWRITEENABLE_BLUE );
}

// -----------------------------------------------------------------------------
// Section: SimulationCell
// -----------------------------------------------------------------------------

/**
 *	SimulationCell constructor
 */
SimulationCell::SimulationCell() : 
	edge_( 0 ),
	simulationTarget_( 0 ),
	edgeActivation_( false ),
	tickMark_( 0 )
{
	BW_GUARD;
	static bool firstTime=true;
	if ( firstTime )
	{
		s_hitTime = -1.f;
		s_waveSpeedSquared = 200.0f;
		s_landScaleX = 50.f;
		s_landScaleY = 20.f;
		s_landScaleZ = 0.0f;

		firstTime = false;
		MF_WATCH( "Client Settings/Water/water speed square", s_waveSpeedSquared,
						Watcher::WT_READ_WRITE,
						"Simulated wave propagation speed" );
						//TODO: move smoothness into a per-surface variable.
		MF_WATCH( "Client Settings/Water/Rain dead zone", s_rainDeadZone,
						Watcher::WT_READ_WRITE,
						"Area of rain texture border that wont receive rain drops (percentage)" );

#ifdef _DEBUG
		MF_WATCH( "Client Settings/Water/pause sim", SimulationManager::s_paused_,
						Watcher::WT_READ_WRITE,
						"SimulationManager::s_paused_" );
#endif						

	}
}


/**
 *	Binds this cell to the second neighbour texture.
 */
void SimulationCell::bindAsNeighbour( Moo::EffectMaterialPtr effect, const BW::string& binding )
{
	BW_GUARD;
	if (simulationTarget_)
	{
		ComObjectWrap<ID3DXEffect> pEffect = effect->pEffect()->pEffect();

		if ( this->mark() == Waters::s_nextMark_ ) // already ticked
		{
			pEffect->SetTexture( binding.c_str(), 
							simulationTarget_->previous()->pTexture());
		}
		else
		{
			pEffect->SetTexture( binding.c_str(), 
							simulationTarget_->current()->pTexture());
		}
	}
}

/**
 *	Calculates the simulation texture result
 */
void SimulationCell::simulate( Moo::EffectMaterialPtr effect, float dTime, Waters& group )
{
	BW_GUARD;
	updateTime(dTime);
	if ( !isActive() )
		return;

	ComObjectWrap<ID3DXEffect> pEffect = effect->pEffect()->pEffect();

	if (edgeActivation())
	{
		pEffect->SetInt( "edge", edge_ );

		int y0=(edge_ & 1) ? 1 : 0;
		int x0=(edge_ & 2) ? 1 : 0;
		int y2=(edge_ & 4) ? 1 : 0;
		int x2=(edge_ & 8) ? 1 : 0;	

		// Require texture to be available for active textures
		// that will be sampled in shader.
		bindNeighbour( effect, 0 );
		bindNeighbour( effect, 1 );
		bindNeighbour( effect, 2 );
		bindNeighbour( effect, 3 );

		pEffect->SetInt( "Y0_enabled", y0 );
		pEffect->SetInt( "X0_enabled", x0 );
		pEffect->SetInt( "Y2_enabled", y2 );
		pEffect->SetInt( "X2_enabled", x2 );
	}
	else
		pEffect->SetInt( "edge", 0 );

	SimulationManager::instance().setupSim( simulationTarget_, pEffect, dTime );

	if ( simulationTarget_->push() )
	{
		SimulationManager::instance().simulateBlock( effect, simulationTarget_,
						Waters::s_highQuality_ ? SIM_BORDER_SIZE : 0 );

		if( perturbed() )
		{
			SimulationManager::instance().drawMovements( effect, this );
		}
		clearMovements();
		simulationTarget_->pop();
	}

	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, 
								D3DCOLORWRITEENABLE_RED |
								D3DCOLORWRITEENABLE_GREEN |
								D3DCOLORWRITEENABLE_BLUE );
}


/**
 *	Calculates the simulation texture result
 */
void SimulationCell::stitchEdges( Moo::EffectMaterialPtr effect, float dTime, Waters& group )
{
	BW_GUARD;
	if ( !isActive() )
		return;

	ComObjectWrap<ID3DXEffect> pEffect = effect->pEffect()->pEffect();

	if (edgeActivation())
	{
		// Shader will always stitch all edges, so make cell neighbour textures 
		// (fall back to sim null texture) always available for stitching.
		// This allows decay of stale sim pixels inside the borders which 
		// can otherwise create simulation artifacts along cell boundaries.
		bindNeighbour( effect, 0 );
		bindNeighbour( effect, 1 );
		bindNeighbour( effect, 2 );
		bindNeighbour( effect, 3 );

		Moo::RenderTargetPtr rt = simulationTarget_->result();

		if ( rt->push() )
		{
			SimulationManager::instance().simulateBlock( effect, simulationTarget_ );
			rt->pop();
		}
		Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, 
									D3DCOLORWRITEENABLE_RED |
									D3DCOLORWRITEENABLE_GREEN |
									D3DCOLORWRITEENABLE_BLUE );
	}
}


/**
 *	Initialise a simulation cell
 */
void SimulationCell::init( int newSize, float cellSize )
{
	BW_GUARD;
	cellSize_	= cellSize;	
	edge_		= 0;	
	size(newSize);
	perturbed(false);	
}


/**
 *	Create the unmanaged resources
 */
void SimulationCell::createUnmanaged()
{
	BW_GUARD;
	SimulationManager::instance().recreateBlocks();
}


/**
 *	Clear a cells simulation texture block
 */
void SimulationCell::clear()
{
	BW_GUARD;
	if ( simulationTarget_ )
		simulationTarget_->clear();
}

BW_END_NAMESPACE

// water_simulation.cpp
