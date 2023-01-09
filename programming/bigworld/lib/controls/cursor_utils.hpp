#ifndef CURSOR_UTILS_HPP
#define CURSOR_UTILS_HPP

BW_BEGIN_NAMESPACE

namespace controls
{

// IMPORTANT: The caller is responsible for deleting the returned HCURSOR.
HCURSOR addPlusSignToCursor( HCURSOR origCursor );

// IMPORTANT: The caller is responsible for deleting the returned HCURSOR.
HCURSOR addOverlayToCursor( HCURSOR origCursor, unsigned char * bitmap, unsigned char * mask );

}; // namespace controls

BW_END_NAMESPACE

#endif // CURSOR_UTILS_HPP
