#include "pch.hpp"

#include "cstdmf/guard.hpp"
#include "cstdmf/string_builder.hpp"

#include "chunk.hpp"
#include "chunk_umbra.hpp"
#include "chunk_manager.hpp"
#include "chunk_space.hpp"
#include "umbra_draw_item.hpp"
#include "speedtree/speedtree_renderer.hpp"
#include "moo/dynamic_vertex_buffer.hpp"
#include "moo/dynamic_index_buffer.hpp"


#if UMBRA_ENABLE

// The library to use, to link with the debug version of umbra change it to umbraobXX_d.lib,
// that will also need the debug dll of umbra.
#ifdef _WIN64
#pragma comment(lib, "umbraob64.lib")
#else
#pragma comment(lib, "umbraob32.lib")
#endif

#include "cstdmf/slot_tracker.hpp"
#include "moo/line_helper.hpp"

#include "moo/material.hpp"
#include "moo/visual.hpp"
#include "terrain/base_terrain_renderer.hpp"

#ifdef EDITOR_ENABLED
	#include "appmgr/options.hpp"
#endif

#include <ob/umbraCommander.hpp>
#include <ob/umbraObject.hpp>
#include <ob/umbraLibrary.hpp>

#include "umbra_proxies.hpp"

#include "resmgr/datasection.hpp"

#include "cstdmf/log_msg.hpp"


DECLARE_DEBUG_COMPONENT2( "Chunk", 0 );


BW_BEGIN_NAMESPACE

MEMTRACKER_DECLARE( ChunkUmbra, "ChunkUmbra", 0 );

// -----------------------------------------------------------------------------
// Section: ChunkCommander definition
// -----------------------------------------------------------------------------

/*
 *	This class implements the Umbra commander interface.
 *	The commander is the callback framework from the umbra scene
 *	traversal.
 */
class ChunkCommander : public Umbra::OB::Commander
{
public:
	ChunkCommander( ChunkUmbra& controller, ChunkUmbraGraphicsServices* );
	~ChunkCommander();

	virtual void terrainOverride( SmartPointer< BWBaseFunctor0 > pTerrainOverride );
	virtual void command	(Command c);
	void repeat( Moo::DrawContext& drawContext );
	virtual void clearCachedItems();

protected:
	static const uint32 NUM_QUERY_TEST_BOXES = 10;

	virtual void flush(bool forceFlush = false);
	virtual void enableDepthTestState();
	virtual void disableDepthTestState();

	Chunk*						pLastChunk_;
	SmartPointer< BWBaseFunctor0 > pTerrainOverride_;
	BW::vector< UmbraDrawItem* >	cachedItems_;
	bool						depthStateEnabled_;
	ChunkUmbraGraphicsServices*		pGraphicsServices_;

	typedef BW::vector< UmbraDrawItem* > UmbraDrawItems;
    UmbraDrawItems	drawBatch_;
	UmbraDrawItems	depthItems_;
	ChunkUmbra&			controller_;
	Moo::IndexBuffer	queryTestIndexBuffer_;
public:
	// set explicitly before handling FLUSH_OCCLUDERS command
	Moo::DrawContext*	remoteDrawContext_;
};

// -----------------------------------------------------------------------------
// Section: ChunkUmbraSystemServices definition
// -----------------------------------------------------------------------------

class ChunkUmbraSystemServices : public Umbra::OB::LibraryDefs::SystemServices
{
public:
	virtual void error(const char* message);
	virtual void* allocateMemory(size_t bytes);
    virtual void releaseMemory(void* mem);
};

// -----------------------------------------------------------------------------
// Section: ChunkUmbraGraphicsServices definition
// -----------------------------------------------------------------------------

class ChunkUmbraGraphicsServices : public Umbra::OB::LibraryDefs::GraphicsServices
		{
public:
	virtual void* allocateQueryObject();
	virtual void  releaseQueryObject(void* query);
};

// -----------------------------------------------------------------------------
// Section: ChunkUmbra
// -----------------------------------------------------------------------------

/*
 *	The constructor inits umbra and creates our commander
 */
