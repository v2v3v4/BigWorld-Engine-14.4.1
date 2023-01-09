#ifndef ELEVATION_BLIT_HPP
#define ELEVATION_BLIT_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "moo/image.hpp"

BW_BEGIN_NAMESPACE

/**
 *  These functions hide the details of blitting new terrain data to a space.
 */
namespace ElevationBlit
{
    enum Mode
    {
        ADDITIVE   ,
        SUBTRACTIVE,
        OVERLAY    ,
        MIN        ,
        MAX        ,
        REPLACE
    };

    size_t importUndoSize
    (
        float               left,
        float               top,
        float               right,
        float               bottom
    );

    bool saveUndoForImport
    (
        float               left,
        float               top,
        float               right,
        float               bottom,
        BW::string         const &description,
        bool                showProgress
    );

    bool import
    (
        ImportImage			const &elevationImage,
        float               left,
        float               top,
        float               right,
        float               bottom,
        Mode                mode,
        float               minv,
        float               maxv,
        float               alpha,
        bool                showProgress,
        bool                forceToMemory
    );

    bool exportTo
    (
        ImportImage			&image,
        float               left,
        float               top,
        float               right,
        float               bottom,
		float				minHeight,
		float				maxHeight,
		bool				&allHeightsOk,
        bool                showProgress
    );

	bool heightRange
	(
		float               left,
        float               top,
        float               right,
        float               bottom,
		float				&minHeight,
		float				&maxHeight,
		bool                showProgress
	);
}

BW_END_NAMESPACE

#endif // ELEVATION_BLIT_HPP
