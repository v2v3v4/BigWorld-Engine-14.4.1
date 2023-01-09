#include "pch.hpp"
#include "worldeditor/import/import_codec_bmp.hpp"
#include "worldeditor/import/import_image.hpp"
#include "resmgr/multi_file_system.hpp"

BW_BEGIN_NAMESPACE

namespace
{
    #pragma pack(1)

    // This struct defines the BMP file header.
    struct BMPHeader
    {
	    unsigned short int  type;                   // Magic identifier 'BM'   
	    unsigned int        size;                   // File size in bytes         
	    unsigned short int  reserved1, reserved2;
	    unsigned int        offset;                 // Offset to image data, bytes
    };


    // This struct defines the BMP information header.
    struct BMPInfoHeader
    {
	    unsigned int        size;                   // Header size in bytes     
	    unsigned int        width;                  // Width and height of image
	    unsigned int        height;                 // Width and height of image
	    unsigned short int  planes;                 // Number of colour planes  
	    unsigned short int  bits;                   // Bits per pixel           
	    unsigned int        compression;            // Compression type         
	    unsigned int        imageSize;              // Image size in bytes      
	    int                 xResolution;            // Pixels per metre         
        int                 yResolution;            // Pixels per metre         
	    unsigned int        nColours;               // Number of colours        
	    unsigned int        importantColours;       // Important colours        
    };

    #pragma pack()

    // Numbers to convert from sRGB to luminance:
    const float R_TO_L      = 0.299f;
    const float G_TO_L      = 0.587f;
    const float B_TO_L      = 0.114f;

    // The range that BMP files are read into:
    const float BMP_HEIGHT  = 50.0f;

    // Images more than 250 megapixels probably won't fit into memory:
    const uint64 LARGE_IMAGE = 250000000;

    const uint16 BMP_MARKER = ('M' << 8) + 'B';
}


/**
 *  This function returns true if the extension of filename is "BMP".
 *
 *  @param filename     The name of the file.
 *  @returns            True if the filename ends in "bmp", ignoring case.
 */
/*virtual*/ bool 
ImportCodecBMP::canHandleFile
(
    BW::string const   &filename
)
{
	BW_GUARD;
    return BWResource::getExtension(filename).equals_lower( "bmp" );
}


