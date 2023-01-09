#include "pch.hpp"
#include "worldeditor/gui/controls/chunk_watch_control.hpp"
#include "worldeditor/gui/pages/chunk_watcher.hpp"
#include "worldeditor/misc/world_editor_camera.hpp"
#include "worldeditor/import/terrain_utils.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/project/space_map.hpp"
#include "appmgr/options.hpp"
#include "chunk/chunk_manager.hpp"
#include "controls/memdc.hpp"
#include "common/format.hpp"
#include "common/user_messages.hpp"
#include "resmgr/string_provider.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	// Chunk state colours:
	const COLORREF	CLR_UNLOADED				= RGB(255, 255, 255);
	const COLORREF	CLR_LOADED					= RGB(192, 192, 255);
	const COLORREF	CLR_DIRTY_NEEDS_SHADOW_CALC	= RGB(255, 192, 192);
	const COLORREF	CLR_DIRTY					= RGB(192, 255, 192);

	// Transparency of the state colours:
	const uint8		CLR_ALPHA					= 0x70; 

	// Colour to draw the unloadable state in:
	const COLORREF	CLR_CANNOT_UNLOAD			= RGB(255,   0,   0);

	// Colour of the grid lines:
	const COLORREF	CLR_GRID					= RGB(192, 192, 192);

	// Colour of the user arrow:
	const COLORREF	CLR_USER_POS				= RGB(  0,   0,   0);

	// Colour of the working chunk:
	const COLORREF	CLR_WORKING					= RGB(  0, 255,   0);

	// Colour of the frustum:
	const COLORREF	CLR_FRUSTUM					= RGB(  0,   0, 255);

	// Colour of the outside rectangle:
	const COLORREF	CLR_BORDER					= RGB(128, 128, 128);

	// Beyond the following grid size don't draw lines:
	const uint32	TOO_MANY_GRID_LINES			= 50;

	// The size of the user arrow:
	const uint32	USER_POS_SIZE				=  5;

	// The gap between the client edge and the actual drawing:
	const uint32	EDGE_GAP					=  4;

	// The thickness of the pen for the working chunk:
	const uint32	WORKING_PEN_THICKNESS		=  2;

	// The thickness of the pend for the unloadable cross:
	const uint32	UNLOADABLE_PEN_THICKNESS	=  1;

	// It takes a lot of time to convert the project mode's texture to a DIB
	// (because of the compression).  Hence we limit updates to the project
	// mode to be greater than the following number of seconds
	const float		PROJECT_UPDATE_DELTA		= 10.0f; 

	// Cap the update rate to the following (in seconds):
	const float		UPDATE_DELTA				=  0.25f; 

	// Epsilon for comparing the view matrix position between frames.  The
	// comparison can be a little fuzzy due to floating point error.
	const float		MATRICES_COMP_EPSILON		= 0.05f;

	// Frustum's whose clipping distance is below this threshold aren't drawn.
	// Mostly this happens in project and terrain import views.
	const float		SMALL_FRUSTUM				= 20.0f;

	/**
	 *	This converts RGB to pre-alpha blended ARGB.
	 *
	 *  @param rgb		The colour.
	 *  @param alpha	The alpha to apply.
	 *  @returns		The pre-alpha'd colour formed by multiplying the
	 *					r, g, b components by alpha and adding in the alpha
	 *					channel.
	 */
	DWORD RGBToARGB(DWORD rgb, uint8 alpha)
	{
		uint8 red    = ((int)GetRValue(rgb)*alpha)/255;
		uint8 green  = ((int)GetGValue(rgb)*alpha)/255;
		uint8 blue	 = ((int)GetBValue(rgb)*alpha)/255;
		return (alpha << 24) + (red << 16) + (green << 8) + blue;
	}


	/**
	 *	This compares to matrices and returns true if all of their elements
	 *	are withing epsilon of one another.
	 *
	 *  @param m1		Matrix 1.
	 *  @param m2		Matrix 2.
	 *	@param epsilon	The comparison epsilon.
	 *	@returns		True if the corresponding elements in the matrices are
	 *					within epsilon of one another.
	 */
	bool similar(Matrix const &m1, Matrix const &m2, float epsilon)
	{
		for (uint32 col = 0; col < 4; ++col)
		{
			for (uint row = 0; row < 4; ++row)
			{
				if (std::abs(m1(col, row) - m2(col, row)) > epsilon)
					return false;
			}
		}
		return true;
	}

	DogWatch s_chunkWatchPaint("chunk_watch_paint");
}


