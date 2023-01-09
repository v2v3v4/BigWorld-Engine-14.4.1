#ifndef DRAG_IMAGE_HPP
#define DRAG_IMAGE_HPP

BW_BEGIN_NAMESPACE

namespace controls
{

class DragImage
{
public:
	DragImage( CImageList & imgList, const CPoint & pos, const CPoint & offset );
	~DragImage();

	void update( const CPoint & pos, BYTE alpha = 160 );

private:
	CImageList & imgList_;
	HWND hwnd_;
	CRect rect_;
	CPoint offset_;
};

} // namespace controls

BW_END_NAMESPACE

#endif // DRAG_IMAGE_HPP