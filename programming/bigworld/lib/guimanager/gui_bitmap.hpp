#ifndef GUI_BITMAP_HPP__
#define GUI_BITMAP_HPP__

#include "gui_forward.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"

/**************************************************
Some words about bitmap type
As we know, any status bitmap could be normal/disabled/enabled/hover/pressed/.......
It is a bit hard to give a full enumeration of is, so we use a string as BITMAP type

Currently we are using the following types:

NORMAL		normal, and pressed in toolbar
HOVER		when mouse put on
DISABLED	a grayed one

You may define your own freely since their are no real panelty except using a bit more resources.

TODO: for now, we doesn't support any register + post processing, will be finished when
	everything else is ok
**************************************************/

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

static const LONG SIZE_DEFAULT = 0xffffffff;

class Bitmap : public SafeReferenceCount
{
protected:
	BW::string type_;
	HBITMAP bitmap_;
	bool defaultSize_;
public:
	Bitmap( const BW::wstring& name, COLORREF transparent, const BW::string& type = "NORMAL", unsigned int width = SIZE_DEFAULT, unsigned int height = SIZE_DEFAULT );
	~Bitmap();
	const BW::string& type() const;
	unsigned int width() const;
	unsigned int height() const;
	bool defaultSize() const;
	operator HBITMAP();
};

typedef SmartPointer<Bitmap> BitmapPtr;

class BitmapManager
{
	BW::map<BW::string, BW::vector<BitmapPtr> > bitmaps_;
public:
	BitmapPtr find( const BW::string& name, const BW::string& type = "NORMAL",
		unsigned int width = SIZE_DEFAULT, unsigned int height = SIZE_DEFAULT );
	BitmapPtr get( const BW::string& name, COLORREF transparent,
		const BW::string& type = "NORMAL", unsigned int width = SIZE_DEFAULT, unsigned int height = SIZE_DEFAULT );
	void free( BitmapPtr& bitmap );
	void clear();
};

END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif//GUI_BITMAP_HPP__
