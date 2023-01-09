#include "pch.hpp"
#include "worldeditor/import/import_codec_raw.hpp"
#include "worldeditor/import/import_image.hpp"
#include "worldeditor/gui/dialogs/raw_import_dlg.hpp"
#include "resmgr/multi_file_system.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This function returns true if the extension of filename is "RAW" or "R16".
 *
 *  @param filename     The name of the file.
 *  @returns            True if the filename ends in "bmp", ignoring case.
 */
/*virtual*/ bool 
ImportCodecRAW::canHandleFile
(
    BW::string const   &filename
)
{
	BW_GUARD;
	BW::StringRef ext = BWResource::getExtension(filename);
    return 
        ext.equals_lower( "raw" )
        ||
        ext.equals_lower( "r16" );
}


/**
 *  This function loads the RAW file in filename into image.  
 *
 *  @param filename     The file to load.
 *  @param image        The image to read.
 *	@param left			The left coordinate that the image should be used at.
 *	@param top			The top coordinate that the image should be used at.
 *	@param right		The right coordinate that the image should be used at.
 *	@param bottom		The bottom coordinate that the image should be used at.
 *	@param absolute		If true then the values are absolute.
 *  @returns            The result of loading.
 */
/*virtual*/ ImportCodec::LoadResult 
ImportCodecRAW::load
(
    BW::string         const &filename, 
    ImportImage			&image,
    float               *left           /* = NULL*/,
    float               *top            /* = NULL*/,
    float               *right          /* = NULL*/,
    float               *bottom         /* = NULL*/,
	bool				*absolute		/* = NULL*/,
    bool                loadConfigDlg   /* = false*/
)
{
	BW_GUARD;

    FILE *file = NULL;
    try
    {   
        int   width         = -1;
        int   height        = -1;
        float minv          =  0.0f;
        float maxv          = 50.0f;
        float leftv         = -1.0f;
        float topv          = -1.0f;
        float rightv        = -1.0f;
        float bottomv       = -1.0f;
        bool  littleEndian  = true;
		bool  absoluteVal	= false;

        // Try reading the meta-contents file:
        BW::string xmlFilename = 
            BWResource::instance().removeExtension(filename) + ".xml";
        DataSectionPtr xmlFile = BWResource::openSection(xmlFilename, false);
        if (xmlFile != NULL)
        {
            width           = xmlFile->readInt  ("export/imageWidth"  , -1   );
            height          = xmlFile->readInt  ("export/imageHeight" , -1   );
            minv            = xmlFile->readFloat("export/minHeight"   ,  0.0f);
            maxv            = xmlFile->readFloat("export/maxHeight"   , 50.0f);
            leftv           = xmlFile->readFloat("export/left"        , -1.0f);
            topv            = xmlFile->readFloat("export/top"         , -1.0f);
            rightv          = xmlFile->readFloat("export/right"       , -1.0f);
            bottomv         = xmlFile->readFloat("export/bottom"      , -1.0f);
            littleEndian    = xmlFile->readBool ("export/littleEndian", true );
			absoluteVal		= xmlFile->readBool ("export/absolute"    , false);
        }
        else if (loadConfigDlg)
        {
            RawImportDlg rawImportDlg(filename.c_str());
            if (rawImportDlg.DoModal() == IDOK)
            {
                unsigned int iwidth, iheight;
                rawImportDlg.getResult(iwidth, iheight, littleEndian);
                width  = (int)iwidth;
                height = (int)iheight;
            }
            else
            {
                return LR_CANCELLED;
            }
        }

        if (width <= 0 || height <= 0)
            return LR_BAD_FORMAT;

        if (left != NULL && leftv != -1.0f)
            *left = leftv;
        if (top != NULL && topv != -1.0f)
            *top = topv;
        if (right != NULL && rightv != -1.0f)
            *right = rightv;
        if (bottom != NULL && bottomv != -1.0f)
            *bottom = bottomv;

		if (absolute != NULL)
			*absolute = absoluteVal;

        // Read the height data:
        file = BWResource::instance().fileSystem()->posixFileOpen(filename, "rb");
        if (file == NULL)
            return LR_BAD_FORMAT;
        image.resize(width, height);
        fread
        (
            image.getRow(0), 
            image.width()*image.height(), 
            sizeof(uint16), 
            file
        );

        // Twiddle if big endian format:
        if (!littleEndian)
        {
            uint16 *data = image.getRow(0);
            size_t sz = width*height;
            for (size_t i = 0; i < sz; ++i, ++data)
            {
                *data = ((*data & 0xff) << 8) + (*data >> 8);
            }
        }

		uint16 minv16, maxv16;
		image.rangeRaw(minv16, maxv16, true /*force recalculate*/);
		image.setScale(minv, maxv, minv16, maxv16);

        // Cleanup:
        fclose(file); file = NULL;
    }
    catch (...)
    {
        if (file != NULL)
        {
            fclose(file);
            file = NULL;
        }
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
 *	@param minVal		If absolute then this is the minimum height value.
 *	@param maxVal		If absolute then this is the maximum height value.
 *  @returns            True if the image could be saved, false otherwise.
 */
/*virtual*/ bool 
ImportCodecRAW::save
(
    BW::string         const &filename, 
    ImportImage			const &image,
    float               *left           /* = NULL*/,
    float               *top            /* = NULL*/,
    float               *right          /* = NULL*/,
    float               *bottom         /* = NULL*/,
	bool				*absolute		/* = NULL*/,
	float				*minVal			/* = NULL*/,
	float				*maxVal			/* = NULL*/
)
{
	BW_GUARD;

    if (image.isEmpty())
        return false;

    FILE  *file = NULL;
    uint8 *row  = NULL;
    try
    {
        // Get the range of data:
		float minv = (minVal != NULL) ? *minVal : 0.0f;
		float maxv = (maxVal != NULL) ? *maxVal : 0.0f;
		if 
		(
			absolute == NULL || (absolute != NULL && !*absolute) 
			|| 
			minVal == NULL || maxVal == NULL
		)
		{
			image.rangeHeight(minv, maxv);
		}

        // Write the raw data file:
        file = BWResource::instance().fileSystem()->posixFileOpen(filename, "wb");
        if (file == NULL)
            return false;
		for (uint32 y = 0; y < image.height(); ++y)
		{
			fwrite
			(
				image.getRow(image.height() - y - 1), // flip in y-direction
				image.width(), 
				sizeof(uint16), 
				file
			);
		}
        fclose(file); file = NULL;

        // Write out the meta-contents file with the size of the terrain:
        BW::string xmlFilename = 
            BWResource::instance().removeExtension(filename) + ".xml";
        DataSectionPtr xmlFile = BWResource::openSection(xmlFilename, true);
        if (xmlFile == NULL)
            return false;
	    xmlFile->writeFloat("export/minHeight"   , minv);
	    xmlFile->writeFloat("export/maxHeight"   , maxv);
	    xmlFile->writeInt  ("export/imageWidth"  , image.width ());
	    xmlFile->writeInt  ("export/imageHeight" , image.height());
        xmlFile->writeBool ("export/littleEndian", true);
        if (left != NULL)
            xmlFile->writeFloat("export/left", *left);
        if (top != NULL)
            xmlFile->writeFloat("export/top", *top);
        if (right != NULL)
            xmlFile->writeFloat("export/right", *right);
        if (bottom != NULL)
            xmlFile->writeFloat("export/bottom", *bottom);
		if (absolute != NULL)
			xmlFile->writeBool("export/absolute", *absolute);
        xmlFile->save(xmlFilename);
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
/*virtual*/ bool ImportCodecRAW::canLoad() const
{
    return true;
}


/**
 *  This function indicates that we can write BMP files.
 *
 *  @returns            True.
 */
/*virtual*/ bool ImportCodecRAW::canSave() const
{
    return true;
}
BW_END_NAMESPACE