BEGIN_MESSAGE_MAP(ChunkWatchControl, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()


/**
 *	This is the ChunkWatchControl constructor.
 */
ChunkWatchControl::ChunkWatchControl():
	projectMark_((uint32)-1),
	drawPos_(Matrix::identity),
	lastUpdateTime_(0),
	lastUpdateProjectTime_(0),
	chunkSize_(0.f),
	minX_(0),
	minZ_(0),
	chunksWide_(0),
	chunksHigh_(0),
	drawOptions_(DRAW_ALL) 
{
}


/**
 *	This is the ChunkWatchControl destructor.
 */
ChunkWatchControl::~ChunkWatchControl()
{
}


/**
 *	This is called to create the ChunkWatchControl's window.
 *
 *  @param extents		The extents of the window.
 *  @param parent		The parent window.
 *  @return				TRUE if successfully created.
 */
BOOL ChunkWatchControl::Create(CRect const &extents, CWnd *parent)
{
	BW_GUARD;

    BOOL ok = 
		CreateEx
        (
            0,			// extended style
            AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW)), 
            _T("ChunkWatchControl"),
            WS_CHILD | WS_VISIBLE,
            extents.left,
            extents.top,
            extents.Width(),
            extents.Height(),
			(parent != NULL) ? parent->GetSafeHwnd() : 0,
            NULL,		// id
            0			// lparam of the window
        );
	return ok;
}


/**
 *	This is called when a new space is created.
 */
void ChunkWatchControl::onNewSpace
(
	float	chunkSize,
	int32	minX, 
	int32	minZ, 
	int32	maxX,
	int32	maxZ
)
{
	BW_GUARD;

	chunkSize_ = chunkSize;
	int32 width  = maxX - minX;
	int32 height = maxZ - minZ;
	redraw(true);
	projectDib_.clear();
	chunksDib_.resize(width, height, RGBToARGB(CLR_UNLOADED, CLR_ALPHA));
}


/**
 *	This is called when the status of a chunk is changed.
 *
 *  @param x			The x-coordinate of the changed chunk.
 *	@param z			The z-coordinate of the changed chunk.
 */
void ChunkWatchControl::onChangedChunk(int32 x, int32 z)
{
	BW_GUARD;

	int32 gx = x - minX_;
	int32 gz = z - minZ_;
	if (chunksDib_.contains(gx, gz))
	{
		ChunkWatcher const &cw = WorldManager::instance().chunkWatcher();
		DWORD clr;
		switch (cw.state(x, z))
		{
		case ChunkWatcher::UNLOADED:
			clr = RGBToARGB(CLR_UNLOADED, CLR_ALPHA);
			break;
		case ChunkWatcher::LOADED:
			clr = RGBToARGB(CLR_LOADED, CLR_ALPHA);
			break;
		case ChunkWatcher::DIRTY_NEEDS_SHADOW_CALC:
			clr = RGBToARGB(CLR_DIRTY_NEEDS_SHADOW_CALC, CLR_ALPHA);
			break;
		case ChunkWatcher::DIRTYSHADOW_CALCED:
			clr = RGBToARGB(CLR_DIRTY, CLR_ALPHA);
			break;
		default:
			return;
		}
		// Note we have to invert the y coordinate below because the image
		// is upside down and ::AlphaBlend does not support mirroring like
		// ::StretchBlt.
		chunksDib_.set(gx, chunksDib_.height() - gz - 1, clr);
		redraw();
	}
}


/**
 *	This is called when the control is repainted.
 */
