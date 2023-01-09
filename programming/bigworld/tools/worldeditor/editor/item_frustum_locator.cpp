#include "pch.hpp"
#include "worldeditor/editor/item_frustum_locator.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/misc/selection_filter.hpp"
#include "appmgr/options.hpp"
#include "chunk/chunk_umbra.hpp"
#include "chunk/chunk_item.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_vlo.hpp"
#include "gizmo/item_view.hpp"
#include "physics2/worldtri.hpp"
#include "moo/geometrics.hpp"
#include "romp/water.hpp"
#include "input/input.hpp"
#include "resmgr/auto_config.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/bw_unordered_set.hpp"
#include "moo/draw_context.hpp"


DECLARE_DEBUG_COMPONENT2( "Tool", 0 )

//#define DRAW_DEBUG_GEOMETRY


#ifdef DRAW_DEBUG_GEOMETRY
#include "moo/debug_geometry.hpp"
#endif


// Define this macro to save the marquee render target texture to disk for
// debugging.
//#define DEBUG_RENDER_TARGET 1

BW_BEGIN_NAMESPACE

static Moo::RenderTargetPtr s_renderTarget;

static Moo::RenderTargetPtr renderTarget()
{
	BW_GUARD;

	if( s_renderTarget == NULL )
	{
		s_renderTarget = new Moo::RenderTarget( "ChunkFrustumLocator" );
		s_renderTarget->create(
				Options::getOptionInt( "marqueeSelect/bufferWidth", 2048 ),
				Options::getOptionInt( "marqueeSelect/bufferHeight", 1024 ),
				false );
	}
	return s_renderTarget;
}

// -----------------------------------------------------------------------------
// Section: ChunkItemFrustumLocatorRevealer
// -----------------------------------------------------------------------------

class ChunkItemFrustumLocatorRevealer : public ChunkItemRevealer
{
	Py_Header( ChunkItemFrustumLocatorRevealer, ChunkItemRevealer )
public:
	ChunkItemFrustumLocatorRevealer( SmartPointer<ChunkItemFrustumLocator> pLoc,
			PyTypeObject * pType = &s_type_ ) :
		ChunkItemRevealer( pType ),
		pLoc_( pLoc )
	{
	}

	// should really be a C++ virtual method...
	//  with attribute in base class
	PY_RO_ATTRIBUTE_DECLARE( pLoc_->items.size(), size );

private:
	virtual void reveal( BW::vector< ChunkItemPtr > & items )
	{
		BW_GUARD;

		items.clear();

		BW::vector<ChunkItemPtr>::iterator i = pLoc_->items.begin();
		for (; i != pLoc_->items.end(); ++i)
			items.push_back( *i );
	}

	SmartPointer<ChunkItemFrustumLocator>	pLoc_;
};

PY_TYPEOBJECT( ChunkItemFrustumLocatorRevealer )

PY_BEGIN_METHODS( ChunkItemFrustumLocatorRevealer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ChunkItemFrustumLocatorRevealer )
	PY_ATTRIBUTE( size )
PY_END_ATTRIBUTES()


// -----------------------------------------------------------------------------
// Section: ChunkItemFrustumLocator
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( ChunkItemFrustumLocator )

PY_BEGIN_METHODS( ChunkItemFrustumLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ChunkItemFrustumLocator )
	PY_ATTRIBUTE( revealer )
	PY_ATTRIBUTE( subLocator )
	PY_ATTRIBUTE( enabled )
PY_END_ATTRIBUTES()

PY_FACTORY( ChunkItemFrustumLocator, Locator )

PY_SCRIPT_CONVERTERS( ChunkItemFrustumLocator );

ChunkItemFrustumLocator::ChunkItemFrustumLocator( ToolLocatorPtr pSub, PyTypeObject * pType ) :
	ToolLocator( pType ),
	subLocator_( pSub ),
	enabled_( true ),
	oldDrawReflection_( false )
{
	BW_GUARD;

	startPosition_.x = -1;
	startPosition_.y = -1;
	currentPosition_.x = -1;
	currentPosition_.y = -1;
	
	// push_back is fairly expensive if it doesn't have
	// capacity for the first few element, so reserve some
	// space in advance.
	items.reserve( 1024 );
}

ChunkItemFrustumLocator::~ChunkItemFrustumLocator()
{
}


/*static*/ void ChunkItemFrustumLocator::fini()
{
	BW_GUARD;
	s_renderTarget = NULL;
}

