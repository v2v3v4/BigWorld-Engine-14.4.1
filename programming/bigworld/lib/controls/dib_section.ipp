#ifndef CONTROLS_DIB_SECTION_IPP
#define CONTROLS_DIB_SECTION_IPP

BW_BEGIN_NAMESPACE

/**
 *  This is the default ctor for DibSection.  It creates an empty image.
 */
template<typename PIXELTYPE>
inline controls::DibSection<PIXELTYPE>::DibSection(): Base(),
    hbitmap_(NULL)
{
	BW_GUARD;

    ::ZeroMemory(&bmi_, sizeof(bmi_));
}


/**
 *  This is the ctor for DibSection that creates an image of size 
 *  width*height.
 *
 *  @param width        The width of the image.
 *  @param height       The height of the image.
 */
template<typename PIXELTYPE>
inline controls::DibSection<PIXELTYPE>::DibSection(uint32 width, uint32 height):
    Base(),
    hbitmap_(NULL)
{
	BW_GUARD;

    ::ZeroMemory(&bmi_, sizeof(bmi_));
    resize(width, height);
}


/**
 *  This is the copy ctor for DibSections.
 *
 *  @param other        The DibSection to copy.
 */
template<typename PIXELTYPE>
inline controls::DibSection<PIXELTYPE>::DibSection(DibSection<PIXELTYPE> const &other):
    Base(),
    hbitmap_(NULL)
{
	BW_GUARD;

    ::ZeroMemory(&bmi_, sizeof(bmi_));
    copy(other);
}


/**
 *  DibSection destructor.
 */
template<typename PIXELTYPE>
inline controls::DibSection<PIXELTYPE>::~DibSection()
{
	BW_GUARD;

    destroy();
}


/**
 *  DibSection assignment.
 *
 *  @param other        The DibSection to copy.     
 *  @returns            *this.
 */
template<typename PIXELTYPE>
inline controls::DibSection<PIXELTYPE> &
controls::DibSection<PIXELTYPE>::operator=(DibSection<PIXELTYPE> const &other)
{
	BW_GUARD;

    if (this != &other)
        copy(other);
    return *this;
}


/**
 *  This returns the size of the image's buffer.
 *
 *  @returns            The size of the image buffer in bytes.
 */
template<typename PIXELTYPE>
inline size_t controls::DibSection<PIXELTYPE>::bufferSizeBytes() const
{
	BW_GUARD;

    return height()*rowSizeBytes();
}


/**
 *  This returns the size of a row of pixels in the image.
 *
 *  @returns            The size of a pixel row in bytes.
 */
template<typename PIXELTYPE>
inline size_t controls::DibSection<PIXELTYPE>::rowSizeBytes() const
{
	BW_GUARD;

    MF_ASSERT(0); // defined in specializations
    return 0;
}


/**
 *  This returns the BITMAPINFO associated with the image.
 *
 *  @returns            The BITMAPINFO associated with the image.
 */
template<typename PIXELTYPE>
inline BITMAPINFO const &controls::DibSection<PIXELTYPE>::getBITMAPINFO() const
{
    return bmi_;
}


/**
 *  This returns the BITMAPINFOHEADER associated with the image.
 *
 *  @returns            The BITMAPINFOHEADER associated with the image.
 */
template<typename PIXELTYPE>
inline BITMAPINFOHEADER const &controls::DibSection<PIXELTYPE>::getBITMAPINFOHEADER() const
{
    return bmi_.bmiHeader;
}


/**
 *  This returns the HBITMAP of the DibSection.  This is null if the image is
 *  empty.
 *
 *  @returns            The handle to the underlying DibSection.
 */
template<typename PIXELTYPE>
inline controls::DibSection<PIXELTYPE>::operator HBITMAP() const
{
    return hbitmap_;
}


/**
 *  This detaches the HBITMAP from the object and returns it.  The callee is
 *  then responsible for calling ::DeleteObject on it.
 */
