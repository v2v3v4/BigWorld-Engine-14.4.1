#ifndef IMPORT_CODEC_HPP
#define IMPORT_CODEC_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This is the base class for import codecs that can read and write
 *  elevation/mask data from/to non-native formats.
 */
class ImportCodec : public ReferenceCount
{
public:
    virtual ~ImportCodec();

    virtual bool canHandleFile(BW::string const &filename) = 0;

    enum LoadResult
    {
        LR_OK,
        LR_BAD_FORMAT,
        LR_CANCELLED,
        LR_UNKNOWN_CODEC
    };

    virtual LoadResult load
    (
        BW::string         const &filename, 
        ImportImage			&image,
        float               *left           = NULL,
        float               *top            = NULL,
        float               *right          = NULL,
        float               *bottom         = NULL,
		bool				*absolute		= NULL,
        bool                loadConfigDlg   = false
    ) = 0;

    virtual bool save
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
    ) = 0;

    virtual bool canLoad() const;

    virtual bool canSave() const;
};


typedef SmartPointer< ImportCodec > ImportCodecPtr;

BW_END_NAMESPACE

#endif // IMPORT_CODEC_HPP