/*afx_msg*/ void ChunkWatchControl::OnPaint()
{
	BW_GUARD;

	ScopedDogWatch dogWatchScope(s_chunkWatchPaint);

	// The extents of the control:
	CRect fullExtents;
	GetClientRect(fullExtents);

	// The drawing contexts:
	CPaintDC				paintDC(this);
	controls::MemDCScope	memDCScope(memDC_, paintDC, &fullExtents);

	memDC_.FillSolidRect(fullExtents, ::GetSysColor(COLOR_BTNFACE));

	// Calculate the drawing area:
	getDrawConstants();

	// Draw the details:
	if ((drawOptions_ & DRAW_PROJECTVIEW) != 0) drawProject   (memDC_);
	if ((drawOptions_ & DRAW_CHUNKS     ) != 0) drawChunks    (memDC_);
	if ((drawOptions_ & DRAW_UNLOADABLE ) != 0) drawUnloadable(memDC_);
	if ((drawOptions_ & DRAW_GRID       ) != 0) drawGrid      (memDC_);
	if ((drawOptions_ & DRAW_USERPOS    ) != 0) drawArrow     (memDC_);
	if ((drawOptions_ & DRAW_FRUSTUM    ) != 0) drawFrustum   (memDC_);

	// Draw the outsides:
	memDC_.Draw3dRect
	(
		extents_, // around the tiles itself
		CLR_BORDER,
		CLR_BORDER
	);
	memDC_.Draw3dRect
	(
		fullExtents, 
		::GetSysColor(COLOR_3DHILIGHT),
		::GetSysColor(COLOR_3DSHADOW )
	);
}


/**
 *	This is called when the window should erase its background.
 * 
 *  @param dc		The device context to erase with.
 *  @return			TRUE - the background is erased - which is not needed for
 *					this control.	
 */	
/*afx_msg*/ BOOL ChunkWatchControl::OnEraseBkgnd(CDC * /*dc*/)
{
	return TRUE;
}


/**
 *	This is called every frame.  We use this as an oportunity to RedrawWindow() 
 *  the control if the user has moved.
 */
/*afx_msg*/ void ChunkWatchControl::OnUpdateControl()
{
	BW_GUARD;

	// Don't update to frequently.
	if (lastUpdateTime_ != 0)
	{
		uint64 now = timestamp();
		if ((now - lastUpdateTime_)/(double)stampsPerSecond() < UPDATE_DELTA)
		{
			return;
		}
	}
	lastUpdateTime_ = timestamp();

	Matrix view = WorldEditorCamera::instance().currentCamera().view();
	view.invert();
	if 
	(
		!similar(view, drawPos_, MATRICES_COMP_EPSILON)
	)
	{
		RedrawWindow();
	}
}


/**
 *	This is called when the mouse button is released.
 *
 *  @param flags	The flags which shift-key status etc.
 *  @param point	The coordinates of the mouse.
 */
/*afx_msg*/ void ChunkWatchControl::OnLButtonUp(UINT /*flags*/, CPoint point)
{
	BW_GUARD;

	getDrawConstants();

    float x, z;
    if (screenToWorld(point, &x, &z))
    {    
		// Find the height at this point
		float fheight = TerrainUtils::heightAtPos(x, z, true);
		float y = fheight + Options::getOptionInt( "graphics/farclip", 500 )/10.0f;

		// Set the view matrix to the new world coords and preserve the 
		// current orientation:
		Matrix view = WorldEditorCamera::instance().currentCamera().view();
		view.invert();
		view.translation(Vector3(x, y, z));
		view.invert();
		WorldEditorCamera::instance().currentCamera().view(view);
	}
}


/**
 *	This is called when the mouse is moved.
 *
 *  @param flags	The flags which shift-key status etc.
 *  @param point	The coordinates of the mouse.
 */
/*afx_msg*/ void ChunkWatchControl::OnMouseMove(UINT /*flags*/, CPoint /*point*/)
{
	BW_GUARD;

	CWnd *parent = GetParent();
	if (parent != NULL)
	{
		parent->SendMessage(WM_CHUNK_WATCH_MOUSE_MOVE, (WPARAM)0, (LPARAM)0);
	}
}


/**
 *	This gets the colour of loaded chunks in the display.
 *
 *  @returns		The colour of loaded chunks in the display.
 */
