#include "pch.hpp"
#include "worldeditor/import/import_codec_tiff.hpp"
#include "worldeditor/import/import_image.hpp"
#include "resmgr/multi_file_system.hpp"
#include <vector>
#include <algorithm>

BW_BEGIN_NAMESPACE

namespace
{
    #pragma pack(1)

	/*
	 * TIFF header.
	 */
	typedef struct {
		uint16 tiff_magic;      /* magic number (defines byte order) */
		uint16 tiff_version;    /* TIFF version number */
	} TIFFHeaderCommon;
	typedef struct {
		uint16 tiff_magic;      /* magic number (defines byte order) */
		uint16 tiff_version;    /* TIFF version number */
		uint32 tiff_diroff;     /* byte offset to first directory */
	} TIFFHeaderClassic;
	typedef struct {
		uint16 tiff_magic;      /* magic number (defines byte order) */
		uint16 tiff_version;    /* TIFF version number */
		uint16 tiff_offsetsize; /* size of offsets, should be 8 */
		uint16 tiff_unused;     /* unused word, should be 0 */
		uint64 tiff_diroff;     /* byte offset to first directory */
	} TIFFHeaderBig;
	
	#define TIFF_VERSION_CLASSIC 42
	#define TIFF_VERSION_BIG	43

	#define TIFF_BIGENDIAN      0x4d4d
	#define TIFF_LITTLEENDIAN   0x4949
	#define MDI_LITTLEENDIAN    0x5045
	#define MDI_BIGENDIAN       0x4550


	/*
	 * Tag data type information.
	 *
	 * Note: RATIONALs are the ratio of two 32-bit integer values.
	 */
	typedef enum {
		TIFF_NOTYPE = 0,      /* placeholder */
		TIFF_BYTE = 1,        /* 8-bit unsigned integer */
		TIFF_ASCII = 2,       /* 8-bit bytes w/ last byte null */
		TIFF_SHORT = 3,       /* 16-bit unsigned integer */
		TIFF_LONG = 4,        /* 32-bit unsigned integer */
		TIFF_RATIONAL = 5,    /* 64-bit unsigned fraction */
		TIFF_SBYTE = 6,       /* !8-bit signed integer */
		TIFF_UNDEFINED = 7,   /* !8-bit untyped data */
		TIFF_SSHORT = 8,      /* !16-bit signed integer */
		TIFF_SLONG = 9,       /* !32-bit signed integer */
		TIFF_SRATIONAL = 10,  /* !64-bit signed fraction */
		TIFF_FLOAT = 11,      /* !32-bit IEEE floating point */
		TIFF_DOUBLE = 12,     /* !64-bit IEEE floating point */
		TIFF_IFD = 13,        /* %32-bit unsigned integer (offset) */
		TIFF_LONG8 = 16,      /* BigTIFF 64-bit unsigned integer */
		TIFF_SLONG8 = 17,     /* BigTIFF 64-bit signed integer */
		TIFF_IFD8 = 18        /* BigTIFF 64-bit unsigned integer (offset) */
	} TIFFDataType;


	/*
	 * TIFF Tag Definitions.
	 */
	#define	TIFFTAG_SUBFILETYPE		254	/* subfile data descriptor */
	#define	    FILETYPE_REDUCEDIMAGE	0x1	/* reduced resolution version */
	#define	    FILETYPE_PAGE			0x2	/* one page of many */
	#define	    FILETYPE_MASK			0x4	/* transparency mask */
	#define	TIFFTAG_OSUBFILETYPE		255	/* +kind of data in subfile */
	#define	    OFILETYPE_IMAGE			1	/* full resolution image data */
	#define	    OFILETYPE_REDUCEDIMAGE	2	/* reduced size image data */
	#define	    OFILETYPE_PAGE			3	/* one page of many */
	#define	TIFFTAG_IMAGEWIDTH		256	/* image width in pixels */
	#define	TIFFTAG_IMAGELENGTH		257	/* image height in pixels */
	#define	TIFFTAG_BITSPERSAMPLE	258	/* bits per channel (sample) */
	#define	TIFFTAG_COMPRESSION		259	/* data compression technique */
	#define	    COMPRESSION_NONE		1	/* dump mode */
	#define	TIFFTAG_PHOTOMETRIC		262	/* photometric interpretation */
	#define	    PHOTOMETRIC_MINISWHITE	0	/* min value is white */
	#define	    PHOTOMETRIC_MINISBLACK	1	/* min value is black */
	#define	TIFFTAG_STRIPOFFSETS	273	/* offsets to data strips */
	#define	TIFFTAG_SAMPLESPERPIXEL	277	/* samples per pixel */
	#define	TIFFTAG_ROWSPERSTRIP	278	/* rows per strip of data */
	#define	TIFFTAG_STRIPBYTECOUNTS	279	/* bytes counts for strips */
	#define	TIFFTAG_XRESOLUTION		282	/* pixels/resolution in x */
	#define	TIFFTAG_YRESOLUTION		283	/* pixels/resolution in y */
	#define	TIFFTAG_RESOLUTIONUNIT	296	/* units of resolutions */
	#define	    RESUNIT_NONE			1	/* no meaningful units */
	#define	    RESUNIT_INCH			2	/* english */
	#define	    RESUNIT_CENTIMETER		3	/* metric */
	
