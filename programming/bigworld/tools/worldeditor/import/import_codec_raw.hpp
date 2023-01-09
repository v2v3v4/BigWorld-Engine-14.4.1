#ifndef IMPORT_CODEC_RAW_HPP
#define IMPORT_CODEC_RAW_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/import/import_codec.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This codec reads/writes BMP data.
 */
class ImportCodecRAW : public ImportCodec
{
public:
    /*virtual*/ bool canHandleFile(BW::string const &filename);

    /*virtual*/ LoadResult load
    (
        BW::string         const &filename, 
        ImportImage			&image,
        float               *left           = NULL,
        float               *top            = NULL,
        float               *right          = NULL,
        float               *bottom         = NULL,
		bool				*absolute		= NULL,
        bool                loadConfigDlg   = false
    );

    /*virtual*/ bool save
    (
        BW::string         const &filename, 
        ImportImage			const &image,
        float               *left           = NULL,
        float               *top            = NULL,
        float               *right          = NULL,
        float               *bottom         = NULL,
		bool				*absolute		= NULL,
		float				*minVal			= NULL,
		float				*maxVal			= NULL
    );

    /*virtual*/ bool canLoad() const;

    /*virtual*/ bool canSave() const;
};

BW_END_NAMESPACE

#endif // IMPORT_CODEC_RAW_HPP