/*static*/ COLORREF ChunkWatchControl::loadColour()
{
	return CLR_UNLOADED;
}


/**
 *	This gets the colour of unloaded chunks in the display.
 *
 *  @returns		The colour of unloaded chunks in the display.
 */
/*static*/ COLORREF ChunkWatchControl::unloadColour()
{
	return CLR_LOADED;
}


/**
 *	This gets the colour of dirty chunks in the display.
 *
 *  @returns		The colour of dirty chunks in the display.
 */
/*static*/ COLORREF ChunkWatchControl::dirtyColour()
{
	return CLR_DIRTY_NEEDS_SHADOW_CALC;
}


/**
 *	This gets the colour of calculated chunks in the display.
 *
 *  @returns		The colour of calculated chunks in the display.
 */
/*static*/ COLORREF ChunkWatchControl::calcedColour()
{
	return CLR_DIRTY;
}


/**
 *	This gets the drawing options.
 *
 *  @returns		The drawing options.
 */
uint32 ChunkWatchControl::drawOptions() const
{
	return drawOptions_;
}


/**
 *	This function is used to enable the options of what gets drawn.
 *
 *  @param options	The draw options to enable.
 *  @param update	If true then there is an immediate update.
 */
void ChunkWatchControl::enableDrawOptions(uint32 options, bool update /*= true*/)
{
	BW_GUARD;

	drawOptions_ |= options;
	if (update)
		RedrawWindow();
}


/**
 *	This function is used to disable the options of what gets drawn.
 *
 *  @param options	The draw options to disable.
 *  @param update	If true then there is an immediate update.
 */
void ChunkWatchControl::disableDrawOptions(uint32 options, bool update /*= true*/)
{
	BW_GUARD;

	drawOptions_ &= ~options;
	if (update)
		RedrawWindow();
}


/**
 *	This function gets information text about a point at pos.
 *
 *  @param pos		The position (in client coordinates) to get information 
 *					about.
 *  @param chunkText	This is set to information about the chunk at this
 *					position.
 *  @param posText	This is set to information about the given position.
 *  @returns		True if information could be obtained.
 */
bool 
ChunkWatchControl::getInfoText
(
	CPoint			const &pos, 
	BW::string		&chunkText, 
	BW::string		&posText
) const
{
	BW_GUARD;

	float wx, wz;
	if (screenToWorld(pos, &wx, &wz))
	{
		Vector3 pos3(wx, 0.0f, wz);
		posText = sformat("({0}, {1})", wx, wz).c_str();

		Chunk *chunk = EditorChunk::findOutsideChunk(pos3);
		if (chunk != NULL)
			chunkText = chunk->identifier();
		else
			chunkText = LocaliseUTF8(L"WORLDEDITOR/GUI/CONTROLS/CHUNK_WATCH_CONTROL/NOT_LOADED_STRING");
		return true;
	}
	else
	{
		return false;
	}
}


/**
 *	This calculates some constants used in the geometry of the control such
 *	as the drawing area.
 */
void ChunkWatchControl::getDrawConstants()
{
	BW_GUARD;

	// Get the extents of the drawing area:
	GetClientRect(extents_);
	// Preserve the aspect ratio of the extents
	if (extents_.Width() > extents_.Height())
	{
		extents_.left += (extents_.Width() - extents_.Height())/2;
		extents_.right = extents_.left + extents_.Height(); 
	}
	else
	{
		extents_.top += (extents_.Height() - extents_.Width())/2;
		extents_.bottom = extents_.top + extents_.Width();
	}
	// Offset a little:
	extents_.DeflateRect(EDGE_GAP, EDGE_GAP);

	// Get the size of the space:
	ChunkWatcher const &cw = WorldManager::instance().chunkWatcher();
	minX_   = cw.minX();
	minZ_   = cw.minZ();
	chunksWide_ = cw.maxX() - cw.minX();
	chunksHigh_ = cw.maxZ() - cw.minZ();
}


/**
 *	This draws the project view.
 *
 *  @param dc		The drawing context.
 */