	/*
	 * TIFF Image File Directories are comprised of a table of field
	 * descriptors of the form shown below.  The table is sorted in
	 * ascending order by tag.  The values associated with each entry are
	 * disjoint and may appear anywhere in the file (so long as they are
	 * placed on a word boundary).
	 *
	 * If the value is 4 bytes or less, in ClassicTIFF, or 8 bytes or less in
	 * BigTIFF, then it is placed in the offset field to save space. If so,
	 * it is left-justified in the offset field.
	 */
	typedef struct {
		uint16 tdir_tag;        /* see below */
		uint16 tdir_type;       /* data type; see below */
		uint32 tdir_count;      /* number of items; length in spec */
		union {
			uint16 toff_short;
			uint32 toff_long;
		} tdir_offset;		/* either offset or the data itself if fits */
	} TIFFDirEntry;
	
	// TIFFDirEntry 32-bit value wrapper
	struct TIFFDirEntry32 : public TIFFDirEntry
	{
		TIFFDirEntry32(uint16 tag, uint32 value, uint32 count = 1) : 
			TIFFDirEntry()
		{
			tdir_tag = tag;
			tdir_type = TIFF_LONG;
			tdir_count = count;
			tdir_offset.toff_long = value;
		}
	};

	// TIFFDirEntry 16-bit value wrapper
	struct TIFFDirEntry16 : public TIFFDirEntry
	{
		TIFFDirEntry16(uint16 tag, uint16 value, uint32 count = 1) : 
			TIFFDirEntry()
		{
			tdir_tag = tag;
			tdir_type = TIFF_SHORT;
			tdir_count = count;
			tdir_offset.toff_short = value;
		}
	};
	
    #pragma pack()
}


/**
 *  This function returns true if the extension of filename is "TIF".
 *
 *  @param filename     The name of the file.
 *  @returns            True if the filename ends in "TIF", ignoring case.
 */
/*virtual*/ bool 
ImportCodecTIFF::canHandleFile
(
    BW::string const   &filename
)
{
	BW_GUARD;
    return BWResource::getExtension(filename).equals_lower( "tif" );
}


/**
 *  This function loads the TIFF file in filename into image.  
 *
 *  @param filename     The file to load.
 *  @param image        The image to read.
 *	@param left			The left coordinate that the image should be used at.
 *	@param top			The top coordinate that the image should be used at.
 *	@param right		The right coordinate that the image should be used at.
 *	@param bottom		The bottom coordinate that the image should be used at.
 *	@param absolute		If true then the values are absolute.
 *  @returns            The result of reading (e.g. ok, bad format etc).
 */
/*virtual*/ ImportCodec::LoadResult 
ImportCodecTIFF::load
(
    BW::string         const &filename, 
    ImportImage			&image,
    float               * /*left           = NULL*/,
    float               * /*top            = NULL*/,
    float               * /*right          = NULL*/,
    float               * /*bottom         = NULL*/,
	bool				* /*absolute	   = NULL*/,
    bool                /*loadConfigDlg    = false*/
)
{
	BW_GUARD;

    return LR_UNKNOWN_CODEC;
}

// subtracts than multiplies the value
template <class Type>
class ScaleValue
{
private:
	Type deduction_, factor_;
public:
	ScaleValue ( const Type& deduction, const Type& factor ) : 
		deduction_ ( deduction ),  factor_ ( factor ) {	}

	Type operator() ( Type& value ) const
	{
		return (value - deduction_) * factor_;
	}
};


/**
 *  This function saves the image into the given file.
 *
 *  @param filename     The name of the file to save.
 *  @param image        The image to save.
 *	@param left			The left coordinate that the image should be used at.
 *	@param top			The top coordinate that the image should be used at.
 *	@param right		The right coordinate that the image should be used at.
 *	@param bottom		The bottom coordinate that the image should be used at.
 *	@param absolute		If true then the values are absolute.
 *  @returns            True if the image could be saved, false otherwise.
 */
