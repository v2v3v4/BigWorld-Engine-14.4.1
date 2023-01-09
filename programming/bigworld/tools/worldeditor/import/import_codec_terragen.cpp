#include "pch.hpp"
#include "worldeditor/import/import_codec_terragen.hpp"
#include "worldeditor/import/import_image.hpp"
#include "resmgr/multi_file_system.hpp"

BW_BEGIN_NAMESPACE

namespace
{
    bool compareChunkType(DWORD chunkType, char const *str)
    {
		BW_GUARD;

        if (((chunkType & 0x000000ff) >>  0) != str[0])
            return false;
        if (((chunkType & 0x0000ff00) >>  8) != str[1])
            return false;
        if (((chunkType & 0x00ff0000) >> 16) != str[2])
            return false;
        if (((chunkType & 0xff000000) >> 24) != str[3])
            return false;
        return true;
    }

    DWORD genChunkType(char const *str)
    {
		BW_GUARD;

        DWORD result = 0;
        result |= str[0] <<  0;
        result |= str[1] <<  8;
        result |= str[2] << 16;
        result |= str[3] << 24;
        return result;
    }

    bool checkFRead(FILE *&file, void *buffer, size_t sz)
    {
		BW_GUARD;

        size_t readSz = fread(buffer, 1, sz, file);
        if (readSz != sz)
        {
            fclose(file); file = NULL;
            return false;
        }
        return true;
    }
}


/*virtual*/ bool 
ImportCodecTerragen::canHandleFile
(
    BW::string         const &filename
)
{
	BW_GUARD;
    return BWResource::getExtension(filename).equals_lower( "ter" );
}


/*virtual*/ ImportCodec::LoadResult 
ImportCodecTerragen::load
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

    FILE *file = NULL;
    try
    {
        file = BWResource::instance().fileSystem()->posixFileOpen( filename, "rb" );
        if (file == NULL)
            return LR_BAD_FORMAT;

        uint16 width  = 65535;
        uint16 height = 65535;
        float  scale  = 30.0;

        // Read the header:
        DWORD headerDWORD;
        if (!checkFRead(file, &headerDWORD, sizeof(DWORD)))
            return LR_BAD_FORMAT;
        if (!compareChunkType(headerDWORD, "TERR"))
            return LR_BAD_FORMAT;
        if (!checkFRead(file, &headerDWORD, sizeof(DWORD)))
            return LR_BAD_FORMAT;
        if (!compareChunkType(headerDWORD, "AGEN"))
            return LR_BAD_FORMAT;
        if (!checkFRead(file, &headerDWORD, sizeof(DWORD)))
            return LR_BAD_FORMAT;
        if (!compareChunkType(headerDWORD, "TERR"))
            return LR_BAD_FORMAT;
        if (!checkFRead(file, &headerDWORD, sizeof(DWORD)))
            return LR_BAD_FORMAT;
        if (!compareChunkType(headerDWORD, "AIN "))
            return LR_BAD_FORMAT;

        // Read the chunks:
        bool done = false;
        while (!done)
        {
            DWORD chunkType = 0;
            fread(&chunkType, sizeof(DWORD), 1, file);
            if (compareChunkType(chunkType, "XPTS"))
            {
                if (!checkFRead(file, &width, sizeof(uint16)))
                    return LR_BAD_FORMAT;
                uint16 dummy;                 
                if (!checkFRead(file, &dummy, sizeof(uint16)))
                    return LR_BAD_FORMAT;
            }
            else if (compareChunkType(chunkType, "YPTS"))
            {
                if (!checkFRead(file, &height, sizeof(uint16)))
                    return LR_BAD_FORMAT;
                uint16 dummy;                 
                if (!checkFRead(file, &dummy, sizeof(uint16)))
                    return LR_BAD_FORMAT;
            }
            else if (compareChunkType(chunkType, "SIZE"))
            {
                DWORD dummy;
                if (!checkFRead(file, &dummy, sizeof(DWORD)))
                    return LR_BAD_FORMAT;
            }
            else if (compareChunkType(chunkType, "SCAL"))
            {
                if (!checkFRead(file, &scale, sizeof(float)))
                    return LR_BAD_FORMAT;
                if (!checkFRead(file, &scale, sizeof(float)))
                    return LR_BAD_FORMAT;
                if (!checkFRead(file, &scale, sizeof(float)))
                    return LR_BAD_FORMAT;
            }
            else if (compareChunkType(chunkType, "CRAD"))
            {
                float dummy;
                if (!checkFRead(file, &dummy, sizeof(float)))
                    return LR_BAD_FORMAT;
            }
            else if (compareChunkType(chunkType, "CRVM"))
            {
                unsigned int dummy;
                if (!checkFRead(file, &dummy, sizeof(unsigned int)))
                    return LR_BAD_FORMAT;
            }
            else if (compareChunkType(chunkType, "ALTW"))
            {
                int16 heightScale;
                if (!checkFRead(file, &heightScale, sizeof(int16)))
                    return LR_BAD_FORMAT;
                int16 baseHeight;
                if (!checkFRead(file, &baseHeight, sizeof(int16)))
                    return LR_BAD_FORMAT;
                if (width == 65535 || height == 65535)
                {
                    fclose(file); file = NULL;
                    return LR_BAD_FORMAT;
                }
                image.resize(width, height);
				uint16 minRaw    = std::numeric_limits<uint16>::max();
				uint16 maxRaw    = 0;
				float  minHeight = +std::numeric_limits<float>::max();
				float  maxHeight = -std::numeric_limits<float>::max();
                for (uint16 y = 0; y < height; ++y)
                {
                    for (uint16 x = 0; x < width; ++x)
                    {
                        int16 height;
                        if (!checkFRead(file, &height, sizeof(int16)))
                            return LR_BAD_FORMAT;
						uint16 h = height + 32768;
						image.set(x, y, h);
						minRaw = std::min(minRaw, h);
						maxRaw = std::max(maxRaw, h);
                        float heightf = baseHeight + height*heightScale/65535.0f;
                        heightf *= scale;
						minHeight = std::min(minHeight, heightf);   
						maxHeight = std::max(maxHeight, heightf); 
                    }
                }
				image.setScale(minHeight, maxHeight, minRaw, maxRaw);
                done = true;
            }
            else
            {
                fclose(file); file = NULL;
                return LR_BAD_FORMAT;
            }
        }
    }
    catch (...)
    {
        if (file != NULL)
            fclose(file);
        file = NULL;
        throw;
    }
    return LR_OK;
}


