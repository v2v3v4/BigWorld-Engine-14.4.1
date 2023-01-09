#ifndef CONTROLS_NULL_SIZER_HPP
#define CONTROLS_NULL_SIZER_HPP

#include "sizer.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    /**
     *  This Sizer represents a blank area.
     */
    class NullSizer : public Sizer
    {
    public:
        NullSizer(uint32 minWidth, uint32 minHeight);

        /*virtual*/ void onSize(CRect const &extents);
        /*virtual*/ CSize minimumSize() const;
        /*virtual*/ void draw(CDC *dc);

    private:
        uint32					minWidth_;
        uint32		            minHeight_;
    };
}

BW_END_NAMESPACE

#endif // CONTROLS_NULL_SIZER_HPP