ChunkUmbra::ChunkUmbra( DataSectionPtr configSection ):
	occlusionCulling_( true ),
	umbraEnabled_( true ),
	flushTrees_( false ),
	flushBatchedGeometry_ ( false ),
	wireFrameTerrain_( false ),
	latentQueries_( true ),
	castShadowCullingEnabled_( false ),
	drawShadowBoxes_(false),
	cullMask_( SCENE_OBJECT | SHADOW_OBJECT ),
	distanceEnabled_( true ),
	shadowDistanceVisible_( 0.0f ),
	shadowDistanceDelta_( 150.0f )
#if ENABLE_WATCHERS
	,umbraWatcherEnabled_(true)
#endif
{
	BW_GUARD;

	pSystemServices_ = new ChunkUmbraSystemServices();
	pGraphicsServices_ = new ChunkUmbraGraphicsServices();

	Umbra::OB::Library::setEvaluationKey("xxxxxxxxxxxxxxxxxxxx");
	Umbra::OB::Library::init(Umbra::OB::Library::COLUMN_MAJOR, pSystemServices_);

	pCommander_ = new ChunkCommander( *this, pGraphicsServices_ );

	bool enableUmbra = 
#ifdef EDITOR_ENABLED
		(Options::getOptionInt( "render/useUmbra", 1 ) != 0);
#else
		configSection && configSection->readBool( "renderer/enableUmbra", true );
#endif

	this->initControls();

	if (LogMsg::automatedTest())
	{
		char log[80];
		bw_snprintf( log, sizeof( log ), "Umbra: %s",
			enableUmbra ? "On" : "Off" );
		LogMsg::logToFile( log );
	}

	this->umbraEnabled( enableUmbra );
}

/**
 *	The destructor uninits umbra and destroys our umbra related objects.
 */
ChunkUmbra::~ChunkUmbra()
{
	BW_GUARD;
	
	UmbraModelProxy::invalidateAll();
	UmbraObjectProxy::invalidateAll();

	bw_safe_delete(pCommander_);

	Umbra::OB::Library::exit();

	bw_safe_delete(pSystemServices_);
	bw_safe_delete(pGraphicsServices_);
}

/**
 *	Return the commander instance.
 */
Umbra::OB::Commander* ChunkUmbra::pCommander()
{
	return pCommander_;
}

void ChunkUmbra::terrainOverride( SmartPointer< BWBaseFunctor0 > pTerrainOverride )
{
	BW_GUARD;
	pCommander_->terrainOverride( pTerrainOverride );
}


/*
 *	Repeat the drawing calls from the last query
 */
void ChunkUmbra::repeat( Moo::DrawContext& drawContext )
{
	BW_GUARD;
	pCommander_->repeat( drawContext );
}

/*
 *	Tick method, needs to be called once per frame
 */
void ChunkUmbra::tick()
{
	BW_GUARD_PROFILER( UMBRA_tick );
	Umbra::OB::Library::resetStatistics();
	pCommander_->clearCachedItems();

	if (!updateItems_.empty())
	{
		BW::set<UmbraDrawItem*> tempUpdatableItems;
		tempUpdatableItems.swap( updateItems_ );
		
		while (!tempUpdatableItems.empty())
		{
			BW::set<UmbraDrawItem*>::iterator pItemIt = tempUpdatableItems.begin();
			UmbraDrawItem* pItem = *pItemIt;
			tempUpdatableItems.erase( pItem );

			pItem->update();
		}
	}
}


void ChunkUmbra::clearUpdateItems()
{
	updateItems_.clear();
}


void ChunkUmbra::addUpdateItem(UmbraDrawItem* pItem)
{
	updateItems_.insert( pItem );
}


void ChunkUmbra::delUpdateItem( UmbraDrawItem* pItem )
{
	BW::set<UmbraDrawItem*>::iterator diIter = updateItems_.find( pItem );
	if (diIter != updateItems_.end())
	{
		updateItems_.erase( diIter );
	}
}

void ChunkUmbra::drawContext( Moo::DrawContext* drawContext )
{
	pCommander_->remoteDrawContext_ = drawContext;
}

// -----------------------------------------------------------------------------
// Section: ChunkCommander
// -----------------------------------------------------------------------------