template<typename PIXELTYPE>
inline HBITMAP controls::DibSection<PIXELTYPE>::detach()
{
	BW_GUARD;

    HBITMAP result = hbitmap_;
    hbitmap_ = NULL;
    destroy();
    return result;
}


/**
 *  This loads the DibSection.
 *
 *  @param filename     The (fully qualified file) to load.
 *  @returns            True if successfully loaded, false otherwise.
 */ 
template<typename PIXELTYPE>
inline bool controls::DibSection<PIXELTYPE>::load(BW::string const &filename)
{
	BW_GUARD;

    clear();

    CImage image;
    if (SUCCEEDED(image.Load(bw_utf8tow(  filename ).c_str() )))
    {
        resize(image.GetWidth(), image.GetHeight());
        CDC drawDC;
        drawDC.CreateCompatibleDC(NULL);
        drawDC.SelectObject(hbitmap_);
        image.Draw(drawDC, 0, 0);
        return true;
    }
    else
    {
        return loadDDS(filename);
    }
}


/**
 *  This saves the DibSection. 
 *
 *  @param filename     The file to save.  DDS files are not supported yet.
 *  @returns            True if successfully saved, false otherwise.
 */ 
template<typename PIXELTYPE>
inline bool controls::DibSection<PIXELTYPE>::save(BW::string const &filename) const
{
	BW_GUARD;

    if (isEmpty())
        return false;

    CImage image;
    image.Create(width(), height(), 32);
    CDC *drawDC = CDC::FromHandle(image.GetDC());
    draw(*drawDC, 0, 0);
    image.ReleaseDC();
    image.Save( CString( filename.c_str() ) );

    return true;
}


/**
 *  This loads the DibSection from a resource.  It's currently quit slow and
 *  only appropriate for icons and small images.
 *
 *  @param resourceID   The id of the resource to load the image from.
 *  @returns            True if successfully loaded.
 */
template<typename PIXELTYPE>
inline bool controls::DibSection<PIXELTYPE>::load(uint32 resourceID)
{
	BW_GUARD;

    CImage image;
    image.LoadFromResource(NULL, resourceID);
    resize(image.GetWidth(), image.GetHeight());
    CDC drawDC;
    drawDC.CreateCompatibleDC(NULL);
    drawDC.SelectObject(hbitmap_);
    image.Draw(drawDC, 0, 0);
    return true;
}


/**
 *  This copies a texture into the DibSection.
 *
 *  @param texture      The texture to copy into the DibSection.
 */
template<typename PIXELTYPE>
inline bool controls::DibSection<PIXELTYPE>::copyFromTexture(
	const Moo::BaseTexturePtr& baseTexture )
{
	BW_GUARD;

    this->clear();

    if (baseTexture == NULL)
	{
        return false;
	}
	DX::Texture* pDXTexture =
		static_cast< DX::Texture * >( baseTexture->pTexture() );

    bool ok = true;

    // If the format is D3DFMT_A8R8G8B8, we can lock it and copy it directly:
    D3DSURFACE_DESC description;
    if (!SUCCEEDED(pDXTexture->GetLevelDesc(0, &description)))
        return false;
    if (description.Format == D3DFMT_A8R8G8B8)
    {
        ok = copyFromA8R8G8B8Texture(pDXTexture);
        if (ok)
            return true;
    }

    // The texture is not D3DFMT_A8R8G8B8 or could not be copied, create a 
    // D3DFMT_A8R8G8B8 surface, blit the texture to the surface and then copy 
    // the D3DFMT_A8R8G8B8 surface.    
    IDirect3DSurface9 *surface = NULL;
    ok = 
        SUCCEEDED
        (
            Moo::rc().device()->CreateOffscreenPlainSurface
            (
                description.Width, description.Height,
                D3DFMT_A8R8G8B8,
                D3DPOOL_SCRATCH,
                &surface,
                NULL // shared handle
            )
        );
    if (!ok)
        return false;
    IDirect3DSurface9 *textureSurface = NULL;

	Moo::BaseTexturePtr validTexture( baseTexture );

	D3DSURFACE_DESC desc;
	pDXTexture->GetLevelDesc( 0, &desc );

	if (Moo::rc().usingD3DDeviceEx() && (desc.Pool != D3DPOOL_SYSTEMMEM))
	{
		validTexture = Moo::TextureManager::instance()->
			getSystemMemoryTexture( baseTexture->resourceID() );
	}

	pDXTexture =
		static_cast< DX::Texture * >( validTexture->pTexture() );

    ok = SUCCEEDED( pDXTexture->GetSurfaceLevel( 0, &textureSurface ) );
    if (!ok)
    {
        surface->Release();
		surface = NULL;
        return false;
    }
    ok =
        SUCCEEDED
        (
            D3DXLoadSurfaceFromSurface
            (
                surface, 
                NULL,               // dest. palette 
                NULL,               // dest. rect. = full surface
                textureSurface,
                NULL,               // src. palette
                NULL,               // src. rect = full surface
                D3DX_DEFAULT,       // default filtering
                0                   // no colour key
            )
        );
    if (!ok)
    {
        textureSurface->Release(); textureSurface = NULL;
        surface->Release(); surface = NULL;
        return false;
    }
    textureSurface->Release(); textureSurface = NULL;
    ok = copyFromA8R8G8B8Surface(surface);
    surface->Release(); surface = NULL;
    return ok;
}


