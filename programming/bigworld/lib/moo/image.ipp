#ifndef MOO_IMAGE_IPP
#define MOO_IMAGE_IPP


namespace Moo
{
    /** 
     *  This is the default constructor for a Image.
     *
	 *	@param allocator	A string describing the allocator of this object.
     */
    template<typename TYPE>
    inline Image<TYPE>::Image(const BW::string& allocator):
        pixels_(NULL),
        lines_(NULL),
        width_(0),
        height_(0),
        owns_(true),
        stride_(0)
#if ENABLE_RESOURCE_COUNTERS
		, allocator_(allocator)
#endif
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif
    }


    /**
     *  This constructs a Image of the given size.
     *
     *  @param w			The width of the image.
     *  @param h			The height of the image.
	 *	@param allocator	A string describing the allocator of this object.
     */
    template<typename TYPE>
    inline Image<TYPE>::Image(uint32 w, uint32 h, const BW::string& allocator):
        pixels_(NULL),
        lines_(NULL),
        width_(0),
        height_(0),
        owns_(true),
        stride_(0)
#if ENABLE_RESOURCE_COUNTERS
		, allocator_(allocator)
#endif
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif

		resize(w, h);
    }


    /**
     *  This constructs a Image of the given size.
     *
     *  @param w			The width of the image.
     *  @param h			The height of the image.
     *  @param value		The value to initialise the image with.
	 *	@param allocator	A string describing the allocator of this object.
     */
    template<typename TYPE>
    inline Image<TYPE>::Image(uint32 w, uint32 h, TYPE const &value, const BW::string& allocator):
        pixels_(NULL),
        lines_(NULL),
        width_(0),
        height_(0),
        owns_(true),
        stride_(0)
#if ENABLE_RESOURCE_COUNTERS
		, allocator_(allocator)
#endif
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif

        resize(w, h, value);
    }


    /**
     *  This constructor allows for pretending an existing bit of memory is 
     *  an image.
     *
     *  @param w			The width of the image.
     *  @param h			The height of the image.
     *  @param pixels		The image's pixels from left to right, top to 
     *						bottom.
     *  @param owns			If true then the pixels are deleted in the 
     *						destructor,
     *						if false then the pixels are left alone.
     *  @param stride		The distance between lines in bytes.  If zero then
     *						this will be sizeof(PixelType)*w.
     *  @param flipped		If true then the image is flipped in the 
     *						y-direction (e.g. like a Windows DIB section).
	 *	@param allocator	A string describing the allocator of this object.
     */
    template<typename TYPE>
    inline Image<TYPE>::Image
    (
        uint32				w, 
        uint32				h, 
        PixelType			*pixels, 
        bool				owns        /*= true*/,
        size_t				stride      /*= 0*/,
        bool				flipped     /*= false*/,
		const BW::string&	allocator
    ):
        pixels_(NULL),
        lines_(NULL),
        width_(0),
        height_(0),
        owns_(owns),
        stride_(0)
#if ENABLE_RESOURCE_COUNTERS
		, allocator_(allocator)
#endif
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif

        resize(w, h, pixels, owns, stride, flipped);
    }


    /**
     *  This is the copy constructor for Images.
     *
     *  @param other    The image to copy from.
     */
    template<typename TYPE>
    inline Image<TYPE>::Image(Image const &other):
		BW::ReferenceCount( other ),
        pixels_(NULL),
        lines_(NULL),
        width_(0),
        height_(0),
        owns_(true),
        stride_(0)
#if ENABLE_RESOURCE_COUNTERS
		, allocator_(other.allocator_)