/**
 *  This function loads the BMP file in filename into image.  
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
ImportCodecBMP::load
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

    FILE  *file = NULL;
    uint8 *row  = NULL;
    bool  valid = false; // assume things didn't work
    try
    {
        size_t numRead;
        file = BWResource::instance().fileSystem()->posixFileOpen( filename, "rb" );
        if (file == NULL)
            return LR_BAD_FORMAT;

        // Head the header:
        BMPHeader header;
        numRead = fread(&header, 1, sizeof(header), file);
        if (numRead != sizeof(header))
        {
            fclose(file); file = NULL;
            return LR_BAD_FORMAT;
        }

        // Does the header have the BMP magic marker?
        if (header.type != BMP_MARKER)
            return LR_BAD_FORMAT;

        // Read the info header:
        BMPInfoHeader infoHeader;
        numRead = fread(&infoHeader, 1, sizeof(infoHeader), file);
        if (numRead != sizeof(infoHeader))
        {
            fclose(file); file = NULL;
            return LR_BAD_FORMAT;
        }

        // Perform a sanity check on the size of the image.  If it's larger
        // than 250 megapixels then we are going to have trouble allocating
        // that much memory for the image.
        uint64 imageSize = (uint64)infoHeader.width*(uint64)infoHeader.height;
        if (imageSize > LARGE_IMAGE)
            return LR_BAD_FORMAT;

        image.resize(infoHeader.width, infoHeader.height);

        // Read 8 bpp images:
        if (infoHeader.bits == 8)
        {
            // Read the palette (convert from sRGB to luminance):
            uint16 palette[256];
            for (int i = 0; i < 256; ++i)
            {
                uint8 r, g, b, a;
                fread(&b, 1, sizeof(uint8), file);
                fread(&g, 1, sizeof(uint8), file);
                fread(&r, 1, sizeof(uint8), file);
                fread(&a, 1, sizeof(uint8), file);
                palette[i] = (uint16)(257*(R_TO_L*r + G_TO_L*g + B_TO_L*b));
            }

            // Read in the image data, converting it to normalised luminance:
            unsigned int rowSize = ((infoHeader.width + 3)/4)*4;
            row = new uint8[rowSize];
            for (unsigned int y = 0; y < infoHeader.height; ++y)
            {
                fread(row, sizeof(uint8), rowSize, file);
                uint16 *p = image.getRow(y);
                uint8  *q = row;
                for (unsigned int x = 0; x < infoHeader.width; ++x, ++p, ++q)
                {
                    *p = palette[*q];
                }
            }

            valid = true;
        }
        // Read in 24 bpp images:
        else if (infoHeader.bits == 24)
        {
            valid = true;
            unsigned int rowSize = ((infoHeader.width*3 + 3)/4)*4;
            row = new uint8[rowSize];
            for (unsigned int y = 0; y < infoHeader.height; ++y)
            {
                fread(row, sizeof(uint8), rowSize, file);
                uint16 *p = image.getRow(y);
                for (unsigned int x = 0; x < infoHeader.width; ++x, ++p)
                {
                    uint16 b = (uint16)(row[x*3    ]);
			        uint16 g = (uint16)(row[x*3 + 1]);
			        uint16 r = (uint16)(row[x*3 + 2]);
                    *p = (uint16)(257*(R_TO_L*r + G_TO_L*g + B_TO_L*b));
                }
            }
        }
        // Read in 32 bpp images:
        else if (infoHeader.bits == 32)
        {
            valid = true;
            unsigned int rowSize = ((infoHeader.width*4 + 3)/4)*4;
            row = new uint8[rowSize];
            for (unsigned int y = 0; y < infoHeader.height; ++y)
            {
                fread(row, sizeof(uint8), rowSize, file);
                uint16 *p = image.getRow(y);
                for (unsigned int x = 0; x < infoHeader.width; ++x, ++p)
                {
                    uint16 b = (uint16)(row[x*4    ]);
			        uint16 g = (uint16)(row[x*4 + 1]);
			        uint16 r = (uint16)(row[x*4 + 2]);
                    *p = (uint16)(257*(R_TO_L*r + G_TO_L*g + B_TO_L*b));
                }
            }
        }
        else
        {
            fclose(file); file = NULL;
            return LR_BAD_FORMAT;
        }

        // BMP files are upside down:
        image.flip(false);

		// Set the height range:
		image.setScale(0.0f, BMP_HEIGHT, 0, std::numeric_limits<uint16>::max());
        
        // Cleanup:
        fclose(file); file = NULL;
        bw_safe_delete_array(row);
    }
    catch (...)
    {
        if (file != NULL)
        {
            fclose(file);
            file = NULL;
        }
        bw_safe_delete_array(row);
        throw;
    }

    return LR_OK;
}


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
ImportCodecBMP::save
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

    if (image.isEmpty())
        return false;

    FILE  *file = NULL;
    uint8 *row  = NULL;
    try
    {
        file = BWResource::instance().fileSystem()->posixFileOpen(filename.c_str(), "wb");
        if (file == NULL)
            return false;

        unsigned int rowsize = ((image.width() + 3)/4)*4;

        // Write the header:
        BMPHeader header; 
        header.type = BMP_MARKER;
        header.size = 
            sizeof(BMPHeader) 
            + 
            sizeof(BMPInfoHeader)
            +
            1024 // palette size
            +
            image.height()*rowsize;
        header.reserved1 = header.reserved2 = 0;
        header.offset = 
            sizeof(BMPHeader) 
            + 
            sizeof(BMPInfoHeader)
            + 
            1024; // palette size
        fwrite(&header, 1, sizeof(BMPHeader), file);

        // Write the bitmap info header:
        BMPInfoHeader infoHeader; 
	    infoHeader.size                 = sizeof(BMPInfoHeader);  
	    infoHeader.width                = image.width();
	    infoHeader.height               = image.height();
	    infoHeader.planes               = 1; 
	    infoHeader.bits                 = 8;    
	    infoHeader.compression          = 0;   
	    infoHeader.imageSize            = image.height()*rowsize;   
	    infoHeader.xResolution          = 0;        
        infoHeader.yResolution          = 0;       
	    infoHeader.nColours             = 256;       
	    infoHeader.importantColours     = 256;
        fwrite(&infoHeader, 1, sizeof(BMPInfoHeader), file);

        // Write the palette:
        uint8 zero = 0;
        for (int i = 0; i < 256; ++i)
        {
            uint8 val = (uint8)i;
            fwrite(&val , 1, sizeof(uint8), file);
            fwrite(&val , 1, sizeof(uint8), file);
            fwrite(&val , 1, sizeof(uint8), file);
            fwrite(&zero, 1, sizeof(uint8), file);
        }

        // Write the image:
        row = new uint8[rowsize];
        ::memset(row, 0, sizeof(uint8)*rowsize);
		uint16 minv, maxv;
		if 
		(
			absolute == NULL || (absolute != NULL && !*absolute) 
			|| 
			minVal == NULL || maxVal == NULL
		)
		{
			
			image.rangeRaw(minv, maxv); // not absolute heights
		}
		else
		{
			image.rangeHeight(*minVal, *maxVal);
			minv = 0;
			maxv = std::numeric_limits<uint16>::max();
		}
        float scale = (minv == maxv) ? 0.0f : 1.0f/(maxv - minv);
        for (unsigned int y = 0; y < image.height(); ++y)
        {
            uint16 const *p = image.getRow(y);
            uint8        *q = row;
            for (unsigned int x = 0; x < image.width(); ++x, ++p)
            {
                uint8 v = static_cast<uint8>(255.0f*(*p - minv)*scale);
                *q++ = v;
            }
            fwrite(row, sizeof(uint8), rowsize, file);
        }

        // Cleanup:
        fclose(file); file = NULL;
        bw_safe_delete_array(row);
    }
    catch (...)
    {
        if (file != NULL)
            fclose(file);
        bw_safe_delete_array(row);
        throw;
    }

    return true; // not yet implemented
}


/**
 *  This function indicates that we can read BMP files.
 *
 *  @returns            True.
 */
/*virtual*/ bool ImportCodecBMP::canLoad() const
{
    return true;
}


/**
 *  This function indicates that we can write BMP files.
 *
 *  @returns            True.
 */
/*virtual*/ bool ImportCodecBMP::canSave() const
{
    return true;
}
BW_END_NAMESPACE