void ChunkItemFrustumLocator::enterSelectionMode()
{
	BW_GUARD;

	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA |
		D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED );
	renderTarget()->push();
	Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1, 0 );

	// set the default texture factor to 0, so if no objects are being drawn,
	// the pixels are set to 0 (NULL)
	Moo::rc().setRenderState( D3DRS_TEXTUREFACTOR, (DWORD)0x00000000 );

	WorldManager::instance().drawSelection( true );

	// Additional states that need changing
	oldDrawReflection_ = Waters::drawReflection();
	Waters::drawReflection( false );

	// Tick to make sure everything updates (i.e. water reflection mirrors)
	ChunkManager::instance().tick( 0.f );
}

void ChunkItemFrustumLocator::leaveSelectionMode()
{
	BW_GUARD;

	WorldManager::instance().drawSelection( false );

	// Restoring additional states changed
	Waters::drawReflection( oldDrawReflection_ );

	renderTarget()->pop();
	
#ifdef DEBUG_RENDER_TARGET
	static int count = 0;
	static char buf[256];
	bw_snprintf( buf, sizeof(buf), "marquee%04d.bmp", count++ );
	BW::wstring screenShotName = bw_utf8tow( buf );
	if(D3DXSaveTextureToFile( screenShotName.c_str(), D3DXIFF_BMP, renderTarget()->pTexture(), NULL ) != D3D_OK)
	{
		ERROR_MSG("Unable to write selection buffer to file: %s\n", buf);
	}
#endif // DEBUG_RENDER_TARGET

	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
		D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED );
}

void ChunkItemFrustumLocator::calculatePosition( const Vector3& worldRay, Tool& tool )
{
	BW_GUARD;

	// first call our sublocator to set the matrix
	if (subLocator_)
	{
		subLocator_->calculatePosition( worldRay, tool );
		transform_ = subLocator_->transform();
	}
	else
	{
		transform_ = Matrix::identity;
	}

	// This is for debugging, so we can hold the right mouse button down and
	// inspect the generated triangles that get swept through the collision
	// scene
	if (InputDevices::isKeyDown( KeyCode::KEY_RIGHTMOUSE ) )
		return;

	// now find the chunk item
	if (enabled_)
	{
		if (startPosition_.x == -1)
		{
			startPosition_ = WorldManager::instance().currentCursorPosition();
		}

		POINT pt = WorldManager::instance().currentCursorPosition();

#ifndef DRAW_DEBUG_GEOMETRY
		// Don't do anything if the cursor hasn't moved
		if (pt.x == currentPosition_.x && pt.y == currentPosition_.y)
			return;
#endif

		currentPosition_ = pt;

		if (currentPosition_.x == startPosition_.x || currentPosition_.y == startPosition_.y)
			return;

		items.clear();

		enterSelectionMode();

		Moo::rc().beginScene();

		WorldManager::instance().renderChunksInUpdate();

		Moo::LightContainerPtr lc = new Moo::LightContainer;

		lc->addDirectional(
			ChunkManager::instance().cameraSpace()->sunLight() );
		lc->ambientColour(
			ChunkManager::instance().cameraSpace()->ambientLight() );

		Moo::rc().lightContainer( lc );

		#if UMBRA_ENABLE	
		if (!ChunkManager::instance().umbra()->umbraEnabled())
		{
			// With umbra, terrain gets rendered inside "renderChunks" via the
			// ChunkUmbra terrainOverride.
		#endif
			WorldManager::instance().renderTerrain();
		#if UMBRA_ENABLE	
		}
		#endif

		Moo::rc().endScene();

		leaveSelectionMode();
		
		DX::Texture* texture = (DX::Texture*)( renderTarget()->pTexture() );
		ComObjectWrap<DX::Surface> inmemSurface;
		ComObjectWrap<DX::Texture> inmemTexture;
		ComObjectWrap<DX::Surface> rtSurface;

		D3DSURFACE_DESC desc;
		D3DLOCKED_RECT lockRect;

		if( SUCCEEDED( texture->GetSurfaceLevel( 0, &rtSurface ) ) &&
			SUCCEEDED( rtSurface->GetDesc( &desc ) ) &&
			!!( inmemTexture = Moo::rc().createTexture( desc.Width, desc.Height, 0, 0, desc.Format, D3DPOOL_SYSTEMMEM,
				"texture/chunk item frustum" ) ) &&
			SUCCEEDED( inmemTexture->GetSurfaceLevel( 0, &inmemSurface ) ) &&
			SUCCEEDED( Moo::rc().device()->GetRenderTargetData( rtSurface.pComObject(), inmemSurface.pComObject() ) ) &&
			SUCCEEDED( inmemSurface->LockRect( &lockRect, NULL, 0 ) ) )
		{
			BW::unordered_set< ChunkItem * > fastItems;
			
			// This variable reduces processing time by a massive amount
			DWORD lastItemPixel = 0;
			// Look for objects
			EditorChunkItem::updateSelectionMark();
		
			uint32 minx = min( startPosition_.x, currentPosition_.x ) * renderTarget()->width() / uint32(Moo::rc().screenWidth());
			uint32 maxx = max( startPosition_.x, currentPosition_.x ) * renderTarget()->width() / uint32(Moo::rc().screenWidth());
			uint32 miny = min( startPosition_.y, currentPosition_.y ) * renderTarget()->height() / uint32(Moo::rc().screenHeight());
			uint32 maxy = max( startPosition_.y, currentPosition_.y ) * renderTarget()->height() / uint32(Moo::rc().screenHeight());

			for( uint32 y = miny; y <= maxy; ++y )
			{
				DWORD* pixel = (DWORD*)( (unsigned char*)lockRect.pBits + lockRect.Pitch * y ) + minx;
				DWORD* end = pixel + (maxx - minx + 1);
				for( ; pixel < end; ++pixel )
				{
					if( *pixel != 0 && *pixel != lastItemPixel )
					{
						lastItemPixel = *pixel;
						ChunkItem * item = WorldManager::instance().getDrawSelectionItem( *pixel );
						if (item)
						{
							// The pixel corresponds to an actual selectable chunk item.
							if (fastItems.find( item ) == fastItems.end() &&
								SelectionFilter::canSelect( item ))
							{
								if( item->edIsVLO() && !item->edCheckMark( EditorChunkItem::selectionMark() ) )
									continue;
								fastItems.insert( item );
								items.push_back( item );
							}
						}
					}
				}
			}
			inmemSurface->UnlockRect();
		}
	}
	else
	{
		items.clear();
	}
}


