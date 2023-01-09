#include "pch.hpp"
#include "worldeditor/import/import_image.hpp"
#include "worldeditor/import/import_codec_bmp.hpp"
#include "worldeditor/import/import_codec_dted.hpp"
#include "worldeditor/import/import_codec_raw.hpp"
#include "worldeditor/import/import_codec_terragen.hpp"
#include "worldeditor/import/import_codec_tiff.hpp"
#include <limits>

BW_BEGIN_NAMESPACE

//-----------------------------------------------------------------------------
// Section: CodecHolder
//-----------------------------------------------------------------------------

/**
 *	This class holds all existing codecs while there are images that need to be
 *	decoded.
 */
class CodecHolder : public ReferenceCount
{
public:
	/**
	 *	Constructor. Populates the codecs_ vector with instances of the
	 *	available codecs.
	 */
	CodecHolder()
	{
		BW_GUARD;

		codecs_.push_back( new ImportCodecBMP() );
		codecs_.push_back( new ImportCodecDTED() );
		codecs_.push_back( new ImportCodecRAW() );
		codecs_.push_back( new ImportCodecTerragen() );
		codecs_.push_back( new ImportCodecTIFF() );
	}


	/**
	 *  This static method is called whenever an ImportImage is created and, if
	 *  it's the first ImportImage object created, it creates an instance of
	 *  this class, CodecHolder. Subsequent ImportImage instances reuse the
	 *	codecs.
	 */
	static void addUser()
	{
		BW_GUARD;

		if (s_userCount_ == 0)
		{
			MF_ASSERT( !s_pInstance_ );
			s_pInstance_ = new CodecHolder();
		}
		s_userCount_++;
	}


	/**
	 *  This static method is called whenever an ImportImage is destroyed, and
	 *	when the number of users of CodecHolder reaches zero, the CodecHolder
	 *	instance gets destroyed.
	 */
	static void removeUser()
	{
		BW_GUARD;

		s_userCount_--;
		if (s_userCount_ == 0)
		{
			MF_ASSERT( s_pInstance_ );
			s_pInstance_ = NULL;
		}
	}


	/**
	 *	This static method returns the appropriate codec for the file type.
	 *
	 *	@param filename	The image's file name, used to determine the codec.
	 *	@return		The appropriate codec if available, NULL otherwise.
	 */
	static ImportCodecPtr getCodec( const BW::string & filename )
	{
		if (s_pInstance_)
		{
			BW_GUARD;

			for( BW::vector< ImportCodecPtr >::iterator
				it = s_pInstance_->codecs_.begin();
				it != s_pInstance_->codecs_.end(); ++it)
			{
				if ((*it)->canHandleFile( filename ))
				{
					return (*it);
				}
			}
		}

		return NULL;
	}

private:
	BW::vector< ImportCodecPtr > codecs_;
	static int s_userCount_;
	static SmartPointer< CodecHolder > s_pInstance_;
};

/// Number of images needing codecs, to create/delete the CodecHolder instance.
/*static*/ int CodecHolder::s_userCount_ = 0;

/// Instance holder.
/*static*/ SmartPointer< CodecHolder > CodecHolder::s_pInstance_;


//-----------------------------------------------------------------------------
// Section: ImportImage
//-----------------------------------------------------------------------------

/**
 *  This creates an empty ImportImage.
 */
ImportImage::ImportImage():
	Base(),
	scaleAdd_(0.0f),
	scaleMul_(1.0f),
	minRaw_(std::numeric_limits<uint16>::max()),
	maxRaw_(0)
{
	BW_GUARD;

	CodecHolder::addUser();
}


/**
 *  This creates an ImportImage of the given size.
 *
 *  @param w			The width of the elevation image.
 *  @param h			The height of the elevation image.
 */
ImportImage::ImportImage(unsigned int w, unsigned int h):
	Base(w, h),
	scaleAdd_(0.0f),
	scaleMul_(1.0f),
	minRaw_(std::numeric_limits<uint16>::max()),
	maxRaw_(0)
{
	BW_GUARD;

	CodecHolder::addUser();
}


