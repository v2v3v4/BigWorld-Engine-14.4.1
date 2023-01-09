#include "pch.hpp"
#include "worldeditor/gui/controls/terrain_textures_control.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "terrain/editor_base_terrain_block.hpp"
#include "terrain/terrain_texture_layer.hpp"
#include "common/user_messages.hpp"
#include "controls/memdc.hpp"
#include "controls/dib_section32.hpp"

BW_BEGIN_NAMESPACE


namespace
{
    const uint32	IMAGE_OFFSET			= 8;

	// This is the thickness for the selections:
	const uint32	LAST_SEL_THICKNESS		= 3;
	const uint32	OTHER_SEL_THICKNESS		= 1;

	// This is the colour for the selections:
	const COLORREF	LAST_SEL_COLOUR			= RGB(255, 0, 0);
	const COLORREF	OTHER_SEL_COLOUR		= RGB(255, 0, 0);
}


BEGIN_MESSAGE_MAP(TerrainTexturesControl, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


/**
 *  This is the TerrainTexturesControl constructor.
 */
TerrainTexturesControl::TerrainTexturesControl():
    CWnd(),
    terrain_(NULL)
{
}


/**
 *  This is the TerrainTexturesControl destructor.
 */
TerrainTexturesControl::~TerrainTexturesControl()
{
}


/**
 *  This creates the TerrainTexturesControl's window.
 *
 *  @param style    WS_CHILD etc, see CWnd::Create for possible values.
 *  @param extents  The extents of the window.
 *  @param parent   The parent window.
 *  @param id       The id of the control.
 */
BOOL 
TerrainTexturesControl::Create
(
    DWORD           style, 
    CRect           const &extents, 
    CWnd            *parent, 
    UINT            id              /*= 0*/
)
{
	BW_GUARD;

    if
    (
        !CreateEx
        (
            0,
            AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW)), 
            _T("TerrainTexturesControl"),
            style,
            extents,
            parent,
            id
        )
    )        
    {
        TRACE0("Failed to create ColorTimelineWnd\n");
        return FALSE;
    }

    ShowWindow(SW_SHOW);
    UpdateWindow();
    RedrawWindow();

    return TRUE;
}


/**
 *  This sets the terrain used by the control.
 *
 *  @param terrain  The new terrain whose textures the control is showing.
 *                  This can be NULL.
 */
void TerrainTexturesControl::terrain(EditorChunkTerrainPtr terrain)
{
	BW_GUARD;

    terrain_ = terrain;
    rebuildTextureLayerEntries();
	Invalidate();
}


/**
 *  This gets the terrain used by the control.
 *
 *  @returns	    The terrain whose textures the control is showing.
 *                  This can be NULL.
 */
EditorChunkTerrainPtr TerrainTexturesControl::terrain() const
{
	return terrain_;
}


/**
 *  This gets the terrain block that the control is showing.
 *
 *  @returns        The terrain block whose textures/blends the control is
 *                  showing.
 */
Terrain::EditorBaseTerrainBlock* TerrainTexturesControl::terrainBlock() const
{
	BW_GUARD;

	if (terrain_)
		return &terrain_->block();
	else
		return NULL;
}


/**
 *	This forces a refresh of the terrain block's data, probably because the
 *	texture layer information has changed.
 */
void TerrainTexturesControl::refresh()
{
	BW_GUARD;

    rebuildTextureLayerEntries();
    Invalidate();
}


/**
 *	Calculates and returns the height required to display all the textures
 *
 *	@returns				Height required to display all the textures
 */
int TerrainTexturesControl::height() const
{
	BW_GUARD;

	CRect extent = textureExtent( 0 );
	return static_cast<int>(((textures_.size() + 1) / 2) * (extent.Height() + IMAGE_OFFSET));
}


/**
 *	This finds which texture layer is under the point.
 *
 *  @param point	coords to test for intersection with a texture
 *	@returns		index to the texture, -1 if no texture in point
 */
int TerrainTexturesControl::layerAtPoint( const CPoint& point ) const
{
	BW_GUARD;

	int idx = -1;
	for( unsigned int i = 0; i < textureNames_.size(); ++i )
	{
		if ( textureExtent(i).PtInRect( point ) ||
			blendExtent(i).PtInRect( point ) )
		{
			idx = i;
			break;
		}
	}
	return idx;
}


/**
 *	This finds the extents of the layer under the point.
 *
 *  @param point	Coordinates to test for intersection with the layers.
 *	@param rect		If the test point is inside a layer then this is set to
 *					be a rect which contains the texture and blend images.
 *	@return 		True if the point is inside a layer, false otherwise.
 */