/**
 *  This copies a surface into the DibSection.
 *
 *  @param texture      The texture to copy into the DibSection.
 */
template<typename PIXELTYPE>
inline bool controls::DibSection<PIXELTYPE>::copyFromSurface(DX::Surface *surface)
{
	BW_GUARD;

    clear();

    if (surface == NULL)
        return false;

    bool ok = true;

    ok = copyFromA8R8G8B8Surface(surface);
    if (ok)
        return true;
    D3DSURFACE_DESC description;
    surface->GetDesc(&description);
    IDirect3DSurface9 *mysurface = NULL;
    ok = 
        SUCCEEDED
        (
            Moo::rc().device()->CreateOffscreenPlainSurface
            (
                description.Width, description.Height,
                D3DFMT_A8R8G8B8,
                D3DPOOL_SCRATCH,
                &mysurface,
                NULL // shared handle
            )
        );
    if (!ok)
        return false;
    ok =
        SUCCEEDED
        (
            D3DXLoadSurfaceFromSurface
            (
                mysurface, 
                NULL,               // dest. palette 
                NULL,               // dest. rect. = full surface
                surface,
                NULL,               // src. palette
                NULL,               // src. rect = full surface
                D3DX_DEFAULT,       // default filtering
                0                   // no colour key
            )
        );
    if (!ok)
    {
        mysurface->Release(); mysurface = NULL;
        return false;
    }
    ok = copyFromA8R8G8B8Surface(mysurface);
    mysurface->Release(); mysurface = NULL;
    return ok;
}


