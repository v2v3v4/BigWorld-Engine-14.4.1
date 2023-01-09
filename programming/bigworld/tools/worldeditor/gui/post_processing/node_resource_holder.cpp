
#include "pch.hpp"
#include "view_skin.hpp"
#include "node_resource_holder.hpp"
#include "controls/utils.hpp"

BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
//	Section: GradientBrush
///////////////////////////////////////////////////////////////////////////////

/**
 *	This class creates a gradient brush, used to fill the background of nodes.
 */
class GradientBrush : public ReferenceCount
{
public:

	/**
	 *	Constructor.
	 *
	 *	@param col1	Starting colour of the gradient.
	 *	@param col2	Starting colour of the gradient.
	 *	@param size	Size of the gradient image.
	 *	@param vertical True if it's vertical, false for horizontal.
	 */
	GradientBrush( COLORREF col1, COLORREF col2, int size, bool vertical ) :
		brush_( NULL ),
		bmp_( NULL )
	{
		BW_GUARD;

		// Init DIB header.
		BITMAPINFOHEADER head;
		ZeroMemory( &head, sizeof( head ) );
		head.biSize = sizeof( head );
		head.biWidth = vertical ? 1 : size;
		head.biHeight = vertical ? size : 1;
		head.biPlanes = 1;
		head.biBitCount = 32;
		head.biCompression = BI_RGB;
		
		// Init memory block.
		bmp_ = new char[ sizeof( head ) + size * 4 ];
		
		memcpy( bmp_, &head, sizeof( head ) );

		// Fill DIB memory with the gradient.
		char * p = bmp_ + sizeof( head );

		if (vertical)
		{
			std::swap( col1, col2 );
		}

		int r1 = GetRValue( col1 );
		int g1 = GetGValue( col1 );
		int b1 = GetBValue( col1 );

		int r2 = GetRValue( col2 );
		int g2 = GetGValue( col2 );
		int b2 = GetBValue( col2 );

		int maxI = size - 1;

		for (int i = 0; i < size; ++i)
		{
			*(p++) = (unsigned char)((b1 * (maxI - i) + b2 * i) / maxI);
			*(p++) = (unsigned char)((g1 * (maxI - i) + g2 * i) / maxI);
			*(p++) = (unsigned char)((r1 * (maxI - i) + r2 * i) / maxI);
			*(p++) = (unsigned char)(255);
		}

		brush_ = new CBrush();
		brush_->CreateDIBPatternBrush( (const void *)bmp_, DIB_RGB_COLORS );
	}


	/**
	 *	Constructor.
	 *
	 *	@retunr An MFC CBrush object corresponding to this object's gradient.
	 */
	CBrush * brush()
	{
		BW_GUARD;

		return brush_;
	}


	/**
	 *	Destructor.
	 */
	~GradientBrush()
	{
		BW_GUARD;

		bw_safe_delete(brush_);
		bw_safe_delete(bmp_);
	}

private:
	CBrush * brush_;
	char * bmp_;
};



///////////////////////////////////////////////////////////////////////////////
//	Section: GradientBrush
///////////////////////////////////////////////////////////////////////////////


// Statics.
/*static*/ NodeResourceHolder * NodeResourceHolder::s_instance_ = NULL;
/*static*/ int NodeResourceHolder::s_users_ = 0;


/**
 *	Constructor.
 */
NodeResourceHolder::NodeResourceHolder() :
	fontHeight_( 12 ),
	smallFontHeight_( 11 ),
	bigFontHeight_( 16 )
{
	BW_GUARD;

	NONCLIENTMETRICS metrics;

	// Init font
	metrics.cbSize = sizeof( metrics );
	SystemParametersInfo( SPI_GETNONCLIENTMETRICS, metrics.cbSize, (void*)&metrics, 0 );
	metrics.lfSmCaptionFont.lfWeight = FW_NORMAL;
	metrics.lfSmCaptionFont.lfHeight = fontHeight_;

	if (!font_.CreateFontIndirect( &metrics.lfSmCaptionFont ))
	{
		ERROR_MSG( "Could not create font.\n" );
	}

	// Init small font
	metrics.cbSize = sizeof( metrics );
	SystemParametersInfo( SPI_GETNONCLIENTMETRICS, metrics.cbSize, (void*)&metrics, 0 );
	metrics.lfSmCaptionFont.lfWeight = FW_NORMAL;
	metrics.lfSmCaptionFont.lfHeight = smallFontHeight_;

	if (!smallFont_.CreateFontIndirect( &metrics.lfSmCaptionFont ))
	{
		ERROR_MSG( "Could not create small font.\n" );
	}

	// Init big font
	metrics.cbSize = sizeof( metrics );
	SystemParametersInfo( SPI_GETNONCLIENTMETRICS, metrics.cbSize, (void*)&metrics, 0 );
	metrics.lfCaptionFont.lfWeight = FW_NORMAL;
	metrics.lfCaptionFont.lfHeight = bigFontHeight_;

	if (!bigFont_.CreateFontIndirect( &metrics.lfCaptionFont ))
	{
		ERROR_MSG( "Could not create big font.\n" );
	}
}


