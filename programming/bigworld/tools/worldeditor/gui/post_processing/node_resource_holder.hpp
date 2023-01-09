#ifndef NODE_RESOURCE_HOLDER_HPP
#define NODE_RESOURCE_HOLDER_HPP

BW_BEGIN_NAMESPACE

// Forward declarations
class GradientBrush;
typedef SmartPointer< GradientBrush > GradientBrushPtr;


/**
 *	This class works as an base interface class for other class that want to 
 *	receive notifications from an Effect node or a Phase node.
 */
class NodeResourceHolder
{
public:
	NodeResourceHolder();
	~NodeResourceHolder();

	static void addUser( UINT * bmpResIDs, int numIDs );
	static void removeUser();

	static NodeResourceHolder * instance() { return s_instance_; }

	CBitmap & bitmap( UINT bmpResID ) { return *bmps_[ bmpResID ]; }
	int bitmapWidth( UINT bmpResID ) { return bmpWidths_[ bmpResID ]; }
	int bitmapHeight( UINT bmpResID ) { return bmpHeights_[ bmpResID ]; }

	CFont & font() { return font_; }
	int fontHeight() const { return fontHeight_; }

	CFont & smallFont() { return smallFont_; }
	int smallFontHeight() const { return smallFontHeight_; }

	CFont & bigFont() { return bigFont_; }
	int bigFontHeight() const { return bigFontHeight_; }

	CBrush * gradientBrush( COLORREF grad1, COLORREF grad2, int size, bool vertical );

private:
	/**
	 *	This private struct stores an identifier key for a particular gradient
	 *	brush.
	 */
	struct GradientKey
	{
		GradientKey( COLORREF col1, COLORREF col2, int size, bool vertical ) :
			col1_( col1 ),
			col2_( col2 ),
			size_( size ),
			vertical_( vertical )
		{
		}

		bool operator<( const GradientKey & other ) const
		{
			if (col1_ < other.col1_) return true;
			if (col1_ == other.col1_ && col2_ < other.col2_) return true;
			if (col1_ == other.col1_ && col2_ == other.col2_ && size_ < other.size_) return true;
			if (col1_ == other.col1_ && col2_ == other.col2_ && size_ == other.size_ && (int)vertical_ < (int)other.vertical_) return true;
			return false;
		}

	private:
		COLORREF col1_;
		COLORREF col2_;
		int size_;
		bool vertical_;
	};

	typedef BW::map< UINT, CBitmap * > BmpMap;
	typedef BW::map< UINT, int > IntMap;
	typedef BW::map< GradientKey, GradientBrushPtr > GradientMap;

	static NodeResourceHolder * s_instance_ ;
	static int s_users_;

	CFont font_;
	CFont smallFont_;
	CFont bigFont_;
	int fontHeight_;
	int smallFontHeight_;
	int bigFontHeight_;
	BmpMap bmps_;
	IntMap bmpWidths_;
	IntMap bmpHeights_;
	GradientMap gradientBrushes_;

	void addResources( UINT bmpResIDs[], int numIDs );
};

BW_END_NAMESPACE

#endif // NODE_RESOURCE_HOLDER_HPP