bool TerrainTexturesControl::layerRectAtPoint( 
	const CPoint &	point, 
	CRect &			rect ) const
{
	BW_GUARD;

	for (size_t i = 0; i < textureNames_.size(); ++i)
	{
		if (textureExtent( i ).PtInRect( point ) ||
			blendExtent( i ).PtInRect( point ))
		{
			rect =  textureExtent( i );
			rect |= blendExtent  ( i );
			return true;
		}
	}
	return false;
}



/**
 *	Resizes the control the width specified and to the full height
 *
 *	@param width			The width to set the control to.
 */
void TerrainTexturesControl::resizeFullHeight( int width )
{
	BW_GUARD;

    SetWindowPos( NULL,
		0, 0,
		width, height(),
		SWP_NOMOVE | SWP_NOZORDER );
}


/**
 *	This gets the selected items.
 *
 *	@returns				The selected items.  The array is the indices of
 *							the selected layers.
 */
BW::vector<size_t> const &TerrainTexturesControl::selection() const
{
	return selection_;
}


/**
 *	This sets the selected items.
 */
void TerrainTexturesControl::selection(BW::vector<size_t> const &sel)
{
	BW_GUARD;

	selection_ = sel;
	Invalidate();
	UpdateWindow();
}


/**
 *  This is called by MFC to paint the control.
 */
/*afx_msg*/ void TerrainTexturesControl::OnPaint()
{
	BW_GUARD;

    CPaintDC	paintDC(this);
	controls::MemDC		dc;
	controls::MemDCScope	dcScope(dc, paintDC);

    CRect extents;
    GetClientRect(&extents);

    dc.FillSolidRect(extents, ::GetSysColor(COLOR_BTNFACE));

	CPen lastSelPen (PS_SOLID, LAST_SEL_THICKNESS , LAST_SEL_COLOUR );
	CPen otherSelPen(PS_SOLID, OTHER_SEL_THICKNESS, OTHER_SEL_COLOUR);

    int oldMode = dc.SetStretchBltMode(HALFTONE);
    for (size_t i = 0; i < textures_.size(); ++i)
    {
		// Draw the texture and the blends:
        CRect textureExt = textureExtent(i, &extents);
        textures_[i]->draw(dc, textureExt);
        CRect blendExt = blendExtent(i, &extents);
        blends_[i]->draw(dc, blendExt);
        
		// Draw the selection:
		int selIdx = selectionIdx(i);
		CPen *pen = NULL;
		if (selIdx != -1 && selIdx == (int)selection_.size() - 1)
		{
			++textureExt.top; // Fix a drawing issue with thick pens
			++blendExt  .top;
			pen = &lastSelPen;
		}
		else if (selIdx != -1)
		{
			pen = &otherSelPen;
		}
		if (pen != NULL)
		{
			// Don't create a thick line by drawing two adjacent rectangles
			--blendExt.left;	
			CPen       *oldPen   = dc.SelectObject(pen);
			CGdiObject *oldBrush = dc.SelectStockObject(NULL_BRUSH);
			dc.Rectangle(textureExt);
			dc.Rectangle(blendExt);
			dc.SelectObject(oldBrush);
			dc.SelectObject(oldPen);
		}
    }
    dc.SetStretchBltMode(oldMode);
}


/**
 *  This is called by MFC to erase the background of the window before 
 *  painting.  We override this to do nothing since we use double-buffering
 *  in the OnPaint call.
 */
/*afx_msg*/ BOOL TerrainTexturesControl::OnEraseBkgnd(CDC * /*dc*/)
{
    return TRUE; // erasing is not necesary
}


/**
 *  This function rebuilds the images of the texture layers when the terrain 
 *  block is changed.
 */
void TerrainTexturesControl::rebuildTextureLayerEntries()
{
	BW_GUARD;

    DibSecVec oldTextures = textures_;
    BW::vector<BW::string> oldTextureNames(textureNames_);

    textures_.clear();
    blends_.clear();
    textureNames_.clear();
	selection_.clear();

	Terrain::EditorBaseTerrainBlock *block = terrainBlock();

    if (block != 0)
    {
        for (size_t i = 0; i < block->numberTextureLayers(); ++i)
        {
            Terrain::TerrainTextureLayer const &layer = block->textureLayer(i);
            addLayer(layer, oldTextureNames, oldTextures);
        }
    }
}


/**
 *  This function adds images to textures_ and blends_ for the given layer.
 *
 *  @param layer            The layer to add images for.
 *  @param oldTextureNames  The names of the textures in the previous chunk.
 *  @param oldTextures      The textures in the previous chunk.
 */