/**
 *  This is the copy constructor for ImportImage.
 *
 *  @param  other           The ImportImage to copy from.
 */
ImportImage::ImportImage(ImportImage const &other):
	Base(),
	scaleAdd_(0.0f),
	scaleMul_(1.0f),
	minRaw_(std::numeric_limits<uint16>::max()),
	maxRaw_(0)
{
	BW_GUARD;

	CodecHolder::addUser();
	copy( other );
}


/**
 *  This is the ImportImage destructor.
 */
ImportImage::~ImportImage()
{
	BW_GUARD;

    destroy();
	CodecHolder::removeUser();
}


/**
 *  This is the ImportImage assignment operator.
 *
 *  @param  other           The ImportImage to copy from.
 *  @returns                *this.
 */
ImportImage &ImportImage::operator=(ImportImage const &other)
{
	BW_GUARD;

    if (this != &other)
        copy(other);
    return *this;
}


/**
 *  Load the elevation data from a file.
 *
 *  @param filename     The file to load from.
 *	@param left			The left coordinate that the image should be placed.
 *	@param top			The top coordinate that the image should be placed.
 *	@param right		The right coordinate that the image should be placed.
 *	@param bottom		The bottom coordinate that the image should be placed.
 *	@param absolute		If true then use absolute coordinates.
 *  @returns            The result of loading.
 */
ImportCodec::LoadResult ImportImage::load
(
    BW::string     const & filename,
    float           *left,
    float           *top,
    float           *right,
    float           *bottom,
	bool			*absolute,
    bool            configDialog
)
{
	BW_GUARD;

	ImportCodecPtr codec = CodecHolder::getCodec( filename );
    if (codec == NULL)
    {
        return ImportCodec::LR_UNKNOWN_CODEC;
    }
    else
    {
        return 
            codec->load
            (
                filename, 
                *this, 
                left , top   , 
                right, bottom,
				absolute,
                configDialog
            );
    }
}


/**
 *  Save the elevation data to a file.
 *
 *  @param filename     The file to save to.
 *	@param left			The left coordinate that the image should be placed.
 *	@param top			The top coordinate that the image should be placed.
 *	@param right		The right coordinate that the image should be placed.
 *	@param bottom		The bottom coordinate that the image should be placed.
 *	@param absolute		If true then use absolute coordinates.
 *	@param minHeight	If absolute is true then this is the minimum height.
 *	@param maxHeight	If absolute is true then this is the maximum height. 
 *  @returns            True if the file could be saved, false otherwise.
 */
bool ImportImage::save
(
    BW::string     const & filename,
    float           *left,
    float           *top,
    float           *right,
    float           *bottom,
	bool			*absolute,
	float			*minHeight,
	float			*maxHeight
)
{
	BW_GUARD;

    ImportCodecPtr codec = CodecHolder::getCodec( filename );
    if (codec == NULL)
	{
        return false;
	}
    else
	{
        return 
			codec->save
			(
				filename, 
				*this, 
				left, top, right, bottom, 
				absolute,
				minHeight, maxHeight
			);
	}
}


/**
 *  This function flips the height data so that the points of minimum value
 *  become points of maximum value and vice-versa.
 */
void ImportImage::flipHeight()
{
	BW_GUARD;

    if (isEmpty())
        return;

    uint16 minV, maxV;
    rangeRaw(minV, maxV);

    size_t sz  = width()*height();
    uint16  *p  = getRow(0);    
    uint32  sum = (uint32)minV + (uint32)maxV;
    for (size_t i = 0; i < sz; ++i, ++p)
        *p = (uint16)(sum - *p);
}


/**
 *	This function gets the addition that converts raw pixel values into
 *	floating point values.
 *
 *	@returns			The addition factor to convert uint16 values to
 *						heights.
 */
float ImportImage::scaleAdd() const
{
	return scaleAdd_;
}


/**
 *	This function gets the multpilier that converts raw pixel values into
 *	floating point values.
 *
 *	@returns			The multiplication factor to convert uint16 values to
 *						heights.
 */