void ChunkWatchControl::drawProject(CDC &dc)
{
	BW_GUARD;

	bool dibUpToDate = updateProjectDib();

	if (!projectDib_.isEmpty())
	{
		if 
		(
			!dibUpToDate
			||
			smallProjectDib_.width()  != extents_.Width() 
			|| 
			smallProjectDib_.height() != extents_.Height()
		)
		{
			smallProjectDib_.resize(extents_.Width(), extents_.Height());
			CDC smallDC;
			smallDC.CreateCompatibleDC(NULL);
			smallDC.SelectObject(smallProjectDib_);
			int oldStretchMode = smallDC.SetStretchBltMode(HALFTONE);
			projectDib_.draw(smallDC, 0, 0, extents_.Width(), extents_.Height());
			smallDC.SetStretchBltMode(oldStretchMode);
			smallProjectDib_.toGreyScale();
		}
		int oldStretchMode = dc.SetStretchBltMode(COLORONCOLOR);
		CRect yInvExtents = extents_;
		std::swap(yInvExtents.top, yInvExtents.bottom);
		smallProjectDib_.draw(dc, yInvExtents);
		dc.SetStretchBltMode(oldStretchMode);
	}
}


/**
 *	This draws the state of the chunks.
 *
 *  @param dc		The drawing context.
 */
void ChunkWatchControl::drawChunks(CDC &dc)
{
	BW_GUARD;

	chunksDib_.drawAlpha(dc, extents_);
}


/** 
 *	This draws the status of unloadable chunks.
 *
 *  @param dc		The drawing context.
 */
void ChunkWatchControl::drawUnloadable(CDC &dc)
{
	BW_GUARD;

	CPen unloadablePen(PS_SOLID, UNLOADABLE_PEN_THICKNESS, CLR_CANNOT_UNLOAD);
	CPen *oldPen = dc.SelectObject(&unloadablePen);

	ChunkWatcher const &cw = WorldManager::instance().chunkWatcher();
	for (uint32 z = 0; z < chunksHigh_; ++z)
	{
		// Calculate the z-position of the top and bottom of this row.  Note 
		// that there is a z-flip due to the upside-down coordinate system:
		int32 z1 = Math::lerp((int32)z    , (int32)0, (int32)chunksHigh_, extents_.bottom, extents_.top);
		int32 z2 = Math::lerp((int32)z + 1, (int32)0, (int32)chunksHigh_, extents_.bottom, extents_.top);
		for (uint32 x = 0; x < chunksWide_; ++x)
		{
			// Calculate the x extents of this tile:
			int32 x1 = Math::lerp(x    , (uint32)0, chunksWide_, extents_.left, extents_.right);				
			int32 x2 = Math::lerp(x + 1, (uint32)0, chunksWide_, extents_.left, extents_.right);
			// Draw the unloadable state:
			if (!cw.canUnload(x + minX_, z + minZ_))
			{
				dc.MoveTo(x1, z1);
				dc.LineTo(x2, z2);
				dc.MoveTo(x2, z1);
				dc.LineTo(x1, z2);
			}
		}
	}
	dc.SelectObject(oldPen);
}


/**
 *	This draws the grid.
 *
 *  @param dc		The drawing context.
 */
void ChunkWatchControl::drawGrid(CDC &dc)
{
	BW_GUARD;

	ChunkWatcher const &cw = WorldManager::instance().chunkWatcher();

	// Draw a grid:
	if (chunksWide_ < TOO_MANY_GRID_LINES && chunksHigh_ < TOO_MANY_GRID_LINES)
	{
		CPen pen(PS_SOLID, 1, CLR_GRID);
		CPen *oldPen = dc.SelectObject(&pen);
		for (uint32 z = 1; z < chunksHigh_; ++z)
		{
			int sy = Math::lerp(z, (uint32)0, chunksHigh_, extents_.top , extents_.bottom);
			dc.MoveTo(extents_.left , sy);
			dc.LineTo(extents_.right, sy);
		}
		for (uint32 x = 1; x < chunksWide_; ++x)
		{
			int sx = Math::lerp(x, (uint32)0, chunksWide_, extents_.left , extents_.right);
			dc.MoveTo(sx, extents_.top   );
			dc.LineTo(sx, extents_.bottom);
		}	
		dc.SelectObject(oldPen);
	}
}