void TerrainTexturesControl::addLayer
(
    Terrain::TerrainTextureLayer         const &layer,
    BW::vector<BW::string>    const &oldTextureNames,
    DibSecVec                   const &oldTextures
)
{
	BW_GUARD;

    // Create the texture:
    bool found = false;
    for (size_t i = 0; i < oldTextures.size(); ++i)
    {
        if (oldTextureNames[i] == layer.textureName())
        {
            found = true;
            textures_.push_back(oldTextures[i]);
			break;
        }
    }
    if (!found)
    {    
        controls::DibSection32Ptr texture = new controls::DibSection32();
        // Can we copy directly from the d3d texture?
        IDirect3DBaseTexture9 *d3dBaseTexture = layer.texture()->pTexture();
		bool success = false;
        if (d3dBaseTexture && d3dBaseTexture->GetType() == D3DRTYPE_TEXTURE)
        {
            success = texture->copyFromTexture( layer.texture() );
			if (!success)
			{
				ERROR_MSG( "TerrainTexturesControl::addLayer: "
					"Could not convert texture %s to DIB\n",
					layer.textureName().c_str() );
			}
        }
        // We cannot get it directly from the d3d texture, let's just load the 
        // file ourselves:
        else
        {
            BW::string filename = BWResource::resolveFilename(
				layer.textureName() );
            success = texture->load( filename );
			if (!success)
			{
				ERROR_MSG( "TerrainTexturesControl::addLayer: "
					"Could not load DIB for texture %s\n",
					layer.textureName().c_str() );
			}
        }

		if (success)
		{
			textures_.push_back(texture);
		}
		else
		{
			return;
		}
    }

    // Create the blend:    
    Terrain::TerrainTextureLayer &nclayer = const_cast<Terrain::TerrainTextureLayer &>(layer);
    Terrain::TerrainTextureLayerHolder holder(&nclayer, true);
    Terrain::TerrainTextureLayer::ImageType const &img = nclayer.image();
    controls::DibSection32Ptr blend = new controls::DibSection32(img.width(), img.height());
    for (uint32 y = 0; y < img.height(); ++y)
    {
        uint8 const *p = img.getRow(y);
        uint8 const *q = p + img.width();
        COLORREF    *d = blend->getRow(img.height() - y - 1);
        for( ; p != q; ++p, ++d)
        {
            uint8 grey = *p;
            *d = RGB(grey, grey, grey);
        }
    }
    blends_.push_back(blend);

    textureNames_.push_back(layer.textureName());
}


/**
 *  This function calculates the area that the idx'th texture should be
 *  drawn at.
 *
 *  @param idx      The index of the texture.
 *  @param nclient  The client extents.  If this is NULL then we use the
 *                  value given by GetClientRect().
 *  @returns        The area that the texture should be drawn in.
 */
CRect 
TerrainTexturesControl::textureExtent
(
    size_t      idx, 
    CRect       const *nclient /*= NULL*/
) const
{
	BW_GUARD;

    CRect client;
    if (nclient != NULL)
        client = *nclient;
    else
        GetClientRect(&client);

    int midx = (client.left + client.right)/2;

    // x-positions are left to right, x1 to x5:
    int x1 = client.left;
    int x5 = midx;
    int x2 = x1 + IMAGE_OFFSET/2;
    int x4 = x5 - IMAGE_OFFSET/2;
    int x3 = (x1 + x4)/2;

    if (idx%2 == 1)
    {
        x1 += midx;
        x2 += midx;
        x3 += midx;
        x4 += midx;
        x5 += midx;
    }    

    int h = x3 - x2;

    CRect result;
    result.left   = x2;
    result.top    = static_cast<LONG>((IMAGE_OFFSET + h)*(idx/2));
    result.right  = x3;
    result.bottom = result.top + h;
    return result;
}


/**
 *  This function calculates the area that the idx'th blend should be
 *  drawn at.
 *
 *  @param idx      The index of the texture.
 *  @param nclient The client extents.  If this is NULL then we use the
 *                  value given by GetClientRect().
 *  @returns        The area that the blend should be drawn in.
 */
CRect 
TerrainTexturesControl::blendExtent
(
    size_t      idx, 
    CRect       const *nclient /*= NULL*/
) const
{
	BW_GUARD;

    CRect client;
    if (nclient != NULL)
        client = *nclient;
    else
        GetClientRect(&client);

    int midx = (client.left + client.right)/2;

    // x-positions are left to right, x1 to x5:
    int x1 = client.left;
    int x5 = midx;
    int x2 = x1 + IMAGE_OFFSET/2;
    int x4 = x5 - IMAGE_OFFSET/2;
    int x3 = (x1 + x4)/2;

    if (idx%2 == 1)
    {
        x1 += midx;
        x2 += midx;
        x3 += midx;
        x4 += midx;
        x5 += midx;
    }    

    int h = x3 - x2;

    CRect result;
    result.left   = x3;
    result.top    = static_cast<LONG>((IMAGE_OFFSET + h)*(idx/2));
    result.right  = x4;
    result.bottom = result.top + h;
    return result;
}