float ImportImage::scaleMul() const
{
	return scaleMul_;
}


/**
 *	This function sets the scale used by the addition and multiplication
 *	factors.  The multipliers are set up so that:
 *	
 *		minSrc*scaleMul + scaleAdd	= minV and
 *		maxSrc*scaleMul + scaleAdd	= maxV
 *
 *	@param minV			The minimum height value.
 *	@param maxV			The maximum height value.
 *	@param minSrc		The minimum raw value.
 *	@param maxSrc		The maximum raw value.
 */
void ImportImage::setScale
(
	float		minV, 
	float		maxV, 
	uint16		minSrc /*= 0*/, 
	uint16		maxSrc /*= 65535*/
)
{
	BW_GUARD;

	if (minSrc == maxSrc)
	{
		scaleAdd_	= minV;
		scaleMul_	= 0.0f;
	}
	else
	{
		scaleMul_ = (maxV - minV)/(float)(maxSrc - minSrc);
		scaleAdd_ = minV - scaleMul_*minSrc;
	}
}


/**
 *	This gets the scale.  If these values were fed into setScale with
 *	minSrc and maxSrc untouched then the scale would not be changed.
 *
 *	@param minV		Set to the minimum height value at 0.
 *	@param maxV		Set to the maximum height value at 65535.
 */
void ImportImage::getScale(float &minV, float &maxV) const
{
	BW_GUARD;

	minV = toHeight((uint16)0);
	maxV = toHeight(std::numeric_limits<uint16>::max());
}


/**
 *	This sets the scale so that the minimum height is 0.0f and the maximum
 *	is 1.0f.
 */
void ImportImage::normalise()
{
	BW_GUARD;

	uint16 minSrc, maxSrc;
	rangeRaw(minSrc, maxSrc);
	setScale(0.0f, 1.0f, minSrc, maxSrc);
}


/**
 *  Return the range of values within the elevation data.
 *
 *  @param minV         The minimum height in the image.
 *  @param maxV         The maximum height in the image.
 *  @param recalculate	Force a recalculation instead of using cached values.
 */
void 
ImportImage::rangeRaw
(
	uint16		&minV, 
	uint16		&maxV, 
	bool		recalculate /*=false*/
) const
{
	BW_GUARD;

	if (minRaw_ == std::numeric_limits<uint16>::max() || recalculate)
	{
		minV = +std::numeric_limits<uint16>::max();
		maxV = 0;
		size_t sz = width()*height();
		uint16 const *p = getRow(0);
		uint16 const *q = p + sz;
		while (p != q)
		{
			minV = std::min(minV, *p);
			maxV = std::max(maxV, *p);
			++p;
		}
		minRaw_ = minV;
		maxRaw_ = maxV;
	}

	minV = minRaw_;
	maxV = maxRaw_;
}



/**
 *	This gets the range of heights in the image.
 *
 *	@param minV			The minimum height.
 *	@param maxV			The maximum height.
 *	@param recalc		If true then recalculate the range rather than
 *						use cached values.
 */
void ImportImage::rangeHeight
(
	float	&minV, 
	float	&maxV, 
	bool	recalc /*= false*/
) const
{
	BW_GUARD;

	uint16 minv16, maxv16;
	rangeRaw(minv16, maxv16, recalc);

	minV = toHeight(minv16);
	maxV = toHeight(maxv16);
}



/**
 *  This function does the actual copying from another ElevationData.
 *
 *  @param other        The ElevationData to copy from.
 */
void ImportImage::copy(ImportImage const &other)
{
	BW_GUARD;

    destroy();
	Base::copy(other);

	scaleAdd_	= other.scaleAdd_;
	scaleMul_	= other.scaleMul_;
	minRaw_		= other.minRaw_;
	maxRaw_		= other.maxRaw_;
}


/**
 *  This function cleans up resources owned by an ElevationData.
 */
void ImportImage::destroy()
{
	BW_GUARD;

    Base::destroy();
}
BW_END_NAMESPACE