/**
 *	Destructor.
 */
NodeResourceHolder::~NodeResourceHolder()
{
	BW_GUARD;

	for (BmpMap::iterator it = bmps_.begin(); it != bmps_.end(); ++it)
		bw_safe_delete((*it).second);

	bmps_.clear();
}


/**
 *	This static method increments the count of users of this class, if one or
 *	more, it creates the static NodeResourceHolder instance, and then loads
 *	the specified resources.
 *
 *	@param bmpResIDs	IDs of the resources wanted by this user.
 *	@param numIDs		Number of IDs on the "bmpResIDs" array.
 */
/*static*/ void NodeResourceHolder::addUser( UINT * bmpResIDs, int numIDs )
{
	BW_GUARD;

	if (s_users_ == 0)
	{
		s_instance_ = new NodeResourceHolder();
	}

	s_instance_->addResources( bmpResIDs, numIDs );

	++s_users_;
}


/**
 *	This static method decrements the count of users of this class.  If the 
 *	user count reaches 0, the static NodeResourceHolder instance is deleted.
 */
/*static*/ void NodeResourceHolder::removeUser()
{
	BW_GUARD;

	--s_users_;
	if (s_users_ == 0)
	{
		bw_safe_delete(s_instance_);
	}
}


/**
 *	This method loads the specified resources and keeps them until the instance
 *	is destroyed.
 *
 *	@param bmpResIDs	IDs of the wanted resources.
 *	@param numIDs		Number of IDs on the "bmpResIDs" array.
 */
void NodeResourceHolder::addResources( UINT bmpResIDs[], int numIDs )
{
	BW_GUARD;

	// Add bitmaps
	for (int i = 0; i < numIDs; ++i)
	{
		UINT id = bmpResIDs[ i ];

		if (bmps_.find( id ) == bmps_.end())
		{
			bmps_[ id ] = new CBitmap();
			bmpWidths_[ id ] = 0;
			bmpHeights_[ id ] = 0;

			if (bmps_[ id ]->LoadBitmap( id ))
			{
				if (!ViewSkin::keyColourTransparency())
				{
					controls::replaceColour( (HBITMAP)(*bmps_[ id ]), ViewSkin::bkColour() );
				}

				BITMAP bmpInfo;
				bmps_[ id ]->GetObject( sizeof( BITMAP ), &bmpInfo );
				bmpWidths_[ id ] = bmpInfo.bmWidth;
				bmpHeights_[ id ] = bmpInfo.bmHeight;
			}
			else
			{
				ERROR_MSG( "Could not create background bitmap for ID %d.\n", id );
			}
		}
	}
}


/**
 *	This method returns a gradient brush created as requested in the params.
 *
 *	@param grad1	Start colour of the gradient.
 *	@param grad2	End colour of the gradient.
 *	@param size		Size of the gradient.
 *	@param vertical	True if the gradient is vertical, false if horizontal.
 *	@return	The gradient brush.
 */
CBrush * NodeResourceHolder::gradientBrush( COLORREF grad1, COLORREF grad2, int size, bool vertical )
{
	BW_GUARD;

	GradientKey key( grad1, grad2, size, vertical );

	GradientMap::iterator it = gradientBrushes_.find( key );
	GradientBrushPtr gbrush;
	if (it == gradientBrushes_.end())
	{
		gbrush = new GradientBrush( grad1, grad2, size, vertical );
		gradientBrushes_.insert( std::make_pair( key, gbrush ) );
	}
	else
	{
		gbrush = (*it).second;
	}

	return gbrush->brush();
}

BW_END_NAMESPACE