ChunkCommander::ChunkCommander( ChunkUmbra& controller,
							   ChunkUmbraGraphicsServices* pGraphicsServices ):
	controller_( controller ),
	remoteDrawContext_( NULL ),
	pLastChunk_(NULL),
	depthStateEnabled_(false),
	pGraphicsServices_(pGraphicsServices),
	Umbra::OB::Commander(pGraphicsServices)
{
	// Create a static index buffer for query test shapes
	HRESULT res = queryTestIndexBuffer_.create( NUM_QUERY_TEST_BOXES * 3 * 12, D3DFMT_INDEX16,
		D3DUSAGE_WRITEONLY, D3DPOOL_MANAGED );

	if ( res != D3D_OK )
	{
		CRITICAL_MSG( "Could not create index buffer for occlusion test shapes" );
		return;
	}

	Moo::IndicesReference ref = queryTestIndexBuffer_.lock();
	if (!ref.valid())
	{
		ERROR_MSG( "Could not lock the index buffer for occlusion test shapes" );
		return;
	}

	static const uint16 indices[36] =
	{
		0, 3, 1,
		0, 2, 3, 
		6, 5, 7,
		6, 4, 5,
		2, 4, 6,
		2, 0, 4,
		1, 7, 5,
		1, 3, 7,
		6, 7, 2,
		2, 7, 3,
		0, 1, 4,
		4, 1, 5
	};

	uint offset = 0;
	uint icount = 0;

	for ( uint j = 0; j < NUM_QUERY_TEST_BOXES; j++ )
	{
		for ( uint i = 0; i < 36; i++ )
		{
			ref.set( icount++, indices[i] + offset );
		}

		offset += 8;
	}

	queryTestIndexBuffer_.unlock();
}

ChunkCommander::~ChunkCommander()
{
	queryTestIndexBuffer_.release();

}


void ChunkCommander::terrainOverride( SmartPointer< BWBaseFunctor0 > pTerrainOverride )
{
	pTerrainOverride_ = pTerrainOverride;
}


/**
 * Redraws the scene, used for wireframe mode
 */
void ChunkCommander::repeat( Moo::DrawContext& drawContext )
{
	BW_GUARD;
	Moo::rc().push();
	Chunk* pLastChunk = NULL;
	size_t count = cachedItems_.size();
	for (size_t i = 0; i < count; i++)
	{
		UmbraDrawItem* pItem = cachedItems_[i];
		pLastChunk = pItem->draw( drawContext, pLastChunk);
	}
	Moo::rc().pop();
	flush();
	Chunk::nextMark();
	cachedItems_.clear();
}


/**
 * Clear the cached items, as they are only valid for one frame
 */
void ChunkCommander::clearCachedItems()
{
	cachedItems_.clear();
}


/**
 * Flush delayed rendering commands for occluders
 */