/*virtual*/ bool 
ImportCodecTerragen::save
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
	BW_GUARD;

    FILE *file = NULL;
    try
    {
        file = BWResource::instance().fileSystem()->posixFileOpen(filename, "wb");
        if (file == NULL)
            return false;

        // Write the header:
        DWORD dword = 0;
        dword = genChunkType("TERR");
        fwrite(&dword, sizeof(DWORD), 1, file);
        dword = genChunkType("AGEN");
        fwrite(&dword, sizeof(DWORD), 1, file);
        dword = genChunkType("TERR");
        fwrite(&dword, sizeof(DWORD), 1, file);
        dword = genChunkType("AIN ");
        fwrite(&dword, sizeof(DWORD), 1, file);

        // Write the SIZE chunk:
        dword = genChunkType("SIZE");
        fwrite(&dword, sizeof(DWORD), 1, file);
        dword = (DWORD)std::min(image.width() - 1, image.height() - 1);
        fwrite(&dword, sizeof(DWORD), 1, file);

        // Write the width:
        dword = genChunkType("XPTS");
        fwrite(&dword, sizeof(DWORD), 1, file);
        dword = image.width();
        fwrite(&dword, sizeof(DWORD), 1, file);

        // Write the height:
        dword = genChunkType("YPTS");
        fwrite(&dword, sizeof(DWORD), 1, file);
        dword = image.height();
        fwrite(&dword, sizeof(DWORD), 1, file);

        // Write the altitude data:
        dword = genChunkType("ALTW");
        fwrite(&dword, sizeof(DWORD), 1, file);
        float minv, maxv;
        image.rangeHeight(minv, maxv);
        if (maxv == minv) 
            maxv = minv + 1.0f;
        uint16 heightScale = 65535/30; // 30 metres per unit
        fwrite(&heightScale, sizeof(uint16), 1, file);
        int16 baseHeight = 0;
        fwrite(&baseHeight, sizeof(int16), 1, file);
        for (unsigned int y = 0; y < image.height(); ++y)
        {
            for (unsigned int x = 0; x < image.width(); ++x)
            {
                int16 elevation = 
                    Math::lerp(image.toHeight(image.get(x, y)), minv, maxv, 0, 10000);
                fwrite(&elevation, sizeof(int16), 1, file);
            }
        }

        // Write the EOF:
        dword = genChunkType("EOF ");
        fwrite(&dword, sizeof(DWORD), 1, file);

        // Cleanup:
        fclose(file); file = NULL;
    }
    catch (...)
    {
        if (file != NULL)
            fclose(file);
        file = NULL;
        throw;
    }

    return true;
}


/*virtual*/ bool ImportCodecTerragen::canLoad() const
{
    return true;
}


/*virtual*/ bool ImportCodecTerragen::canSave() const
{
    return true;
}

BW_END_NAMESPACE

