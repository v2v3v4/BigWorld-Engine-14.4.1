
#include "pch.hpp"
#include "view_skin.hpp"

BW_BEGIN_NAMESPACE

/// Current node zoom (see ViewSkin::nodeZoom)
/*static*/ int ViewSkin::s_nodeZoom_ = 0;

/// Node zoom factor, how much each step of nodeZoom affects scaling of nodes.
/*static*/ float ViewSkin::s_nodeZoomFactor_ = 0.05f;


/**
 *	Graph view background colour.
 */
/*static*/ COLORREF ViewSkin::bkColour()
{
	return RGB( 64, 64, 64 );
}


/**
 *	Graph view footer text colour.
 */
/*static*/ COLORREF ViewSkin::footerColour()
{
	return RGB( 32, 32, 32 );
}


/**
 *	Node key colour for transparency.
 */
/*static*/ COLORREF ViewSkin::keyColour()
{
	return RGB( 255, 0, 255 );
}


/**
 *	Returns whether or not key colour transparency is enabled.
 *	true = nicer, false = faster.
 */
/*static*/ bool ViewSkin::keyColourTransparency()
{
	// Disabled since it's much slower and it's not noticeable.
	return false;
}


/**
 *	Font colour for when the effect node is active.
 */
/*static*/ COLORREF ViewSkin::effectFontActive()
{
	return RGB( 255, 255, 255 );
}


/**
 *	Font colour for when the effect node is inactive.
 */
/*static*/ COLORREF ViewSkin::effectFontInactive()
{
	return RGB( 192, 192, 192 );
}


/**
 *	Active effect gradient start colour.
 */
/*static*/ COLORREF ViewSkin::effectActiveGradient1()
{
	return RGB( 71, 117, 199 );
}


/**
 *	Active effect gradient end colour.
 */
/*static*/ COLORREF ViewSkin::effectActiveGradient2()
{
	return RGB( 43, 67, 111 );
}


/**
 *	Inactive effect gradient start colour.
 */
/*static*/ COLORREF ViewSkin::effectInactiveGradient1()
{
	return RGB( 119, 130, 151 );
}


/**
 *	Inactive effect gradient end colour.
 */
/*static*/ COLORREF ViewSkin::effectInactiveGradient2()
{
	return RGB( 69, 75, 85 );
}


/**
 *	Font colour for when the phase node is active.
 */
/*static*/ COLORREF ViewSkin::phaseFontActive()
{
	return RGB( 255, 255, 255 );
}


/**
 *	Font colour for when the phase node is inactive.
 */
/*static*/ COLORREF ViewSkin::phaseFontInactive()
{
	return RGB( 192, 192, 192 );
}


/**
 *	Active phase gradient start colour.
 */
/*static*/ COLORREF ViewSkin::phaseActiveGradient1()
{
	return RGB( 128, 152, 126 );
}


/**
 *	Active phase gradient end colour.
 */
/*static*/ COLORREF ViewSkin::phaseActiveGradient2()
{
	return RGB( 80, 106, 78 );
}


/**
 *	Inactive phase gradient start colour.
 */
/*static*/ COLORREF ViewSkin::phaseInactiveGradient1()
{
	return RGB( 136, 143, 135 );
}


/**
 *	Inactive phase gradient end colour.
 */
/*static*/ COLORREF ViewSkin::phaseInactiveGradient2()
{
	return RGB( 88, 96, 88 );
}


/**
 *	Normal node border colour.
 */
/*static*/ COLORREF ViewSkin::nodeNormalEdge()
{
	return RGB( 0, 0, 0 );
}


/**
 *	Selected node border colour.
 */
/*static*/ COLORREF ViewSkin::nodeSelectedEdge()
{
	return RGB( 255, 220, 0 );
}


/**
 *	Node rectangle corner roundness radius.
 */
/*static*/ int ViewSkin::nodeEdgeCurve()
{
	return 14;
}


/**
 *	Normal node rectangle border width.
 */
/*static*/ int ViewSkin::nodeNormalEdgeSize()
{
	return 1;
}


/**
 *	Selected node rectangle border width.
 */
/*static*/ int ViewSkin::nodeSelectedEdgeSize()
{
	return 2;
}


/**
 *	Effect-to-effect EdgeView colour.
 */
/*static*/ COLORREF ViewSkin::effectEdgeColour()
{
	return RGB( 0, 0, 0 );
}


/**
 *	*-to-Phase EdgeView colour.
 */
/*static*/ COLORREF ViewSkin::phaseEdgeColour()
{
	return RGB( 0, 0, 0 );
}


/**
 *	EdgeView line width.
 */
/*static*/ int ViewSkin::edgeLineSize()
{
	return 3;
}


/**
 *	EdgeView arrow size.
 */
/*static*/ int ViewSkin::edgeArrowSize()
{
	return 5;
}


/**
 *	Node text margins.
 */
/*static*/ CSize ViewSkin::nodeRectTextMargin()
{
	return CSize( 6, 4 );
}


/**
 *	Effect node size.
 */
/*static*/ CSize ViewSkin::effectNodeSize()
{
	BW_GUARD;

	return CSize( int( 80 * nodeScale() ), std::min( 46, int( 46 * nodeScale() ) ) );
}


/**
 *	Phase node size.
 */
/*static*/ CSize ViewSkin::phaseNodeSize()
{
	BW_GUARD;

	return CSize( int( 80 * nodeScale() ), int( 46 * nodeScale() ) );
}


/**
 *	Node margins.
 */
/*static*/ CSize ViewSkin::nodeMargin()
{
	return CSize( 20, 14 );
}


/**
 *	Separation between effect nodes.
 */
/*static*/ CSize ViewSkin::effectNodeSeparation()
{
	BW_GUARD;

	CSize sz = ViewSkin::effectNodeSize();
	return CSize( std::min( 30, int( 30 * nodeScale() ) ) + sz.cx, std::min( 14, int( 14 * nodeScale() ) ) + sz.cy );
}


/**
 *	Separation between phase nodes.
 */
/*static*/ CSize ViewSkin::phaseNodeSeparation()
{
	BW_GUARD;

	CSize sz = ViewSkin::phaseNodeSize();
	return CSize( std::min( 30, int( 30 * nodeScale() ) ) + sz.cx, std::min( 14, int( 14 * nodeScale() ) ) + sz.cy );
}


/**
 *	Set the node zoom.
 *
 *	@param nodeZoom	0 is normal value, >0 is zoomed in (scaled up) and <0 is
 *					zoomed out (scaled down).
 */
/*static*/ void ViewSkin::nodeZoom( int nodeZoom )
{
	s_nodeZoom_ = nodeZoom;
}


/**
 *	Transparency used when dragging a node.
 */
/*static*/ int ViewSkin::dragAlpha()
{
	return 128;
}


/**
 *	Calculates the node scaling factor from the nodeZoom.
 */
/*static*/ float ViewSkin::nodeScale()
{
	BW_GUARD;

	float scale = 1.0f + s_nodeZoom_ * s_nodeZoomFactor_;
	if (scale < 0.1f)
	{
		scale = 0.1f;
	}
	// scale cubically
	scale = scale * scale * scale;
	return scale;
}

BW_END_NAMESPACE