/*virtual*/ bool 
ImportCodecTIFF::save
(
    BW::string         const &filename, 
    ImportImage			const &image,
    float               * /*left		= NULL*/,
    float               * /*top			= NULL*/,
    float               * /*right		= NULL*/,
    float               * /*bottom		= NULL*/,
	bool				*absolute,
	float				*minVal,
	float				*maxVal
)
{
	BW_GUARD;

    if ( image.isEmpty() )
        return false;

    FILE  *file = NULL;
    try
    {
        file = BWResource::instance().fileSystem()->posixFileOpen(
			filename.c_str(), "wb" );
        if ( file == NULL )
            return false;

		// calculate image extremities
		uint16 minv, maxv;
		if ( absolute == NULL || ( absolute != NULL && !*absolute ) || 
			minVal == NULL || maxVal == NULL)
		{
			image.rangeRaw( minv, maxv ); // not absolute heights
		}
		else
		{
			image.rangeHeight( *minVal, *maxVal );
			minv = 0;
			maxv = std::numeric_limits<uint16>::max();
		}
		float scale = (minv == maxv) ? 0.0f : 1.0f / (maxv - minv);

		// copy and scale image data
		vector<uint16> data( image.width() * image.height() );
		vector<uint16>::iterator dataIt = data.begin();
		for (uint32 y = image.height(); y-- > 0;)
		{
			const uint16* row = image.getRow( y );
			dataIt = std::transform( row, row + image.width(), dataIt,
				ScaleValue<const uint16>(
					minv,
					static_cast<uint16>(
						scale * std::numeric_limits<uint16>::max()))
			);
		}

		// calculate offsets in TIFF
		const uint32 imageOffset = 
			sizeof( TIFFHeaderClassic );
		const uint32 resOffset = 
			static_cast<uint32>(imageOffset + data.size() * sizeof( uint16 ));
		const uint32 dirOffset = 
			resOffset + sizeof( uint64 );

		// write the header
		TIFFHeaderClassic header;
		header.tiff_magic = TIFF_LITTLEENDIAN;
		header.tiff_version = TIFF_VERSION_CLASSIC;
		header.tiff_diroff = dirOffset;
		fwrite( &header, sizeof( header ), 1, file );

		// write the image data
		fwrite( &data[0], data.size() * sizeof( uint16 ), 1, file );

		// write resolution (referred from TIFF directory. see below)
		uint64 resolution = 0x000000010000012C; // 300 DPI
		fwrite( &resolution, sizeof( resolution ), 1, file );
		
		// write TIFF directory
		BW::vector<TIFFDirEntry> entries;
		entries.push_back(
			TIFFDirEntry32( TIFFTAG_IMAGEWIDTH, image.width()) );
		entries.push_back(
			TIFFDirEntry32( TIFFTAG_IMAGELENGTH, image.height()) );
		entries.push_back(
			TIFFDirEntry16( TIFFTAG_BITSPERSAMPLE, 16) );
		entries.push_back(
			TIFFDirEntry16( TIFFTAG_COMPRESSION, COMPRESSION_NONE) );
		entries.push_back(
			TIFFDirEntry16( TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK) );
		entries.push_back(
			TIFFDirEntry32( TIFFTAG_STRIPOFFSETS,  imageOffset ) );
		entries.push_back(
			TIFFDirEntry32( TIFFTAG_ROWSPERSTRIP, image.height() ) );
		entries.push_back(
			TIFFDirEntry32(
				TIFFTAG_STRIPBYTECOUNTS,
				static_cast<uint32>(data.size() * sizeof( uint16 )) ) );
		entries.push_back(
			TIFFDirEntry32( TIFFTAG_XRESOLUTION, resOffset ) );
		entries.push_back(
			TIFFDirEntry32( TIFFTAG_YRESOLUTION, resOffset ) );
		entries.push_back(
			TIFFDirEntry16(TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH) );

		uint16 numEntries = static_cast<uint16>( entries.size() );
		fwrite( &numEntries, sizeof( numEntries ), 1, file );

		fwrite( &entries[0], sizeof( TIFFDirEntry ), entries.size(), file );

		uint32 nextIFDOffset = 0;
		fwrite( &nextIFDOffset, sizeof( nextIFDOffset ), 1, file );

        // cleanup
        fclose( file ); 
		file = NULL;
    }
    catch (...)
    {
        if ( file != NULL )
            fclose( file );
        throw;
    }

    return true;
}


/**
 *  This function indicates that we can read TIFF files.
 *
 *  @returns            True.
 */
/*virtual*/ bool ImportCodecTIFF::canLoad() const
{
    return false;
}


/**
 *  This function indicates that we can write TIFF files.
 *
 *  @returns            True.
 */
/*virtual*/ bool ImportCodecTIFF::canSave() const
{
    return true;
}

BW_END_NAMESPACE