/**
 *	Returns the index of the idx'th item in the selection.
 *
 *	@param idx			The index to look up.
 *	@returns			The index of idx in the selection and -1 if it's not
 *						in the selection.
 */
int TerrainTexturesControl::selectionIdx(size_t idx) const
{
	BW_GUARD;

	for (size_t i = 0; i < selection_.size(); ++i)
	{
		if (selection_[i] == idx)
			return static_cast<int>(i);
	}
	return -1;
}


/**
 *	Message handler for left mouse button down
 *
 *	@param flags		Flags indicating keypress state etc.
 *	@param point		The point that the button went down at.
 */
/*afx_msg*/ void TerrainTexturesControl::OnLButtonDown(UINT flags, CPoint point)
{
	BW_GUARD;

	SetFocus();

	// Add/remove from the selection
	int idx = layerAtPoint( point );
	if (idx != -1)
	{
		bool toggleSelect = ((flags & MK_CONTROL) != 0) || ((flags & MK_SHIFT) != 0);
		bool groupSelect  = (GetKeyState(VK_MENU) & 0x8000) != 0; // alt key

		// Get the elements we are about to select:
		BW::vector<size_t> newSel;
		if (groupSelect)
		{
			// Find all textures that are the same name as the selected texture
			// and add them to the list.  Note we don't add the idx'th item
			// since this gets added always (and last):
			BW::string textureName = textureNames_[idx];
			for (size_t i = 0; i < textureNames_.size(); ++i)
			{
				if (textureName == textureNames_[i] && i != idx)
					newSel.push_back(i);
			}
		}
		newSel.push_back(idx);

		if (toggleSelect)
		{
			int oldIdx = selectionIdx(idx);
			if (oldIdx != -1)
			{
				// Remove all items in newSel from the selection:
				for (size_t i = 0; i < newSel.size(); ++i)
				{
					int oldIdx = selectionIdx(newSel[i]);
					if (oldIdx != -1)
						selection_.erase(selection_.begin() + oldIdx);
				}
			}
			else
			{
				// Add all items in newSel from the selection:
				for (size_t i = 0; i < newSel.size(); ++i)
				{
					int oldIdx = selectionIdx(newSel[i]);
					if (oldIdx == -1)
						selection_.push_back(newSel[i]);
				}				
			}
		}
		else
		{
			selection_.clear();
			for (size_t i = 0; i < newSel.size(); ++i)
				selection_.push_back(newSel[i]);
		}
	}
	else
	{
		selection_.clear();
	}
	Invalidate();
	UpdateWindow();	

	CWnd::OnLButtonDown(flags, point);
}


/**
 *	Message handler for right mouse button down.
 *
 *	@param nFlags		Flags indicating keypress state etc.
 *	@param point		The point that the button went down at.
 */
/*afx_msg*/ void TerrainTexturesControl::OnRButtonDown( UINT nFlags, CPoint point )
{
	BW_GUARD;

	SetFocus();

	CWnd* parent = GetParent();
	if ( !parent )
		return;

	int idx = layerAtPoint( point );
	if ( idx != -1 )
	{
		// Update the selection:
		int selIdx = selectionIdx(idx);
		if (selIdx == -1)
		{
			selection_.clear();
			selection_.push_back(idx);
			Invalidate();
			UpdateWindow();
		}

		// Tell the parent
		parent->SendMessage
		( 
			WM_TERRAIN_TEXTURE_RIGHT_CLICK, 
			(WPARAM)textureNames_[ idx ].c_str(), 
			(LPARAM)idx 
		);
	}
}


/**
 *	Message handler for left mouse button double click.
 *
 *	@param nFlags		Flags indicating keypress state etc.
 *	@param point		The point that the button went down at.
 */
/*afx_msg*/ void TerrainTexturesControl::OnLButtonDblClk( UINT nFlags, CPoint point )
{
	BW_GUARD;

	SetFocus();

	CWnd* parent = GetParent();
	if ( !parent )
		return;

	int idx = layerAtPoint( point );
	if ( idx != -1 )
	{
		parent->SendMessage
		( 
			WM_TERRAIN_TEXTURE_SELECTED, 
			(WPARAM)textureNames_[ idx ].c_str(), 
			(LPARAM)idx 
		);
	}
}
BW_END_NAMESPACE