void ChunkCommander::flush(bool forceFlush)
{
	BW_GUARD;
	if (remoteDrawContext_)
	{
		remoteDrawContext_->end( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
		remoteDrawContext_->flush(  Moo::DrawContext::OPAQUE_CHANNEL_MASK );
		remoteDrawContext_->begin( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
	}

#if SPEEDTREE_SUPPORT
	if (controller_.flushTrees())
	{
		speedtree::SpeedTreeRenderer::flush();
	}
#endif

	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
	if (ChunkManager::instance().cameraSpace())
	{
		Moo::LightContainerPtr pRCLC = Moo::rc().lightContainer();
		Moo::LightContainerPtr pRCSLC = Moo::rc().specularLightContainer();
		static Moo::LightContainerPtr pLC = new Moo::LightContainer;
		static Moo::LightContainerPtr pSLC = new Moo::LightContainer;

		Moo::rc().lightContainer( pSpace->lights() );

		static DogWatch s_drawTerrain("Terrain draw");

		Moo::rc().pushRenderState( D3DRS_FILLMODE );

		Moo::rc().setRenderState( D3DRS_FILLMODE,
			( controller_.wireFrameTerrain() ) ?
				D3DFILL_WIREFRAME : D3DFILL_SOLID );

		s_drawTerrain.start();
		if (pTerrainOverride_)
		{
			(*pTerrainOverride_)();
		}
		else
		{
			Terrain::BaseTerrainRenderer::instance()->drawAll( 
				Moo::RENDERING_PASS_COLOR );
		}
		s_drawTerrain.stop();

		Moo::rc().popRenderState();

		pLC->directionals().clear();
		pLC->ambientColour( ChunkManager::instance().cameraSpace()->ambientLight() );
		if (ChunkManager::instance().cameraSpace()->sunLight())
			pLC->addDirectional( ChunkManager::instance().cameraSpace()->sunLight() );
		pSLC->directionals().clear();
		pSLC->ambientColour( ChunkManager::instance().cameraSpace()->ambientLight() );
		if (ChunkManager::instance().cameraSpace()->sunLight())
			pSLC->addDirectional( ChunkManager::instance().cameraSpace()->sunLight() );

		Moo::rc().lightContainer( pLC );
		Moo::rc().specularLightContainer( pSLC );

		Moo::rc().lightContainer( pRCLC );
		Moo::rc().specularLightContainer( pRCSLC );

		pLastChunk_ = NULL;
	}
}


/**
 * 
 */
void ChunkCommander::enableDepthTestState()
{
	BW_GUARD;

	DX::Device* dev = Moo::rc().device();

	Moo::rc().pushRenderState( D3DRS_CULLMODE );
	Moo::rc().pushRenderState( D3DRS_STENCILENABLE );
	Moo::rc().pushRenderState( D3DRS_ALPHATESTENABLE );
	Moo::rc().pushRenderState( D3DRS_ZENABLE );
	Moo::rc().pushRenderState( D3DRS_ZWRITEENABLE );

	//-- Note: should be in sync with the MRT buffers count.
	Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE  );
	Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE1 );
	Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE2 );
	Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE3 );
	
	Moo::rc().setWriteMask( 0, 0 );
	Moo::rc().setWriteMask( 1, 0 );
	Moo::rc().setWriteMask( 2, 0 );
	Moo::rc().setWriteMask( 3, 0 );

	// Set up state and draw.
	Moo::rc().setFVF( Moo::VertexXYZ::fvf() );
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setPixelShader( NULL );
	
	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, D3DZB_FALSE );
	Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, D3DZB_FALSE );
	Moo::rc().setRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	Moo::rc().setRenderState( D3DRS_STENCILENABLE, FALSE );	
	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE  );

	dev->SetTransform( D3DTS_WORLD, &Matrix::identity );
	dev->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
	dev->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
}

/**
 *
 */
void ChunkCommander::disableDepthTestState()
{
	BW_GUARD;

	// restore state
	Moo::rc().popRenderState();
	Moo::rc().popRenderState();
	Moo::rc().popRenderState();
	Moo::rc().popRenderState();
	Moo::rc().popRenderState();
	Moo::rc().popRenderState();
	Moo::rc().popRenderState();
	Moo::rc().popRenderState();
	Moo::rc().popRenderState();
}

/**
 * This method is for debugging purposes only..
 * It flushes the rendering queue.
 */

static void sync()
{
	BW_GUARD;
	IDirect3DQuery9* pEventQuery;
	Moo::rc().device()->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);

	// Add an end marker to the command buffer queue.
	pEventQuery->Issue(D3DISSUE_END);

	// Force the driver to execute the commands from the command buffer.
	// Empty the command buffer and wait until the GPU is idle.
	while(S_FALSE == pEventQuery->GetData( NULL, 0, D3DGETDATA_FLUSH )
		&& Moo::rc().device()->TestCooperativeLevel() != D3DERR_DEVICELOST)
		;

	pEventQuery->Release();
}

void setDepthRange(float n, float f)
{
	BW_GUARD;
	DX::Viewport vp;
	Moo::rc().getViewport(&vp);
	vp.MinZ = n;
	vp.MaxZ = f;
	Moo::rc().setViewport(&vp);
}


static bool colorPass = false;


/**
 *	This method implements our callback from the umbra scene traversal
 */