/**
 *	This draws the user's position.
 *
 *  @param dc		The drawing context.
 */
void ChunkWatchControl::drawArrow(CDC &dc)
{
	BW_GUARD;

	// Draw the user's position:	
	drawPos_ = WorldEditorCamera::instance().currentCamera().view();
	drawPos_.invert();
	Vector3 userPos   = drawPos_.applyToOrigin();
	Vector3 direction = drawPos_.applyToUnitAxisVector(2);		
	float ux, uz;
	worldToScreen(&ux, &uz, userPos.x, userPos.z);
	float dx = direction.x;
	float dz = -direction.z; // invert y coord to match screen
	float len = std::sqrt(dx*dx + dz*dz);
	if (len != 0.0f)
	{
		dx /= len; dz /= len;
	}
	dx *= USER_POS_SIZE; dz *= USER_POS_SIZE;
	CPen pen(PS_SOLID, 1, CLR_USER_POS);
	CPen *oldPen = dc.SelectObject(&pen);
	dc.MoveTo((int)(ux - dx), (int)(uz - dz));
	dc.LineTo((int)(ux + dx), (int)(uz + dz));
	dc.MoveTo((int)(ux + dx), (int)(uz + dz));
	dc.LineTo((int)(ux - dz), (int)(uz + dx));
	dc.MoveTo((int)(ux + dx), (int)(uz + dz));
	dc.LineTo((int)(ux + dz), (int)(uz - dx));
	dc.SelectObject(oldPen);
}


/**
 *	Draw the view frustum.
 *
 *  @param dc		The drawing context.
 */
void ChunkWatchControl::drawFrustum(CDC &dc)
{
	BW_GUARD;

	Matrix view	= WorldEditorCamera::instance().currentCamera().view();
	view.invert();

	float   fov			= Moo::rc().camera().fov();
	float   clipDist	= Moo::rc().camera().farPlane();
	Vector3 userPos     = view.applyToOrigin();
	Vector3 dir		    = view.applyToUnitAxisVector(2);
	Vector2 ndir		= Vector2(dir.x, dir.z); 
	float   k			= ::tanf(0.5f*fov)*clipDist;

	if (clipDist < SMALL_FRUSTUM)
		return;

	float ux, uz;
	worldToScreen(&ux, &uz, userPos.x, userPos.z);

	ndir.normalise();
	float x1 = userPos.x + ndir.x*clipDist + k*ndir.y;
	float z1 = userPos.z + ndir.y*clipDist - k*ndir.x;
	float x2 = userPos.x + ndir.x*clipDist - k*ndir.y;
	float z2 = userPos.z + ndir.y*clipDist + k*ndir.x;
	worldToScreen(&x1, &z1, x1, z1);
	worldToScreen(&x2, &z2, x2, z2);

	CPen pen(PS_SOLID, 1, CLR_FRUSTUM);
	CPen *oldPen = dc.SelectObject(&pen);
	dc.MoveTo((int)ux, (int)uz);
	dc.LineTo((int)x1, (int)z1);
	dc.LineTo((int)x2, (int)z2);
	dc.LineTo((int)ux, (int)uz);
	dc.SelectObject(oldPen);
}


/**
 *	This converts a point in the world to screen coordinates.
 *
 *  @param sx	The result x-screen coordinate.
 *  @param sy	The result y-screen coordinate.
 *  @param wx	The world x-coordinate.
 *  @param wz	The world z-coordinate.
 */
void ChunkWatchControl::worldToScreen
(
	float	*sx, 
	float	*sy, 
	float	wx, 
	float	wz
) const
{
	BW_GUARD;

	if (sx != NULL)
	{
		*sx = 
			Math::lerp
			(
				wx, 
				chunkSize_*minX_, 
				chunkSize_*(chunksWide_ + minX_), 
				(float)extents_.left, 
				(float)extents_.right
			);
	}
	if (sy != NULL)
	{
		*sy = 
			Math::lerp
			(
				wz, 
				chunkSize_*minZ_, 
				chunkSize_*(chunksHigh_ + minZ_), 
				(float)extents_.bottom,  // swap because +y is down in screen
				(float)extents_.top		 // coordinates
			);
	}
}


