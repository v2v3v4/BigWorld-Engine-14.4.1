#ifndef IMAGE_CONTROL_HPP
#define IMAGE_CONTROL_HPP


#include "controls/dib_section8.hpp"
#include "controls/dib_section32.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    /**
    *  This control displays an image inside itself.
    */
	template<typename IMAGETYPE>
    class ImageControl : public CWnd
    {
    public:
		typedef IMAGETYPE		ImageType;

		enum AspectRatio
		{
			FIT_TO_WINDOW,
			PRESERVE,
			PRESERVE_FUZZY
		};

        ImageControl();

        BOOL Create(DWORD style, RECT const &extents, CWnd *parent, unsigned int id = 0);

        BOOL subclass(unsigned int resourceID, CWnd *parent);

        void image(ImageType const &dibSection);
        ImageType const &image() const;
        ImageType &image();

        enum BorderStyle
        {
            BS_NONE,
            BS_BLACKRECT,
            BS_SUNKEN,
            BS_RAISED
        };

        BorderStyle border() const;
        void border(BorderStyle border);

        AspectRatio preserveAspectRatio() const;
        void preserveAspectRatio(AspectRatio preserve);

        COLORREF backgroundColour() const;
        void backgroundColour(COLORREF colour);

        unsigned int borderPadding() const;
        void borderPadding(unsigned int padding);

        void beginSet();
        void endSet();

		void text(BW::string const &str);
		BW::string text() const;

    protected:
		const static uint32 FUZZY_ASPECT_PIXELS = 4; // fuzziness pixels for fixed aspect ratio

        void OnPaint();

        BOOL OnEraseBkgnd(CDC *dc);

    private:
        ImageType			image_;
        BorderStyle         border_;
        AspectRatio         aspect_;   
        COLORREF            background_;
        unsigned int        padding_;
        size_t              setCount_;
		BW::wstring		textNoImage_;
    };


	class ImageControl8 : public ImageControl<DibSection8>
	{
	public:
		typedef ImageControl<DibSection8>		Base;

	protected:
        afx_msg void OnPaint();

        afx_msg BOOL OnEraseBkgnd(CDC *dc);

        DECLARE_MESSAGE_MAP()
	};


	class ImageControl32 : public ImageControl<DibSection32>
	{
	public:
		typedef ImageControl<DibSection32>		Base;

	protected:
        afx_msg void OnPaint();

        afx_msg BOOL OnEraseBkgnd(CDC *dc);

        DECLARE_MESSAGE_MAP()
	};
}

BW_END_NAMESPACE

#include "controls/image_control.ipp"


#endif // IMAGE_CONTROL_HPP