/**
 *	Get a revealer object for the chunkitem in this locator
 */
PyObject * ChunkItemFrustumLocator::pyGet_revealer()
{
	BW_GUARD;

	return new ChunkItemFrustumLocatorRevealer( this );
}

/**
 *	Python factory method
 */
PyObject * ChunkItemFrustumLocator::pyNew( PyObject * args )
{
	BW_GUARD;

	PyObject * pSubLoc = NULL;
	if (!PyArg_ParseTuple( args, "|O", &pSubLoc ) ||
		pSubLoc && !ToolLocator::Check( pSubLoc ))
	{
		PyErr_SetString( PyExc_TypeError, "ChunkItemFrustumLocator() "
			"expects an optional ToolLocator argument" );
		return NULL;
	}

	ToolLocatorPtr spSubLoc = static_cast<ToolLocator*>( pSubLoc );

	return new ChunkItemFrustumLocator( spSubLoc );
}

// -----------------------------------------------------------------------------
// Section: DragBoxView
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( DragBoxView )

PY_BEGIN_METHODS( DragBoxView )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( DragBoxView )
PY_END_ATTRIBUTES()

PY_FACTORY( DragBoxView, View )


DragBoxView::DragBoxView( ChunkItemFrustumLocatorPtr locator, Moo::Colour colour,
						 PyTypeObject * pType) :
	ToolView( pType ),
	locator_( locator ),
	colour_( colour )
{
}

DragBoxView::~DragBoxView()
{
}


/**
 *	Update the animations in this tool view.
 *	@param tool the tool.
 */
void DragBoxView::updateAnimations( const Tool& tool )
{
	BW_GUARD;
}


/**
 *	This method draws the appropriate item selected
 */
void DragBoxView::render( Moo::DrawContext& drawContext, const Tool& tool )
{
	int minx = min( locator_->startPosition_.x, locator_->currentPosition_.x );
	int maxx = max( locator_->startPosition_.x, locator_->currentPosition_.x );
	int miny = min( locator_->startPosition_.y, locator_->currentPosition_.y );
	int maxy = max( locator_->startPosition_.y, locator_->currentPosition_.y );


	Geometrics::drawRect(
		Vector2( (float) minx, (float) miny ),
		Vector2( (float) maxx, (float) maxy ),
		colour_ );
}

BW_END_NAMESPACE