/**
 *	This converts screen coordinates to world coordinates.
 *
 *  @param pos		The screen position.
 *  @param wx		This is set to the x world position.
 *  @param wz		This is set to the z world position.
 *  @returns		True if the point is inside the world, false otherwise.
 */
bool ChunkWatchControl::screenToWorld
(
	CPoint		const &pos, 
	float		*wx, 
	float		*wz
) const
{
	BW_GUARD;

	if (extents_.PtInRect(pos))
	{
		if (wx != NULL)
		{
			*wx = 
				Math::lerp
				(
					(float)pos.x,
					(float)extents_.left, 
					(float)extents_.right,
					chunkSize_*minX_, 
					chunkSize_*(chunksWide_ + minX_)			
				);
		}
		if (wz != NULL)
		{
			*wz = 
				Math::lerp
				(
					(float)pos.y, 
					(float)extents_.bottom,  // swap because +y is down in screen
					(float)extents_.top,	 // coordinates
					chunkSize_*minZ_, 
					chunkSize_*(chunksHigh_ + minZ_)
				);
		}
		return true;
	}
	else
	{
		return false;
	}
}


/**
 *	This makes sure that the project view is up to date.
 *
 *  @returns		True if the dib was up to date.
 */
bool ChunkWatchControl::updateProjectDib()
{
	BW_GUARD;

	// Are we up to date?
	if (!projectDib_.isEmpty() && projectMark_ == SpaceMap::instance().mark())
		return true;

	// Are we updating too frequently?
	if (lastUpdateProjectTime_ != 0)
	{
		uint64 now = timestamp();
		if ((now - lastUpdateProjectTime_)/(double)stampsPerSecond() < PROJECT_UPDATE_DELTA)
			return true;
		lastUpdateProjectTime_ = now;
	}
	else
	{
		lastUpdateProjectTime_ = timestamp();
	}

	Moo::BaseTexturePtr baseTexture; 
	DX::Texture *texture = 
		static_cast<DX::Texture *>(SpaceMap::instance().texture());
	if (texture == NULL)
	{
		GeometryMapping *dirMap = WorldManager::instance().geometryMapping();
		if (dirMap != NULL)
		{
			// TODO: Check which is first, space.thumbnail.dds or
			// space.temp_thumbnail.dds.
			BW::string mapName = dirMap->path() + "space.temp_thumbnail.dds";
			baseTexture = Moo::TextureManager::instance()->get(mapName, true, false, true, "texture/chunk watcher");
			if (baseTexture.hasObject())
			{
				texture = static_cast<DX::Texture *>(baseTexture->pTexture());
			}
			else
			{
				mapName = dirMap->path() + "space.thumbnail.dds";
				baseTexture = Moo::TextureManager::instance()->get(mapName, true, false, true, "texture/chunk watcher");
				if (baseTexture.hasObject())
					texture = static_cast<DX::Texture *>(baseTexture->pTexture());
			}
		}
	}
	if (texture != NULL)
	{
		projectDib_.copyFromTexture( baseTexture );
		projectMark_ = SpaceMap::instance().mark();
	}
	return false;
}


/**
 *	This forces the window to be repainted.  This function prevents repainting
 *	too rapidly by selectively choosing between Invalidate and RedrawWindow.
 *
 *  @param force		If true then the repaint is done immmediately.
 */
void ChunkWatchControl::redraw(bool force /*= false*/)
{
	BW_GUARD;

	if (!force)
	{
		if (lastUpdateTime_ != 0)
		{
			uint64 now = timestamp();
			if ((now - lastUpdateTime_)/(double)stampsPerSecond() < UPDATE_DELTA)
			{
				Invalidate(); // add to message queue for painting later
				return;
			}
			lastUpdateTime_ = now;
		}
		else
		{
			lastUpdateTime_ = timestamp();
		}
	}
	RedrawWindow();
}
BW_END_NAMESPACE