void ChunkCommander::command(Command c)
{
	BW_GUARD_PROFILER( ChunkCommander_command );
	static DogWatch s_commander( "Commander callback" );
	ScopedDogWatch commanderDW( s_commander );

	switch (c)
	{
		// Begin traversal
        case RESOLVE_BEGIN:
		{
			static DogWatch s_commander( "Resolve begin" );
			ScopedDogWatch dw( s_commander );

			pLastChunk_ = NULL;
			
			cachedItems_.clear();

			Moo::rc().push();
			break;
		}

		// We have finished traversal
        case RESOLVE_END:
		{
			static DogWatch s_commander( "Resolve end" );
			ScopedDogWatch dw( s_commander );

			// force flush all batched objects.
			flush(true);

			Moo::rc().pop();
			break;
		}
	
        case ISSUE_OCCLUSION_QUERIES:
		{
			BW_GUARD_PROFILER( ChunkCommander_occlusionQuery );
			static DogWatch s_renderTestDepth( "IssueOcclusionQuery" );
			ScopedDogWatch dw( s_renderTestDepth );


			// Enable depth test state.
			enableDepthTestState();

			// Get query count.
			int dataCount = getDataCount();

			for (int i = 0; i < dataCount; i++)
			{
                // Get query data for the current query.
                const OcclusionQueryData* qd = getOcclusionQueryData(i);

                // Issue occlusion query begin.
                Moo::OcclusionQuery* pOcclusionQuery = (Moo::OcclusionQuery*)qd->getQueryObject();
				MF_ASSERT( pOcclusionQuery );

                if (pOcclusionQuery)
				{
                    Moo::rc().beginQuery( pOcclusionQuery );
				}

                // Render query test shape.
				DX::Device* dev = Moo::rc().device();

				uint32 vertexBase = 0;
				typedef Moo::VertexXYZ VertexType;
				int vertexSize = sizeof(VertexType);

				Moo::DynamicVertexBuffer& vb =
					Moo::DynamicVertexBuffer::instance( vertexSize );

				bool locked = vb.lockAndLoad( 
					reinterpret_cast<const VertexType*>(qd->getVertices()), 
					qd->getVertexCount(), vertexBase );

				MF_ASSERT( qd->getTriangleCount() <= ( NUM_QUERY_TEST_BOXES * 12 ) );

				queryTestIndexBuffer_.set();
				vb.set();

				Moo::rc().drawIndexedPrimitive( 	D3DPT_TRIANGLELIST,  
					vertexBase,  
					0,  
					qd->getVertexCount(),   
					0, 
					qd->getTriangleCount() 
					); 

					// Issue occlusion query end.
					// DEBUG_MSG( "Query End: %x\n", pOcclusionQuery );
					Moo::rc().endQuery( pOcclusionQuery );
			}

			disableDepthTestState();

			break;
		}

        case GET_OCCLUSION_QUERY_RESULTS:
		{
			BW_GUARD_PROFILER( ChunkCommander_occlusionStall );
			static DogWatch s_boxQuery( "GetOcclusionResult" );

			ScopedDogWatch dw( s_boxQuery );

            // Number of query results to read.
            int count = getDataCount();

            for (int i = 0; i < count; i++)
			{
                // Get OcclusionQueryResultData for this occlusion query
                OcclusionQueryResultData* queryData = getOcclusionQueryResultData(i);

                int result = 0;
                Moo::OcclusionQuery* pOcclusionQuery = (Moo::OcclusionQuery*)queryData->getQueryObject();

                // Read the query results back.
                bool available = Moo::rc().getQueryResult(result, pOcclusionQuery, queryData->getWaitForResult());
				//DEBUG_MSG( "Query Get: %x, %d\n", pOcclusionQuery, available );

                if (!available)
				{
                    SwitchToThread();
                    break;
				}

                queryData->setResult(result);
            }

			break;
		}

        case OBJECTS_VISIBLE:
		{
			static DogWatch s_commander( "Object Visible" );
			ScopedDogWatch dw( s_commander );

			int count = getDataCount();

            for (int i = 0; i < count; i++)
            {
                const ObjectData* objectData = getObjectData(i);
                UmbraDrawItem* pItem = (UmbraDrawItem*)objectData->getUserPointer();

				drawBatch_.push_back(pItem);

				// This is only needed for wireframe mode
				cachedItems_.push_back(pItem);
            }
			break;
        }

        case FLUSH_OCCLUDERS:
        {
			static DogWatch s_commander( "Flush Occluders" );
			ScopedDogWatch dw( s_commander );
			// usually we pass draw context by referece, but this is a third party lib callback
			// have to explicitly set remote draw context by pointer and reset it to NULL afterwards
			MF_ASSERT( remoteDrawContext_ );
            
			// Draw all batched items.
	        for (UmbraDrawItems::iterator di = drawBatch_.begin();
				di != drawBatch_.end();
				++di)
	        {
                UmbraDrawItem* pItem = *di;
	            pLastChunk_ = pItem->draw( *remoteDrawContext_, pLastChunk_ );
            }

            drawBatch_.clear();
            flush();

			break;
        }

		// Umbra wants us to draw a 2d debug line
		case DRAW_LINE_2D:
		{

			static Vector4 begin(0,0,0,1);
			static Vector4 end(0,0,0,1);
			static Moo::Colour colour;

			// Get a linehelper from umbra
			getLine2D((Umbra::Vector2&)begin, (Umbra::Vector2&)end, (Umbra::Vector4&)colour);

			// Add line to the linehelper
			LineHelper::instance().drawLineScreenSpace( begin, end, colour );
			break;
		}

		// Umbra wants us to draw a 3d debug line
		case DRAW_LINE_3D:
		{

			Vector3 begin;
			Vector3 end;
			Moo::Colour colour;

			// Get a linehelper from umbra
			getLine3D((Umbra::Vector3&)begin, (Umbra::Vector3&)end, (Umbra::Vector4&)colour);

			// Add line to the linehelper
			LineHelper::instance().drawLine( begin, end, colour );
			break;
		}

		// Unhandled commands
		case VIEW_PARAMETERS_CHANGED:
		case TEXT_MESSAGE:
		case COMMAND_MAX:
		{
			break;
		}
	};
}