/**
 *  This function draws the DibSection to a device context.
 *
 *  @param dc           The device to draw on.
 *  @param x            The x coordiante to draw at.
 *  @param y            The y coordinate to draw at.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::draw(CDC &dc, int x, int y) const
{
	BW_GUARD;

    draw(dc, x, y, width(), height());
}


/**
 *  This function draws the DibSection to a device context in the given
 *  rectangle.
 *
 *  @param dc           The device to draw on.
 *  @param left         The left coordiante to draw at.
 *  @param top          The top coordinate to draw at.
 *  @param w            The width to draw with.
 *  @param h            The height to draw with.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::draw(CDC &dc, int left, int top, int w, int h) const
{
	BW_GUARD;

    CDC memDC;
    memDC.CreateCompatibleDC(NULL);
    HGDIOBJ oldBmp = ::SelectObject(memDC, hbitmap_);
    dc.StretchBlt
    (
        left, top, w, h,
        &memDC,
        0, 0, width(), height(),
        SRCCOPY
    );    
    ::SelectObject(memDC, oldBmp);
}


/**
 *  This function draws a sub-image of the DibSection to a device context in 
 *  the given rectangle.
 *
 *  @param dc           The device to draw on.
 *  @param dleft        The destination left coordinate.
 *  @param dtop         The destination top coordinate.
 *  @param dwidth       The destination width to draw with.
 *  @param dheight      The destination height to draw with.
 *  @param sleft        The source left coordinate.
 *  @param stop         The source top coordinate.
 *  @param swidth       The source width to draw with.
 *  @param sheight      The source height to draw with.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::draw
(
    CDC     &dc, 
    int     dleft, 
    int     dtop, 
    int     dwidth, 
    int     dheight,
    int     sleft, 
    int     stop, 
    int     swidth, 
    int     sheight
) const
{
	BW_GUARD;

    CDC memDC;
    memDC.CreateCompatibleDC(NULL);
    HGDIOBJ oldBmp = ::SelectObject(memDC, hbitmap_);
    dc.StretchBlt
    (
        dleft, dtop, dwidth, dheight,
        &memDC,
        sleft, stop, swidth, sheight,
        SRCCOPY
    );    
    ::SelectObject(memDC, oldBmp);
}


/**
 *  This function draws the DibSection to a device context in the given
 *  rectangle.
 *
 *  @param dc       The device to draw on.
 *  @param extents  The drawing extents.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::draw(CDC &dc, CRect const &extents) const
{
	BW_GUARD;

    draw(dc, extents.left, extents.top, extents.Width(), extents.Height());
}


/**
 *  This function draws a sub-image of the DibSection to a device context in 
 *  the given rectangles.
 *
 *  @param dc       The device to draw on.
 *  @param dextents The destination extents.
 *  @param sextents The source extents.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::draw(CDC &dc, CRect const &dextents, CRect const &sextents) const
{
	BW_GUARD;

    draw
    (
        dc, 
        dextents.left, dextents.top, dextents.Width(), dextents.Height(),
        sextents.left, sextents.top, sextents.Width(), sextents.Height()
    );
}


/**
 *  This function alpha blends the dib onto a drawing surface.
 *
 *  @param dc           The device to draw on.
 *  @param x            The x coordiante to draw at.
 *  @param y            The y coordinate to draw at.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::drawAlpha(CDC &dc, int x, int y) const
{
	BW_GUARD;

    drawAlpha(dc, x, y, width(), height());
}


/**
 *  This function alpha draws the DibSection to a device context in the given
 *  rectangle.
 *
 *  @param dc           The device to draw on.
 *  @param left         The left coordiante to draw at.
 *  @param top          The top coordinate to draw at.
 *  @param w            The width to draw with.
 *  @param h            The height to draw with.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::drawAlpha(CDC &dc, int left, int top, int w, int h) const
{
	BW_GUARD;

    CDC memDC;
    memDC.CreateCompatibleDC(NULL);
    HGDIOBJ oldBmp = ::SelectObject(memDC, hbitmap_);
    BLENDFUNCTION blendFunc;
    blendFunc.BlendOp             = AC_SRC_OVER;
    blendFunc.BlendFlags          =   0;
    blendFunc.SourceConstantAlpha = 255;
    blendFunc.AlphaFormat         = AC_SRC_ALPHA;
    dc.AlphaBlend 
    (
        left, top, w, h,
        &memDC,        
        0, 0, width(), height(),
        blendFunc
    );    
    ::SelectObject(memDC, oldBmp);
}


/**
 *  This function alpha draws a sub-image of the DibSection to a device context 
 *  in the given rectangle.
 *
 *  @param dc           The device to draw on.
 *  @param dleft        The destination left coordinate.
 *  @param dtop         The destination top coordinate.
 *  @param dwidth       The destination width to draw with.
 *  @param dheight      The destination height to draw with.
 *  @param sleft        The source left coordinate.
 *  @param stop         The source top coordinate.
 *  @param swidth       The source width to draw with.
 *  @param sheight      The source height to draw with.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::drawAlpha
(
    CDC     &dc, 
    int     dleft, 
    int     dtop, 
    int     dwidth, 
    int     dheight,
    int     sleft, 
    int     stop, 
    int     swidth, 
    int     sheight
) const
{
	BW_GUARD;

    CDC memDC;
    memDC.CreateCompatibleDC(NULL);
    HGDIOBJ oldBmp = ::SelectObject(memDC, hbitmap_);
    BLENDFUNCTION blendFunc;
    blendFunc.BlendOp             = AC_SRC_OVER;
    blendFunc.BlendFlags          =   0;
    blendFunc.SourceConstantAlpha = 255;
    blendFunc.AlphaFormat         = AC_SRC_ALPHA;
    dc.AlphaBlend
    (
        dleft, dtop, dwidth, dheight,
        &memDC,
        sleft, stop, swidth, sheight,
        blendFunc
    );    
    ::SelectObject(memDC, oldBmp);
}


/**
 *  This function alpha draws the DibSection to a device context in the given
 *  rectangle.
 *
 *  @param dc       The device to draw on.
 *  @param extents  The drawing extents.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::drawAlpha(CDC &dc, CRect const &extents) const
{
	BW_GUARD;

    drawAlpha(dc, extents.left, extents.top, extents.Width(), extents.Height());
}


/**
 *  This function alpha draws a sub-image of the DibSection to a device context 
 *  in the given rectangles.
 *
 *  @param dc       The device to draw on.
 *  @param dextents The destination extents.
 *  @param sextents The source extents.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::drawAlpha(CDC &dc, CRect const &dextents, CRect const &sextents) const
{
	BW_GUARD;

    drawAlpha
    (
        dc, 
        dextents.left, dextents.top, dextents.Width(), dextents.Height(),
        sextents.left, sextents.top, sextents.Width(), sextents.Height()
    );
}


/**
 *    This converts the DibSection to a grey-scale image.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::toGreyScale()
{
    MF_ASSERT(0); // defined in specializations
}


/**
 *  Create the DIBSECTION, the image buffer etc.
 *
 *  @param w        Width of the requested buffer.
 *  @param h        Height of the requested buffer.
 *  @param buffer   The created buffer.
 *  @param owns     Should we delete the memory in the destructor?
 *  @param stride   The stride between rows.
 *  @param flipped  Is the image upside down?
 *  @returns        True if the buffer could be created.
 */