#endif
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif

        copy(other);
    }


    /**
     *  This is the destructor for Images.
     */
    template<typename TYPE>
    inline Image<TYPE>::~Image()
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_SUB(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif

        destroy();
    }


        /**
         *  This is the assignment operator for Images.
         *  
         *  @param other    The image to copy from.
         */
    template<typename TYPE>
    inline Image<TYPE> &Image<TYPE>::operator=(Image const &other)
    {
        BW_GUARD;
		if (this != &other)
		{
#if ENABLE_RESOURCE_COUNTERS
			// Track memory usage
			RESOURCE_COUNTER_SUB(	ResourceCounters::DescriptionPool(allocator_,
									(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
									(uint)( sizeof(*this) + allocator_.capacity() ), 0)
			
			allocator_ = other.allocator_;

			// Track memory usage
			RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
									(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
									(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif

            copy(other);
		}
        return *this;
    }


    /**
     *  This function returns true if the image does not yet represent 
     *  anything.
     *
     *  @returns        True if the image is empty.
     */
    template<typename TYPE>
    inline bool Image<TYPE>::isEmpty() const
    {
        return pixels_ == NULL;
    }


    /**
     *  This method frees the resources allocated by the image
     */
    template<typename TYPE>
    inline void Image<TYPE>::clear()
    {
        BW_GUARD;
		destroy();
    }


    /**
     *  This function fills the image with a particular 'colour'.
     */
    template<typename TYPE>
    inline void Image<TYPE>::fill(PixelType const &value)
    {
        BW_GUARD;
		for (uint32 y = 0; y < height_; ++y)
        {
            PixelType *p = lines_[y];
            PixelType *q = p + width_;
            for (; p != q; ++p)
                *p = value;
        }
    }


    /**
     *  This function returns the width of the image.
     *
     *  @returns        The width of the image, 0 if the image is empty.
     */
    template<typename TYPE>
    inline uint32 Image<TYPE>::width() const
    {
        return width_;
    }


    /**
     *  This function returns the height of the image.
     *
     *  @returns        The height of the image, 0 if the image is empty.
     */
    template<typename TYPE>
    inline uint32 Image<TYPE>::height() const
    {
        return height_;
    }


	/** 
	 *	This function returns true if the given point is inside the image.
	 *
	 *	@param x		The x coordinate.
	 *	@param y		The y coordinate.
	 *	@returns		True if the point is inside the image.
	 */
	template<typename TYPE>
	inline bool Image<TYPE>::contains(int x, int y) const
	{
		BW_GUARD;
		if (isEmpty())
			return false;
		else
			return 0 <= x && x < (int)width_ && 0 <= y && y < (int)height_;
	}


    /**
     *  This function returns the address of the first elevation value in 
     *  the given row.  Elevation values in the row are guaranteed to be 
     *  linearly accessible from this pixel.
     *
     *  @param y        The y coordinate of the row to get.
     *  @returns        The address of the first pixel in row y.  
     */
    template<typename TYPE>
    inline const TYPE * Image<TYPE>::getRow(uint32 y) const
    {
        return lines_[y];
    }


    /**
     *  This function returns the address of the first elevation value in 
     *  the given row.  Elevation values in the row are guaranteed to be 
     *  linearly accessible from this pixel.
     *
     *  @param y        The y coordinate of the row to get.
     *  @returns        The address of the first pixel in row y.  
     */
    template<typename TYPE>
    inline TYPE *Image<TYPE>::getRow(uint32 y)
    {
        return lines_[y];
    }


    /**
     *  This safely gets the value of a pixel at (x, y).  Safe means 
     *  boundary clamping wrapping conditions.
     *
     *  @param x        The x coordinate.
     *  @param y        The y coordinate.
     *  @returns        The value of the image at (x, y).
     */
    template<typename TYPE>
    inline typename Image<TYPE>::PixelType
		Image<TYPE>::get(int x, int y) const
    {
        x = Math::clamp(0, x, int(width_)  - 1);
        y = Math::clamp(0, y, int(height_) - 1);
        return lines_[y][x];
    }


    /**
     *  This safely sets the value of a pixel at (x, y).  Safe means if x 
     *  or y are outside the valid range then nothing is done.
     *
     *  @param x        The x coordinate.
     *  @param y        The y coordinate.
	 *	@param value	The value to assign to the specified co-ordinates.
     */
    template<typename TYPE>
    inline void Image<TYPE>::set(int x, int y, PixelType const &value)
    {
        if (0 <= x && x < (int)width_ && 0 <= y && y < (int)height_)
            lines_[y][x] = value;
    }



	/**
	 *	This safely gets the value of a pixel at (x, y) using bilinear
	 *	interpolation.  Safe means boundary clamping wrapping conditions.
	 *	It assumes that you can multiply a pixel by a scalar and add pixels
	 *	to interpolate them.
	 *
     *  @param x        The x coordinate.
     *  @param y        The y coordinate.
	 *	@returns		The bilinear value at (x, y) using clamping wrapping
	 *					conditions.
	 */
	template<typename TYPE>
    inline typename Image<TYPE>::PixelType Image<TYPE>::getBilinear(float x, float y) const
    {
		BW_GUARD;
		int   ix = (int)::floorf(x);
        int   iy = (int)::floorf(y);
		float fx = x - ix;
		float fy = y - iy;

		TYPE p00 = get(ix    , iy    );
		TYPE p10 = get(ix + 1, iy    );
		TYPE p01 = get(ix    , iy + 1);
		TYPE p11 = get(ix + 1, iy + 1);

		float i1 = p00 + fx*(p10 - p00);
		float i2 = p01 + fx*(p11 - p01);

		return (TYPE)(i1 + fy*(i2 - i1));
	}


    /**
     *  This safely gets the value of a pixel at (x, y) using bicubic 
     *  interpolation.  Safe means boundary clamping wrapping conditions.  
     *  It assumes that you can multiply a pixel by a scalar and add pixels 
     *  to interpolate them.
     *
     *  @param x        The x coordinate.
     *  @param y        The y coordinate.
     *  @returns        The value of the image at (x, y).
     */
    template<typename TYPE>
    inline typename Image<TYPE>::PixelType Image<TYPE>::getBicubic(float x, float y) const
    {
        BW_GUARD;
		int ix = (int)Math::clamp(0.0f, x, width_  - 1.0f);
        int iy = (int)Math::clamp(0.0f, y, height_ - 1.0f);

        TYPE p00 = this->get(ix - 1, iy - 1);
        TYPE p01 = this->get(ix    , iy - 1);
        TYPE p02 = this->get(ix + 1, iy - 1);
        TYPE p03 = this->get(ix + 2, iy - 1);

        TYPE p10 = this->get(ix - 1, iy    );
        TYPE p11 = this->get(ix    , iy    );
        TYPE p12 = this->get(ix + 1, iy    );
        TYPE p13 = this->get(ix + 2, iy    );

        TYPE p20 = this->get(ix - 1, iy + 1);
        TYPE p21 = this->get(ix    , iy + 1);
        TYPE p22 = this->get(ix + 1, iy + 1);
        TYPE p23 = this->get(ix + 2, iy + 1);

        TYPE p30 = this->get(ix - 1, iy + 2);
        TYPE p31 = this->get(ix    , iy + 2);
        TYPE p32 = this->get(ix + 1, iy + 2);
        TYPE p33 = this->get(ix + 2, iy + 2);

        float dx = x - floorf(x);
        float dy = y - floorf(y);

        float kx[4], ky[4];
        for (int i = 0; i <= 3; ++i)
        {
            kx[i] = bicubicKernel(i - 1 - dx);
            ky[i] = bicubicKernel(i - 1 - dy);
        }

        float sum = 0.0;

        sum += p00*kx[0]*ky[0];
        sum += p01*kx[1]*ky[0];
        sum += p02*kx[2]*ky[0];
        sum += p03*kx[3]*ky[0];

        sum += p10*kx[0]*ky[1];
        sum += p11*kx[1]*ky[1];
        sum += p12*kx[2]*ky[1];
        sum += p13*kx[3]*ky[1];

        sum += p20*kx[0]*ky[2];
        sum += p21*kx[1]*ky[2];
        sum += p22*kx[2]*ky[2];
        sum += p23*kx[3]*ky[2];

        sum += p30*kx[0]*ky[3];
        sum += p31*kx[1]*ky[3];
        sum += p32*kx[2]*ky[3];
        sum += p33*kx[3]*ky[3];

		return (TYPE)sum;
	}


    /**
     *  This safely gets the value of a pixel at (x, y) using bicubic 
     *  interpolation.  Safe means boundary clamping wrapping conditions.  
     *  It assumes that you can multiply a pixel by a scalar and add pixels 
     *  to interpolate them.
     *
     *  This returns double as a hack.  It's needed for extra precision in
     *  terrain import.  To get rid of it we should define a traits class
     *  that can be used to override the return type of this and the float
     *  version.
     *
     *  @param x        The x coordinate.
     *  @param y        The y coordinate.
     *  @returns        The value of the image at (x, y).
     */
    template<typename TYPE>
    inline double Image<TYPE>::getBicubic(double x, double y) const
    {
        BW_GUARD;
		int ix = (int)Math::clamp(0.0, x, width_  - 1.0);
        int iy = (int)Math::clamp(0.0, y, height_ - 1.0);

        TYPE p00 = this->get(ix - 1, iy - 1);
        TYPE p01 = this->get(ix    , iy - 1);
        TYPE p02 = this->get(ix + 1, iy - 1);
        TYPE p03 = this->get(ix + 2, iy - 1);

        TYPE p10 = this->get(ix - 1, iy    );
        TYPE p11 = this->get(ix    , iy    );
        TYPE p12 = this->get(ix + 1, iy    );
        TYPE p13 = this->get(ix + 2, iy    );

        TYPE p20 = this->get(ix - 1, iy + 1);
        TYPE p21 = this->get(ix    , iy + 1);
        TYPE p22 = this->get(ix + 1, iy + 1);
        TYPE p23 = this->get(ix + 2, iy + 1);

        TYPE p30 = this->get(ix - 1, iy + 2);
        TYPE p31 = this->get(ix    , iy + 2);
        TYPE p32 = this->get(ix + 1, iy + 2);
        TYPE p33 = this->get(ix + 2, iy + 2);

        double dx = x - floor(x);
        double dy = y - floor(y);

        double kx[4], ky[4];
        for (int i = 0; i <= 3; ++i)
        {
            kx[i] = bicubicKernel(i - 1 - dx);
            ky[i] = bicubicKernel(i - 1 - dy);
        }

        double sum = 0;

        sum += p00*kx[0]*ky[0];
        sum += p01*kx[1]*ky[0];
        sum += p02*kx[2]*ky[0];
        sum += p03*kx[3]*ky[0];

        sum += p10*kx[0]*ky[1];
        sum += p11*kx[1]*ky[1];
        sum += p12*kx[2]*ky[1];
        sum += p13*kx[3]*ky[1];

        sum += p20*kx[0]*ky[2];
        sum += p21*kx[1]*ky[2];
        sum += p22*kx[2]*ky[2];
        sum += p23*kx[3]*ky[2];

        sum += p30*kx[0]*ky[3];
        sum += p31*kx[1]*ky[3];
        sum += p32*kx[2]*ky[3];
        sum += p33*kx[3]*ky[3];

        return sum;
    }


    /**
     *  Resize the underlying image.  Existing pixels are not preserved.
     *
     *  @param w        The new width.
     *  @param h        The new height.
     */
    template<typename TYPE>
    inline void Image<TYPE>::resize(uint32 w, uint32 h)
    {
		BW_GUARD;
		if (w != width() || h != height())
		{
			destroy();

			if (w != 0 && h != 0)
			{
				PixelType *newBuf = NULL;
				bool owns, flipped;
				size_t stride;
				createBuffer(w, h, newBuf, owns, stride, flipped);
				initInternal(w, h, newBuf, owns, stride, flipped);
			}
		}
    }


	/**
	 *  Resize the underlying image.  Existing pixels are not preserved.
	 *
	 *  @param w        The new width.
	 *  @param h        The new height.
	 *  @param value    The new value to put into the image.
	 */
    template<typename TYPE>
    inline void Image<TYPE>::resize(uint32 w, uint32 h, PixelType const &value)
    {
        BW_GUARD;
		destroy();

        if (w != 0 && h != 0)
        {
            PixelType *newBuf = NULL;
            bool owns, flipped;
            size_t stride;
            createBuffer(w, h, newBuf, owns, stride, flipped);
            initInternal(w, h, newBuf, owns, stride, flipped);
            for (uint32 y = 0; y < h; ++y)
            {
                PixelType *p = getRow(y);
                PixelType *q = p + w;
                for ( ; p != q; ++p)
                    *p = value;
            }
        }
    }


    /**
     *  Resize the underlying image.  Existing pixels are not preserved.
     *
     *  @param w        The new width.
     *  @param h        The new height.
     *  @param pixels   The new pixel data.
     *  @param owns     Does the image delete pixels in the dtor?
     *  @param stride   The distance between lines in bytes.  If zero then
     *                  this will be sizeof(PixelType)*w.
     *  @param flipped  If true then the image is flipped in the 
     *                  y-direction (e.g. like a Windows DIB section).
     */
    template<typename TYPE>
    inline void Image<TYPE>::resize
    (
        uint32      w, 
        uint32      h, 
        PixelType   *pixels, 
        bool        owns,
        size_t      stride      /* = 0*/,
        bool        flipped     /* = false*/
    )
    {
        BW_GUARD;
		init(w, h, pixels, owns, stride, flipped);
    }


    /**
     *  Copy from a sub-image.  We assume that the coordinates in the
     *  sub-image are valid.  We clip in the destination image.
     *
	 *	@param other	Source Image to copy from.
     *  @param srcX     The source x coordinate.
     *  @param srcY     The source y coordinate.
     *  @param srcW     The source width, if -1 then the source's width is
     *                  used.
     *  @param srcH     The source height if -1 then the source's height is
     *                  used.
     *  @param dstX     The destination x coordinate.
     *  @param dstY     The destination y coordinate.
     */
    template<typename TYPE>
    inline void 
    Image<TYPE>::blit
    (
        Image           const &other, 
        uint32          srcX, 
        uint32          srcY, 
        uint32          srcW, 
        uint32          srcH,
        int32           dstX,
        int32           dstY
    )
    {     
        BW_GUARD;
		if (srcW == (uint32)-1) 
            srcW = other.width();
        if (srcH == (uint32)-1) 
            srcH = other.height();

        // Check if totally clipped in the destination:
        if (((dstX + int32(srcW)) < 0) || ((dstY + int32(srcH) < 0)))
		{
			return;
		}
        if (dstX >= (int32)width_ || dstY >= (int32)height_)
		{
			return;
		}

        // The clipping below has not been tested.

        // Clip the destination coordinates:
        if (dstX < 0)
        {
            srcW += dstX;
            dstX = 0;
        }
        if (dstY < 0)
        {
            srcH += dstY;
            dstY = 0;
        }
        // Clip the width and height:
        if (srcX + srcW > width_)
		{
            srcW = width_ - srcX;
		}
        if (srcY + srcH > height_)
		{
            srcH = height_ - srcY;
		}

        size_t sz = sizeof(PixelType)*srcW;
        for (uint32 y = 0; y < srcH; ++y)
        {
            TYPE const *src = other.getRow(srcY + y) + srcX;
            TYPE       *dst = getRow(dstY + y) + dstX;
            ::memcpy(dst, src, sz);
        }
    }


    /**
     *  This function flips the image around the given axis.
     *
     *  @param flipX    If true then flip horizontally, if false then flip
     *                  vertically.
     */
    template<typename TYPE>
    inline void Image<TYPE>::flip(bool flipX)
    {
    	BW_GUARD;
		// This code has not been fully tested.

        if (isEmpty())
            return;

        if (flipX)
        {
            for (uint32 y = 0; y < height_; ++y)
            {
                PixelType *p = getRow(y);
                PixelType *q = p + width_ - 1;
                for (uint32 x = 0; x < width_/2; ++x, ++p, --q)
                {
                    std::swap(*p, *q);
                }
            }
        }
        else
        {
            for (uint32 x = 0; x < width_; ++x)
            {
                for (uint32 y = 0; y < height_/2; ++y)
                {
                    std::swap(lines_[y][x], lines_[height_ - y - 1][x]);
                }
            }
        } 
    }


    /**
     *  This function rotates the image.
     *
     *  @param clockwise    If true then rotate clockwise, if false then 
     *                      flip anticlockwise.
     */
    template<typename TYPE>
    inline void Image<TYPE>::rotate(bool clockwise)
    {
    	BW_GUARD;
		// This code has not been fully tested.

        if (isEmpty())
            return;

        PixelType *result = NULL;
        bool owns, flipped;
        size_t stride;
        createBuffer(height_, width_, result, owns, stride, flipped);
        if (clockwise)
        {
            for (uint32 y = 0; y < height_; ++y)
            {
                PixelType const *p = lines_[y];
                PixelType *q       = result + (height_ - 1) - y;
                for (uint32 x = 0; x < width_; ++x)
                {
                    *q = *p;
                    ++p;
                    q += height_;
                }
            }
        }
        else
        {
            for (uint32 y = 0; y < height_; ++y)
            {
                PixelType const *p = lines_[y];
                PixelType *q       = result + height_*(width_ - 1) + y;
                for (uint32 x = 0; x < width_; ++x)
                {
                    *q = *p;
                    ++p;
                    q -= height_;
                }
            }
        }
        initInternal(height_, width_, result, owns, stride, flipped);
    }


    /**
     *  This function writes the image as a CSV file to disk.  It's useful 
     *  for debugging.
     *
     *  @param filename     The file to write the image to.
     */
    template<typename TYPE>
    inline void Image<TYPE>::toCSV(BW::string const &filename) const
    {
		BW_GUARD;
		// TODO: Fix this for server
#ifndef MF_SERVER
        std::ofstream out(filename.c_str());
        for (uint32 y = 0; y < height(); ++y)
        {
            PixelType const *p = getRow((int)y);
            for (uint32 x = 0; x < width(); ++x, ++p)
            {
                out << *p << ", ";
            }
            out << std::endl;
        }
        out.close();
#endif
    }


	/**
	 *	This function copies the data to a contiguous memory block.
	 *
	 *	@param contiguousMemBuffer	The buffer to store the image into.
	 */
	template<typename TYPE>
	inline void Image<TYPE>::copyTo(PixelType* contiguousMemBuffer) const
	{	
        BW_GUARD;
		for (uint32 y = 0; y < height(); ++y)
        {
            PixelType const *row = getRow(y);
		    ::memcpy(contiguousMemBuffer, row, sizeof(PixelType)*width()); 
            contiguousMemBuffer += width();
        }
	}


	/**
	 *	This function copies the data from a contiguous memory block.
	 *	You must reserve enough space in the image first.
	 *
	 *	@param contiguousMemBuffer	The buffer to copy the image from.
	 */
	template<typename TYPE>
	inline void Image<TYPE>::copyFrom(PixelType const* contiguousMemBuffer)
	{	
        BW_GUARD;
		for (uint32 y = 0; y < height(); ++y)
        {
            PixelType *row = getRow(y);
		    ::memcpy(row, contiguousMemBuffer, sizeof(PixelType)*width());
            contiguousMemBuffer += width();
        }
	}


    /**
     *  This method returns how much memory is used by one row of data.
     *  This will be greater than or equal to sizeof(PixelType)*width(),
     *  since we allow arbitrary strides.
	 *
	 *	@returns		The number of bytes that is needed to store one row.
     */
    template<typename TYPE>
    size_t Image<TYPE>::rawRowSize() const
    {
        return stride_;
    }


	/**
	 *	This method returns how much raw memory would be needed to copy
     *  the image's data to/from memory.  It does not include overhead
     *  such as member variables size.  In practice the amount of memory
     *  used internally by the image class can be larger since you can
     *  set an arbitrary stride.
	 *	This is the size of the contiguousMemoryBuffer used in the
	 *	copyTo and copyFrom methods.
	 *
	 *	@returns		The amount of memory that the image would take if
	 *					it was stored as a contiguous buffer.
	 */
	template<typename TYPE>
	inline size_t Image<TYPE>::rawDataSize() const
	{
		return (this->width() * this->height() * sizeof(PixelType));
	}


	/**
	 *	This returns true if the image is upside down in memory (i.e. like
	 *	a Windows DIB Section.
	 *
	 *	@returns		True if the image is stored upside down.
	 */
	template<typename TYPE>
	inline bool Image<TYPE>::isUpsideDown() const
	{
		if (isEmpty() || height() == 1)
			return false;
		return getRow(0) > getRow(1);
	}


    /**
     *  This is the image equality operator.
     *
     *  @param other        The other image.
     *  @returns            True if the images are exactly the same.
     */
    template<typename TYPE>
    inline bool Image<TYPE>::operator==(Image const &other) const
    {
        BW_GUARD;
		if (width_ != other.width_ || height_ != other.height_)
            return false;

        if (isEmpty()) // both images must be empty
            return true;

        size_t sz = sizeof(PixelType)*width_;
        for (uint32 y = 0; y < height_; ++y)
        {
            PixelType const *src1 = getRow(y);
            PixelType const *src2 = other.getRow(y);
            if (::memcmp(src1, src2, sz) != 0)
                return false;
        }

        return true;
    }


    /**
     *  This is the inequality operator.
     *
     *  @param other        The other image.
     *  @returns            True if the images are different in any way.
     */
    template<typename TYPE>
    inline bool Image<TYPE>::operator!=(Image const &other) const
    {
        BW_GUARD;
		return !operator==(other);
    }


    /**
     *  This is pixel equality operator.
     *  
     *  @param value        The value to test against.
     *  @returns            True if the entire image is equal to value.
     *                      If the image is empty then this returns false.
     */
    template<typename TYPE>
    inline bool Image<TYPE>::operator==(PixelType const &value) const
    {
        BW_GUARD;
		if (isEmpty())
            return false;

        for (size_t y = 0; y < height(); ++y)
        {
            PixelType const *p = getRow(y);
            PixelType const *q = p + width();
            for (; p != q; ++p)
            {
                if (*p != value)
                    return false;
            }
        }
        return true;
    }


    /**
     *  This is pixel inequality operator.
     *  
     *  @param value        The value to test against.
     *  @returns            False if the entire image is equal to value.
     *                      If the image is empty then this returns true.
     */
    template<typename TYPE>
    inline bool Image<TYPE>::operator!=(PixelType const &value) const
    {
        return !operator==(value);
    }


	/**
	 *	This gets the memory allocation pool.
	 *
	 *	@returns			The string id for the memory allocation pool.
	 */
	template<typename TYPE>
	inline BW::string Image<TYPE>::allocator() const
	{
#if ENABLE_RESOURCE_COUNTERS
		return allocator_;
#else
		return BW::string();
#endif
	}


    /**
     *  This creates a buffer of the appropriate size and return a pointer to
     *  the first pixel.
     *
     *  @param w        Width of the requested buffer.
     *  @param h        Height of the requested buffer.
     *  @param buffer   The created buffer.
     *  @param owns     Should we delete the memory in the destructor?
     *  @param stride   The stride between rows.
     *  @param flipped  Is the image upside down?
     *  @returns        True if the buffer could be created.
     */
    template<typename TYPE>
    /*virtual*/ bool
    Image<TYPE>::createBuffer
    (
        uint32      w, 
        uint32      h,
        PixelType   *&buffer,
        bool        &owns,
        size_t      &stride,
        bool        &flipped
    )
    {
        BW_GUARD;
		owns    = true;
        flipped = false;
        stride  = 0;
        if (w != 0 && h != 0)
        {
			// Track memory usage
			RESOURCE_COUNTER_ADD(
				ResourceCounters::DescriptionPool(allocator_+"_buffer", 
				(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
				(uint)(sizeof(PixelType) * w * h), 0)
            buffer = new PixelType[w*h];
            return true;
        }
        else
        {
            buffer = NULL;
            return false;
        }
    }


    /**
     *  This initialises the image with raw data.  It should only be used
     *  by derived classes where the image wraps some structure other than
     *  memory allocated by new.
     *
     *  @param w        The new width.
     *  @param h        The new height.
     *  @param pixels   The new pixel data.
     *  @param owns     Does the image delete pixels in the dtor?
     *  @param stride   The distance between lines in bytes.  If zero then
     *                  this will be sizeof(pixels)*w.
     *  @param flipped  If true then the image is flipped in the 
     *                  y-direction (e.g. like a Windows DIB section).
     */
    template<typename TYPE>
    void Image<TYPE>::init
    (
        uint32      w, 
        uint32      h, 
        PixelType   *pixels, 
        bool        owns, 
        size_t      stride, 
        bool        flipped        
    )
    {
        BW_GUARD;
		initInternal( w, h, pixels, owns, stride, flipped );

        if ( w != 0 && h != 0 )
        {
			if ( owns_ && pixels_ )
			{
				// Track memory usage
				RESOURCE_COUNTER_ADD(
					ResourceCounters::DescriptionPool(allocator_+"_buffer", 
					(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
					(uint)(sizeof(PixelType) * w * h), 0)
			}
        }
    }


	/**
	 * This sets the member data of the image class, destroying any previous
	 * pixel data.
	 *
	 *  @param w        The new width.
	 *  @param h        The new height.
	 *  @param pixels   The new pixel data.
	 *  @param owns     Does the image delete pixels in the dtor?
	 *  @param stride   The distance between lines in bytes.  If zero then
	 *                  this will be sizeof(pixels)*w.
 	 *  @param flipped  If true then the image is flipped in the 
	 *                  y-direction (e.g. like a Windows DIB section).
	 */
	template<typename TYPE>
	void Image<TYPE>::initInternal
	(
		uint32      w, 
		uint32      h, 
		PixelType   *pixels, 
		bool        owns, 
		size_t      stride, 
		bool        flipped        
	)
	{
		BW_GUARD;
		destroy();

		if (w != 0 && h != 0)
		{
			pixels_ = pixels;
			width_  = w;
			height_ = h;
			owns_   = owns;
			stride_ = (stride == 0) ? w*sizeof(PixelType) : stride;
			createLines(width_, height_, pixels_, stride_, flipped);
		}
	}


    /**
     *  This constructs the lines_ member.
     *
     *  @param w        The new width.
     *  @param h        The new height.
     *  @param pixels   The new pixel data.
     *  @param stride   The distance between lines in bytes.  If zero then
     *                  this will be sizeof(pixels)*w.
     *  @param flipped  If true then the image is flipped in the 
     *                  y-direction (e.g. like a Windows DIB section).
     */
    template<typename TYPE>
    void Image<TYPE>::createLines
    (
        uint32          w, 
        uint32          h, 
        PixelType       *pixels,
        size_t          stride,
        bool            flipped
    )
    {
		BW_GUARD;
		if (lines_)
		{
#if ENABLE_RESOURCE_COUNTERS
			// Track memory usage
			RESOURCE_COUNTER_SUB(
				ResourceCounters::DescriptionPool(allocator_ + "_lines", 
				(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
				(uint)((sizeof(PixelType*) * height_)), 0)
#endif
			bw_safe_delete_array(lines_);
		}
        
		if (w == 0 || h == 0)
        {
            lines_ = NULL;
            return;
        }

#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(
			ResourceCounters::DescriptionPool(allocator_ + "_lines", 
			(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
			(uint)((sizeof(PixelType*) * h)), 0)
#endif
        lines_ = new PixelType *[h];

		for (uint32 y = 0; y < h; ++y)
        {
            uint8 *addr = 
                flipped 
                    ? reinterpret_cast<uint8 *>(pixels) + (h - y - 1)*stride
                    : reinterpret_cast<uint8 *>(pixels) + y*stride;
            lines_[y] = reinterpret_cast<PixelType *>(addr);
        }
    }


    /**
     *  This copies from the other image.
     *
     *  @param other        The image to copy.
     */
    template<typename TYPE>
    inline void Image<TYPE>::copy(Image const &other)
    {
        BW_GUARD;
		destroy();
        if (!other.isEmpty())
        {
            PixelType *newBuf = NULL;
            bool owns, flipped;
            size_t stride;
            createBuffer(other.width_, other.height_, newBuf, owns, stride, 
						flipped);
            initInternal(other.width_, other.height_, newBuf, owns, stride, 
						flipped);
            for (uint32 y = 0; y < height_; ++y)
            {
                PixelType const *src = other.getRow(y);
                PixelType *dst       = getRow(y);
                ::memcpy(dst, src, width_*sizeof(PixelType));
            }
        }
    }


    /**
     *  This cleans up resources used by the image.
     */
    template<typename TYPE>
    inline void Image<TYPE>::destroy()
    {
        BW_GUARD;
		if (owns_)
		{
			if (pixels_)
			{
				// Track memory usage
				RESOURCE_COUNTER_SUB(
					ResourceCounters::DescriptionPool(allocator_ + "_buffer", 
					(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),				 
					(uint)((sizeof(PixelType) * width_ * height_)), 0)
				bw_safe_delete_array(pixels_);						   
			}
		}

		if (lines_)
		{
			// Track memory usage
			RESOURCE_COUNTER_SUB(
				ResourceCounters::DescriptionPool(allocator_ + "_lines", 
				(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
				(uint)((sizeof(PixelType*) * height_)), 0)
			bw_safe_delete_array(lines_);
		}
        width_ = height_ = 0U;
        stride_ = 0;
    }


    /**
     *  This returns the positive part of a number.
     *
     *  @param x            The number to get the positive part of.
     *  @returns            x if x >= 0,
     *                      0 if x <  0.
     */
    template<typename TYPE>
    inline float Image<TYPE>::positive(float x) const
    {
        return (x > 0.0f) ? x : 0.0f;
    }


    /**
     *  This returns the positive part of a number.
     *
     *  @param x            The number to get the positive part of.
     *  @returns            x if x >= 0,
     *                      0 if x <  0.
     */
    template<typename TYPE>
    inline double Image<TYPE>::positive(double x) const
    {
        return (x > 0.0) ? x : 0.0;
    }


    /**
     *  This calculates the bicubic kernel.
     *
     *  @param x            A number in the range [0.0f, 1.0f].
     *  @returns            The bicubic kernel for x.
     */
    template<typename TYPE>
    inline float Image<TYPE>::bicubicKernel(float x) const
    {
        float p0 = positive(x + 2.0f);
        float p1 = positive(x + 1.0f);
        float p2 = positive(x       );
        float p3 = positive(x - 1.0f);
        return ((p0*p0*p0) - 4.0f*(p1*p1*p1) + 6.0f*(p2*p2*p2) - 4*(p3*p3*p3))/6.0f; 
    }


    /**
     *  This calculates the bicubic kernel.
     *
     *  @param x            A number in the range [0.0, 1.0].
     *  @returns            The bicubic kernel for x.
     */
    template<typename TYPE>
    inline double Image<TYPE>::bicubicKernel(double x) const
    {
        double p0 = positive(x + 2.0);
        double p1 = positive(x + 1.0);
        double p2 = positive(x      );
        double p3 = positive(x - 1.0);
        return ((p0*p0*p0) - 4.0*(p1*p1*p1) + 6.0*(p2*p2*p2) - 4.0*(p3*p3*p3))/6.0; 
    }


    /**
     *  Specialisation of the Image class for booleans, we pack the booleans
     *  to eight booleans per byte.  At the moment this class is not used a
	 *	great deal, and so a lot of optimisations have not been made on it.
     */ 
    template<>
    class Image<bool> : public ReferenceCount
    {
    public:
        explicit Image(const BW::string& allocator = "unknown image<bool>");
        Image(uint32 w, uint32 h, const BW::string& allocator = "unknown image<bool>");
        Image(uint32 w, uint32 h, bool value, const BW::string& allocator = "unknown image<bool>");
        Image(uint32 w, uint32 h, bool *pixels, bool owns = true, const BW::string& allocator = "unknown image<bool>");
        Image(uint32 w, uint32 h, uint8 const *packedValues, const BW::string& allocator = "unknown image<bool>");
        Image(Image const &other);
        ~Image();
        Image &operator=(Image const &other);

        bool isEmpty() const;
        void clear();
		void fill(bool value);

        uint32 width() const;
        uint32 height() const;

		bool contains(int x, int y) const;

        const bool *getRow(uint32 y) const;
        bool *getRow(uint32 y);

        uint8 *getRawRow(unsigned int y);
        uint32 rawRowSize() const;     

        bool get(int x, int y) const;
        void set(int x, int y, bool value);

        void resize(uint32 w, uint32 h);
        void resize(uint32 w, uint32 h, bool value);
        void resize(uint32 w, uint32 h, bool *pixels, bool owns);
        void resize(uint32 w, uint32 h, uint8 const *packedValues);

        void 
        blit
        (
            Image<bool>     const &other, 
            uint32          srcX         = 0, 
            uint32          srcY         = 0, 
            uint32          srcW         = (uint32)-1, 
            uint32          srcH         = (uint32)-1,
            int32           dstX         = 0,
            int32           dstY         = 0
        );        

		size_t rawDataSize() const;

		bool isUpsideDown() const;

		void copyTo(bool* contiguousMemBuffer) const;
		void copyFrom(bool const* contiguousMemBuffer);

        bool operator==(Image<bool> const &other) const;
        bool operator!=(Image<bool> const &other) const;

        bool operator==(bool const &value) const;
        bool operator!=(bool const &value) const;

		BW::string allocator() const;

    protected:
        void copy(Image const &other);
        void destroy();

        void packRow() const;
        void unpackRow(uint32 y) const;

        static uint32 stride(uint32 w);

    private:
        uint8               *data_;
        uint32              width_;
        uint32              stride_;
        uint32              height_;
        mutable bool        *unpackedRowData_;
        mutable uint32      unpackedRow_;

#if ENABLE_RESOURCE_COUNTERS
		BW::string			allocator_;
#endif
    };


    inline Image<bool>::Image(const BW::string& allocator):
        data_(NULL),
        width_(0),
        stride_(0),
        height_(0),
        unpackedRowData_(NULL),
        unpackedRow_(0)
#if ENABLE_RESOURCE_COUNTERS
		, allocator_(allocator)
#endif
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif
    }


    inline Image<bool>::Image(uint32 w, uint32 h, const BW::string& allocator):
        data_(NULL),
        width_(0),
        stride_(0),
        height_(0),
        unpackedRowData_(NULL),
        unpackedRow_(0)
#if ENABLE_RESOURCE_COUNTERS
		, allocator_(allocator)
#endif
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif
		resize(w, h);
    }


    inline Image<bool>::Image(uint32 w, uint32 h, bool value, const BW::string& allocator):
        data_(NULL),
        width_(0),
        stride_(0),
        height_(0),
        unpackedRowData_(NULL),
        unpackedRow_(0)
#if ENABLE_RESOURCE_COUNTERS
		, allocator_(allocator)
#endif
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif
		resize(w, h, value);
    }


    inline Image<bool>::Image(uint32 w, uint32 h, bool *pixels, bool owns /*= true*/, const BW::string& allocator):
        data_(NULL),
        width_(w),
        stride_(Image<bool>::stride(w)),
        height_(h),
        unpackedRowData_(NULL),
        unpackedRow_(0)
#if ENABLE_RESOURCE_COUNTERS
		, allocator_(allocator)
#endif
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif
		resize(w, h, pixels, owns);
    }


    inline Image<bool>::Image(uint32 w, uint32 h, uint8 const *packedValues, const BW::string& allocator):
        data_(NULL),
        width_(w),
        stride_(Image<bool>::stride(w)),
        height_(h),
        unpackedRowData_(NULL),
        unpackedRow_(0)
#if ENABLE_RESOURCE_COUNTERS
		, allocator_(allocator)
#endif
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif
		resize(w, h, packedValues);
    }


    inline Image<bool>::Image(Image const &other):
		BW::ReferenceCount( other ),
        data_(NULL),
        width_(0),
        stride_(0),
        height_(0),
        unpackedRowData_(NULL),
        unpackedRow_(0)
#if ENABLE_RESOURCE_COUNTERS
		, allocator_(other.allocator_)
#endif
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif

		copy(other);
    }


    inline Image<bool>::~Image()
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_SUB(	ResourceCounters::DescriptionPool(allocator_,
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif

		destroy();
    }


    inline Image<bool> &Image<bool>::operator=(Image const &other)
    {
        BW_GUARD;
		if (this != &other)
		{
#if ENABLE_RESOURCE_COUNTERS
			// Track memory usage
			RESOURCE_COUNTER_SUB(	ResourceCounters::DescriptionPool(allocator_,
									(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
									(uint)( sizeof(*this) + allocator_.capacity() ), 0)

			allocator_ = other.allocator_;

			RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_,
									(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
									(uint)( sizeof(*this) + allocator_.capacity() ), 0)
#endif
			copy(other);
		}
        return *this;
    }


    inline bool Image<bool>::isEmpty() const
    {
        return data_ == NULL;
    }


    inline void Image<bool>::clear()
    {
        destroy();
    }


    inline void Image<bool>::fill(bool value)
    {
    	BW_GUARD;
		// This can be optimised quite a bit:
        for (uint32 y = 0; y < height(); ++y)
        {
            for (uint32 x = 0; x < width(); ++x)
            {
                set(x, y, value);
            }
        }
    }


    inline uint32 Image<bool>::width() const
    {
        return width_;
    }


    inline uint32 Image<bool>::height() const
    {
        return height_;
    }


	inline bool Image<bool>::contains(int x, int y) const
	{
		if (isEmpty())
			return false;
		else
			return 0 <= x && x < (int)width_ && 0 <= y && y < (int)height_;
	}


    inline const bool *Image<bool>::getRow(uint32 y) const
    {
        packRow();
        unpackRow(y);
        return unpackedRowData_;
    }


    inline bool *Image<bool>::getRow(uint32 y)
    {
        packRow();
        unpackRow(y);
        return unpackedRowData_;
    }


    inline uint8 *Image<bool>::getRawRow(unsigned int y)
    {
        packRow();
        if (y >= height_ - 1)
            y = height_ - 1;
        return data_+ y*stride_;
    }


    inline uint32 Image<bool>::rawRowSize() const
    {
        return stride_;
    }


    inline bool Image<bool>::get(int x, int y) const
    {
        BW_GUARD;
		packRow();
        x = Math::clamp(0, x, (int)width_  - 1);
        y = Math::clamp(0, y, (int)height_ - 1);
        uint32 hx = x >> 3;
        uint32 mx = (x & 0x07);
        return (data_[y*stride_ + hx] & (1 << mx)) != 0;
    }


    inline void Image<bool>::set(int x, int y, bool value)
    {
        BW_GUARD;
		packRow();
        x = Math::clamp(0, x, (int)width_  - 1);
        y = Math::clamp(0, y, (int)height_ - 1);
        uint32 hx = x >> 3;
        uint8  mx = (uint8)(x & 0x07);
        if (value)
            data_[y*stride_ + hx] |= (1 << mx);
        else
            data_[y*stride_ + hx] &= ~(1 << mx);
    }


    inline void Image<bool>::resize(uint32 w, uint32 h)
    {
        BW_GUARD;
		resize(w, h, false);
    }


    inline void Image<bool>::resize(uint32 w, uint32 h, bool value)
    {
        BW_GUARD;
		destroy();
        width_  = w;
        height_ = h;
        stride_ = Image<bool>::stride(w);

#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool(allocator_, (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
							 (uint)(stride_*height_), 0)
#endif
        data_   = new uint8[stride_*height_];
        ::memset(data_, value ? 0xff : 0x00, stride_*height_);
    }


    inline void Image<bool>::resize(uint32 w, uint32 h, bool *pixels, bool /*owns*/)
    {
        destroy();
        width_  = w;
        stride_ = Image<bool>::stride(w);
        height_ = h;
		
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool(allocator_, (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
							 (uint)(stride_*height_), 0)
#endif
        data_   = new uint8[stride_*height_];
		for (uint32 y = 0; y < h; ++y)
        {
            for (uint32 x = 0; x < w; ++x)
                set(x, y, *pixels++);
        }
    }


    inline void Image<bool>::resize(uint32 w, uint32 h, uint8 const *packedValues)
    {
        BW_GUARD;
		destroy();
        width_  = w;
        stride_ = Image<bool>::stride(w);
        height_ = h;

#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool(allocator_, (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
							 (uint)(stride_*height_), 0)
#endif
        data_   = new uint8[stride_*height_];
		::memcpy(data_, packedValues, stride_*height_);
    }


    inline void 
    Image<bool>::blit
    (
        Image<bool>     const &other, 
        uint32          srcX         /*= 0*/, 
        uint32          srcY         /*= 0*/, 
        uint32          srcW         /*= (uint32)-1*/, 
        uint32          srcH         /*= (uint32)-1*/,
        int32           dstX         /*= 0*/,
        int32           dstY         /*= 0*/
    )
    {
        if (srcW == (uint32)-1) 
            srcW = other.width();
        if (srcH == (uint32)-1) 
            srcH = other.height();

        // Check if totally clipped in the destination:
        if (((dstX + int32(srcW)) < 0) || ((dstY + int32(srcH)) < 0))
		{
			return;
		}
        if (dstX >= (int32)width_ || dstY >= (int32)height_)
		{
			return;
		}

        // The clipping below has not been tested.

        // Clip the destination coordinates:
        if (dstX < 0)
        {
            srcW += dstX;
            dstX = 0;
        }
        if (dstY < 0)
        {
            srcH += dstY;
            dstY = 0;
        }
        // Clip the width and height:
        if (srcX + srcW >= width_)
            srcW = width_ - srcX;
        if (srcY + srcH >= height_)
            srcH = height_ - srcY;

        for (uint32 y = 0; y < srcH; ++y)
        {
            for (uint32 x = 0; x < srcW; ++x)
            {
                bool v = other.get(srcX + x, srcY + y);
                set(dstX + x, dstY + y, v);
            }
        }
    }


    inline bool Image<bool>::operator==(Image<bool> const&other) const
    {
        BW_GUARD;
		if (width_ != other.width_ || height_ != other.height_)
            return false;

        for (uint32 y = 0; y < height_; ++y)
        {
            for (uint32 x = 0; x < width_; ++x)
            {
                if (get(x, y) != other.get(x, y))
                    return false;
            }
        }
        return true;
    }


    inline bool Image<bool>::operator!=(Image<bool> const&other) const
    {
        return !operator==(other);
    }


	inline BW::string Image<bool>::allocator() const
	{
#if ENABLE_RESOURCE_COUNTERS
		return allocator_;
#else
		return BW::string();
#endif
	}


    inline bool Image<bool>::operator==(bool const&value) const
    {
    	BW_GUARD;
		// This can be optimised quite a bit more if necessary
        if (isEmpty())
            return false;

        for (uint32 y = 0; y < height_; ++y)
        {
            for (uint32 x = 0; x < width_; ++x)
            {
                if (get(x, y) != value)
                    return false;
            }
        }
        return true;
    }


    inline bool Image<bool>::operator!=(bool const&value) const
    {
        return !operator==(value);
    }


    inline void Image<bool>::copy(Image const &other)
    {
        BW_GUARD;
		destroy();

        other.packRow();

        width_  = other.width_;
        stride_ = other.stride_;
        height_ = other.height_;

        if (other.data_ != NULL)
        {
#if ENABLE_RESOURCE_COUNTERS
			// Track memory usage
			RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool(allocator_, (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								 (uint)(stride_*height_), 0)
#endif
            data_ = new uint8[stride_*height_];
            ::memcpy(data_, other.data_, stride_*height_);
        }
        else
        {
            data_ = NULL;
        }
    }


    inline void Image<bool>::destroy()
    {
		BW_GUARD;
		if (data_)
		{
#if ENABLE_RESOURCE_COUNTERS
			// Track memory usage
			RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool(allocator_, (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								 (uint)(stride_*height_), 0)
#endif
			bw_safe_delete_array(data_);
		}
        
		if (unpackedRowData_)
		{
#if ENABLE_RESOURCE_COUNTERS
			// Track memory usage
			RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool(allocator_, (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								 (uint)(sizeof(bool) * width_), 0)
#endif
			bw_safe_delete_array(unpackedRowData_);
		}

        unpackedRow_ = 0;
        width_ = stride_ = height_ = 0;
    }


    inline void Image<bool>::packRow() const
    {
        if (unpackedRowData_ != NULL)
        {
            Image<bool> *myself = const_cast< Image<bool> *>(this);
            for (uint32 x = 0; x < width_; ++x)
			{
                myself->set(x, unpackedRow_, unpackedRowData_[x]);
			}

			if (unpackedRowData_)
			{
#if ENABLE_RESOURCE_COUNTERS
				// Track memory usage
				RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool(allocator_, (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
									 (uint)(sizeof(bool) * width_), 0)
#endif
				bw_safe_delete_array( unpackedRowData_ );
			}
        }
    }


    inline void Image<bool>::unpackRow(uint32 y) const
    {
		BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool(allocator_, (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
							 (uint)(sizeof(bool) * width_), 0)
#endif
        unpackedRowData_ = new bool[width_];
        unpackedRow_     = y < height_ ? y : height_ - 1;
        for (uint32 x = 0; x < width_; ++x)
            unpackedRowData_[x] = get(x, y);
    }


    inline /*static*/ uint32 Image<bool>::stride(uint32 w)
    {
        return (w >> 3) + (((w % 8) != 0) ? 1 : 0);
    }


	inline size_t Image<bool>::rawDataSize() const
	{
		return (this->height() * this->stride(this->width()));
	}

	inline bool Image<bool>::isUpsideDown() const
	{
		return false;
	}

	inline void Image<bool>::copyTo(bool* contiguousMemBuffer) const
	{		
		BW_GUARD;
		::memcpy(contiguousMemBuffer, data_, this->rawDataSize());        
	}

	inline void Image<bool>::copyFrom(bool const* contiguousMemBuffer)
	{	
		BW_GUARD;
		::memcpy(data_, contiguousMemBuffer, this->rawDataSize());        
	}
}

#endif // MOO_IMAGE_IPP