// -----------------------------------------------------------------------------
// Section: ChunkUmbraSystemServices
// -----------------------------------------------------------------------------
/*
 *	This method outputs a umbra error
 */

void ChunkUmbraSystemServices::error(const char* message)
{
	BW_GUARD;
	CRITICAL_MSG( "%s\n", message );
}

void* ChunkUmbraSystemServices::allocateMemory(size_t bytes)
{
	BW_GUARD;
	MEMTRACKER_SCOPED( ChunkUmbra );
	void* mem = bw_malloc(bytes);
	if (!mem)
	{
		CRITICAL_MSG( "Out of memory. Exiting.\n" );
	}
	return mem;
}

void ChunkUmbraSystemServices::releaseMemory(void* mem)
{
	BW_GUARD;
	bw_free(mem);
}

// -----------------------------------------------------------------------------
// Section: ChunkUmbraGraphicsServices
// -----------------------------------------------------------------------------

void* ChunkUmbraGraphicsServices::allocateQueryObject()
{
	BW_GUARD;
    Moo::OcclusionQuery* query = Moo::rc().createOcclusionQuery();
	return query;
}

void ChunkUmbraGraphicsServices::releaseQueryObject(void* query)
{
	BW_GUARD;
    Moo::rc().destroyOcclusionQuery( (Moo::OcclusionQuery*)query );
}

// -----------------------------------------------------------------------------
// Section: UmbraHelper
// -----------------------------------------------------------------------------

/*
 *	Helper class to contain one umbra statistic
 */
class ChunkUmbra::Statistic
{
public:
	void set( Umbra::OB::LibraryDefs::Statistic statistic )
	{
		statistic_ = statistic;
	}
	float get() const
	{
		return Umbra::OB::Library::getStatistic( statistic_ );
	}
private:
	Umbra::OB::LibraryDefs::Statistic statistic_;
};

/*
 *	This method inits the umbra helper
 */
