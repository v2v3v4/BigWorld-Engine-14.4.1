#ifndef CONTROLS_DIBSECTION_HPP
#define CONTROLS_DIBSECTION_HPP


#include "controls/defs.hpp"
#include "controls/fwd.hpp"
#include "moo/image.hpp"
#include "moo/moo_dx.hpp"
#include "moo/render_context.hpp"
#include "moo/render_target.hpp"
#include "resmgr/bwresource.hpp"
#include <atlimage.h>


BW_BEGIN_NAMESPACE

namespace controls
{
	template<typename PIXELTYPE>
	class DibSection : public Moo::Image<PIXELTYPE>
	{
	public:
		typedef Moo::Image<PIXELTYPE>		Base;

        DibSection();
        DibSection(uint32 width, uint32 height);
        DibSection(DibSection const &other);
        ~DibSection();

        DibSection &operator=(DibSection const &other);

        size_t bufferSizeBytes() const;
        size_t rowSizeBytes() const;

        BITMAPINFO const &getBITMAPINFO() const;
        BITMAPINFOHEADER const &getBITMAPINFOHEADER() const;

        operator HBITMAP() const;
        HBITMAP detach();

        bool load(BW::string const &filename);
        bool save(BW::string const &filename) const;
        bool load(uint32 resourceID);
        bool copyFromTexture( const Moo::BaseTexturePtr& baseTexture );
		bool copyFromSurface(DX::Surface *surface);

        void draw(CDC &dc, int x, int y) const;
        void draw(CDC &dc, int left, int top, int width, int height) const;

        void draw
        (
            CDC     &dc, 
            int     dleft, 
            int     dtop, 
            int     dwidth, 
            int     dheight,
            int     sleft, 
            int     stop, 
            int     swidth, 
            int     sheight
        ) const;

        void draw(CDC &dc, CRect const &extents) const;
        void draw(CDC &dc, CRect const &dextents, CRect const &sextents) const;
        void drawAlpha(CDC &dc, int x, int y) const;
        void drawAlpha(CDC &dc, int left, int top, int width, int height) const;

        void drawAlpha
        (
            CDC     &dc, 
            int     dleft, 
            int     dtop, 
            int     dwidth, 
            int     dheight,
            int     sleft, 
            int     stop, 
            int     swidth, 
            int     sheight
        ) const;

        void drawAlpha(CDC &dc, CRect const &extents) const;
        void drawAlpha(CDC &dc, CRect const &dextents, CRect const &sextents) const;

		void toGreyScale();

    protected:
        /*virtual*/ bool createBuffer
        (
            uint32      w, 
            uint32      h,
            PixelType   *&buffer,
            bool        &owns,
            size_t      &stride,
            bool        &flipped
        );

        void copy(DibSection const &other);
        void destroy();

        bool loadDDS(BW::string const &filename);

        bool copyFromA8R8G8B8Texture(DX::Texture *texture);
        bool copyFromA8R8G8B8Surface(IDirect3DSurface9 *surface);

        void copyFromLockedRect
        (
            D3DLOCKED_RECT      const &lockedRect,
            uint32              w,
            uint32              h
        );

    private:
        BITMAPINFO              bmi_;
        HBITMAP                 hbitmap_;
	};
}


BW_END_NAMESPACE

#include "dib_section.ipp"


#endif // CONTROLS_DIBSECTION_HPP