template<typename PIXELTYPE>
inline /*virtual*/ bool 
controls::DibSection<PIXELTYPE>::createBuffer
(
    uint32      w, 
    uint32      h,
    PixelType   *&buffer,
    bool        &owns,
    size_t      &stride,
    bool        &flipped
)
{
    MF_ASSERT(0); // defined in specializations

    return false;
}


/**
 *  This copys from other to this DibSection.
 *
 *  @param other            The other image to copy from.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::copy(DibSection const &other)
{
	BW_GUARD;

    destroy();
    if (!other.isEmpty())
    {
        Base::copy(other);
        ::memcpy(&bmi_, &other.bmi_, sizeof(bmi_));
    }
}


/**
 *  This cleans up the resources owned by the image.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::destroy()
{
	BW_GUARD;

    if (hbitmap_ != NULL)
    {
        ::DeleteObject(hbitmap_);
        hbitmap_ = NULL;
        ::ZeroMemory(&bmi_, sizeof(bmi_));
    }
    Base::destroy();
}


/**
 *  This tries to load the DibSection from a .dds file.
 *
 *  @param filename     The file to load.
 *  @returns            True if successfully loaded.
 */
template<typename PIXELTYPE>
inline bool controls::DibSection<PIXELTYPE>::loadDDS(BW::string const &filename)
{
	BW_GUARD;

    D3DXIMAGE_INFO imgInfo;
    if ( !SUCCEEDED(D3DXGetImageInfoFromFile(bw_utf8tow(filename).c_str(), &imgInfo)))
        return false;

    DX::Texture* texture = NULL;
    bool ok = 
        SUCCEEDED
        ( 
            D3DXCreateTextureFromFileEx
            (
                Moo::rc().device(),
                bw_utf8tow(filename).c_str(),
                imgInfo.Width, imgInfo.Height,
                1,                      // mip-map levels
                0,                      // usage
                D3DFMT_A8R8G8B8,
                D3DPOOL_SYSTEMMEM,
                D3DX_FILTER_TRIANGLE,
                D3DX_DEFAULT,           // mip-map filter
                0,                      // colour key
                NULL,                   // D3DXIMAGE_INFO
                NULL,                   // palette
                &texture  
            ) 
        );

    if (!ok)
        return false;
    ok = copyFromA8R8G8B8Texture(texture);

    texture->Release(); texture = NULL;

    return ok;
}