void ChunkUmbra::initControls()
{
	BW_GUARD;

	static Statistic stats[Umbra::OB::LibraryDefs::STAT_MAX];

	// Register our debug watchers
	MF_WATCH( "Render/Umbra/occlusionCulling",	*this, MF_ACCESSORS( bool, ChunkUmbra, occlusionCulling ),
		"Enable/disable umbra occlusion culling, this still uses umbra for frustum culling" );
	MF_WATCH( "Render/Umbra/enabled",		   *this, MF_ACCESSORS( bool, ChunkUmbra, umbraEnabled ),
		"Enable/disable umbra, this causes the rendering to bypass umbra and use the BigWorld scene traversal" );
	MF_WATCH( "Visibility/Umbra",				*this, MF_ACCESSORS( bool, ChunkUmbra, umbraWatcherEnabled ),
		"Enable/disable umbra." );
	MF_WATCH( "Render/Umbra/drawObjectBounds",	*this, MF_ACCESSORS( bool, ChunkUmbra, drawObjectBounds ),
		"Draw the umbra object bounds" );
	MF_WATCH( "Render/Umbra/drawVoxels",		*this, MF_ACCESSORS( bool, ChunkUmbra, drawVoxels ),
		"Draw the umbra voxels" );
	MF_WATCH( "Render/Umbra/drawQueries",		*this, MF_ACCESSORS( bool, ChunkUmbra, drawQueries ),
		"Draw the umbra occlusion queries (hardware mode only)" );
	MF_WATCH( "Render/Umbra/flushTrees",		*this, MF_ACCESSORS( bool, ChunkUmbra, flushTrees ),
		"Flush speedtrees as part of umbra, if this is set to false, all speedtrees are flushed after "
		"the occlusion queries have been issued. "
		"Which means that speedtrees do not act as occluders" );
	MF_WATCH( "Render/Umbra/flushBatchedGeometry",	*this, MF_ACCESSORS( bool, ChunkUmbra, flushBatchedGeometry ),
		"Flush batched geometry as part of umbra, if this is set to false, all batched geometry are flushed after "
		"the occlusion queries have been issued. "
		"Which means that batched geometry do not act as occluders" );
	MF_WATCH( "Render/Umbra/castShadowCullingEnabled", *this, MF_ACCESSORS( bool, ChunkUmbra, castShadowCullingEnabled ),
		"Umbra enabled for cast shadows." );
	MF_WATCH( "Render/Umbra/drawShadowBoxes", *this, MF_ACCESSORS( bool, ChunkUmbra, drawShadowBoxes ),
		"Draw bounding boxes for shadow casters." );
	MF_WATCH( "Render/Umbra/distanceEnabled", *this, MF_ACCESSORS( bool, ChunkUmbra, distanceEnabled ), 
		"Using distance information for culling objects ( use shadow )." );
	MF_WATCH( "Render/Umbra/shadowDistanceDelta", *this, MF_ACCESSORS( float, ChunkUmbra, shadowDistanceDelta ), 
		"Added to the value shadowDistance for a smooth transition." );

	// Set up the watchers for the statistics
	for (uint32 i = 0; i < (uint32)Umbra::OB::LibraryDefs::STAT_MAX; i++)
	{
		BW::string statName(Umbra::OB::Library::getStatisticName( (Umbra::OB::LibraryDefs::Statistic)i ));
		BW::string statNameBegin = statName;

		stats[i].set( (Umbra::OB::LibraryDefs::Statistic)i );

		MF_WATCH( (BW::string( "Render/Umbra/Statistics/" ) + statNameBegin + "/" + statName).c_str(), 
			stats[i], &ChunkUmbra::Statistic::get );
	}
}

/*
 *	macro to help implement the umbra flag switches
 */
#define IMPLEMENT_CHUNK_UMBRA_FLAG( method, flag )\
bool ChunkUmbra::method() const\
{\
	return (Umbra::OB::Library::getFlags(Umbra::OB::LibraryDefs::LINEDRAW) & Umbra::OB::LibraryDefs::flag) != 0;\
}\
void ChunkUmbra::method(bool b) \
{ \
	if (b)\
		Umbra::OB::Library::setFlags(Umbra::OB::LibraryDefs::LINEDRAW, Umbra::OB::LibraryDefs::flag); \
	else\
		Umbra::OB::Library::clearFlags(Umbra::OB::LibraryDefs::LINEDRAW, Umbra::OB::LibraryDefs::flag); \
}

IMPLEMENT_CHUNK_UMBRA_FLAG( drawObjectBounds, LINE_OBJECT_BOUNDS )
IMPLEMENT_CHUNK_UMBRA_FLAG( drawVoxels, LINE_VOXELS )
IMPLEMENT_CHUNK_UMBRA_FLAG( drawQueries, LINE_OCCLUSION_QUERIES )

#undef IMPLEMENT_CHUNK_UMBRA_FLAG

BW_END_NAMESPACE

#endif
