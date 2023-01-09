#include "pch.hpp"
#include "worldeditor/import/import_codec_dted.hpp"
#include "worldeditor/import/import_image.hpp"
#include "resmgr/multi_file_system.hpp"

BW_BEGIN_NAMESPACE

namespace
{
    // This is the header for DT2 files:
    struct DTEDHeader
    {
        char    userHeaderLabel_[  80];
        char    dataDescription_[ 648];
        char    accuracyRecord_ [2700];

        uint32 numRecords() const
        {
			BW_GUARD;

            uint32 nRecordsFromHeader;
            char charRecords[5];
            strncpy( charRecords,&userHeaderLabel_[47], 4 );
            charRecords[4] = 0;
            sscanf( charRecords, "%d", &nRecordsFromHeader );
            return nRecordsFromHeader;
        };

        uint32 recordSize() const
        {
			BW_GUARD;

            uint32 recordSizeFromHeader;
            char charRecords[5];
            strncpy( charRecords,&userHeaderLabel_[51], 4 );
            charRecords[4] = 0;
            sscanf( charRecords, "%d", &recordSizeFromHeader );
            return recordSizeFromHeader;
        }
    };

    // This is a helper struct to convert from DT2 records to more standard
    // formats.
    class DTEDRecord
    {
    public: 
        int16 asInt16(uint32 index) const
        {
			BW_GUARD;

            uint16 b1    = (uint16)data_[index*2 +     prefixBytes()] & 0xff;
            uint16 b2    = (uint16)data_[index*2 + 1 + prefixBytes()] & 0xff;
            int16  value = (int16)(b1<<8 | b2);
            // Take care of null (-32767) and incorrect large -ve values
            // -12000 is global minimum given in spec
            if (value < -12000)
                value = 0;
            return value;
        }

        float asFloat(uint32 index) const
        {
            return (float)asInt16(index);
        }

        //Each row has a sentinel and sequential block count
        static int prefixBytes()    { return    6; }
        static int numEntries()     { return 3601; }

    private:
        char        data_[7214];
    };
}


/**
 *  This function returns true if the extension of filename is "dt2".
 *
 *  @param filename     The name of the file.
 *  @returns            True if the filename ends in "dt2", ignoring case.
 */
/*virtual*/ bool ImportCodecDTED::canHandleFile(BW::string const &filename)
{
	BW_GUARD;
    return BWResource::getExtension(filename).equals_lower( "dt2" );
}


/**
 *  This function loads the DT2 file in filename into image.  
 *
 *  @param filename     The file to load.
 *  @param image        The image to read.
 *	@param left			The left coordinate that the image should be used at.
 *	@param top			The top coordinate that the image should be used at.
 *	@param right		The right coordinate that the image should be used at.
 *	@param bottom		The bottom coordinate that the image should be used at.
 *	@param absolute		If true then the values are absolute.
 *  @returns            True if the image could be read, false otherwise.
 */
/*virtual*/ ImportCodec::LoadResult 
ImportCodecDTED::load
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

    FILE        *file       = NULL;
    bool        worked      = false;

    try
    {
        size_t numRead;

        // Open the file:
        file = BWResource::instance().fileSystem()->posixFileOpen( filename, "rb" );
        if (file == NULL)
            return LR_BAD_FORMAT;

        // Get the size of the file:
        MF_VERIFY( fseek(file, 0, SEEK_END) == 0 );
        long sz = ftell(file);
        MF_VERIFY( fseek(file, 0, SEEK_SET) == 0 );

        // Read the header:
        DTEDHeader header;
        numRead = fread(&header, 1, sizeof(header), file);
        if (numRead != sizeof(header))
        {
            fclose(file); file = NULL;
            return LR_BAD_FORMAT;
        }

        unsigned int width  = (unsigned int)((sz - sizeof(header))/sizeof(DTEDRecord));
        unsigned int height = header.recordSize();
        image.resize(width, height);
        DTEDRecord record;

        float min = 32767.f, max = -32768.f;

        for (unsigned int x = 0; x < width; ++x)
        {
            fread(&record, sizeof(record), 1, file);

            for (unsigned int y = 0; y < height; ++y)
            {
                int16 elev = record.asInt16(y);
                uint16 v = elev + 32768;

                min = std::min<float>( elev, min );
                max = std::max<float>( elev, max );

                image.set(x, y, v);
            }
        }

        uint16 minSrc, maxSrc;
        image.rangeRaw( minSrc, maxSrc );
        image.setScale( min, max, minSrc, maxSrc);
        image.flip(true); // x is back to front

        // Cleanup:
        fclose(file); file = NULL;
        worked = true;
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
    return worked ? LR_OK : LR_BAD_FORMAT;
}


/**
 *  This function is where DT2 saving should be implemented if it ever is.
 *  Currently it return false, meaning the save failed.
 *
 *  @param filename     The name of the file to save.
 *  @param image        The image to save.
 *	@param left			The left coordinate that the image should be used at.
 *	@param top			The top coordinate that the image should be used at.
 *	@param right		The right coordinate that the image should be used at.
 *	@param bottom		The bottom coordinate that the image should be used at.
 *	@param absolute		If true then the values are absolute.
 *  @returns            false.
 */
/*virtual*/ bool 
ImportCodecDTED::save
(
    BW::string         const &filename, 
    ImportImage			const &image,
    float               * /*left           = NULL*/,
    float               * /*top            = NULL*/,
    float               * /*right          = NULL*/,
    float               * /*bottom         = NULL*/,
	bool				* /*absolute	   = NULL*/,
	float				* /*minVal		   = NULL*/,
	float				* /*maxVal		   = NULL*/ 
)
{
    return false; // not yet implemented
}


/**
 *  This function indicates that we can read DT2 files.
 *
 *  @returns            True.
 */
/*virtual*/ bool ImportCodecDTED::canLoad() const
{
    return true;
}
BW_END_NAMESPACE