/**
 *  This copies the texture, which is in A8R8G8B8 format, to the DibSection.
 *
 *  @param texture      The texture to copy.
 *  @returns            True if succesfull.
 */
template<typename PIXELTYPE>
inline bool controls::DibSection<PIXELTYPE>::copyFromA8R8G8B8Texture(DX::Texture *texture)
{
	BW_GUARD;

    D3DSURFACE_DESC description;
    if (!SUCCEEDED(texture->GetLevelDesc(0, &description)))
        return false;

    ComObjectWrap<DX::Surface> surface;
    if (description.Usage == D3DUSAGE_RENDERTARGET)
    {// it is an rtt
        Moo::rc().device()->CreateOffscreenPlainSurface( description.Width,
            description.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, NULL );
        ComObjectWrap<DX::Surface> rtSurface;
        texture->GetSurfaceLevel( 0, &rtSurface );
        if (!rtSurface.hasComObject())
        {
            return false;
        }
        if (FAILED( Moo::rc().device()->GetRenderTargetData( &*rtSurface, &*surface ) ))
        {
            return false;
        }
    }
    else
    {
        texture->GetSurfaceLevel( 0, &surface );
        if (!surface.hasComObject())
        {
            return false;
        }
    }

    bool ok = true;

    D3DLOCKED_RECT lockedRect;
    ok = SUCCEEDED(surface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY));
    if (!ok)
        return false;

    resize(description.Width, description.Height);

    copyFromLockedRect(lockedRect, width(), height());

    ok = SUCCEEDED(surface->UnlockRect());
    if (!ok)
    {
        clear();
        return false;
    } 
    return true;
}


/**
 *  This copies the surface, which is in A8R8G8B8 format, to the DibSection.
 *
 *  @param surface      The surface to copy.
 *  @returns            True if succesfull.
 */
template<typename PIXELTYPE>
inline bool controls::DibSection<PIXELTYPE>::copyFromA8R8G8B8Surface(IDirect3DSurface9 *surface)
{
	BW_GUARD;

    D3DSURFACE_DESC description;
    if (!SUCCEEDED(surface->GetDesc(&description)))
        return false;

    bool ok = true;

    D3DLOCKED_RECT lockedRect;
    ok = SUCCEEDED(surface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY));
    if (!ok)
        return false;

    resize(description.Width, description.Height);

    copyFromLockedRect(lockedRect, width(), height());

    ok = SUCCEEDED(surface->UnlockRect());
    if (!ok)
    {
        clear();
        return false;
    } 
    return true;
}


/**
 *  This copies the DibSection from a D3DLOCKED_RECT.  We assume that the
 *  image has been resized correctly.
 */
template<typename PIXELTYPE>
inline void controls::DibSection<PIXELTYPE>::copyFromLockedRect
(
    D3DLOCKED_RECT      const &lockedRect,
    uint32              w,
    uint32              h
)
{
    MF_ASSERT(0); // defined in specializations
}


BW_END_NAMESPACE

#endif // CONTROLS_DIB_SECTION_IPP
